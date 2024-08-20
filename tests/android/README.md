# Android device tests

<!--
Copyright 2024, Collabora, Ltd.

SPDX-License-Identifier: CC-BY-4.0
-->

This directory contains some tests to run on an Android device over ADB,
primarily to verify some lifecycle handling right now.

It uses [pytest][] as the test runner, which is more readable, maintainable, and
usable than the earlier bash scripts, and gives the option for e.g. junit-style
output for CI usage.

The actual tests are in the `test_*.py` files, while `conftest.py` configures
the pytest framework, as well as provides constants and fixtures for use in the
tests themselves.

[pytest]: https://pytest.org

## Preconditions

- Have `adb` in the path, and only the device under test connected. (or, have
  environment variables set appropriately so that
  `adb shell getprop ro.product.vendor.device` shows you the expected device.)
- Have Python and pytest installed, either system-wide or in a `venv`. The
  version of pytest in Debian Bookworm is suitable.
- Have built and installed the "outOfProcessDebug" build of Monado.
  (`./gradlew installOOPD` in root dir)
- Have set the out-of-process runtime as active in the OpenXR Runtime Broker (if
  applicable).
- Have installed both Vulkan and OpenGL ES versions of the `hello_xr` sample app
  from Khronos. (Binaries from a release are fine.)
- If you want the tests to pass, turn on "Draw over other apps" -- but this is
  actually a bug to require this.

## Instructions

Change your current working directory to this one. (Otherwise the extra
argument you need to enable the ADB tests will not be recognized by pytest.)

```sh
cd tests/android/
```

Run the following command, or one based on it:

```sh
pytest -v --adb
```

Or, if you want junit-style output, add a variation on these two args:
`--junit-xml=android-tests.xml -o junit_family=xunit1` for an overall command
like:

```sh
pytest -v --adb --junit-xml=android-tests.xml -o junit_family=xunit1
```

Another useful options is `-k` which can be followed by a pattern used to
selecet a subset of tests to run. Handy if you only want to run one or two
tests.
