// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Get-or-create cache for specialized compute pipelines.
 * @ingroup aux_render
 */

#include "glue/g_catch_guard.hpp"
#include "vk/vk_mini_helpers.h"

#include "cache/render_compute_pipeline_cache.hpp"

#include <cstring>
#include <functional>
#include <new>
#include <string_view>


/*
 *
 * Helpers.
 *
 */

#define RENDER_PIPELINE_CACHE_ASSERT(condition)                                                                        \
	do {                                                                                                           \
		if (!(condition)) {                                                                                    \
			U_LOG_E("Condition failed: " #condition);                                                      \
			assert(condition);                                                                             \
		}                                                                                                      \
	} while (0)


/*
 *
 * Hash and equal functors.
 *
 */

template <typename Key>
size_t
ComputePipelineCache<Key>::Hash::operator()(const Key &key) const noexcept
{
	return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char *>(&key), sizeof(Key)));
}

template <typename Key>
bool
ComputePipelineCache<Key>::Equal::operator()(const Key &a, const Key &b) const noexcept
{
	return std::memcmp(&a, &b, sizeof(Key)) == 0;
}


/*
 *
 * Pipeline cache implementation.
 *
 */

template <typename Key> ComputePipelineCache<Key>::ComputePipelineCache(std::string_view name) : mName(name)
{
	// Empty constructor
}

template <typename Key> ComputePipelineCache<Key>::~ComputePipelineCache()
{
	RENDER_PIPELINE_CACHE_ASSERT(mPipelines.empty());
	RENDER_PIPELINE_CACHE_ASSERT(mShader == VK_NULL_HANDLE);
	RENDER_PIPELINE_CACHE_ASSERT(mLayout == VK_NULL_HANDLE);
	RENDER_PIPELINE_CACHE_ASSERT(mCache == VK_NULL_HANDLE);
}

template <typename Key>
void
ComputePipelineCache<Key>::init(VkShaderModule shader, VkPipelineLayout layout, VkPipelineCache pipelineCache)
{
	mShader = shader;
	mLayout = layout;
	mCache = pipelineCache;
}

template <typename Key>
VkResult
ComputePipelineCache<Key>::get(struct vk_bundle *vk, const Key &key, VkPipeline &out)
{
	if (auto it = mPipelines.find(key); it != mPipelines.end()) {
		out = it->second;
		return VK_SUCCESS;
	}

	VkSpecializationInfo spec_info = {
	    .mapEntryCount = static_cast<uint32_t>(ShaderSpecializationMap<Key>::kEntries.size()),
	    .pMapEntries = ShaderSpecializationMap<Key>::kEntries.data(),
	    .dataSize = sizeof(Key),
	    .pData = &key,
	};

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkResult ret = vk_create_compute_pipeline( //
	    vk,                                    //
	    mCache,                                //
	    mShader,                               //
	    mLayout,                               //
	    &spec_info,                            //
	    &pipeline);                            //
	VK_CHK_AND_RET(ret, "vk_create_compute_pipeline");

	if (!mName.empty()) {
		VK_NAME_PIPELINE(vk, pipeline, mName.c_str());
	}

	mPipelines.emplace(key, pipeline);
	out = pipeline;
	return VK_SUCCESS;
}

template <typename Key>
VkResult
ComputePipelineCache<Key>::prewarm(struct vk_bundle *vk, std::span<const Key> keys)
{
	for (const Key &key : keys) {
		VkPipeline ignored = VK_NULL_HANDLE;
		VkResult ret = get(vk, key, ignored);
		if (ret != VK_SUCCESS) {
			return ret;
		}
	}
	return VK_SUCCESS;
}

template <typename Key>
void
ComputePipelineCache<Key>::destroy(struct vk_bundle *vk)
{
	for (auto &entry : mPipelines) {
		VkPipeline pipeline = entry.second;
		D(Pipeline, pipeline);
	}
	mPipelines.clear();

	// Reset borrowed resources.
	mShader = VK_NULL_HANDLE;
	mLayout = VK_NULL_HANDLE;
	mCache = VK_NULL_HANDLE;
}

#define RENDER_PIPELINE_CACHE_INSTANTIATE(Key) template class ComputePipelineCache<Key>;
RENDER_SHADER_SPECIALIZATION_KEYS(RENDER_PIPELINE_CACHE_INSTANTIATE)
#undef RENDER_PIPELINE_CACHE_INSTANTIATE

static inline xrt_result_t
render_pipeline_cache_vk_to_xrt(VkResult ret)
{
	return ret == VK_SUCCESS ? XRT_SUCCESS : XRT_ERROR_VULKAN;
}

extern "C" {

#define RENDER_PIPELINE_CACHE_DEFINE(CACHE, SPEC)                                                                      \
                                                                                                                       \
	struct CACHE##_pipeline_cache                                                                                  \
	{                                                                                                              \
		ComputePipelineCache<SPEC> impl;                                                                       \
	};                                                                                                             \
                                                                                                                       \
	xrt_result_t CACHE##_pipeline_cache_create(const char *debug_name, struct CACHE##_pipeline_cache **out_cache)  \
	try {                                                                                                          \
		if (out_cache == NULL) {                                                                               \
			return XRT_ERROR_INVALID_ARGUMENT;                                                             \
		}                                                                                                      \
		const char *name = debug_name != NULL ? debug_name : "";                                               \
		struct CACHE##_pipeline_cache *cache =                                                                 \
		    new (std::nothrow) CACHE##_pipeline_cache{ComputePipelineCache<SPEC>{name}};                       \
		if (cache == NULL) {                                                                                   \
			return XRT_ERROR_ALLOCATION;                                                                   \
		}                                                                                                      \
		*out_cache = cache;                                                                                    \
		return XRT_SUCCESS;                                                                                    \
	}                                                                                                              \
	G_CATCH_GUARDS                                                                                                 \
                                                                                                                       \
	xrt_result_t CACHE##_pipeline_cache_init(struct CACHE##_pipeline_cache *cache, VkShaderModule shader,          \
	                                         VkPipelineLayout layout, VkPipelineCache pipeline_cache)              \
	try {                                                                                                          \
		cache->impl.init(shader, layout, pipeline_cache);                                                      \
		return XRT_SUCCESS;                                                                                    \
	}                                                                                                              \
	G_CATCH_GUARDS                                                                                                 \
                                                                                                                       \
	xrt_result_t CACHE##_pipeline_cache_get(struct CACHE##_pipeline_cache *cache, struct vk_bundle *vk,            \
	                                        const struct SPEC *key, VkPipeline *out_pipeline)                      \
	try {                                                                                                          \
		if (key == NULL || out_pipeline == NULL) {                                                             \
			return XRT_ERROR_INVALID_ARGUMENT;                                                             \
		}                                                                                                      \
		return render_pipeline_cache_vk_to_xrt(cache->impl.get(vk, *key, *out_pipeline));                      \
	}                                                                                                              \
	G_CATCH_GUARDS                                                                                                 \
                                                                                                                       \
	xrt_result_t CACHE##_pipeline_cache_prewarm(struct CACHE##_pipeline_cache *cache, struct vk_bundle *vk,        \
	                                            const struct SPEC *keys, size_t key_count)                         \
	try {                                                                                                          \
		if (key_count > 0 && keys == NULL) {                                                                   \
			return XRT_ERROR_INVALID_ARGUMENT;                                                             \
		}                                                                                                      \
		return render_pipeline_cache_vk_to_xrt(                                                                \
		    cache->impl.prewarm(vk, std::span<const SPEC>(keys, key_count)));                                  \
	}                                                                                                              \
	G_CATCH_GUARDS                                                                                                 \
                                                                                                                       \
	void CACHE##_pipeline_cache_destroy(struct CACHE##_pipeline_cache **cache, struct vk_bundle *vk)               \
	try {                                                                                                          \
		if (cache == NULL || *cache == NULL) {                                                                 \
			return;                                                                                        \
		}                                                                                                      \
		(*cache)->impl.destroy(vk);                                                                            \
		delete *cache;                                                                                         \
		*cache = NULL;                                                                                         \
	}                                                                                                              \
	G_CATCH_GUARDS_VOID

RENDER_SHADER_SPECIALIZATION_ENTRIES(RENDER_PIPELINE_CACHE_DEFINE)

#undef RENDER_PIPELINE_CACHE_DEFINE

} // extern "C"
