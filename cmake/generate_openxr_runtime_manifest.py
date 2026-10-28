#!/usr/bin/env python3
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""
Generate OpenXR runtime manifest JSON files.
"""

import argparse
import json
import sys
from pathlib import Path


def format_manifest_path(path: str, win32: bool) -> str:
    if win32:
        return path.replace("/", "\\")
    return path


def json_string_contents(value: str) -> str:
    """Return *value* escaped for use inside a JSON double-quoted string."""
    return json.dumps(value, ensure_ascii=False)[1:-1]


def join_install_path(install_prefix: str, *parts: str) -> str:
    path = Path(install_prefix)
    for part in parts:
        if part:
            path /= part
    return str(path)


def join_relative_manifest_path(relative_dir: str, filename: str) -> str:
    # Keep a directory component even when relative_dir is ".".
    # pathlib collapses Path(".") / "foo.dll" to "foo.dll", and a bare
    # filename makes the OpenXR loader search PATH/CWD instead of the
    # JSON directory. The previous CMake generator used
    # "${dir}/${filename}" which produced "./foo.dll".
    if relative_dir in ("", "."):
        return f"./{filename}"
    return str(Path(relative_dir) / filename)


def write_manifest(
    out_path: Path,
    library_path: str,
    libmonado_path: str | None,
    win32: bool,
    template_path: Path | None,
) -> None:
    library_path = format_manifest_path(library_path, win32)
    libmonado_path = (
        format_manifest_path(libmonado_path, win32) if libmonado_path else None
    )

    if template_path is not None:
        extra_fields = ""
        if libmonado_path is not None:
            extra_fields = (
                ',\n        "MND_libmonado_path": '
                f"{json.dumps(libmonado_path, ensure_ascii=False)}"
            )
        content = template_path.read_text(encoding="utf-8")
        content = content.replace("@target_path@", json_string_contents(library_path))
        content = content.replace("@extra_fields@", extra_fields)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(content, encoding="utf-8")
        return

    runtime = {
        "name": "Monado",
        "library_path": library_path,
    }
    if libmonado_path is not None:
        runtime["MND_libmonado_path"] = libmonado_path

    manifest = {
        "file_format_version": "1.0.0",
        "runtime": runtime,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as f:
        json.dump(manifest, f, indent="\t", ensure_ascii=False)
        f.write("\n")


def compute_installed_paths(args: argparse.Namespace) -> tuple[str, str | None]:
    runtime_filename = args.runtime_filename

    if args.path_type == "absolute":
        if not args.install_prefix:
            print(
                "Error: --install-prefix is required with --path-type absolute",
                file=sys.stderr,
            )
            sys.exit(1)
        library_path = join_install_path(
            args.install_prefix,
            args.relative_runtime_dir,
            runtime_filename,
        )
        libmonado_path = None
        if args.libmonado_filename:
            libmonado_path = join_install_path(
                args.install_prefix,
                args.relative_runtime_dir,
                args.libmonado_filename,
            )
    elif args.path_type == "relative":
        if not args.runtime_dir_relative_to_manifest:
            print(
                "Error: --runtime-dir-relative-to-manifest is required with "
                "--path-type relative",
                file=sys.stderr,
            )
            sys.exit(1)
        library_path = join_relative_manifest_path(
            args.runtime_dir_relative_to_manifest, runtime_filename
        )
        libmonado_path = None
        if args.libmonado_filename:
            libmonado_path = join_relative_manifest_path(
                args.runtime_dir_relative_to_manifest, args.libmonado_filename
            )
    else:
        library_path = runtime_filename
        libmonado_path = args.libmonado_filename

    return library_path, libmonado_path


def cmd_buildtree(args: argparse.Namespace) -> None:
    template_path = Path(args.template) if args.template else None
    write_manifest(
        Path(args.out_file),
        args.target_path,
        args.libmonado_path,
        args.win32,
        template_path,
    )


def cmd_installed(args: argparse.Namespace) -> None:
    library_path, libmonado_path = compute_installed_paths(args)
    template_path = Path(args.template) if args.template else None
    write_manifest(
        Path(args.out_file),
        library_path,
        libmonado_path,
        args.win32,
        template_path,
    )


def add_common_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--out-file", required=True)
    parser.add_argument(
        "--template",
        help="Optional manifest template with @target_path@ and @extra_fields@",
    )
    parser.add_argument(
        "--win32",
        action="store_true",
        help="Use backslashes in manifest paths",
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate OpenXR runtime manifest JSON files.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    buildtree = subparsers.add_parser(
        "buildtree",
        help="Generate a manifest with absolute paths for build-tree usage",
    )
    add_common_args(buildtree)
    buildtree.add_argument("--target-path", required=True)
    buildtree.add_argument("--libmonado-path")
    buildtree.set_defaults(func=cmd_buildtree)

    installed = subparsers.add_parser(
        "installed",
        help="Generate a manifest for the configured install layout",
    )
    add_common_args(installed)
    installed.add_argument(
        "--path-type",
        choices=["absolute", "relative", "soname"],
        default="soname",
        help="How to express the runtime library path in the manifest",
    )
    installed.add_argument("--install-prefix")
    installed.add_argument("--runtime-filename", required=True)
    installed.add_argument("--relative-runtime-dir", required=True)
    installed.add_argument("--libmonado-filename")
    installed.add_argument(
        "--runtime-dir-relative-to-manifest",
        help="Runtime directory relative to the manifest directory "
        "(required with --path-type relative)",
    )
    installed.set_defaults(func=cmd_installed)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    args.func(args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
