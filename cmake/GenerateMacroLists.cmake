# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

#.rst:
# GenerateMacroLists
# ------------------
#
# Provides a function to generate X_macro lists from enum declarations in header
# files.
#
# This module parses C enum declarations and generates X_macro lists that can
# be used for iterating over enum values in C code. It's particularly useful
# for generating string conversion functions, validation code, or other
# enum-related utilities.
#
# Usage::
#
#   include(GenerateMacroLists)
#
#   generate_macro_lists(
#       OUTPUT <output_file>
#       INPUTS <header1> <header2> ...
#       ENUMS <enum1> <enum2> ...
#       [IGNORE_OTHER_ENUMS]
#       [VERBOSE]
#       [WORKING_DIRECTORY <dir>]
#   )
#
# Arguments:
#   OUTPUT              - Path to the generated output file (required)
#   INPUTS              - List of header files to scan for enum declarations (required)
#   ENUMS               - List of expected enum names (required)
#   IGNORE_OTHER_ENUMS  - Optional flag to ignore enums not in the ENUMS list
#   VERBOSE             - Optional flag to enable verbose output from the script
#   WORKING_DIRECTORY   - Optional working directory for the command (defaults to current source dir)
#
# The function automatically:
#   - Tracks dependencies so output is regenerated when inputs change
#   - Verifies that expected enums are found in the input headers
#   - Generates X_macro definitions for each enum
#
# Example::
#
#   generate_macro_lists(
#       OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xrt_macro_lists.h.inc"
#       INPUTS xrt_defines.h xrt_device.h xrt_results.h
#       ENUMS xrt_device_feature_type xrt_result xrt_device_name
#       IGNORE_OTHER_ENUMS
#       WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
#   )

function(generate_macro_lists)
	set(options IGNORE_OTHER_ENUMS VERBOSE)
	set(oneValueArgs OUTPUT WORKING_DIRECTORY)
	set(multiValueArgs INPUTS ENUMS)

	cmake_parse_arguments(
		GML "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
		)

	if(NOT GML_OUTPUT)
		message(FATAL_ERROR "generate_macro_lists: OUTPUT argument is required")
	endif()

	if(NOT GML_INPUTS)
		message(FATAL_ERROR "generate_macro_lists: INPUTS argument is required")
	endif()

	if(NOT GML_ENUMS)
		message(FATAL_ERROR "generate_macro_lists: ENUMS argument is required")
	endif()

	# Set working directory (default to current source dir)
	if(NOT GML_WORKING_DIRECTORY)
		set(GML_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
	endif()

	# Build command arguments
	set(GML_COMMAND
	    ${Python3_EXECUTABLE} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generate_macro_lists.py
	    --output "${GML_OUTPUT}" --inputs ${GML_INPUTS} --enums ${GML_ENUMS}
	    )

	if(GML_IGNORE_OTHER_ENUMS)
		list(APPEND GML_COMMAND --ignore-other-enums)
	endif()

	if(GML_VERBOSE)
		list(APPEND GML_COMMAND --verbose)
	endif()

	# Create custom command with proper dependencies
	add_custom_command(
		OUTPUT "${GML_OUTPUT}"
		WORKING_DIRECTORY "${GML_WORKING_DIRECTORY}"
		COMMAND ${GML_COMMAND}
		VERBATIM
		DEPENDS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generate_macro_lists.py ${GML_INPUTS}
		COMMENT "Generating X_macro lists in ${GML_OUTPUT}"
		)
endfunction()
