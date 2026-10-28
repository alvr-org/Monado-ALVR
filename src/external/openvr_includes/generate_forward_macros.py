#!/usr/bin/env python3
# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

"""Generate forwarding macros for versioned OpenVR interface implementations."""

from __future__ import annotations

import argparse
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path
import re
import sys


CLASS_PATTERN = re.compile(r"^\s*class\s+(?P<class_name>[A-Za-z_]\w*)\b")
VERSIONED_CLASS_PATTERN = re.compile(r"^(?P<family>[A-Za-z_]\w*)_(?P<version>[0-9][0-9A-Za-z]*)$")
FNTABLE_STRUCT_PATTERN = re.compile(r"^\s*struct\s+VR_(?P<class_name>[A-Za-z_]\w*)_FnTable\b")
METHOD_PATTERN = re.compile(
    r"^virtual\s+(?P<return_type>.*?)\s*(?P<method_name>~?[A-Za-z_]\w*)\s*\((?P<params>.*)\)\s*(?P<qualifiers>[^;=]*)=\s*0\s*;$"
)


@dataclass(frozen=True)
class MethodDefinition:
    family: str
    version: str
    class_name: str
    return_type: str
    method_name: str
    params_text: str
    qualifiers: str
    call_args: tuple[str, ...]


def label_sort_key(label: str) -> tuple[tuple[int | str, ...], str]:
    parts = re.findall(r"\d+|[A-Za-z]+", label)
    key = tuple(int(part) if part.isdigit() else part.lower() for part in parts)
    return key, label.lower()


def versioned_classes(header_text: str) -> Iterator[tuple[str, str, list[str]]]:
    current_class_name: str | None = None
    current_family: str | None = None
    current_lines: list[str] = []
    in_class = False

    for line in header_text.splitlines():
        if not in_class:
            match = CLASS_PATTERN.match(line)
            if match is None:
                continue

            class_name = match.group("class_name")
            family_match = VERSIONED_CLASS_PATTERN.match(class_name)
            if family_match is None:
                continue

            current_class_name = class_name
            current_family = family_match.group("family")
            current_lines = [line]
            in_class = True
            continue

        current_lines.append(line)
        if line.strip() == "};":
            assert current_class_name is not None
            assert current_family is not None
            yield current_family, current_class_name, current_lines
            current_class_name = None
            current_family = None
            current_lines = []
            in_class = False


def split_top_level(text: str, delimiter: str) -> Iterator[str]:
    current: list[str] = []
    angle_depth = 0
    paren_depth = 0
    bracket_depth = 0
    brace_depth = 0

    for char in text:
        if char == "<":
            angle_depth += 1
        elif char == ">" and angle_depth > 0:
            angle_depth -= 1
        elif char == "(":
            paren_depth += 1
        elif char == ")" and paren_depth > 0:
            paren_depth -= 1
        elif char == "[":
            bracket_depth += 1
        elif char == "]" and bracket_depth > 0:
            bracket_depth -= 1
        elif char == "{":
            brace_depth += 1
        elif char == "}" and brace_depth > 0:
            brace_depth -= 1

        if (
            char == delimiter
            and angle_depth == 0
            and paren_depth == 0
            and bracket_depth == 0
            and brace_depth == 0
        ):
            yield "".join(current).strip()
            current = []
            continue

        current.append(char)

    if current:
        yield "".join(current).strip()


def strip_default_value(param_text: str) -> str:
    return next(split_top_level(param_text, "="), param_text.strip())


def extract_param_name(param_text: str) -> str | None:
    stripped = strip_default_value(param_text)
    if not stripped or stripped == "void":
        return None

    identifiers = re.findall(r"[A-Za-z_]\w*", stripped)
    if not identifiers:
        return None

    ignored = {
        "const",
        "volatile",
        "struct",
        "class",
        "enum",
        "signed",
        "unsigned",
        "short",
        "long",
        "void",
        "bool",
        "char",
        "int",
        "float",
        "double",
        "auto",
        "typename",
        "decltype",
    }
    for identifier in reversed(identifiers):
        if identifier not in ignored:
            return identifier

    return identifiers[-1]


def parse_method(method_text: str, family: str, version: str, class_name: str) -> MethodDefinition | None:
    match = METHOD_PATTERN.match(method_text)
    if match is None:
        return None

    method_name = match.group("method_name")
    if method_name.startswith("~"):
        return None

    params_text = match.group("params").strip()
    params = [] if not params_text else split_top_level(params_text, ",")
    call_args = tuple(
        name
        for name in (extract_param_name(param) for param in params)
        if name is not None
    )

    return MethodDefinition(
        family=family,
        version=version,
        class_name=class_name,
        return_type=match.group("return_type").strip(),
        method_name=method_name,
        params_text=params_text,
        qualifiers=match.group("qualifiers").strip(),
        call_args=call_args,
    )


def class_methods(family: str, version: str, class_name: str, class_lines: list[str]) -> Iterator[MethodDefinition]:
    current: list[str] = []
    collecting = False

    for line in class_lines:
        stripped = line.strip()
        if not collecting and stripped.startswith("virtual "):
            collecting = True
            current = [stripped]
        elif collecting:
            current.append(stripped)

        if collecting and "= 0;" in stripped:
            method = parse_method(" ".join(current), family, version, class_name)
            if method is not None:
                yield method
            collecting = False
            current = []


def collect_fntable_classes(header_text: str) -> set[str]:
    return {m.group("class_name") for m in FNTABLE_STRUCT_PATTERN.finditer(header_text)}


def collect_methods_per_version(header_text: str) -> dict[str, list[MethodDefinition]]:
    result: dict[str, list[MethodDefinition]] = {}
    for family, class_name, class_lines in versioned_classes(header_text):
        family_match = VERSIONED_CLASS_PATTERN.match(class_name)
        assert family_match is not None
        version = family_match.group("version")
        result[class_name] = list(class_methods(family, version, class_name, class_lines))
    return result


def parse_fntable_member(member_text: str, class_name: str) -> MethodDefinition | None:
    stripped = member_text.strip()
    if not stripped or stripped == "{" or stripped == "};":
        return None

    family_match = VERSIONED_CLASS_PATTERN.match(class_name)
    if family_match is None:
        return None

    if not stripped.endswith(";") or "OPENVR_FNTABLE_CALLTYPE*" not in stripped:
        return None

    signature, params_part = stripped[:-1].split("(OPENVR_FNTABLE_CALLTYPE*", 1)
    method_name, params_text = params_part.split(")(", 1)
    params_text = params_text.rstrip(")")
    return_type = signature.strip()
    if not return_type:
        return None

    params = [] if not params_text else split_top_level(params_text, ",")
    call_args = tuple(
        name
        for name in (extract_param_name(param) for param in params)
        if name is not None
    )

    return MethodDefinition(
        family=family_match.group("family"),
        version=family_match.group("version"),
        class_name=class_name,
        return_type=return_type,
        method_name=method_name.strip(),
        params_text=params_text,
        qualifiers="",
        call_args=call_args,
    )


def collect_fntable_methods_per_version(header_text: str) -> dict[str, list[MethodDefinition]]:
    lines = header_text.splitlines()
    methods_by_class: dict[str, list[MethodDefinition]] = {}

    current_class_name: str | None = None
    current_lines: list[str] = []
    in_struct = False

    for line in lines:
        if not in_struct:
            match = FNTABLE_STRUCT_PATTERN.match(line)
            if match is None:
                continue

            current_class_name = match.group("class_name")
            current_lines = []
            in_struct = True
            continue

        current_lines.append(line.strip())
        if line.strip() == "};":
            assert current_class_name is not None
            methods: list[MethodDefinition] = []
            for member_line in current_lines:
                method = parse_fntable_member(member_line, current_class_name)
                if method is not None:
                    methods.append(method)
            methods_by_class[current_class_name] = methods
            current_class_name = None
            current_lines = []
            in_struct = False

    return methods_by_class


def collect_methods(header_text: str) -> tuple[dict[str, str], dict[str, list[MethodDefinition]]]:
    latest_class_by_family: dict[str, str] = {}
    methods_by_family: dict[str, dict[tuple[str, str, str, str], MethodDefinition]] = {}

    for family, class_name, class_lines in versioned_classes(header_text):
        family_match = VERSIONED_CLASS_PATTERN.match(class_name)
        assert family_match is not None
        version = family_match.group("version")

        previous_class = latest_class_by_family.get(family)
        if previous_class is None:
            latest_class_by_family[family] = class_name
        else:
            previous_version = VERSIONED_CLASS_PATTERN.match(previous_class).group("version")
            if label_sort_key(version) > label_sort_key(previous_version):
                latest_class_by_family[family] = class_name

        family_methods = methods_by_family.setdefault(family, {})
        for method in class_methods(family, version, class_name, class_lines):
            key = (
                method.method_name,
                method.return_type,
                method.params_text,
                method.qualifiers,
            )
            family_methods[key] = method

    normalized_methods = {
        family: sorted(
            unique_methods.values(),
            key=lambda method: (
                method.method_name.lower(),
                method.params_text,
                method.return_type,
                method.qualifiers,
            ),
        )
        for family, unique_methods in methods_by_family.items()
    }
    return latest_class_by_family, normalized_methods


def params_without_defaults(params_text: str) -> str:
    if not params_text:
        return params_text
    return ", ".join(strip_default_value(p) for p in split_top_level(params_text, ","))


def normalize_type_text(type_text: str) -> str:
    return re.sub(r"\s+", " ", type_text).strip()


def fntable_lambda(method: MethodDefinition, class_method: MethodDefinition | None) -> str:
    params = params_without_defaults(method.params_text)
    call_args = ", ".join(method.call_args)
    call_expression = f"s_instance->{method.method_name}( {call_args} )"

    if method.return_type == "void":
        return f"[]( {params} ) {{ {call_expression}; }}"

    class_return_type = method.return_type if class_method is None else class_method.return_type
    if (
        "const" in normalize_type_text(class_return_type)
        and "const" not in normalize_type_text(method.return_type)
        and normalize_type_text(class_return_type).replace("const ", "")
        == normalize_type_text(method.return_type).replace("const ", "")
    ):
        call_expression = f"const_cast<{method.return_type}>({call_expression})"

    return f"[]( {params} ) -> {method.return_type} {{ return {call_expression}; }}"


def generate_fntable_macro(
    class_name: str, methods: list[MethodDefinition], class_methods: list[MethodDefinition]
) -> Iterator[str]:
    class_methods_by_name = {method.method_name: method for method in class_methods}
    struct_type = f"vr::VR_{class_name}_FnTable"
    instance_type = f"vr::{class_name}"
    yield f"// Usage: place FnTable_{class_name}() inside the body of a class inheriting {class_name}"
    yield f"#define FnTable_{class_name}() \\"
    yield f"    void *getFnTable() \\"
    yield f"    {{ \\"
    yield f"        static {instance_type} *s_instance = nullptr; \\"
    yield f"        static {struct_type} s_table{{ \\"
    for method in methods:
        yield f"            {fntable_lambda(method, class_methods_by_name.get(method.method_name))}, \\"
    yield f"        }}; \\"
    yield f"        s_instance = static_cast<{instance_type} *>(this); \\"
    yield f"        return &s_table; \\"
    yield f"    }}"


def macro_name(method: MethodDefinition, overload_suffix: str | None) -> str:
    if overload_suffix is None:
        return f"Forward_{method.family}_{method.method_name}"
    return f"Forward_{method.family}_{method.method_name}_{overload_suffix}"


def declaration_with_override(method: MethodDefinition) -> str:
    qualifier_suffix = f" {method.qualifiers}" if method.qualifiers else ""
    return (
        f"virtual {method.return_type} {method.method_name}( {method.params_text} )"
        f"{qualifier_suffix} override"
    )


def forward_body(method: MethodDefinition) -> str:
    call_args = ", ".join(method.call_args)
    qualifier_suffix = f" {method.qualifiers}" if method.qualifiers else ""
    call = f"ForwardBase_{method.family}::{method.method_name}( {call_args} )"
    if method.return_type == "void":
        return (
            f"virtual {method.return_type} {method.method_name}( {method.params_text} ){qualifier_suffix} override {{ \\\n"
            f"        {call}; \\\n"
            f"    }}"
        )

    return (
        f"virtual {method.return_type} {method.method_name}( {method.params_text} ){qualifier_suffix} override {{ \\\n"
        f"        return {call}; \\\n"
        f"    }}"
    )


def generate_header(
    header_name: str,
    latest_class_by_family: dict[str, str],
    methods_by_family: dict[str, list[MethodDefinition]],
    class_methods_per_version: dict[str, list[MethodDefinition]],
    fntable_methods_per_version: dict[str, list[MethodDefinition]],
) -> Iterator[str]:
    yield "//========= Copyright Valve Corporation ============//"
    yield ""
    yield "#pragma once"
    yield ""
    yield f'#include "{header_name}"'
    yield ""
    yield "// Generated forwarding macros for implementing older OpenVR interfaces on top of a newer concrete class."
    yield "// Usage pattern:"
    yield "//   ForwardDeclareBase_IVRSystem(XRTVRSystem_026)"
    yield "//   Forward_IVRSystem_GetRecommendedRenderTargetSize()"
    yield ""

    for family in sorted(methods_by_family):
        yield f"// {family} forwards to a caller-provided base, typically {latest_class_by_family[family]}."
        yield f"#define ForwardDeclareBase_{family}(BaseType) using ForwardBase_{family} = BaseType"
        yield ""

        methods = methods_by_family[family]
        overload_counts: dict[str, int] = {}
        overload_version_counts: dict[str, dict[str, int]] = {}
        for method in methods:
            overload_counts[method.method_name] = overload_counts.get(method.method_name, 0) + 1
            version_counts = overload_version_counts.setdefault(method.method_name, {})
            version_counts[method.version] = version_counts.get(method.version, 0) + 1

        overload_version_positions: dict[str, dict[str, int]] = {}
        for method in methods:
            suffix = None
            if overload_counts[method.method_name] > 1:
                suffix = method.version
                if overload_version_counts[method.method_name][method.version] > 1:
                    version_positions = overload_version_positions.setdefault(method.method_name, {})
                    version_positions[method.version] = version_positions.get(method.version, 0) + 1
                    suffix = f"{method.version}_{version_positions[method.version]}"

            macro = macro_name(method, suffix)
            yield f"// {method.class_name}::{method.method_name}"
            yield f"#define {macro}() \\"
            yield f"    {forward_body(method)}"
            yield ""

    fntable_class_names = sorted(
        class_name for class_name, methods in fntable_methods_per_version.items() if methods
    )
    if fntable_class_names:
        yield "// FnTable macros: implement getFnTable() for each versioned interface."
        yield "// Usage: place FnTable_IVRSystem_026() inside the body of a class inheriting IVRSystem_026."
        yield ""
        for class_name in fntable_class_names:
            yield from generate_fntable_macro(
                class_name,
                fntable_methods_per_version[class_name],
                class_methods_per_version.get(class_name, []),
            )
            yield ""


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate forwarding macros from the unified OpenVR interface header."
    )
    parser.add_argument(
        "-i",
        "--input-file",
        type=Path,
        required=True,
        help="Unified interface header to read.",
    )
    parser.add_argument(
        "-o",
        "--output-file",
        type=Path,
        required=True,
        help="Header file to write.",
    )
    parser.add_argument(
        "--include",
        required=True,
        help="Header to include at the top of the generated file.",
    )
    args = parser.parse_args()

    input_file = args.input_file.resolve()
    if not input_file.exists():
        parser.error(f"{input_file} does not exist")

    output_file = args.output_file.resolve()
    output_file.parent.mkdir(parents=True, exist_ok=True)

    header_text = input_file.read_text(encoding="utf-8")
    latest_class_by_family, methods_by_family = collect_methods(header_text)
    if not methods_by_family:
        print(f"No versioned interface methods found in {input_file}", file=sys.stderr)
        return 1

    class_methods_per_version = collect_methods_per_version(header_text)
    fntable_methods_per_version = collect_fntable_methods_per_version(header_text)

    include_name = args.include
    output_file.write_text(
        "\n".join(
            generate_header(
                include_name,
                latest_class_by_family,
                methods_by_family,
                class_methods_per_version,
                fntable_methods_per_version,
            )
        ).rstrip()
        + "\n",
        encoding="utf-8",
    )
    forward_count = sum(len(methods) for methods in methods_by_family.values())
    fntable_count = sum(1 for methods in fntable_methods_per_version.values() if methods)
    print(f"Wrote {forward_count} forwarding macro(s) and {fntable_count} FnTable macro(s) to {output_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
