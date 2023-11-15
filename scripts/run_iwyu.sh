#!/bin/sh
# Copyright 2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

(
    set -e
    cd "$(dirname $0)" && cd ..
    extra_args=""
    for fn in /usr/share/include-what-you-use/iwyu.gcc.imp /usr/share/include-what-you-use/*intrinsics.imp /usr/share/include-what-you-use/libcxx*.imp; do
        extra_args="${extra_args} -Xiwyu --mapping_file=$fn"
    done
    iwyu_tool -p build src/xrt/ "$@" -- -Xiwyu "--mapping_file=$(pwd)/scripts/mapping.imp" ${extra_args} | tee iwyu.txt
)
