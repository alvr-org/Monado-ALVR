# Copyright 2024, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0
#
# Author: Rylie Pavlik <rylie.pavlik@collabora.com>
"""
pytest configuration and helpers for adb/Android.

Helper functions to run tests using adb on an Android device.

pytest serves as the test runner and collects the results.

See README.md in this directory for requirements and running instructions.
"""

import subprocess
import time
from pathlib import Path
from typing import Optional

import pytest

_NATIVE_ACTIVITY = "android.app.NativeActivity"
helloxr_vulkan_pkg = "org.khronos.openxr.hello_xr.vulkan"
helloxr_vulkan_activity = f"{helloxr_vulkan_pkg}/{_NATIVE_ACTIVITY}"
helloxr_gles_pkg = "org.khronos.openxr.hello_xr.opengles"
helloxr_gles_activity = f"{helloxr_gles_pkg}/{_NATIVE_ACTIVITY}"

RUNTIME = "org.freedesktop.monado.openxr_runtime.out_of_process"

_PKGS_TO_STOP = [
    helloxr_gles_pkg,
    helloxr_vulkan_pkg,
    "org.khronos.openxr.cts",
    "org.freedesktop.monado.openxr_runtime.in_process",
    RUNTIME,
]

_REPO_ROOT = Path(__file__).resolve().parent.parent.parent


class AdbTesting:
    """pytest fixture for testing on an Android device with adb."""

    def __init__(self, tmp_path: Optional[Path]):
        """Initialize members."""
        self.tmp_path = tmp_path
        self.logcat_grabbed = False
        self.logcat_echoed = False
        self.log: Optional[str] = None

        # TODO allow most of the following to be configured
        self._ndk_ver = "26.3.11579264"
        self._build_variant = "outOfProcessDebug"
        self._sdk_root = Path.home() / "Android" / "Sdk"
        self._arch = "arm64-v8a"
        self._ndk_stack = self._sdk_root / "ndk" / self._ndk_ver / "ndk-stack"
        self._caps_build_variant = (
            self._build_variant[0].upper() + self._build_variant[1:]
        )
        self._sym_dir = (
            _REPO_ROOT
            / "src"
            / "xrt"
            / "targets"
            / "openxr_android"
            / "build"
            / "intermediates"
            / "merged_native_libs"
            / self._build_variant
            / f"merge{self._caps_build_variant}NativeLibs"
            / "out"
            / "lib"
            / self._arch
        )

    def adb_call(self, cmd: list[str], **kwargs):
        """Call ADB in a subprocess."""
        return subprocess.check_call(cmd, **kwargs)

    def adb_check_output(self, cmd: list[str], **kwargs):
        """Call ADB in a subprocess and get the output."""
        return subprocess.check_output(cmd, encoding="utf-8", **kwargs)

    def get_device_state(self) -> Optional[str]:
        try:
            state = self.adb_check_output(["adb", "get-state"]).strip()
            return state.strip()

        except subprocess.CalledProcessError:
            return None

    def send_key(self, keyevent):
        """Send a key event with `adb shell input keyevent`."""
        time.sleep(1)
        print(f"*** {keyevent}")
        self.adb_call(["adb", "shell", "input", "keyevent", keyevent])

    def send_tap(self, x, y):
        """Send touchscreen tap at x, y."""
        #     sleep 1
        print(f"*** tap {x} {y}")
        self.adb_call(
            [
                "adb",
                "shell",
                "input",
                "touchscreen",
                "tap",
                str(x),
                str(y),
            ]
        )

    def start_activity(self, activity):
        """Start an activity and wait with `adb shell am start-activity -W`."""
        time.sleep(1)
        print(f"*** starting {activity} and waiting")
        self.adb_call(["adb", "shell", "am", "start-activity", "-W", activity])

    def stop_and_clear_all(self):
        """
        Stop relevant packages and clear their data.

        This second step removes them from recents.
        """
        print("*** stopping all relevant packages and clearing their data")
        for pkg in _PKGS_TO_STOP:
            self.adb_call(["adb", "shell", "am", "force-stop", pkg])
            self.adb_call(["adb", "shell", "pm", "clear", pkg])

    def start_test(
        self,
    ):
        """Ensure a clean starting setup."""
        self.stop_and_clear_all()
        self.adb_call(["adb", "logcat", "-c"])

    @property
    def logcat_fn(self):
        """Get computed path of logcat file."""
        assert self.tmp_path
        return self.tmp_path / "logcat.txt"

    def grab_logcat(self):
        """Save logcat to the named file and return it as a string."""
        log = self.adb_check_output(["adb", "logcat", "-d"])
        log_fn = self.logcat_fn
        with open(log_fn, "w", encoding="utf-8") as fp:
            fp.write(log)
        print(f"Logcat saved to {log_fn}")
        self.logcat_grabbed = True
        self.log = log
        return log

    def dump_logcat(self):
        """Print logcat to stdout."""
        if self.logcat_echoed:
            return
        if not self.logcat_grabbed:
            self.grab_logcat()
        print("*** Logcat:")
        print(self.log)
        self.logcat_echoed = True

    def check_for_crash(self):
        """Dump logcat, and look for a crash."""
        __tracebackhide__ = True
        log = self.grab_logcat()
        log_fn = self.logcat_fn
        crash_name = log_fn.with_name("crashes.txt")
        if "backtrace:" in log:
            subprocess.check_call(
                " ".join(
                    (
                        str(x)
                        for x in (
                            self._ndk_stack,
                            "-sym",
                            self._sym_dir,
                            "-i",
                            log_fn,
                            ">",
                            crash_name,
                        )
                    )
                ),
                shell=True,
            )
            self.dump_logcat()

            with open(crash_name, "r", encoding="utf-8") as fp:
                print("*** Crash Backtrace(s):")
                print(fp.read())
            print(f"*** Crash backtrace in {crash_name}")
            pytest.fail("Native crash backtrace detected in test")

        if "FATAL EXCEPTION" in log:
            self.dump_logcat()
            pytest.fail("Java exception detected in test")

        if "WindowManager: ANR" in log:
            self.dump_logcat()
            pytest.fail("ANR detected in test")

        self.stop_and_clear_all()


@pytest.fixture(scope="function")
def adb(tmp_path):
    """Provide AdbTesting instance and cleanup as pytest fixture."""
    adb_testing = AdbTesting(tmp_path)

    adb_testing.start_test()

    # Body of test happens here
    yield adb_testing

    # Grab logcat in case it wasn't grabbed, and dump to stdout
    adb_testing.dump_logcat()

    # Cleanup
    adb_testing.stop_and_clear_all()


@pytest.fixture(scope="session", autouse=True)
def log_device_and_build(record_testsuite_property):
    """Session fixture, autoused, to record device data."""
    adb = AdbTesting(None)

    try:
        device = adb.adb_check_output(
            ["adb", "shell", "getprop", "ro.product.vendor.device"]
        )
        build = adb.adb_check_output(
            ["adb", "shell", "getprop", "ro.product.build.fingerprint"]
        )
    except subprocess.CalledProcessError:
        pytest.skip("adb shell getprop failed - no device connected?")

    record_testsuite_property("device", device.strip())
    record_testsuite_property("build", build.strip())


def device_is_available():
    """Return True if an ADB device is available."""
    adb = AdbTesting(None)

    try:
        state = adb.adb_check_output(["adb", "get-state"]).strip()
        return state == "device"

    except subprocess.CalledProcessError:
        return False


def pytest_addoption(parser: pytest.Parser):
    """Pytest addoption function."""
    parser.addoption(
        "--adb",
        action="store_true",
        default=False,
        help="run ADB/Android device tests",
    )


def pytest_configure(config):
    """Configure function for pytest."""
    config.addinivalue_line(
        "markers",
        "adb: mark test as needing adb and thus only run when an "
        "Android device is available",
    )
