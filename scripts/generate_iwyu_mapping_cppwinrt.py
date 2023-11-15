#!/usr/bin/env python3
# Copyright 2022-2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0
#
# Original author: Rylie Pavlik <rylie.pavlik@collabora.com
"""Script to set up cppwinrt.imp for include-what-you-use."""
from pathlib import Path
import sys
import re
from generate_iwyu_mapping import write_mapping_file

_SCRIPT_DIR = Path(__file__).parent.resolve()
_OUTPUT_FILENAME = _SCRIPT_DIR / "cppwinrt.imp"

header_guard = re.compile(r"#ifndef WINRT_([a-zA-Z0-9_]+)_H\s*")


def find_namespace_name(header: Path) -> str:
    with open(header, "r", encoding="utf-8") as fp:
        for line in fp:
            result = header_guard.match(line)
            if not result:
                continue
            define_insides = result.group(1)
            return define_insides.replace("_", ".")

    raise RuntimeError("Could not figure out namespace name for " + str(header))


def make_canonical_include(namespace_name: str) -> str:
    return "<winrt/%s.h>" % namespace_name


def make_private_include_pattern(namespace_name: str) -> str:
    def munge_character(c: str):
        if c == ".":
            return "[.]"
        if c.isupper():
            return "[%s%s]" % (c, c.lower())
        return c

    munged_namespace = "".join(munge_character(c) for c in namespace_name)
    return "@<winrt/impl/%s[.][0-9][.]h>" % munged_namespace


def get_all_cppwinrt_entries(cppwinrt_root: str):
    root = Path(cppwinrt_root)
    for header in root.glob("*.h"):
        namespace_name = find_namespace_name(header)
        pattern = make_private_include_pattern(namespace_name)
        canonical = make_canonical_include(namespace_name)
        yield """{ include: ["%s", "private", "%s", "public"] },""" % (
            pattern,
            canonical,
        )


def write_file(cppwinrt_root: str):
    entries = list(get_all_cppwinrt_entries(cppwinrt_root))
    write_mapping_file(entries, _OUTPUT_FILENAME, Path(__file__).resolve().name)


if __name__ == "__main__":
    write_file(sys.argv[1])
