# Copyright 2020-2025, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

# The header-only part of Ceres: TinySolver, Jet and what they need.
add_library(xrt-external-tinyceres INTERFACE)
target_include_directories(
	xrt-external-tinyceres SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
	)
