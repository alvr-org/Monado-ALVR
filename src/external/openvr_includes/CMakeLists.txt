# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/openvr_forward_macros.h"
	COMMAND
		${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_forward_macros.py "-i"
		"${CMAKE_CURRENT_SOURCE_DIR}/openvr_interfaces_unified.h" "-o"
		"${CMAKE_CURRENT_BINARY_DIR}/openvr_forward_macros.h" "--include"
		"openvr_interfaces_unified.h"
	VERBATIM
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/openvr_interfaces_unified.h
		${CMAKE_CURRENT_SOURCE_DIR}/generate_forward_macros.py
	COMMENT "Generating openvr_forward_macros.h"
	)

add_custom_target(
	generate_openvr_forward_macros
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/openvr_forward_macros.h"
	)

add_library(xrt-external-openvr INTERFACE)
target_include_directories(
	xrt-external-openvr SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/
					     ${CMAKE_CURRENT_BINARY_DIR}/
	)

add_dependencies(xrt-external-openvr generate_openvr_forward_macros)
