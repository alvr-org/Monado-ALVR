# Copyright 2019-2024, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

###
# Shaders headers
#

include(ShaderSpecialization)

set(DEPENDS blit.inc.glsl hsv.inc.glsl layer_defines.inc.glsl srgb.inc.glsl)

set(SHADERS
    blit.comp
    blit_ms.comp
    clear.comp
    distortion.comp
    layer.comp
    mesh.frag
    mesh.vert
    layer_cylinder.frag
    layer_cylinder.vert
    layer_equirect2.frag
    layer_equirect2.vert
    layer_projection.vert
    layer_projection.frag
    layer_quad.vert
    layer_quad.frag
	)

#
# Generate shader specialization structs and map entries from GLSL shaders.
#
set(SHADER_SPECIALIZATION_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/inc")

shader_specializations(
	SHADER_SPECIALIZATION_HEADERS
	OUTPUT_DIR
	${SHADER_SPECIALIZATION_OUTPUT_DIR}
	PREFIX
	render_
	STRUCT_NAMES
	${CMAKE_CURRENT_SOURCE_DIR}/shader_specialization_struct_names.json
	NAME_STYLE_SNAKE_CASE
	SOURCES
	distortion.comp
	layer.comp
	blit.comp
	blit_ms.comp
	INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS
	${DEPENDS}
	)

add_custom_target(render_shader_specialization DEPENDS ${SHADER_SPECIALIZATION_HEADERS})

# Compile all the non-specialized shaders
spirv_shaders(
	SHADER_HEADERS
	SPIRV_VERSION
	1.0 # Currently targeting Vulkan 1.0
	SOURCES
	${SHADERS}
	DEPENDS
	${DEPENDS}
	)

# Create a custom target that depends on the shader headers.
# This ensures the headers are built before they're needed.
# Note: Libraries using shaders must also link to comp_includes
# via target_link_libraries() to get the correct include directories.
add_custom_target(comp_shaders_headers DEPENDS ${SHADER_HEADERS})

###
# Shader library
#

add_library(aux_shaders STATIC ${SHADER_HEADERS} render_shaders_interface.h render_shaders.c)

target_link_libraries(
	aux_shaders
	PUBLIC xrt-interfaces aux_vk comp_includes
	PRIVATE aux_util
	)

# Ensure shader headers are built before compiling this library
add_dependencies(aux_shaders comp_shaders_headers render_shader_specialization)

# Shaders - marked SYSTEM so we get no warnings
target_include_directories(aux_shaders SYSTEM PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(
	aux_shaders PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${SHADER_SPECIALIZATION_OUTPUT_DIR}
	)
