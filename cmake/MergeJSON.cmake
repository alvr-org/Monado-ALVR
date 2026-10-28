# Copyright 2025, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

# JSON merge function: merges multiple JSON files into one output file
#
# To use this function in your CMakeLists.txt:
#   include(${CMAKE_SOURCE_DIR}/scripts/CMakeLists.txt)
#
# Usage:
#   merge_json_files(
#       OUTPUT <output_file>
#       SOURCES <json_file1> <json_file2> ...
#       [ALLOW_OVERWRITE]
#       [IGNORE_SCHEMA]
#   )
#
# Arguments:
#   OUTPUT          - Output file path (required)
#   SOURCES         - List of JSON files to merge (required)
#   ALLOW_OVERWRITE - Optional flag to allow duplicate keys
#   IGNORE_SCHEMA   - Optional flag to ignore "$schema" field in output
#
# The function automatically:
#   - Sorts input files alphabetically for consistent results
#   - Tracks dependencies so output is regenerated when inputs change
#   - Errors on duplicate keys unless ALLOW_OVERWRITE is specified
#
# Example:
#   merge_json_files(
#       OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/merged.json"
#       SOURCES file1.json file2.json file3.json
#       IGNORE_SCHEMA
#   )

function(merge_json_files)
	set(options ALLOW_OVERWRITE IGNORE_SCHEMA)
	set(oneValueArgs OUTPUT)
	set(multiValueArgs SOURCES)

	cmake_parse_arguments(
		MERGE_JSON "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
		)

	if(NOT MERGE_JSON_OUTPUT)
		message(FATAL_ERROR "merge_json_files: OUTPUT argument is required")
	endif()

	if(NOT MERGE_JSON_SOURCES)
		message(FATAL_ERROR "merge_json_files: SOURCES argument is required")
	endif()

	# Build command arguments
	set(MERGE_COMMAND ${PYTHON_EXECUTABLE}
			  ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/merge_json.py -o
			  "${MERGE_JSON_OUTPUT}"
		)

	if(MERGE_JSON_ALLOW_OVERWRITE)
		list(APPEND MERGE_COMMAND --allow-overwrite)
	endif()

	if(MERGE_JSON_IGNORE_SCHEMA)
		list(APPEND MERGE_COMMAND --ignore-schema)
	endif()

	list(APPEND MERGE_COMMAND ${MERGE_JSON_SOURCES})

	# Create custom command with proper dependencies
	add_custom_command(
		OUTPUT "${MERGE_JSON_OUTPUT}"
		COMMAND ${MERGE_COMMAND}
		VERBATIM
		DEPENDS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/merge_json.py
			${MERGE_JSON_SOURCES}
		COMMENT "Merging JSON files into ${MERGE_JSON_OUTPUT}"
		)
endfunction()
