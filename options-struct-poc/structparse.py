from __future__ import annotations

import argparse
import json
import logging
import shlex
import subprocess
import sys
from dataclasses import asdict, dataclass
from fnmatch import fnmatch
from itertools import dropwhile, takewhile
from pathlib import Path
from typing import Callable, Iterator, Optional, Self, Sequence, cast

try:
    from clang.cindex import (
        Cursor,
        CursorKind,
        Index,
        SourceLocation,
        TranslationUnit,
        Type,
    )
except ImportError:
    print("`clang` module not found, try `pip install libclang`")
    exit(1)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source")
    parser.add_argument(
        "struct_glob",
        metavar="struct",
        help='target struct glob, eg. "ns1::ns2::*struct"',
    )

    parser.add_argument("--out-json")

    parser.add_argument(
        "--compiler",
        default="clang",
        help="compiler used to find default include paths (with `-E -v`)",
    )
    parser.add_argument("--compiler-args", default="-xc++")

    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    cpp_args = shlex.split(args.compiler_args)
    include_paths = find_default_includes(cpp_args, args.compiler)

    result = {}

    for struct, struct_namespaces in find_structs(
        Path(args.source), args.struct_glob, cpp_args, include_paths
    ):
        variables = {}
        for path, x in walk_StructNode(struct):
            if isinstance(x, StructNode):
                variables[".".join(y.identifier for y in path)] = {
                    "comment": x.comment.splitlines(),
                }
            elif isinstance(x, StructLeaf):
                d = asdict(x)
                d.pop("identifier")
                d["comment"] = d["comment"].splitlines()
                variables[".".join(y.identifier for y in (*path, x))] = d

        result["::".join((*struct_namespaces, struct.identifier))] = variables

    if args.out_json:
        json.dump(result, open(args.out_json, "w"), indent=2, sort_keys=False)
    else:
        json.dump(result, sys.stdout, indent=2, sort_keys=False)


@dataclass(frozen=True)
class StructLeaf:
    identifier: str
    type: str
    canonical_type: str
    is_optional: bool
    has_default: bool
    comment: str


@dataclass(frozen=True)
class StructNode:
    identifier: str
    members: tuple[Self | StructLeaf]
    comment: str


def walk_StructNode(*structs: StructNode | StructLeaf):
    q: list[tuple[StructNode | StructLeaf, tuple[StructNode | StructLeaf, ...]]] = [
        (struct, ()) for struct in structs
    ]
    while q:
        node, parents = q.pop(0)
        yield tuple(reversed(parents)), node
        if isinstance(node, StructNode):
            for member in node.members:
                if isinstance(member, StructNode):
                    q.append((member, (member, *parents)))
                else:
                    yield tuple(reversed(parents)), member


CLANG_SEVERITY = {0: "ignored", 1: "note", 2: "warning", 3: "error", 4: "fatal"}


def find_structs(
    source_path: Path,
    struct_glob: str,
    cpp_args: Sequence[str] = ("-xc++",),
    include_paths: Sequence[Path] = tuple(),
) -> Iterator[tuple["StructNode", tuple[str, ...]]]:
    tu = parse_TranslationUnit(source_path, cpp_args, include_paths)

    if tu.diagnostics:
        for diag in tu.diagnostics:
            loc: SourceLocation = diag.location
            locstr = f"{loc.file}:{loc.line}:{loc.column}"
            print(f"[clang:{CLANG_SEVERITY[diag.severity]}] {locstr}: {diag.spelling}")
        if any(diag.severity > 2 for diag in tu.diagnostics):
            raise IOError(f"could not parse {source_path}")

    def is_struct_decl(cursor: Cursor, parents: tuple[Cursor, ...]):
        return bool(cursor.kind == CursorKind.STRUCT_DECL)

    source_abspath = str(source_path.resolve())
    found: dict[Cursor, tuple[Cursor, ...]] = {}
    for cursor, parents in walk_cursor(tu.cursor, stop_when=is_struct_decl):
        if (
            is_struct_decl(cursor, parents)
            and str(cursor.location.file) == source_abspath
            and cursor not in found
            and not any(p in found for p in parents)
        ):
            namespaces = [
                p.spelling for p in reversed(parents) if p.kind == CursorKind.NAMESPACE
            ] or [""]
            full_name = "::".join((*namespaces, cursor.spelling))
            if fnmatch(full_name, struct_glob):
                found[cursor] = parents
                yield walk_struct(cursor), tuple(namespaces)


def walk_struct(cursor: Cursor) -> StructNode:
    def get_comment(c: Cursor):
        if c.raw_comment:
            indent = 0
            for t in c.get_tokens():
                indent = max(0, t.extent.start.column - 1)
                break

            first_line, *next_lines = c.raw_comment.splitlines()

            def remove_indent(line: str):
                space_indented = all(c == " " for c in line[:indent])
                return line[indent:] if space_indented else line

            return "\n".join((first_line, *map(remove_indent, next_lines)))
        else:
            return ""

    def f(c: Cursor) -> StructNode | StructLeaf:
        if c.type.get_declaration().kind == CursorKind.STRUCT_DECL:
            return StructNode(
                c.spelling,
                members=tuple(
                    f(d)
                    for d in c.type.get_declaration().get_children()
                    if d.kind == CursorKind.FIELD_DECL
                ),
                comment=get_comment(c.type.get_declaration()),
            )
        else:
            t = cast(Type, c.type)
            if t.spelling.startswith("std::optional"):
                t = t.get_template_argument_type(0)
                is_optional = True
            else:
                is_optional = False

            return StructLeaf(
                identifier=c.spelling,
                type=t.spelling,
                canonical_type=t.get_canonical().spelling,
                is_optional=is_optional,
                has_default=any(t.spelling == "=" for t in c.get_tokens()),
                comment=get_comment(c),
            )

    found = f(cursor)
    if not isinstance(found, StructNode):
        raise ValueError("did not find expected struct")
    else:
        return found


def parse_TranslationUnit(
    src_path: Path,
    cpp_args: Sequence[str],
    include_paths: Optional[Sequence[Path]] = None,
) -> TranslationUnit:
    index = Index.create()
    return index.parse(
        str(src_path.resolve()),
        [*cpp_args, *(f"-I{path}" for path in include_paths or [])],
        options=TranslationUnit.PARSE_SKIP_FUNCTION_BODIES,
    )


def find_default_includes(cpp_args: list[str], compiler: str = "clang") -> list[Path]:
    Ev_command = [compiler, "-E", "-v", *cpp_args, "-"]
    try:
        clang_Ev = subprocess.check_output(
            Ev_command,
            text=True,
            universal_newlines=True,
            stderr=subprocess.STDOUT,
            stdin=subprocess.DEVNULL,
        ).splitlines()

        clang_Ev = iter(clang_Ev)
        clang_Ev = dropwhile(lambda line: "#include" not in line, clang_Ev)
        clang_Ev = takewhile(lambda line: "End" not in line, clang_Ev)

        include_candidates = [
            Path(line.strip()) for line in clang_Ev if not line.startswith("#")
        ]
        include_directories = [
            path.resolve() for path in include_candidates if path.is_dir()
        ]
        logging.info(
            "found include directories %s with `%s`",
            [str(x) for x in include_directories],
            subprocess.list2cmdline(Ev_command),
        )
        return include_directories
    except FileNotFoundError:
        logging.warning("could not run `%s`", subprocess.list2cmdline(Ev_command))
        return []


def walk_cursor(
    cursor: Cursor,
    parents: tuple[Cursor, ...] = tuple(),
    stop_when: Optional[Callable[[Cursor, tuple[Cursor, ...]], bool]] = None,
) -> Iterator[tuple[Cursor, tuple[Cursor, ...]]]:
    yield cursor, parents
    if stop_when is None or (callable(stop_when) and not stop_when(cursor, parents)):
        for child in cursor.get_children():
            yield from walk_cursor(child, (cursor, *parents))


if __name__ == "__main__":
    main()
