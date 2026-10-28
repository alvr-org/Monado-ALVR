# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

#.rst:
# ShaderSpecialization
# --------------------
#
# Generate C struct and C++ ShaderSpecializationMap specialization headers from
# GLSL specialization constants.
# Generated files are written under OUTPUT_DIR (default: ${CMAKE_CURRENT_BINARY_DIR})
# as ``${PREFIX}shader_specialization.h.inc`` and ``${PREFIX}shader_specialization.hpp.inc``.
# PREFIX is required so distinct generators (e.g. Monado ``render_`` vs External ``ext_``)
# do not collide when both output dirs are on the include path.
#
# Usage::
#
#   include(ShaderSpecialization)
#
#   shader_specializations(
#       SHADER_SPECIALIZATION_HEADERS
#       OUTPUT_DIR
#       ${CMAKE_CURRENT_BINARY_DIR}/inc
#       PREFIX
#       render_
#       STRUCT_NAMES
#       ${CMAKE_CURRENT_SOURCE_DIR}/shader_specialization_struct_names.json
#       NAME_STYLE_SNAKE_CASE
#       SOURCES
#       distortion.comp
#       INCLUDE_DIRS
#       ${CMAKE_CURRENT_SOURCE_DIR}
#       DEPENDS
#       ${SHADER_INCLUDES}
#   )
#
# Options: OUTPUT_DIR, PREFIX, STRUCT_NAMES, SOURCES, INCLUDE_DIRS, DEPENDS.
# PREFIX is required. Exactly one of ``NAME_STYLE_CAMEL_CASE`` or ``NAME_STYLE_SNAKE_CASE`` is required.
#
find_program(GLSLANGVALIDATOR_COMMAND glslangValidator)
if(NOT GLSLANGVALIDATOR_COMMAND)
	message(
		FATAL_ERROR
			"glslangValidator required - source maintained at https://github.com/KhronosGroup/glslang"
		)
endif()

function(shader_specializations ret)
	set(options NAME_STYLE_CAMEL_CASE NAME_STYLE_SNAKE_CASE)
	set(oneValueArgs OUTPUT_DIR PREFIX STRUCT_NAMES)
	set(multiValueArgs SOURCES INCLUDE_DIRS DEPENDS)
	cmake_parse_arguments(
		_shader_specializations
		"${options}"
		"${oneValueArgs}"
		"${multiValueArgs}"
		${ARGN}
		)

	if(NOT _shader_specializations_SOURCES)
		message(FATAL_ERROR "shader_specializations requires SOURCES")
	endif()

	if(NOT _shader_specializations_STRUCT_NAMES)
		message(FATAL_ERROR "shader_specializations requires STRUCT_NAMES")
	endif()

	if(NOT _shader_specializations_PREFIX)
		message(FATAL_ERROR "shader_specializations requires PREFIX")
	endif()

	if(_shader_specializations_PREFIX MATCHES "[/\\\\]")
		message(
			FATAL_ERROR
				"shader_specializations PREFIX must be a filename prefix, not a path"
			)
	endif()

	if(_shader_specializations_NAME_STYLE_CAMEL_CASE AND _shader_specializations_NAME_STYLE_SNAKE_CASE)
		message(FATAL_ERROR "shader_specializations requires exactly one of NAME_STYLE_CAMEL_CASE or NAME_STYLE_SNAKE_CASE")
	endif()

	if(NOT _shader_specializations_NAME_STYLE_CAMEL_CASE AND NOT _shader_specializations_NAME_STYLE_SNAKE_CASE)
		message(FATAL_ERROR "shader_specializations requires exactly one of NAME_STYLE_CAMEL_CASE or NAME_STYLE_SNAKE_CASE")
	endif()

	if(_shader_specializations_OUTPUT_DIR)
		set(_output_dir "${_shader_specializations_OUTPUT_DIR}")
	else()
		set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}")
	endif()

	if(NOT PYTHON_EXECUTABLE)
		if(TARGET Python3::Interpreter)
			set(PYTHON_EXECUTABLE Python3::Interpreter)
		elseif(Python3_EXECUTABLE)
			set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
		else()
			message(FATAL_ERROR "shader_specializations requires Python3")
		endif()
	endif()

	set(_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generate_shader_specialization.py")
	set(_h_inc "${_shader_specializations_PREFIX}shader_specialization.h.inc")
	set(_hpp_inc "${_shader_specializations_PREFIX}shader_specialization.hpp.inc")
	set(_h_inc_out "${_output_dir}/${_h_inc}")
	set(_hpp_inc_out "${_output_dir}/${_hpp_inc}")

	set(_include_dir_args "")
	foreach(_dir ${_shader_specializations_INCLUDE_DIRS})
		list(APPEND _include_dir_args --include-dir "${_dir}")
	endforeach()

	set(_shader_args "")
	set(_shader_paths "")
	foreach(_shader ${_shader_specializations_SOURCES})
		set(_shader_path "${CMAKE_CURRENT_SOURCE_DIR}/${_shader}")
		list(APPEND _shader_args --shader "${_shader_path}")
		list(APPEND _shader_paths "${_shader_path}")
	endforeach()

	set(_field_naming_arg --snake_case)
	if(_shader_specializations_NAME_STYLE_CAMEL_CASE)
		set(_field_naming_arg --CamelCase)
	endif()

	add_custom_command(
		OUTPUT "${_h_inc_out}" "${_hpp_inc_out}"
		COMMAND
			${PYTHON_EXECUTABLE} "${_script}" --glslang
			"${GLSLANGVALIDATOR_COMMAND}" --struct-names
			"${_shader_specializations_STRUCT_NAMES}" ${_field_naming_arg}
			${_shader_args} ${_include_dir_args} --output-h-inc "${_h_inc_out}"
			--output-hpp-inc "${_hpp_inc_out}"
		DEPENDS
			"${_script}"
			"${_shader_specializations_STRUCT_NAMES}"
			${_shader_paths}
			${_shader_specializations_DEPENDS}
		COMMENT "Generating ${_h_inc} and ${_hpp_inc}"
		VERBATIM
		)

	set(${ret}
	    "${_h_inc_out};${_hpp_inc_out}"
	    PARENT_SCOPE
		)
endfunction(shader_specializations)
