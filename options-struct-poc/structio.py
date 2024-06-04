from __future__ import annotations

import argparse
from fnmatch import fnmatch
import json
import logging
import re
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
from typing import Any, Callable, Iterable, Iterator, Union


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("struct_json")
    parser.add_argument(
        "struct_glob",
        metavar="struct",
        help='target struct glob, eg. "ns1::ns2::*struct"',
    )
    parser.add_argument("incl", metavar="generated.h", help="output .h file")
    parser.add_argument("impl", metavar="generated.cpp", help="output .cpp file")

    parser.add_argument(
        "--include", dest="includes", action="append", metavar="header.h"
    )
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

    key_overides: dict[str, str] = (
        dict(v.split("=", 2) for v in args.key_overides) if args.key_overides else {}
    )

    @lru_cache
    def key_frags(identifier: str):
        identifiers = identifier.split(".")

        def f():
            for i in range(len(identifiers)):
                id = ".".join(identifiers[: i + 1])
                yield key_overides.get(id, identifiers[i])

        return ".".join(f())

    parsers = [CustomIO.From_str(a) for a in args.parse] if args.parse else []
    formatters = [CustomIO.From_str(a) for a in args.format] if args.format else []

    struct_json_path = Path(args.struct_json)
    struct_json = json.load(open(struct_json_path))

    with open(args.incl, "w") as f_incl, open(args.impl, "w") as f_impl:

        for line in incl_preamble(args.includes):
            f_incl.write(line)
            f_incl.write("\n")

        for line in impl_preamble([args.incl]):
            f_impl.write(line)
            f_impl.write("\n")

        for struct_qual_name, struct_json1 in struct_json.items():
            if not fnmatch(struct_qual_name, args.struct_glob):
                continue

            gen_namespace = f"{struct_qual_name}_io"

            sorted_vars = sorted(
                KeyedVar(key_frags(k), Var(identifier=k, **dict(v)))
                for k, v in struct_json1.items()
                if "type" in v
            )
            functions = list(cpp_functions(sorted_vars, parsers, formatters))

            f_incl.write("\n")
            f_incl.write("\n")
            f_incl.write("/" * 80 + "\n")
            for line in generate_incl(
                sorted_vars,
                struct_qual_name,
                gen_namespace,
                functions,
            ):
                f_incl.write(line)
                f_incl.write("\n")

            f_impl.write("\n")
            f_impl.write("\n")
            f_impl.write("/" * 80 + "\n")
            for line in generate_impl(sorted_vars, gen_namespace, functions):
                f_impl.write(line)
                f_impl.write("\n")


def incl_preamble(includes: list[str]):
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

    for h in includes:
        yield f'#include "{h}"'


def generate_incl(
    sorted_vars: list[KeyedVar],
    struct_name: str,
    namespace: str,
    functions: list[CppFunc],
):
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
        yield f"  explicit {name}(const K& key):"
        yield f"    std::out_of_range({what}) {{}}"
        yield "};"
        yield ""
    yield ""

    if namespace:
        yield f"}} // namespace {namespace}"


def impl_preamble(includes: list[str]):
    for h in includes:
        yield f'#include "{h}"'
    yield ""


def generate_impl(
    sorted_vars: list[KeyedVar],
    namespace: str,
    functions: list[CppFunc],
):

    lookup = CppFunc(
        "inline size_t key_index(const K& key)",
        [
            "const auto lower = std::lower_bound("
            "sorted_keys.begin(), sorted_keys.end(), key);",
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
    def keys_switch(
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
            comment += "\nThrows `invalid_key` exception on unknown key."
        if non_optional_key:
            comment += "\nThrows `non_optional_key` exception on non-optional key."
        return comment

    yield CppFunc(
        "std::optional<V> get(const S& s, const K& key)",
        keys_switch(lambda o: f"return s.{o.id};"),
        "Get a value by key." + throws(invlaid_key=True),
    )

    yield CppFunc(
        "void set(S& s, const K& key, const V& value)",
        keys_switch(
            lambda o: f"s.{o.id} = std::get<{o.var.canonical_type}>(value); break;"
        ),
        "Set a value by key." + throws(invlaid_key=True),
    )

    yield CppFunc(
        "void unset(S& s, const K& key)",
        keys_switch(
            lambda o: f"s.{o.id} = std::nullopt; break;",
            default="throw non_optional_key(key);",
            only_optionals=True,
        ),
        "Unset an optional value by key."
        + throws(invlaid_key=True, non_optional_key=True),
    )

    yield CppFunc(
        "Diff diff(const S& current, const S& previous)",
        [
            "Diff d;",
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
        "void apply(S& s, const Diff& diff)",
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
        keys_switch(
            lambda o: f'return "{o.var.type}";',
        ),
        "Retrieve the type name of a value by key."
        + f"\nPossible return values are: {typenames}."
        + throws(invlaid_key=True),
    )

    for parser in parsers:
        yield CppFunc(
            f"V {parser.function_name}(const K& key, const {parser.type}& value)",
            keys_switch(
                lambda o: f"return {parser.user_function_for(o.var.type)}" + "(value);",
            ),
            f"Parse a value for a given key from `{parser.type}`."
            + throws(invlaid_key=True),
        )

    for formatter in formatters:
        yield CppFunc(
            f"{formatter.type} {formatter.function_name}(const K& key, const V& value)",
            keys_switch(
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
    var: Var

    @property
    def id(self):
        return self.var.identifier


@dataclass(frozen=True)
class Var:
    identifier: str
    type: str
    canonical_type: str
    is_optional: bool
    has_default: bool
    comment: str


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
    root: dict[str, Any] = {}
    for v in keyed_vars:
        node = root
        *parents, name = v.id.split(".")
        for parent in parents:
            node = node.setdefault(parent, {})
        node[name] = v

    def generate_keys_struct(
        node: Union[KeyedVar, dict[str, KeyedVar]], name: str, depth: int = 0
    ) -> Iterator[str]:
        indent = "  " * depth
        if isinstance(node, KeyedVar):
            yield indent + f'const K {name} = "{node.key}";'
        else:
            yield indent + f"const struct {name} {{"
            for k, v in node.items():
                yield from generate_keys_struct(v, k, depth + 1)
            yield indent + f"}} {name};"

    return "\n".join(generate_keys_struct(root, var_name))


if __name__ == "__main__":
    main()
