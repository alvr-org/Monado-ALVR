# Copyright 2019-2024, Collabora, Ltd.
# Copyright 2025, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

###
# Mock native compositor
#

add_library(comp_mock STATIC mock_compositor.cpp mock_compositor.h)
target_link_libraries(
	comp_mock
	PUBLIC xrt-interfaces comp_includes
	PRIVATE aux_util
	)
