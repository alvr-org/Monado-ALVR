// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief C API for specialized compute pipeline caches.
 * @ingroup aux_render
 */

#pragma once

#include "cache/render_shader_specialization.h"

#include "vk/vk_helpers.h"
#include "xrt/xrt_results.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RENDER_PIPELINE_CACHE_DECLARE(CACHE, SPEC)                                                                     \
                                                                                                                       \
	struct CACHE##_pipeline_cache;                                                                                 \
                                                                                                                       \
	xrt_result_t CACHE##_pipeline_cache_create(const char *debug_name, struct CACHE##_pipeline_cache **out_cache); \
                                                                                                                       \
	xrt_result_t CACHE##_pipeline_cache_init(struct CACHE##_pipeline_cache *cache, VkShaderModule shader,          \
	                                         VkPipelineLayout layout, VkPipelineCache pipeline_cache);             \
                                                                                                                       \
	xrt_result_t CACHE##_pipeline_cache_get(struct CACHE##_pipeline_cache *cache, struct vk_bundle *vk,            \
	                                        const struct SPEC *key, VkPipeline *out_pipeline);                     \
                                                                                                                       \
	xrt_result_t CACHE##_pipeline_cache_prewarm(struct CACHE##_pipeline_cache *cache, struct vk_bundle *vk,        \
	                                            const struct SPEC *keys, size_t key_count);                        \
                                                                                                                       \
	void CACHE##_pipeline_cache_destroy(struct CACHE##_pipeline_cache **cache, struct vk_bundle *vk);

RENDER_SHADER_SPECIALIZATION_ENTRIES(RENDER_PIPELINE_CACHE_DECLARE)

#undef RENDER_PIPELINE_CACHE_DECLARE

#ifdef __cplusplus
}
#endif
