#!/usr/bin/env python3
# Copyright 2022-2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0
#
# Original author: Rylie Pavlik <rylie.pavlik@collabora.com
"""Script to set up windows.imp for include-what-you-use."""
from pathlib import Path
from generate_iwyu_mapping import write_mapping_file

_SCRIPT_DIR = Path(__file__).parent.resolve()
_OUTPUT_FILENAME = _SCRIPT_DIR / "windows.imp"


_SYMBOLS = {"IUnknown": "<Unknwn.h>"}


def get_all_entries():
    for sym, header in _SYMBOLS.items():
        yield """{ symbol: ["%s", "public", "%s", "public"] },""" % (
            sym,
            header,
        )


def write_file():
    entries = list(get_all_entries())
    write_mapping_file(entries, _OUTPUT_FILENAME, Path(__file__).resolve().name)


if __name__ == "__main__":
    write_file()
