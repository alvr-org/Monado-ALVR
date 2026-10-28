# Copyright 2019-2025, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

# Binding generation: pass filename to generate
# Need to use ${CMAKE_COMMAND} to be able to set environment with -e
# Need to use $<TARGET_FILE:${PYTHON_EXECUTABLE}> to unwrap the cmake target to be able to run with ${CMAKE_COMMAND}
function(ovrd_bindings_gen output)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${output}"
		COMMAND
			${CMAKE_COMMAND} -E env
			"PYTHONPATH=$<SHELL_PATH:${AUX_BINDINGS_DIR};$ENV{PYTHONPATH}>"
			$<TARGET_FILE:${PYTHON_EXECUTABLE}>
			${CMAKE_CURRENT_SOURCE_DIR}/ovrd_bindings.py
			${AUX_BINDINGS_DIR}/bindings.json "${CMAKE_CURRENT_BINARY_DIR}/${output}"
		COMMAND_EXPAND_LISTS VERBATIM
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ovrd_bindings.py
			${AUX_BINDINGS_DIR}/bindings.json
		COMMENT "Generating ${output}"
		)
endfunction()

ovrd_bindings_gen(b_ovrd_generated_bindings.h)
ovrd_bindings_gen(b_ovrd_generated_bindings.c)

# Bindings library.
add_library(
	ovrd_generated_bindings STATIC ${CMAKE_CURRENT_BINARY_DIR}/b_ovrd_generated_bindings.c
				       ${CMAKE_CURRENT_BINARY_DIR}/b_ovrd_generated_bindings.h
	)

target_link_libraries(ovrd_generated_bindings PRIVATE xrt-interfaces)

# So that linking ovrd_generated_bindings makes it possible to include the generated header
target_include_directories(ovrd_generated_bindings PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
