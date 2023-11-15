#!/usr/bin/env python3
# Copyright 2022-2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0
#
# Original author: Rylie Pavlik <rylie.pavlik@collabora.com
"""Script to set up mapping.imp, etc for include-what-you-use."""
from pathlib import Path
from typing import List

_REPO = Path(__file__).parent.parent.resolve()
_SCRIPT_DIR = Path(__file__).parent.resolve()
_OUTPUT_FILENAME = _SCRIPT_DIR / "mapping.imp"

_FILE = Path(__file__).resolve()

_WRAPPERS = [
    """{ include: ["@[<\\"]vulkan/.+", "private", "\\"xrt/xrt_vulkan_includes.h\\"", "public"] },""",
    """{ include: ["\\"EGL/eglplatform.h\\"", "private", "\\"ogl/ogl_api.h\\"", "public"] },""",
    """{ include: ["<math.h>", "public", "\\"math/m_mathinclude.h\\"", "public"] },""",
    """{ include: ["<cmath>", "public", "\\"math/m_mathinclude.h\\"", "public"] },""",
    """{ symbol: ["M_PI", "public", "\\"math/m_mathinclude.h\\"", "public"] },""",
    """{ symbol: ["M_PIl", "public", "\\"math/m_mathinclude.h\\"", "public"] },""",
    """{ symbol: ["M_1_PI", "public", "\\"math/m_mathinclude.h\\"", "public"] },""",
    """{ symbol: ["ssize_t", "public", "\\"xrt/xrt_compiler.h\\"", "public"] },""",
]

_MISC_ERRORS = [
    """{ include: ["@[<\\"]bits/exception.h[>\\"]", "private", "<exception>", "public"] },""",
    """{ include: ["@[<\\"]ext/alloc_traits.h[>\\"]", "private", "<vector>", "public"] },""",
    """{ include: ["@[<\\"]bits/types/struct_iovec.h.", "private", "<sys/uio.h>", "public"] },""",
]

_MISC_DEPS = [
    """{ include: ["@[<\\"]jmoreconfig.h>\\"]", "private", "\\"jpeglib.h\\"", "public"] },""",
]

_OPENCV_ENTRIES = [
    """{ include: ["@<opencv2/core/.*.inl.hpp>", "private", "<opencv2/core.hpp>", "public"] },""",
    """{ include: ["@<opencv2/core/hal/.*", "private", "<opencv2/core.hpp>", "public"] },""",
]


def _generate_openxr_entries():
    for header in ("openxr_platform.h", "openxr_platform_defines.h"):
        yield """{ include: ["@[<\\"]openxr/%s.*", "private", "\\"xrt/xrt_openxr_includes.h\\"", "public"] },""" % header


def _generate_eigen_entries():
    for stubborn_header in ("ArrayBase", "MatrixBase", "DenseBase"):
        yield """{ include: ["@[<\\"]src/Core/%s.h[>\\"]", "private", "<Eigen/Core>", "public"] },""" % (
            stubborn_header,
        )
    for module in ("Core", "Geometry", "LU", "SparseCore", "Sparse"):
        yield """{ include: ["@[<\\"]Eigen/src/%s/.*", "private", "<Eigen/%s>", "public"] },""" % (
            module,
            module,
        )
        yield """{ include: ["@[<\\"]src/%s/.*", "private", "<Eigen/%s>", "public"] },""" % (
            module,
            module,
        )


_CONFIG_HEADERS = (
    "xrt_config_android.h",
    "xrt_config_build.h",
    "xrt_config_drivers.h",
    "xrt_config_have.h",
    "xrt_config_vulkan.h",
)


def _generate_config_header_defines():
    for header in _CONFIG_HEADERS:
        input_filename = header + ".cmake_in"
        input_file = _REPO / "src" / "xrt" / "include" / "xrt" / input_filename
        with open(str(input_file), "r", encoding="utf-8") as fp:
            for line in fp:
                if line.startswith("#cmakedefine"):
                    parts = line.split(" ")
                    symbol = parts[1].strip()
                    yield """{ symbol: ["%s", "public", "\\"xrt/%s\\"", "public"] },""" % (
                        symbol,
                        header,
                    )


def get_all_entries():
    entries = []
    entries.extend(_WRAPPERS)
    entries.extend(_MISC_ERRORS)
    entries.extend(_MISC_DEPS)
    # entries.extend(_OPENCV_ENTRIES)
    # entries.extend(_generate_eigen_entries())
    entries.extend(_generate_openxr_entries())
    entries.extend(_generate_config_header_defines())
    return entries


# REUSE-IgnoreStart


def _find_copyright_lines(fn: Path, *args):
    with open(fn, encoding="utf-8") as f:
        copyright_lines = [line.strip() for line in f if line.startswith("# Copyright")]

    known_lines = set(copyright_lines)
    for alt_fn in args:
        with open(_SCRIPT_DIR / alt_fn, encoding="utf-8") as f:
            new_lines = [line.strip() for line in f if line.startswith("# Copyright")]
        copyright_lines.extend(line for line in new_lines if line not in known_lines)
        known_lines.update(new_lines)

    return copyright_lines


def write_mapping_file(entries: List[str], output_filename: Path, script_name: str):
    """Write an IWYU mapping file with the given entries."""
    # Grab all lines from this and our other script.
    lines = _find_copyright_lines(_FILE, script_name)
    lines += [
        """#""",
        "# SPDX-License" + "-Identifier: BSL-1.0",  # split to avoid breaking REUSE tool
        """#""",
        """# GENERATED - edit %s instead of this file""" % script_name,
        "[",
    ]
    lines.extend(entries)
    lines += "]"
    content = "".join(line + "\n" for line in lines)
    with open(output_filename, "w", encoding="utf-8") as fp:
        fp.write(content)


# REUSE-IgnoreEnd


def write_file():
    my_script_fn = Path(__file__).resolve().name
    eigen_path = _SCRIPT_DIR / "eigen.imp"
    write_mapping_file(list(_generate_eigen_entries()), eigen_path, my_script_fn)

    opencv_path = _SCRIPT_DIR / "opencv.imp"
    write_mapping_file(_OPENCV_ENTRIES, opencv_path, my_script_fn)
    entries = get_all_entries()
    for ref_path in (eigen_path, opencv_path):
        entries.append("""{ ref: "%s" },""" % ref_path.name)
    write_mapping_file(entries, _OUTPUT_FILENAME, my_script_fn)


if __name__ == "__main__":
    write_file()
