#!/usr/bin/env python3
# Copyright 2022-2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0
#
# Original author: Rylie Pavlik <rylie.pavlik@collabora.com
"""Script to set up msvc.imp for include-what-you-use."""
from pathlib import Path
from generate_iwyu_mapping import write_mapping_file

_SCRIPT_DIR = Path(__file__).parent.resolve()
_OUTPUT_FILENAME = _SCRIPT_DIR / "msvc.imp"

_STL_EQUIVS = {
    "<xutility>": ["<utility>"],
    "<xstring>": ["<string>"],
    "<xatomic.h>": ["<atomic>"],
    "<xtr1common>": ["<type_traits>"],
    "<corecrt_math.h>": ["<math.h>", "<cmath>"],
    "<vcruntime_exceptions.h>": ["<exception>"],
    # This header contains common functionality used by a ton of containers
    "<xmemory>": [
        "<deque>",
        "<filesystem>",
        "<forward_list>",
        "<functional>",
        "<future>",
        "<hash_map>",
        "<hash_set>",
        "<list>",
        "<map>",
        "<memory>",
        "<set>",
        "<string>",
        "<unordered_map>",
        "<unordered_set>",
        "<vector>",
    ],
}


def get_all_stl_entries():
    for src, dests in _STL_EQUIVS.items():
        for dest in dests:
            yield """{ include: ["%s", "private", "%s", "public"] },""" % (
                src,
                dest,
            )


def write_file():
    entries = list(get_all_stl_entries())
    write_mapping_file(entries, _OUTPUT_FILENAME, Path(__file__).resolve().name)


if __name__ == "__main__":
    write_file()
