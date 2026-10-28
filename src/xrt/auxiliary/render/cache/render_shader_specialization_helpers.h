// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Build render shader specialization keys from render configuration.
 * @ingroup aux_render
 */

#pragma once

#include "cache/render_shader_specialization.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline struct render_distortion_spec
render_make_distortion_spec(uint32_t distortion_texel_count, VkBool32 do_timewarp, uint32_t view_count)
{
	struct render_distortion_spec spec = {
	    .distortion_texel_count = (int32_t)distortion_texel_count,
	    .do_timewarp = do_timewarp,
	    .view_count = (int32_t)view_count,
	};

	return spec;
}

static inline struct render_layer_spec
render_make_layer_spec(VkBool32 do_timewarp,
                       VkBool32 do_color_correction,
                       uint32_t sampler_array_size,
                       float inset_blend_edge)
{
	struct render_layer_spec spec = {
	    .do_timewarp = do_timewarp,
	    .do_color_correction = do_color_correction,
	    .sampler_array_size = (int32_t)sampler_array_size,
	    .inset_blend_edge = inset_blend_edge,
	};

	return spec;
}

static inline struct render_blit_spec
render_make_blit_spec(uint32_t color_transform_mode)
{
	struct render_blit_spec spec = {
	    .color_transform_mode = (int32_t)color_transform_mode,
	};

	return spec;
}

static inline struct render_blit_ms_spec
render_make_blit_ms_spec(uint32_t color_transform_mode)
{
	struct render_blit_ms_spec spec = {
	    .color_transform_mode = (int32_t)color_transform_mode,
	};

	return spec;
}

#ifdef __cplusplus
}
#endif
