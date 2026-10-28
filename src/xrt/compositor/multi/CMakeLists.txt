# Copyright 2019-2024, Collabora, Ltd.
# Copyright 2025, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

###
# Multi client compositor library
#

add_library(
	comp_multi STATIC comp_multi_compositor.c comp_multi_interface.h comp_multi_private.h
			  comp_multi_system.c
	)
target_link_libraries(
	comp_multi
	PUBLIC xrt-interfaces comp_includes
	PRIVATE aux_util aux_os
	)
