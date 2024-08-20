#!/usr/bin/env python3
# Copyright 2024, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0
#
# Author: Rylie Pavlik <rylie.pavlik@collabora.com>
"""
Tests of a single app without inducing extra session state changes.

See README.md in this directory for requirements and running instructions.
"""

import time

import pytest
from conftest import helloxr_gles_activity, helloxr_gles_pkg

# Ignore missing docstrings:
# flake8: noqa: D103

skipif_not_adb = pytest.mark.skipif(
    "not config.getoption('adb')", reason="--adb not passed to pytest"
)

# All the tests in this module require a device and ADB.
pytestmark = [pytest.mark.adb, skipif_not_adb]


def test_just_launch(adb):
    # Just launch activity and make sure it starts OK.
    adb.start_activity(helloxr_gles_activity)

    time.sleep(5)

    adb.check_for_crash()


def test_launch_and_monkey(adb):
    # Launch activity
    adb.start_activity(helloxr_gles_activity)
    time.sleep(2)

    # Release the monkey! 1k events.
    adb.adb_call(
        [
            "adb",
            "shell",
            "monkey",
            "-p",
            helloxr_gles_pkg,
            "-v",
            "1000",
            # seed
            "-s",
            "100",
            "--pct-syskeys",
            "0",
        ]
    )

    adb.check_for_crash()
