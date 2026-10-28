# Copyright 2020, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

add_subdirectory(xrt)

add_library(xrt-interfaces INTERFACE)
target_include_directories(
	xrt-interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	)

# Make sure the xrt-interfaces target depends on the generated header
add_dependencies(xrt-interfaces generate_xrt_macro_lists)
