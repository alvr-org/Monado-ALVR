// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Get-or-create cache for specialized compute pipelines.
 * @ingroup aux_render
 */

#pragma once

#include "cache/render_shader_specialization.hpp"

#include "vk/vk_helpers.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

/*
 *
 * Specialization key structs.
 *
 * Shader-generated keys (e.g. @c render_distortion_spec) come from
 * @ref render_shader_specialization.hpp. Each struct's bytes are both the
 * specialization data handed to Vulkan and the cache key. To keep the byte key
 * well-defined:
 *   - use only 4-byte scalar fields (VkBool32, int32_t, uint32_t) so the struct
 *     has no padding,
 *   - always value-initialize keys,
 *   - normalize booleans to @c VK_TRUE / @c VK_FALSE so logically equal inputs
 *     produce identical bytes.
 *
 */

/*!
 * Get-or-create cache of specialized compute pipelines that share one shader
 * module and pipeline layout and differ only in specialization constant values.
 *
 * Owns the pipelines it creates. The shader module, pipeline layout, and Vulkan
 * pipeline cache supplied to @ref init are borrowed and are never destroyed by
 * this object.
 *
 * Compute-only: creation needs just those borrowed objects plus specialization
 * constants. Graphics pipelines need far more create-info (render pass or
 * dynamic rendering formats, vertex input, rasterization, blend, and so on), so
 * they cannot use this cache as-is.
 *
 * @tparam Key A trivially-copyable, padding-free POD (see notes above).
 *
 * @note Not thread-safe; the render path is serialized.
 *
 * @ingroup aux_render
 */
template <typename Key> class ComputePipelineCache
{
	static_assert(std::is_trivially_copyable_v<Key>, "Key must be a trivially-copyable POD");

public:
	explicit ComputePipelineCache(std::string_view name);
	~ComputePipelineCache();

	/*!
	 * Sets the borrowed Vulkan objects used to create pipelines.
	 *
	 * The caller retains ownership and must keep these objects valid during
	 * calls to @ref get and @ref prewarm that may create a pipeline.
	 */
	void
	init(VkShaderModule shader, VkPipelineLayout layout, VkPipelineCache pipelineCache);

	XRT_CHECK_RESULT VkResult
	get(struct vk_bundle *vk, const Key &key, VkPipeline &out);

	XRT_CHECK_RESULT VkResult
	prewarm(struct vk_bundle *vk, std::span<const Key> keys);

	void
	destroy(struct vk_bundle *vk);

private:
	struct Hash
	{
		size_t
		operator()(const Key &key) const noexcept;
	};
	struct Equal
	{
		bool
		operator()(const Key &a, const Key &b) const noexcept;
	};

	VkShaderModule mShader{VK_NULL_HANDLE};
	VkPipelineLayout mLayout{VK_NULL_HANDLE};
	VkPipelineCache mCache{VK_NULL_HANDLE};
	std::string mName;

	std::unordered_map<Key, VkPipeline, Hash, Equal> mPipelines;
};

#define RENDER_PIPELINE_CACHE_EXTERN(Key) extern template class ComputePipelineCache<Key>;
RENDER_SHADER_SPECIALIZATION_KEYS(RENDER_PIPELINE_CACHE_EXTERN)
#undef RENDER_PIPELINE_CACHE_EXTERN
