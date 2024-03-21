#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: 2018-2024, Collabora, Ltd. and the Monado contributors

# This runs the command in the README as an extra bit of continuous integration.
set -e

(
    cd "$(dirname "$0")"
    # Getting the path set up right for pipx in CI is a hassle.
    # This script is usage on the CI itself only!
    sh -c "$(grep '^pipx' README.md | sed 's/pipx install/python3 -m pip install --break-system-packages/' )"
)
