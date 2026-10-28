# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

add_library(
	aux_render_cache STATIC render_compute_pipeline_cache.cpp render_compute_pipeline_cache.h
				render_compute_pipeline_cache.hpp
	)

target_link_libraries(
	aux_render_cache
	PUBLIC xrt-interfaces aux_vk aux_shaders
	PRIVATE aux-includes aux_glue
	)

target_include_directories(aux_render_cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_dependencies(aux_render_cache render_shader_specialization)
