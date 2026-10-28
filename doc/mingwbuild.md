# Building on Windows, using Mingw64/Clang64 on Msys2 {#mingwbuild}

<!--
Copyright 2026, Collabora, Ltd. and the Monado contributors
SPDX-License-Identifier: BSL-1.0
-->

[TOC]

Note: checked with Msys2-202610322 on Windows 11 25H2.

## Installing the build environment

- Download the [msys2][] installer and execute it
- Using the console opened at the end of the installation, update the environment:  
  ```
  pacman -Syu
  ```

- From now on, from the `Start Menu`, select `MSYS2 CLANG64` to open a console using the `clang64` environment

- Finish the update  
  ```
  pacman -Su
  ```

- Install the clang64 toolchain (choose to install `all` members)  
  ```
  pacman -S git make mingw-w64-clang-x86_64-toolchain mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-gcc-compat
  ```

[msys2]: https://msys2.org

## Installing Monado Dependencies

```
pacman -S mingw-w64-clang-x86_64-vulkan-devel mingw-w64-clang-x86_64-glslang mingw-w64-clang-x86_64-eigen3 mingw-w64-clang-x86_64-hidapi mingw-w64-clang-x86_64-libusb mingw-w64-clang-x86_64-libjpeg-turbo mingw-w64-clang-x86_64-cjson mingw-w64-clang-x86_64-gstreamer mingw-w64-clang-x86_64-gst-plugins-base mingw-w64-clang-x86_64-SDL2
```


## Building

```
cmake -G "Unix Makefiles" -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF -DPython3_EXECUTABLE=/clang64/bin/python.exe
make -C build -j $(nproc)
make -C build install
```

## Select the Monado runtime manifest

The manifest file is installed in `C:\msys64\usr\openxr_monado.json`. It can be made active by changing a Windows Registry Key (more information [here][manifest_location]), or more easily using [xrpicker-gui.exe][]. It can also be set, temporarily, from within a shell before to run an OpenXR client:

- in a `Cmdtool`, use: `set XR_RUNTIME_JSON=C:\msys64\usr\openxr_monado.json`,

- in a `PowerShell`, use: `$env:XR_RUNTIME_JSON = "C:\msys64\usr\openxr_monado.json"`

[manifest_location]: https://registry.khronos.org/OpenXR/specs/1.1/loader.html#implicit-vs-explicit-api-layers
[xrpicker-gui.exe]: https://github.com/rpavlik/xr-picker

## Running from a non-msys64 environment

The built targets are installed in `C:\msys64\usr\bin`, the needed dlls are in `C:\msys64\clang64\bin`. To run a target from a non-msys64 environment, those paths have to be added to the Windows PATH environment variable.

- to run a target by a double-click on its icon in the File Explorer, use the `System Properties` panel to permanently change the content of the user's or system's `Path`,

- to run a target from a `Cmdtool`, the PATH can be set, temporally, using: `set PATH=C:\msys64\usr\bin;C:\msys64\clang64\bin;%PATH%`

- to run a target from a `PowerShell`, the PATH can be set, temporally, using: `$env:PATH = "C:\msys64\usr\bin;C:\msys64\clang64\bin;"  + $env:PATH`

## Limitation

Currently (2026-06-04), the OpenXR clients and `monado-service` must be run from the same environment (due to the way the IPC `pipe` is currently created):

- if `monado-service` is run from a Windows console (`Cmdtool` or `PowerShell`) or from the File Manager, the OpenXR client must also be run from a Windows console or from the File Manager

- if `monado-service` is run from a Mingw64 console, the OpenXR client must also be run from a Mingw64 console.

## Gitlab Shell Worker configuration

To build Monado from a Gitlab Windows Shell Worker, some configurations are needed on the host (in addition to above installation instructions):

- add `C:\msys64\usr\bin` to the `Path` system envvar

- set `MSYSTEM=CLANG64` in the system envvars (this must be set prior to the execution of the shell worker, hence the use of a system-wide envvar)

- in the runner's config.toml, set:

  ```
  [[runners]]
      executor = "shell"
      shell = "bash"
      environment = ["ACLOCAL_PATH=/clang64/share/aclocal"]
  ```

