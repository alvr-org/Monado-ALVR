#!/usr/bin/env python3
# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

"""Export versioned OpenVR headers from git tags."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys


TARGETS = (
    ("headers/openvr.h", "openvr_{version}.h"),
    ("headers/openvr_api.json", "openvr_api_{version}.json"),
    ("headers/openvr_capi.h", "openvr_capi_{version}.h"),
)
JSON_OUTPUT_PATTERN = "openvr_api_{version}.json"
HEADER_OUTPUT_PATTERN = "openvr_{version}.h"


def run_git(repo_root: Path, args: list[str], check: bool = True) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        ["git", *args],
        cwd=repo_root,
        check=check,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def normalize_version(tag: str) -> str:
    version = re.sub(r"^v[._-]*", "", tag.strip(), flags=re.IGNORECASE)
    if not version:
        raise ValueError(f"Cannot derive a version from tag '{tag}'")
    return f"v{version}"


def version_sort_key(version: str) -> tuple[tuple[int | str, ...], str]:
    normalized = version[1:] if version.startswith("v") else version
    parts = re.findall(r"\d+|[A-Za-z]+", normalized)
    key = tuple(int(part) if part.isdigit() else part.lower() for part in parts)
    return key, normalized.lower()


def collect_tags(repo_root: Path) -> list[tuple[str, str]]:
    result = run_git(repo_root, ["tag", "--list", "--sort=version:refname"])
    selected_tags: dict[str, str] = {}

    for raw_tag in result.stdout.decode("utf-8").splitlines():
        tag = raw_tag.strip()
        if not tag:
            continue
        selected_tags[normalize_version(tag)] = tag

    return [(version, tag) for version, tag in selected_tags.items()]


def read_tagged_file(repo_root: Path, tag: str, repo_path: str) -> bytes | None:
    result = run_git(repo_root, ["show", f"{tag}:{repo_path}"], check=False)
    if result.returncode != 0:
        return None
    return result.stdout


def export_tag(repo_root: Path, version: str, tag: str, output_dir: Path) -> bool:
    exported_any = False

    for repo_path, output_pattern in TARGETS:
        content = read_tagged_file(repo_root, tag, repo_path)
        if content is None:
            print(f"Skipping missing {repo_path} at tag {tag}", file=sys.stderr)
            continue

        output_path = output_dir / output_pattern.format(version=version)
        output_path.write_bytes(content)
        exported_any = True

    return exported_any


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
        const.get("constname")[:-len("_Version")]: const.get("constval")
        for const in payload.get("consts", [])
        if const.get("constname", "").endswith("_Version") and const.get("constval")
    }

    interface_versions = {
        class_name: const_versions[class_name]
        for class_name in sorted(class_names)
        if class_name in const_versions
    }
    if len(interface_versions) != len(class_names):
        return None

    return interface_versions


def deduplicate_output_dir(output_dir: Path, exported_versions: list[str]) -> int:
    signatures: dict[tuple[tuple[str, str], ...], str] = {}
    deleted_versions: list[str] = []

    for version in sorted(exported_versions, key=version_sort_key):
        json_path = output_dir / JSON_OUTPUT_PATTERN.format(version=version)
        if not json_path.exists():
            continue

        interface_versions = load_interface_versions(json_path)
        if interface_versions is None:
            continue

        signature = tuple(sorted(interface_versions.items()))
        previous_version = signatures.get(signature)
        signatures[signature] = version
        if previous_version is None:
            continue

        for path in (
            output_dir / HEADER_OUTPUT_PATTERN.format(version=previous_version),
            output_dir / JSON_OUTPUT_PATTERN.format(version=previous_version),
        ):
            try:
                path.unlink()
            except FileNotFoundError:
                pass

        deleted_versions.append(previous_version)
        print(
            f"Removed duplicate interface version set from {previous_version}; kept {version}",
            file=sys.stderr,
        )

    return len(deleted_versions)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Export versioned openvr.h and openvr_api.json files from all git tags."
    )
    parser.add_argument(
        "repo_dir",
        type=Path,
        help="Path to the OpenVR git repository.",
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        type=Path,
        default=None,
        help="Directory to write exported files into (default: <repo_dir>/headers/versioned)",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Only export the first N normalized versions after sorting, for testing.",
    )
    args = parser.parse_args()

    repo_root = args.repo_dir.resolve()
    if not (repo_root / ".git").exists():
        parser.error(f"{repo_root} does not look like a git repository")

    output_dir = (
        args.output_dir.resolve()
        if args.output_dir is not None
        else repo_root / "headers" / "versioned"
    )
    output_dir.mkdir(parents=True, exist_ok=True)

    tags = collect_tags(repo_root)
    if args.limit is not None:
        tags = tags[: args.limit]

    exported_versions = 0
    written_versions: list[str] = []
    for version, tag in tags:
        if export_tag(repo_root, version, tag, output_dir):
            exported_versions += 1
            written_versions.append(version)
            print(f"Exported {version} from {tag}")
        else:
            print(f"Skipped {tag}: neither target file exists", file=sys.stderr)

    removed_duplicates = deduplicate_output_dir(output_dir, written_versions)
    print(f"Wrote files for {exported_versions} version(s) into {output_dir}")
    if removed_duplicates:
        print(f"Removed {removed_duplicates} older duplicate version set(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
