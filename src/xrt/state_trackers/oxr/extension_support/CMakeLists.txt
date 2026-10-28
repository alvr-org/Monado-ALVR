# Copyright 2019-2024, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/oxr_extension_support.h"
	COMMAND
		${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/oxr_extension_support.py
		"${CMAKE_CURRENT_BINARY_DIR}/oxr_extension_support.h"
	VERBATIM
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/oxr_extension_support.py
		${CMAKE_CURRENT_SOURCE_DIR}/oxr_extension_support.h.template
	COMMENT "Generating oxr_extension_support.h"
	)

add_custom_target(
	generate_oxr_extension_support
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/oxr_extension_support.h"
	)
