from __future__ import annotations

import argparse
import json
import logging
import re
import shlex
import subprocess
from dataclasses import dataclass
from fnmatch import fnmatch
from functools import lru_cache
from itertools import dropwhile, takewhile
from pathlib import Path
from typing import Callable, Iterable, Iterator, Optional, Self, Sequence, cast

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
    parser.add_argument("struct", help='target struct, eg. "ns1::ns2::struct1"')
    parser.add_argument("incl", metavar="generated.h", help="output .h file")
    parser.add_argument("impl", metavar="generated.cpp", help="output .cpp file")
    parser.add_argument("--generate-json", metavar="generated.json")

    parser.add_argument("--include", action="append", metavar="header.h")
    parser.add_argument(
        "--parse",
        action="append",
        metavar="type;from_type;type_to_%",
    )
    parser.add_argument(
        "--format",
        action="append",
        metavar="type;to_type;type_from_%",
    )
    parser.add_argument(
        "--namespace",
        help="namesapce for generated code (default: same as target sruct)",
    )
    parser.add_argument("--key-sep", default="/")
    parser.add_argument(
        "--key", dest="key_overides", action="append", metavar="a.b.c=C"
    )
    parser.add_argument(
        "--compiler",
        default="clang",
        help="compiler used to find default include paths (with `-E -v`)",
    )
    parser.add_argument("--compiler-args", default="-xc++")

    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    source_path = Path(args.source)
    struct_glob = str(args.struct)
    gen_namespace = str(args.namespace)

    cpp_args = shlex.split(args.compiler_args)
    include_paths = find_default_includes(cpp_args, args.compiler)
    found = list(find_structs(source_path, struct_glob, cpp_args, include_paths))
    if not found:
        raise IOError("no struct found")
    if len(found) > 1:
        raise IOError("multiple structs found")

    struct, struct_namespaces = found[0]
    struct_namespace = "::".join(struct_namespaces)

    gen_namespace = args.namespace or struct_namespace

    key_overides: dict[str, str] = (
        dict(v.split("=", 2) for v in args.key_overides) if args.key_overides else {}
    )

    @lru_cache
    def key_frags(*nodes: StructNode | StructLeaf):
        identifiers = [node.identifier for node in nodes]
        for i in range(len(identifiers)):
            id = ".".join(identifiers[: i + 1])
            yield key_overides.get(id, identifiers[i].replace("_", "-"))

    sorted_vars = sorted(
        KeyedVar(
            args.key_sep.join(key_frags(*parents, field)),
            ".".join(o.identifier for o in (*parents, field)),
            field,
        )
        for parents, field in struct.all_leaves()
    )

    parsers = [CustomIO.From_str(a) for a in (args.parse if args.parse else [])]
    formatters = [CustomIO.From_str(a) for a in (args.format if args.format else [])]
    functions = list(cpp_functions(sorted_vars, parsers, formatters))

    with open(args.incl, "w") as f:
        for line in generate_incl(
            sorted_vars,
            f"{struct_namespace}::{struct.identifier}",
            gen_namespace,
            args.include,
            functions,
        ):
            f.write(line)
            f.write("\n")

    with open(args.impl, "w") as f:
        for line in generate_impl(sorted_vars, gen_namespace, [args.incl], functions):
            f.write(line)
            f.write("\n")

    if args.generate_json:
        with open(args.generate_json, "w") as f:
            # TODO keep tree structure with node data
            json.dump(
                {
                    v.id: {
                        "key": v.key,
                        "type": v.var.type,
                        "is_optional": v.var.is_optional,
                        "has_default": v.var.has_default,
                        "comment": v.var.comment.splitlines(),
                    }
                    for v in sorted_vars
                },
                f,
                indent=2,
            )


def generate_incl(
    sorted_vars: list[KeyedVar],
    struct_name: str,
    namespace: str,
    includes: list[str],
    functions: list[CppFunc],
):
    yield "#pragma once"
    yield ""

    for h in (
        "<array>",
        "<functional>",
        "<map>",
        "<optional>",
        "<stdexcept>",
        "<string>",
        "<variant>",
    ):
        yield f"#include {h}"
    yield ""

    if includes:
        for h in includes:
            yield f'#include "{h}"'
    yield ""

    if namespace:
        yield f"namespace {namespace} {{"
        yield ""

    types: dict[str, set[str]] = {}
    for v in sorted_vars:
        types.setdefault(v.var.canonical_type, set()).add(v.var.type)
    types2 = [(f"  {k}", ", ".join(sorted(v))) for k, v in sorted(types.items())]
    types3 = ",\n".join(k if k.strip() == v else f"{k} /* {v} */" for k, v in types2)

    yield f"typedef ::{struct_name} S; // Struct"
    yield "typedef std::string K; // Key"
    yield f"typedef std::variant<\n{types3}\n> V; // Value"
    yield "typedef std::map<K, std::optional<V>> Diff;"
    yield ""

    yield keys_struct_code(sorted_vars, "keys")
    yield ""

    for f in functions:
        if f.comment:
            yield f"/** {f.comment} */"
        yield f"{f.signature};"
        yield ""

    for name, what in [
        ("invalid_key", '"invalid key: " + key'),
        ("non_optional_key", '"non-optional key: " + key'),
    ]:
        yield f"class {name} : public std::out_of_range {{"
        yield "public:"
        yield f"explicit {name}(const K& key):"
        yield f"  std::out_of_range({what}) {{}}"
        yield "};"
        yield ""
    yield ""

    if namespace:
        yield f"}} // namespace {namespace}"


def generate_impl(
    sorted_vars: list[KeyedVar],
    namespace: str,
    includes: list[str],
    functions: list[CppFunc],
):
    for h in includes:
        yield f'#include "{h}"'
    yield ""

    lookup = CppFunc(
        "inline size_t key_index(const K& key)",
        [
            "const auto lower = std::lower_bound(sorted_keys.begin(), sorted_keys.end(), key);",
            "if (lower == sorted_keys.end() || *lower != key)",
            "  throw invalid_key(key);",
            "else",
            "  return std::distance(sorted_keys.begin(), lower);",
        ],
    )

    if namespace:
        yield f"namespace {namespace} {{"
        yield ""

    yield f"const std::array<K, {len(sorted_vars)}> sorted_keys = {{"
    for i, v in enumerate(sorted_vars):
        yield f"  keys.{v.id}, // {i} {json.dumps(v.key)}"
    yield "};"
    yield ""

    for f in [lookup, *functions]:
        yield f"{f.signature} {{"
        yield f.body
        yield "}"
        yield ""

    if namespace:
        yield f"}} // namespace {namespace}"


def cpp_functions(
    sorted_vars: list[KeyedVar],
    parsers: Iterable[CustomIO],
    formatters: Iterable[CustomIO],
):
    def make_key_branches(
        f: Callable[[KeyedVar], str],
        default: str = "throw std::invalid_argument(key); // unreachable",
        only_optionals: bool = False,
    ):
        branches: dict[str, set[int]] = {}
        for i, var in enumerate(sorted_vars):
            if not only_optionals or var.var.is_optional:
                branches.setdefault(f(var), set()).add(i)

        def lines():
            yield "switch(key_index(key)){"
            for branch, indices in branches.items():
                for i in sorted(indices):
                    yield f"  case {i}: // {json.dumps(sorted_vars[i].key)}"
                yield f"    {branch}"
            if default:
                yield f"  default: {default}"
            yield "}"

        return list(lines())

    def throws(invlaid_key: bool = True, non_optional_key: bool = False):
        comment = ""
        if invlaid_key:
            comment += "\nThrows `invalid_key` exception on unknow key."
        if non_optional_key:
            comment += "\nThrows `non_optional_key` exception on non-optional key."
        return comment

    yield CppFunc(
        "std::optional<V> get(const S& s, const K& key)",
        make_key_branches(lambda o: f"return s.{o.id};"),
        "Get a value by key." + throws(invlaid_key=True),
    )

    yield CppFunc(
        "void set(S& s, const K& key, const V& value)",
        make_key_branches(
            lambda o: f"s.{o.id} = std::get<{o.var.canonical_type}>(value); break;"
        ),
        "Set a value by key." + throws(invlaid_key=True),
    )

    yield CppFunc(
        "void unset(S& s, const K& key)",
        make_key_branches(
            lambda o: f"s.{o.id} = std::nullopt; break;",
            default="throw non_optional_key(key);",
            only_optionals=True,
        ),
        "Unset an optional value by key."
        + throws(invlaid_key=True, non_optional_key=True),
    )

    yield CppFunc(
        "std::map<K, std::optional<V>> diff(const S& current, const S& previous)",
        [
            "std::map<K, std::optional<V>> d;",
            *(
                f"if(current.{v.id} != previous.{v.id})"
                f" d[keys.{v.id}] = current.{v.id};"
                for v in sorted_vars
            ),
            "return d;",
        ],
        "Construct a `key->variant` map of differences between two instances.",
    )

    yield CppFunc(
        "void apply(S& s, const std::map<K, std::optional<V>>& diff)",
        [
            "for (const auto item : diff)",
            "  if (item.second.has_value())",
            "    set(s, item.first, item.second.value());",
            "  else",
            "    unset(s, item.first);",
        ],
        "apply a diff (`key->variant` map) to an instance."
        + throws(invlaid_key=True, non_optional_key=True),
    )

    typenames = ", ".join(
        f'`"{t}"`' for t in sorted(set(v.var.type for v in sorted_vars))
    )
    yield CppFunc(
        "std::string type(const K& key)",
        make_key_branches(
            lambda o: f'return "{o.var.type}";',
        ),
        "Retrieve the type name of a value by key."
        + f"\nPossible return values are: {typenames}."
        + throws(invlaid_key=True),
    )

    for parser in parsers:
        yield CppFunc(
            f"V {parser.function_name}(const K& key, const {parser.type}& value)",
            make_key_branches(
                lambda o: f"return {parser.user_function_for(o.var.type)}" + "(value);",
            ),
            f"Parse a value for a given key from `{parser.type}`."
            + throws(invlaid_key=True),
        )

    for formatter in formatters:
        yield CppFunc(
            f"{formatter.type} {formatter.function_name}(const K& key, const V& value)",
            make_key_branches(
                lambda o: f"return {formatter.user_function_for(o.var.type)}"
                + f"(std::get<{o.var.canonical_type}>(value));",
            ),
            f"Format a value for a given key to `{formatter.type}`."
            + throws(invlaid_key=True),
        )


@dataclass
class CustomIO:
    type: str
    function_name: str
    user_function_pattern: str

    def user_function_for(self, type: str):
        return self.user_function_pattern.replace("%", c_identifier(type))

    @classmethod
    def From_str(cls, s: str):
        return cls(*s.split(";"))


@dataclass(frozen=True, order=True)
class KeyedVar:
    key: str
    id: str
    var: StructLeaf


@dataclass(frozen=True)
class CppFunc:
    signature: str
    impl: list[str]
    comment: str = ""

    @property
    def body(self):
        return "\n".join(f"  {line}" for line in self.impl)


def c_identifier(s: str):
    return re.sub(r"\W+|^(?=\d)", "_", s)


def keys_struct_code(keyed_vars: Iterable[KeyedVar], var_name: str):
    root = {}
    for v in keyed_vars:
        node = root
        *parents, name = v.id.split(".")
        for parent in parents:
            node = node.setdefault(parent, {})
        node[name] = v

    def generate_keys_struct(node, name, depth: int = 0):
        indent = "  " * depth
        if isinstance(node, KeyedVar):
            yield indent + f'const K {name} = "{node.key}";'
        else:
            yield indent + f"const struct {name} {{"
            for k, v in node.items():
                yield from generate_keys_struct(v, k, depth + 1)
            yield indent + f"}} {name};"

    return "\n".join(generate_keys_struct(root, var_name))


################################################################################
## Clang stuff

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

    def all_leaves(self) -> Iterator[tuple[tuple[Self, ...], StructLeaf]]:
        q: list[tuple[Self, tuple[Self, ...]]] = [(self, ())]
        while q:
            s, parents = q.pop(0)
            for member in s.members:
                if isinstance(member, StructNode):
                    q.append((member, (member, *parents)))
                else:
                    yield tuple(reversed(parents)), member


def walk_struct(cursor: Cursor) -> StructNode:
    def f(c: Cursor) -> StructNode | StructLeaf:
        is_nested = c.type.get_declaration().kind == CursorKind.STRUCT_DECL
        if is_nested:
            return StructNode(
                c.spelling,
                members=tuple(
                    f(d)
                    for d in c.type.get_declaration().get_children()
                    if d.kind == CursorKind.FIELD_DECL
                ),
                comment=c.raw_comment or "",
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
                comment=c.raw_comment or "",
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


################################################################################

if __name__ == "__main__":
    main()
