# Copyright 2022-2023, Collabora, Ltd.
# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

add_library(target_helpers STATIC target_builder_helpers.c target_builder_helpers.h)
target_link_libraries(
	target_helpers
	PRIVATE
		xrt-interfaces
		aux_util
		aux_util_sink
		drv_includes
		base
	)
target_include_directories(target_helpers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
