#!/bin/sh
# Copyright 2019-2024, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0
# Author: Rylie Pavlik <rylie.pavlik@collabora.com>

# Formats all the source files in this project

set -e

if [ ! "${CLANGFORMAT}" ]; then
        for fn in clang-format-14 clang-format-13 clang-format-12 clang-format-11 clang-format-10 clang-format-9 clang-format-8 clang-format-7 clang-format-6.0 clang-format; do
                if command -v $fn > /dev/null; then
                        CLANGFORMAT=$fn
                        break
                fi
        done
fi

if [ ! "${CLANGFORMAT}" ]; then
        echo "We need some version of clang-format, please install one!" 1>&2
        exit 1
fi

(
        ${CLANGFORMAT} --version

        cd "$(dirname "$0")/.."

        find \
                src/xrt/auxiliary \
                src/xrt/compositor \
                src/xrt/drivers \
                src/xrt/include \
                src/xrt/ipc \
                src/xrt/state_trackers \
                src/xrt/targets \
                src/xrt/tracking \
                tests \
                \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
                -and -not \( -ipath \*/.cxx/\* \) \
                -exec "${CLANGFORMAT}" -i -style=file \{\} +
)
