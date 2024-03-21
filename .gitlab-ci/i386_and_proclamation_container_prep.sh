#!/bin/bash
# Copyright 2018-2024, Collabora, Ltd. and the Monado contributors
# SPDX-License-Identifier: BSL-1.0

(
    cd $(dirname $0)
    bash ./install-cross.sh
    # Using this script "follows the instructions" for some testing of our instructions.
    bash ./install-ci-fairy.sh
)

# Getting the path set up right for pipx in CI is a hassle.
python3 -m pip install --break-system-packages proclamation cmakelang
