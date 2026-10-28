# Copyright 2019, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0

find_program(GLSLANGVALIDATOR_COMMAND
	glslangValidator)
if(NOT GLSLANGVALIDATOR_COMMAND)
	message(FATAL_ERROR "glslangValidator required - source maintained at https://github.com/KhronosGroup/glslang")
endif()

#
# Generate SPIR-V header files from the arguments. Returns a list of headers.
# Options: SPIRV_VERSION, SOURCES, DEPENDS, DEBUG (emit -gVS: shader debug info and embedded shader source file+line).
#
function(spirv_shaders ret)
	set(options DEBUG)
	set(oneValueArgs SPIRV_VERSION)
	set(multiValueArgs SOURCES DEPENDS INCLUDE_DIRS)
	cmake_parse_arguments(_spirvshaders "${options}" "${oneValueArgs}"
	                      "${multiValueArgs}" ${ARGN})

	if(NOT _spirvshaders_SPIRV_VERSION)
		set(_spirvshaders_SPIRV_VERSION 1.0)
	endif()

	# Debug: pass DEBUG to spirv_shaders() to add -gVS.
	set(EXTRA_FLAGS "")
	if(_spirvshaders_DEBUG)
		# -gVS embeds shader debug info and source so debuggers get file:line and source.
		list(APPEND EXTRA_FLAGS "-gVS")
	endif()

	foreach(GLSL ${_spirvshaders_SOURCES})
		string(MAKE_C_IDENTIFIER ${GLSL} IDENTIFIER)
		set(HEADER "${CMAKE_CURRENT_BINARY_DIR}/${GLSL}.h")
		set(GLSL "${CMAKE_CURRENT_SOURCE_DIR}/${GLSL}")

		add_custom_command(
			OUTPUT ${HEADER}
			COMMAND ${GLSLANGVALIDATOR_COMMAND} -V
				"$<$<BOOL:${_spirvshaders_INCLUDE_DIRS}>:-I$<JOIN:${_spirvshaders_INCLUDE_DIRS},;-I>>"
				--target-env spirv${_spirvshaders_SPIRV_VERSION} ${EXTRA_FLAGS} ${GLSL} --vn ${IDENTIFIER} -o ${HEADER}
			COMMAND_EXPAND_LISTS
			DEPENDS ${GLSL} ${_spirvshaders_DEPENDS})
		list(APPEND HEADERS ${HEADER})
	endforeach()

	set(${ret} "${HEADERS}" PARENT_SCOPE)
endfunction(spirv_shaders)
