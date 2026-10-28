#!/usr/bin/env python3
# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

"""Generate a unified header containing versioned OpenVR interface classes."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys


JSON_PATTERN = "openvr_api_v*.json"
HEADER_PATTERN = "openvr_{version}.h"
CAPI_HEADER_PATTERN = "openvr_capi_{version}.h"


@dataclass(frozen=True)
class InterfaceDefinition:
    class_name: str
    interface_version: str
    source_version: str
    header_path: Path
    fn_table_text: str | None = None


def version_sort_key(version: str) -> tuple[tuple[int | str, ...], str]:
    normalized = version[1:] if version.startswith("v") else version
    parts = re.findall(r"\d+|[A-Za-z]+", normalized)
    key = tuple(int(part) if part.isdigit() else part.lower() for part in parts)
    return key, normalized.lower()


def label_sort_key(label: str) -> tuple[tuple[int | str, ...], str]:
    parts = re.findall(r"\d+|[A-Za-z]+", label)
    key = tuple(int(part) if part.isdigit() else part.lower() for part in parts)
    return key, label.lower()


def read_header_text(path: Path) -> str:
    data = path.read_bytes()
    for encoding in ("utf-8", "cp1252", "latin-1"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            continue
    return data.decode("latin-1", errors="replace")


def load_interface_versions(json_path: Path) -> dict[str, str] | None:
    try:
        payload = json.loads(json_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None

    class_names = {
        method.get("classname", "").split("::")[-1]
        for method in payload.get("methods", [])
        if method.get("classname")
    }
    if not class_names:
        return None

    const_versions = {
        const.get("constname")[: -len("_Version")]: const.get("constval")
        for const in payload.get("consts", [])
        if const.get("constname", "").endswith("_Version") and const.get("constval")
    }

    interface_versions = {
        class_name: const_versions[class_name]
        for class_name in sorted(class_names)
        if class_name in const_versions
    }
    if not interface_versions:
        return None

    return interface_versions


def collect_interfaces(input_dir: Path) -> dict[str, InterfaceDefinition]:
    definitions: dict[str, InterfaceDefinition] = {}
    header_text_cache: dict[Path, str] = {}

    json_paths = sorted(
        input_dir.glob(JSON_PATTERN),
        key=lambda path: version_sort_key(path.stem.removeprefix("openvr_api_")),
    )
    for json_path in json_paths:
        source_version = json_path.stem.removeprefix("openvr_api_")
        header_path = input_dir / HEADER_PATTERN.format(version=source_version)
        if not header_path.exists():
            print(f"Skipping {json_path.name}: missing {header_path.name}", file=sys.stderr)
            continue

        interface_versions = load_interface_versions(json_path)
        if interface_versions is None:
            print(f"Skipping {json_path.name}: could not read interface versions", file=sys.stderr)
            continue

        capi_header_path = input_dir / CAPI_HEADER_PATTERN.format(version=source_version)
        capi_header_text: str | None = None
        if capi_header_path.exists():
            capi_header_text = read_header_text(capi_header_path)

        for class_name, interface_version in interface_versions.items():
            header_text = header_text_cache.get(header_path)
            if header_text is None:
                header_text = read_header_text(header_path)
                header_text_cache[header_path] = header_text

            if extract_class_definition(header_text, class_name) is None:
                continue

            fn_table_text: str | None = None
            if capi_header_text is not None:
                raw_fn_table = extract_struct_definition(capi_header_text, f"VR_{class_name}_FnTable")
                if raw_fn_table is not None:
                    renamed_fn_table = rename_fntable_definition(raw_fn_table, class_name, interface_version)
                    fn_table_text = normalize_class_definition(renamed_fn_table)

            definitions[interface_version] = InterfaceDefinition(
                class_name=class_name,
                interface_version=interface_version,
                source_version=source_version,
                header_path=header_path,
                fn_table_text=fn_table_text,
            )

    return definitions


def extract_class_definition(header_text: str, class_name: str) -> str | None:
    lines = header_text.splitlines(keepends=True)
    class_index = None

    class_pattern = re.compile(rf"^\s*class\s+{re.escape(class_name)}\b")
    for index, line in enumerate(lines):
        if class_pattern.search(line):
            class_index = index
            break

    if class_index is None:
        return None

    start_index = class_index
    previous = class_index - 1
    while previous >= 0 and not lines[previous].strip():
        previous -= 1

    if previous >= 0 and lines[previous].lstrip().startswith("*/"):
        start_index = previous
        while start_index >= 0 and "/**" not in lines[start_index]:
            start_index -= 1
        if start_index < 0:
            start_index = class_index
    elif previous >= 0 and lines[previous].lstrip().startswith("//"):
        start_index = previous
        while start_index > 0 and lines[start_index - 1].lstrip().startswith("//"):
            start_index -= 1

    end_index = None
    for index in range(class_index, len(lines)):
        if lines[index].strip() == "};":
            end_index = index
            break

    if end_index is None:
        return None

    return "".join(lines[start_index : end_index + 1]).rstrip()


def strip_comments(text: str) -> str:
    result: list[str] = []
    index = 0
    state = "normal"
    quote = ""

    while index < len(text):
        char = text[index]
        next_char = text[index + 1] if index + 1 < len(text) else ""

        if state == "normal":
            if char == "/" and next_char == "/":
                state = "line_comment"
                index += 2
                continue
            if char == "/" and next_char == "*":
                if result and not result[-1].isspace():
                    result.append(" ")
                state = "block_comment"
                index += 2
                continue
            if char in ('\"', "'"):
                state = "string"
                quote = char
                result.append(char)
                index += 1
                continue

            result.append(char)
            index += 1
            continue

        if state == "line_comment":
            if char == "\n":
                result.append("\n")
                state = "normal"
            index += 1
            continue

        if state == "block_comment":
            if char == "\n":
                result.append("\n")
                index += 1
                continue
            if char == "*" and next_char == "/":
                state = "normal"
                index += 2
                continue
            index += 1
            continue

        result.append(char)
        index += 1
        if char == "\\" and index < len(text):
            result.append(text[index])
            index += 1
            continue
        if char == quote:
            state = "normal"

    return "".join(result)


def normalize_class_definition(definition: str) -> str:
    without_comments = strip_comments(definition)
    normalized_lines = []
    for line in without_comments.splitlines():
        stripped_line = line.rstrip()
        if not stripped_line.strip():
            continue
        normalized_lines.append(stripped_line)

    return "\n".join(normalized_lines)


def extract_struct_definition(header_text: str, struct_name: str) -> str | None:
    lines = header_text.splitlines(keepends=True)
    struct_index = None
    struct_pattern = re.compile(rf"^\s*struct\s+{re.escape(struct_name)}\b")
    for index, line in enumerate(lines):
        if struct_pattern.search(line):
            struct_index = index
            break
    if struct_index is None:
        return None
    end_index = None
    for index in range(struct_index, len(lines)):
        if lines[index].strip() == "};":
            end_index = index
            break
    if end_index is None:
        return None
    return "".join(lines[struct_index:end_index + 1]).rstrip()


def rename_fntable_definition(definition: str, class_name: str, interface_version: str) -> str:
    old_name = f"VR_{class_name}_FnTable"
    new_name = f"VR_{interface_version}_FnTable"
    return re.sub(rf"\b{re.escape(old_name)}\b", new_name, definition)


def rename_class_definition(definition: str, class_name: str, interface_version: str) -> str:
    renamed = re.sub(
        rf"\bclass\s+{re.escape(class_name)}\b",
        f"class {interface_version}",
        definition,
        count=1,
    )
    return re.sub(
        rf"~{re.escape(class_name)}\b",
        f"~{interface_version}",
        renamed,
    )


def run_clang_format(path: Path) -> None:
    subprocess.run([
        "clang-format",
        "-style=file",
        "-i",
        str(path),
    ], check=True)


def generate_header(definitions: dict[str, InterfaceDefinition], include_name: str) -> str:
    header_text_cache: dict[Path, str] = {}
    blocks: list[tuple[str, str, str | None]] = []

    ordered_definitions = sorted(
        definitions.values(),
        key=lambda definition: (
            definition.class_name.lower(),
            label_sort_key(definition.interface_version),
        ),
    )

    for definition in ordered_definitions:
        header_text = header_text_cache.get(definition.header_path)
        if header_text is None:
            header_text = read_header_text(definition.header_path)
            header_text_cache[definition.header_path] = header_text

        class_definition = extract_class_definition(header_text, definition.class_name)
        if class_definition is None:
            print(
                f"Skipping {definition.interface_version}: could not find {definition.class_name} in {definition.header_path.name}",
                file=sys.stderr,
            )
            continue

        renamed_definition = rename_class_definition(
            class_definition,
            definition.class_name,
            definition.interface_version,
        )
        blocks.append(
            (definition.interface_version, normalize_class_definition(renamed_definition), definition.fn_table_text)
        )

    output_lines = [
        "//========= Copyright Valve Corporation ============//",
        "",
        "// Generated from versioned OpenVR headers.",
        "// Each interface version is copied from the newest header that still contains it.",
        "",
        "#pragma once",
        "",
        f'#include "{include_name}"',
        "#include \"openvr_old_types.h\"",
        "",
        "namespace vr",
        "{",
    ]

    for version_name, block, fn_table_text in blocks:
        output_lines.append(
            f'static const char * const {version_name}_Version = "{version_name}";'
        )
        output_lines.append("")
        output_lines.append(block)
        output_lines.append("")
        if fn_table_text is not None:
            output_lines.append(fn_table_text)
            output_lines.append("")
        output_lines.append("")

    if output_lines[-1] == "":
        output_lines.pop()
    if output_lines[-1] == "":
        output_lines.pop()

    output_lines.extend(("", "}", ""))
    return "\n".join(output_lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate a unified header containing all OpenVR interface versions."
    )
    parser.add_argument(
        "-i",
        "--input-dir",
        type=Path,
        required=True,
        help="Directory containing versioned openvr_*.h and openvr_api_*.json files.",
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
        default="openvr.h",
        help='Header to include at the top of the generated file (default: "openvr.h").',
    )
    args = parser.parse_args()

    input_dir = args.input_dir.resolve()
    if not input_dir.exists():
        parser.error(f"{input_dir} does not exist")

    output_file = args.output_file.resolve()
    output_file.parent.mkdir(parents=True, exist_ok=True)

    definitions = collect_interfaces(input_dir)
    if not definitions:
        print(f"No interface versions found in {input_dir}", file=sys.stderr)
        return 1

    output_file.write_text(generate_header(definitions, args.include), encoding="utf-8")
    try:
        run_clang_format(output_file)
    except FileNotFoundError:
        print("clang-format was not found in PATH", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError:
        print(f"clang-format failed for {output_file}", file=sys.stderr)
        return 1

    print(f"Wrote {len(definitions)} interface version definition(s) to {output_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
