#!/usr/bin/env python3
# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""Generate shader specialization structs and map entries from GLSL shaders.

Outputs are written to paths chosen by the caller.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Iterator, Literal, Mapping, Sequence

FieldNaming = Literal["CamelCase", "snake_case"]

DECORATE_SPEC_ID = re.compile(
    r"Decorate (?P<result_id>\d+)\((?P<name>\w+)\) SpecId (?P<id>\d+)"
)
SPEC_CONSTANT = re.compile(
    r"^\s*(?P<result_id>\d+)\((?P<name>\w+)\):\s+\d+\((?P<type>\w+)\)\s+"
    r"SpecConstant(?!Composite|Op)(?:True|False)?(?:\s+\S+)?\s*$"
)
NAME_RESULT = re.compile(r'^\s*Name (?P<result_id>\d+)  "(?P<name>[^"]+)"')
NAME_OPERAND = re.compile(r'^\s*Name (?P<result_id>\d+)\((?P<operand>[^)]+)\)  "(?P<alias>[^"]+)"')


@dataclass(frozen=True)
class SpecConstant:
    glsl_name: str
    constant_id: int
    spirv_type: str


@dataclass(frozen=True)
class ShaderEntry:
    struct_name: str
    short_name: str


@dataclass(frozen=True)
class ShaderSpec:
    shader_path: Path
    struct_name: str
    short_name: str
    constants: tuple[SpecConstant, ...]

    @classmethod
    def gather(
        cls,
        glslang: Path,
        shader_path: Path,
        include_dirs: Sequence[Path],
        shader_entries: Mapping[str, ShaderEntry],
    ) -> ShaderSpec:
        resolved_shader = shader_path.resolve()
        spirv_text = run_glslang(glslang, resolved_shader, include_dirs)
        constants = parse_spec_constants(spirv_text)
        entry = shader_entry_for_shader(resolved_shader, shader_entries)
        return cls(
            shader_path=resolved_shader,
            struct_name=entry.struct_name,
            short_name=entry.short_name,
            constants=tuple(constants),
        )

    def write_h_struct_lines(self, field_naming: FieldNaming) -> Iterator[str]:
        yield f"/* Generated from {self.shader_path.name} by generate_shader_specialization.py */"
        yield f"struct {self.struct_name}"
        yield "{"
        for constant in self.constants:
            field = field_name_for_constant(constant.glsl_name, field_naming)
            cpp_type = cpp_type_for_spirv(constant.spirv_type)
            yield f"\t/* {self.shader_path.name} {constant.glsl_name} (constant_id {constant.constant_id}). */"
            yield f"\t{cpp_type} {field};"
        yield "};"
        yield ""

    def write_hpp_map_lines(
        self,
        field_naming: FieldNaming,
        h_inc_name: str = "render_shader_specialization.h.inc",
    ) -> Iterator[str]:
        yield f"//! Generated from {self.shader_path.name} by generate_shader_specialization.py"
        yield f"//! Uses @c struct {self.struct_name} from @c {h_inc_name}."
        yield f"template <> struct ShaderSpecializationMap<{self.struct_name}>"
        yield "{"
        yield "\tstatic constexpr auto kEntries = std::to_array<VkSpecializationMapEntry>({"
        for constant in self.constants:
            field = field_name_for_constant(constant.glsl_name, field_naming)
            cpp_type = cpp_type_for_spirv(constant.spirv_type)
            yield "\t    {"
            yield f"\t        .constantID = {constant.constant_id},"
            yield f"\t        .offset = offsetof({self.struct_name}, {field}),"
            yield f"\t        .size = sizeof({cpp_type}),"
            yield "\t    },"
        yield "\t});"
        yield "};"
        yield ""


def snake_to_camel(name: str) -> str:
    if name.startswith("k_"):
        name = name[2:]
    parts = name.split("_")
    return parts[0] + "".join(part.capitalize() for part in parts[1:])


def field_name_for_constant(glsl_name: str, field_naming: FieldNaming) -> str:
    if field_naming == "CamelCase":
        return snake_to_camel(glsl_name)
    if glsl_name.startswith("k_"):
        return glsl_name[2:]
    return glsl_name


def default_struct_name_for_shader(shader_path: Path) -> str:
    return f"render_{shader_path.stem}_spec"


def default_short_name_for_shader(shader_path: Path) -> str:
    return f"render_{shader_path.stem}"


def shader_entry_for_shader(shader_path: Path, shader_entries: Mapping[str, ShaderEntry]) -> ShaderEntry:
    return shader_entries.get(
        shader_path.stem,
        ShaderEntry(
            struct_name=default_struct_name_for_shader(shader_path),
            short_name=default_short_name_for_shader(shader_path),
        ),
    )


def load_struct_names(path: Path) -> dict[str, ShaderEntry]:
    with path.open(encoding="utf-8") as fp:
        data = json.load(fp)
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object mapping shader stem to struct metadata")

    entries: dict[str, ShaderEntry] = {}
    for stem, value in data.items():
        stem_key = str(stem)
        if isinstance(value, str):
            struct_name = value
            short_name = default_short_name_for_shader(Path(f"{stem_key}.comp"))
        elif isinstance(value, dict):
            struct_name = value.get("struct", value.get("struct_name"))
            if not struct_name:
                raise ValueError(f"{path}: entry {stem_key!r} is missing a struct name")
            short_name = value.get("short_name")
            if not short_name:
                raise ValueError(f"{path}: entry {stem_key!r} is missing short_name")
        else:
            raise ValueError(f"{path}: entry {stem_key!r} must be a string or object")

        entries[stem_key] = ShaderEntry(str(struct_name), str(short_name))

    return entries


def cpp_type_for_spirv(spirv_type: str) -> str:
    mapping = {
        "bool": "VkBool32",
        "int": "int32_t",
        "uint": "uint32_t",
        "float": "float",
    }
    if spirv_type not in mapping:
        raise ValueError(f"Unsupported SPIR-V specialization type: {spirv_type}")
    return mapping[spirv_type]


def run_glslang(glslang: Path, shader: Path, include_dirs: Sequence[Path]) -> str:
    # Using SPIR-V 1.5 as external repos might use newer SPIR-V versions,
    # this doesn't effect shaders built into the compositor and they are
    # built elsewhere.
    cmd = [
        str(glslang),
        "-V",
        "--target-env",
        "spirv1.5",
        "-S",
        "comp",
        "-H",
        "-o",
        os.devnull,
    ]
    for include_dir in include_dirs:
        cmd.append(f"-I{include_dir}")
    cmd.append(str(shader))

    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
    except FileNotFoundError as exc:
        raise RuntimeError(f"glslang executable not found: {glslang}") from exc
    except subprocess.CalledProcessError as exc:
        message = exc.stderr.strip() or exc.stdout.strip() or str(exc)
        raise RuntimeError(f"{glslang} failed for {shader}:\n{message}") from exc

    return result.stdout


def collect_result_names(spirv_text: str) -> dict[int, set[str]]:
    names_by_result: dict[int, set[str]] = defaultdict(set)

    for line in spirv_text.splitlines():
        match = NAME_RESULT.match(line)
        if match:
            names_by_result[int(match.group("result_id"))].add(match.group("name"))
            continue

        match = NAME_OPERAND.match(line)
        if match:
            result_id = int(match.group("result_id"))
            names_by_result[result_id].add(match.group("operand"))
            names_by_result[result_id].add(match.group("alias"))

    return names_by_result


def choose_k_prefixed_name(result_id: int, names_by_result: dict[int, set[str]], *symbols: str) -> str:
    names = set(names_by_result.get(result_id, set()))
    names.update(symbol for symbol in symbols if symbol)

    k_names = sorted(name for name in names if name.startswith("k_"))
    if not k_names:
        raise RuntimeError(
            f"Specialization constant result id {result_id} has no k_-prefixed name "
            f"(found {sorted(names)}). Declare it as layout(constant_id = N) const ... k_name = ..."
        )
    if len(k_names) > 1:
        raise RuntimeError(
            f"Specialization constant result id {result_id} has ambiguous k_ names: {k_names}"
        )

    return k_names[0]


def parse_spec_constants(spirv_text: str) -> list[SpecConstant]:
    names_by_result = collect_result_names(spirv_text)

    constants: list[SpecConstant] = []
    for line in spirv_text.splitlines():
        match = SPEC_CONSTANT.match(line)
        if not match:
            continue

        result_id = int(match.group("result_id"))
        decorate_symbol = match.group("name")
        spirv_type = match.group("type")

        constant_id = None
        for decorate_match in DECORATE_SPEC_ID.finditer(spirv_text):
            if int(decorate_match.group("result_id")) != result_id:
                continue
            constant_id = int(decorate_match.group("id"))
            break

        if constant_id is None:
            continue

        glsl_name = choose_k_prefixed_name(
            result_id,
            names_by_result,
            decorate_symbol,
        )
        constants.append(
            SpecConstant(
                glsl_name=glsl_name,
                constant_id=constant_id,
                spirv_type=spirv_type,
            )
        )

    if not constants:
        raise RuntimeError("No specialization constants found in glslang SPIR-V output")

    return sorted(constants, key=lambda item: item.constant_id)


def gather_shader_specs(
    glslang: Path,
    shader_paths: Sequence[Path],
    include_dirs: Sequence[Path],
    shader_entries: Mapping[str, ShaderEntry],
) -> list[ShaderSpec]:
    specs: list[ShaderSpec] = []
    for shader_path in shader_paths:
        shader_include_dirs = [shader_path.parent, *include_dirs]
        specs.append(ShaderSpec.gather(glslang, shader_path, shader_include_dirs, shader_entries))
    return specs


def write_specialization_macro_lines(
    specs: Sequence[ShaderSpec],
    *,
    c_style: bool,
) -> Iterator[str]:
    def comment(text: str) -> str:
        if c_style:
            return f"/* {text} */"
        return f"//! {text}"

    yield comment("X-macro list of shader-generated specialization key structs.")
    yield "#define RENDER_SHADER_SPECIALIZATION_KEYS(X) \\"
    for index, spec in enumerate(specs):
        line = f"\tX({spec.struct_name})"
        if index + 1 < len(specs):
            line += " \\"
        yield line

    yield ""
    yield comment("X-macro list of short names for render pipeline caches.")
    yield "#define RENDER_SHADER_SPECIALIZATION_CACHE_NAMES(X) \\"
    for index, spec in enumerate(specs):
        line = f"\tX({spec.short_name})"
        if index + 1 < len(specs):
            line += " \\"
        yield line

    yield ""
    yield comment("X-macro with cache short name and spec struct name pairs.")
    yield "#define RENDER_SHADER_SPECIALIZATION_ENTRIES(X) \\"
    for index, spec in enumerate(specs):
        line = f"\tX({spec.short_name}, {spec.struct_name})"
        if index + 1 < len(specs):
            line += " \\"
        yield line


def generate_h_inc_lines(specs: Sequence[ShaderSpec], field_naming: FieldNaming) -> Iterator[str]:
    for spec in specs:
        yield from spec.write_h_struct_lines(field_naming)

    yield from write_specialization_macro_lines(specs, c_style=True)


def generate_hpp_inc_lines(
    specs: Sequence[ShaderSpec],
    field_naming: FieldNaming,
    h_inc_name: str = "render_shader_specialization.h.inc",
) -> Iterator[str]:
    for spec in specs:
        yield from spec.write_hpp_map_lines(field_naming, h_inc_name)

    yield from write_specialization_macro_lines(specs, c_style=False)


def write_generated_file(output_path: Path, lines: Iterable[str]) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as fp:
        fp.write("/* DO NOT EDIT - Auto-generated by generate_shader_specialization.py */\n")
        fp.write("\n")
        fp.write("\n".join(lines))
        fp.write("\n")


def write_specializations(
    glslang: Path,
    shader_paths: Sequence[Path],
    include_dirs: Sequence[Path],
    struct_names_path: Path,
    field_naming: FieldNaming,
    h_inc_output: Path,
    hpp_inc_output: Path,
) -> None:
    struct_names = load_struct_names(struct_names_path)
    specs = gather_shader_specs(glslang, shader_paths, include_dirs, struct_names)
    write_generated_file(h_inc_output.resolve(), generate_h_inc_lines(specs, field_naming))
    write_generated_file(
        hpp_inc_output.resolve(),
        generate_hpp_inc_lines(specs, field_naming, h_inc_output.name),
    )


def parse_field_naming(args: argparse.Namespace) -> FieldNaming:
    if args.CamelCase and args.snake_case:
        raise ValueError("Exactly one of --CamelCase or --snake_case is required")
    if not args.CamelCase and not args.snake_case:
        raise ValueError("Exactly one of --CamelCase or --snake_case is required")
    return "CamelCase" if args.CamelCase else "snake_case"


def run_self_tests() -> None:
    aliased_swapped_corner = """
                              Name 1074  "k_output_depth_encoding_swapped_upper_left_corner"
                              Name 1074(k_output_depth_encoding_swapped_upper_left_corner)  "swapped_upper_left_corner"
                              Decorate 1074(swapped_upper_left_corner) SpecId 0
1074(swapped_upper_left_corner):    13(bool) SpecConstantTrue
"""
    constants = parse_spec_constants(aliased_swapped_corner)
    assert len(constants) == 1
    assert constants[0].glsl_name == "k_output_depth_encoding_swapped_upper_left_corner"
    assert constants[0].constant_id == 0

    shader_entries = {
        "distortion": ShaderEntry("render_distortion_spec", "render_distortion"),
        "nv12": ShaderEntry("NV12Spec", "NV12"),
    }
    assert shader_entry_for_shader(Path("distortion.comp"), shader_entries).struct_name == "render_distortion_spec"
    assert shader_entry_for_shader(Path("distortion.comp"), shader_entries).short_name == "render_distortion"
    assert shader_entry_for_shader(Path("layer.comp"), shader_entries).struct_name == "render_layer_spec"
    assert shader_entry_for_shader(Path("layer.comp"), shader_entries).short_name == "render_layer"

    assert field_name_for_constant("k_do_timewarp", "snake_case") == "do_timewarp"
    assert field_name_for_constant("k_do_timewarp", "CamelCase") == "doTimewarp"

    distortion_shader = Path("distortion.comp")
    spec = ShaderSpec(
        shader_path=distortion_shader,
        struct_name="render_distortion_spec",
        short_name="render_distortion",
        constants=tuple(constants),
    )

    h_text = "\n".join(spec.write_h_struct_lines("snake_case"))
    assert "struct render_distortion_spec" in h_text
    assert "output_depth_encoding_swapped_upper_left_corner" in h_text
    assert "VkSpecializationMapEntry" not in h_text

    hpp_text = "\n".join(spec.write_hpp_map_lines("snake_case"))
    assert "ShaderSpecializationMap<render_distortion_spec>" in hpp_text
    assert "struct render_distortion_spec\n{" not in hpp_text
    assert "output_depth_encoding_swapped_upper_left_corner" in hpp_text
    assert "render_shader_specialization.h.inc" in hpp_text
    assert "ext_shader_specialization.h.inc" in "\n".join(
        spec.write_hpp_map_lines("snake_case", "ext_shader_specialization.h.inc")
    )

    consolidated_h = "\n".join(generate_h_inc_lines([spec], "snake_case"))
    assert "RENDER_SHADER_SPECIALIZATION_KEYS(X)" in consolidated_h
    assert "RENDER_SHADER_SPECIALIZATION_CACHE_NAMES(X)" in consolidated_h
    assert "RENDER_SHADER_SPECIALIZATION_ENTRIES(X)" in consolidated_h
    assert "X(render_distortion, render_distortion_spec)" in consolidated_h
    assert consolidated_h.count("struct render_distortion_spec") == 1

    consolidated_hpp = "\n".join(generate_hpp_inc_lines([spec], "snake_case"))
    assert "RENDER_SHADER_SPECIALIZATION_KEYS(X)" in consolidated_hpp
    assert "RENDER_SHADER_SPECIALIZATION_CACHE_NAMES(X)" in consolidated_hpp
    assert "RENDER_SHADER_SPECIALIZATION_ENTRIES(X)" in consolidated_hpp
    assert consolidated_hpp.count("ShaderSpecializationMap<render_distortion_spec>") == 1

    try:
        parse_spec_constants("""
                              Name 42  "not_k_prefixed"
                              Decorate 42(not_k_prefixed) SpecId 0
42(not_k_prefixed):    13(bool) SpecConstantTrue
""")
    except RuntimeError as exc:
        assert "no k_-prefixed name" in str(exc)
    else:
        raise AssertionError("expected missing k_ prefix to fail")

    try:
        parse_spec_constants("""
                              Name 42  "k_alpha"
                              Name 42(k_alpha)  "k_beta"
                              Decorate 42(k_alpha) SpecId 0
42(k_alpha):    13(bool) SpecConstantTrue
""")
    except RuntimeError as exc:
        assert "ambiguous k_ names" in str(exc)
    else:
        raise AssertionError("expected ambiguous k_ names to fail")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate shader specialization structs from GLSL.")
    parser.add_argument(
        "--glslang",
        type=Path,
        help="Path to glslangValidator",
    )
    parser.add_argument("--shader", type=Path, action="append", help="Path to a .comp shader")
    parser.add_argument(
        "--include-dir",
        type=Path,
        action="append",
        default=[],
        help="Include directory passed to glslangValidator (-I)",
    )
    parser.add_argument(
        "--struct-names",
        type=Path,
        help="JSON file mapping shader stem to struct name",
    )
    parser.add_argument(
        "--output-h-inc",
        type=Path,
        help="Output .h.inc path for C structs and RENDER_SHADER_SPECIALIZATION_KEYS",
    )
    parser.add_argument(
        "--output-hpp-inc",
        type=Path,
        help="Output .hpp.inc path for C++ ShaderSpecializationMap specializations",
    )
    parser.add_argument(
        "--CamelCase",
        action="store_true",
        help="Use CamelCase field names in generated structs",
    )
    parser.add_argument(
        "--snake_case",
        action="store_true",
        help="Use snake_case field names in generated structs",
    )
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="Run built-in parser tests and exit",
    )
    args = parser.parse_args()

    if args.self_test:
        run_self_tests()
        return 0

    if not args.shader:
        parser.error("At least one --shader is required")
    if not args.glslang:
        parser.error("--glslang is required")
    if not args.struct_names:
        parser.error("--struct-names is required")
    if not args.output_h_inc or not args.output_hpp_inc:
        parser.error("Both --output-h-inc and --output-hpp-inc are required")

    try:
        field_naming = parse_field_naming(args)
    except ValueError as exc:
        parser.error(str(exc))

    write_specializations(
        args.glslang,
        args.shader,
        args.include_dir,
        args.struct_names,
        field_naming,
        args.output_h_inc,
        args.output_hpp_inc,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
