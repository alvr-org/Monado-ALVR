#!/usr/bin/env python3
# Copyright 2024, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0
#
# Author: Rylie Pavlik <rylie.pavlik@collabora.com>
"""
Tests of app lifecycle behavior (app pause/resume and switch).

See README.md in this directory for requirements and running instructions.
"""

import time

import pytest
from conftest import helloxr_gles_activity, helloxr_vulkan_activity, helloxr_gles_pkg

# Ignore missing docstrings:
# flake8: noqa: D103

skipif_not_adb = pytest.mark.skipif(
    "not config.getoption('adb')", reason="--adb not passed to pytest"
)

# All the tests in this module require a device and ADB.
pytestmark = [pytest.mark.adb, skipif_not_adb]


def test_launch_and_back(adb):
    # Launch activity
    adb.start_activity(helloxr_gles_activity)
    time.sleep(2)

    # Press "Back"
    adb.send_key("KEYCODE_BACK")

    time.sleep(5)

    adb.check_for_crash()


def test_home_and_resume(adb):

    # Launch activity
    adb.start_activity(helloxr_gles_activity)
    time.sleep(2)

    # Press "Home"
    adb.send_key("KEYCODE_HOME")
    time.sleep(1)

    # Press "App Switch"
    adb.send_key("KEYCODE_APP_SWITCH")
    time.sleep(1)

    # Tap "somewhat middle" to re-select recent app
    adb.send_tap(400, 400)

    time.sleep(5)

    adb.check_for_crash()


def test_home_and_start(adb):

    # Launch activity
    adb.start_activity(helloxr_gles_activity)
    time.sleep(2)

    # Press "Home"
    adb.send_key("KEYCODE_HOME")
    time.sleep(2)

    # Launch activity again
    adb.start_activity(helloxr_gles_activity)

    time.sleep(5)

    adb.check_for_crash()


def test_launch_second(adb):

    # Launch A
    adb.start_activity(helloxr_gles_activity)
    time.sleep(2)

    # Launch B
    adb.start_activity(helloxr_vulkan_activity)

    time.sleep(5)

    adb.check_for_crash()


def test_home_and_launch_second(adb):

    # Launch A
    adb.start_activity(helloxr_gles_activity)
    time.sleep(2)

    # Press "Home"
    adb.send_key("KEYCODE_HOME")
    time.sleep(2)

    # Launch B
    adb.start_activity(helloxr_vulkan_activity)

    time.sleep(5)

    adb.check_for_crash()


def test_launch_a_b_a(adb):

    # Launch A
    adb.start_activity(helloxr_gles_activity)
    time.sleep(2)

    # Launch B
    adb.start_activity(helloxr_vulkan_activity)
    time.sleep(2)

    # Launch A (again)
    adb.start_activity(helloxr_gles_activity)

    time.sleep(5)

    adb.check_for_crash()
