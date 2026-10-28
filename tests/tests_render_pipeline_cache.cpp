// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Contract and behavioral tests for render compute pipeline cache.
 */

#include "catch_amalgamated.hpp"

#include "cache/render_compute_pipeline_cache.h"
#include "cache/render_compute_pipeline_cache.hpp"
#include "cache/render_shader_specialization_helpers.h"

#include "render/render_interface.h"

#include "util/comp_vulkan.h"
#include "util/u_extension_list.h"
#include "vk/vk_helpers.h"
#include "xrt/xrt_defines.h"

#include "distortion.comp.h"
#include "layer.comp.h"
#include "blit.comp.h"
#include "blit_ms.comp.h"

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace {

static render_distortion_spec
make_test_distortion_spec(VkBool32 do_timewarp, int32_t view_count = 2)
{
	return render_distortion_spec{
	    .distortion_texel_count = 2,
	    .do_timewarp = do_timewarp,
	    .view_count = view_count,
	};
}

static render_layer_spec
make_test_layer_spec(VkBool32 do_timewarp, int32_t sampler_array_size = 16)
{
	return render_make_layer_spec(do_timewarp, VK_TRUE, (uint32_t)sampler_array_size,
	                              RENDER_LAYER_DEFAULT_INSET_BLEND_EDGE);
}

static render_blit_spec
make_test_blit_spec(int32_t color_transform_mode = 0)
{
	return render_make_blit_spec((uint32_t)color_transform_mode);
}

static render_blit_ms_spec
make_test_blit_ms_spec(int32_t color_transform_mode = 0)
{
	return render_make_blit_ms_spec((uint32_t)color_transform_mode);
}

struct MinimalVulkanContext
{
	vk_bundle vk{};

	~MinimalVulkanContext()
	{
		shutdown();
	}

	bool
	init()
	{
		static const char *required_instance_extensions[] = {nullptr};
		static const char *optional_instance_extensions[] = {
		    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		};
		static const char *required_device_extensions[] = {nullptr};
		static const char *optional_device_extensions[] = {nullptr};

		auto required_instance_ext_list = u_extension_list_create_from_array(
		    required_instance_extensions, ARRAY_SIZE(required_instance_extensions));
		auto optional_instance_ext_list = u_extension_list_create_from_array(
		    optional_instance_extensions, ARRAY_SIZE(optional_instance_extensions));
		auto required_device_ext_list = u_extension_list_create_from_array(
		    required_device_extensions, ARRAY_SIZE(required_device_extensions));
		auto optional_device_ext_list = u_extension_list_create_from_array(
		    optional_device_extensions, ARRAY_SIZE(optional_device_extensions));

		struct comp_vulkan_arguments vk_args = {
		    .required_instance_version = VK_MAKE_VERSION(1, 0, 0),
		    .get_instance_proc_address = vkGetInstanceProcAddr,
		    .required_instance_extensions = required_instance_ext_list,
		    .optional_instance_extensions = optional_instance_ext_list,
		    .required_device_extensions = required_device_ext_list,
		    .optional_device_extensions = optional_device_ext_list,
		    .log_level = U_LOGGING_WARN,
		    .only_compute_queue = false,
		    .timeline_semaphore = false,
		    .selected_gpu_index = -1,
		    .lookup_with_uuid = false,
		    .selected_gpu_uuid = {},
		    .client_gpu_index = -1,
		};

		struct comp_vulkan_results vk_res = {};
		const bool ok = comp_vulkan_init_bundle(&vk, &vk_args, &vk_res);

		u_extension_list_destroy(&required_instance_ext_list);
		u_extension_list_destroy(&optional_instance_ext_list);
		u_extension_list_destroy(&required_device_ext_list);
		u_extension_list_destroy(&optional_device_ext_list);

		return ok;
	}

	void
	shutdown()
	{
		if (vk.device != VK_NULL_HANDLE) {
			vk.vkDeviceWaitIdle(vk.device);
			vk.vkDestroyDevice(vk.device, nullptr);
		}
		if (vk.instance != VK_NULL_HANDLE) {
			vk.vkDestroyInstance(vk.instance, nullptr);
		}
		vk = {};
	}
};

VkShaderModule
create_distortion_shader_module(vk_bundle *vk)
{
	VkShaderModuleCreateInfo info = {
	    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .codeSize = sizeof(distortion_comp),
	    .pCode = distortion_comp,
	};

	VkShaderModule shader = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreateShaderModule(vk->device, &info, nullptr, &shader);
	REQUIRE(ret == VK_SUCCESS);
	return shader;
}

VkDescriptorSetLayout
create_distortion_descriptor_set_layout(vk_bundle *vk, uint32_t view_count)
{
	VkDescriptorSetLayoutBinding bindings[4] = {
	    {
	        .binding = 0,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .descriptorCount = view_count,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	    {
	        .binding = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .descriptorCount = 3 * view_count,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	    {
	        .binding = 2,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	    {
	        .binding = 3,
	        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	};

	VkDescriptorSetLayoutCreateInfo create_info = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .bindingCount = ARRAY_SIZE(bindings),
	    .pBindings = bindings,
	};

	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreateDescriptorSetLayout(vk->device, &create_info, nullptr, &layout);
	REQUIRE(ret == VK_SUCCESS);
	return layout;
}

VkPipelineLayout
create_distortion_pipeline_layout(vk_bundle *vk, VkDescriptorSetLayout descriptor_set_layout)
{
	VkPipelineLayoutCreateInfo create_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .setLayoutCount = 1,
	    .pSetLayouts = &descriptor_set_layout,
	    .pushConstantRangeCount = 0,
	    .pPushConstantRanges = nullptr,
	};

	VkPipelineLayout layout = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreatePipelineLayout(vk->device, &create_info, nullptr, &layout);
	REQUIRE(ret == VK_SUCCESS);
	return layout;
}

VkPipelineLayout
create_blit_pipeline_layout(vk_bundle *vk, VkDescriptorSetLayout descriptor_set_layout)
{
	VkPushConstantRange push_constant_range = {
	    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	    .offset = 0,
	    .size = sizeof(render_compute_blit_push_data),
	};
	VkPipelineLayoutCreateInfo create_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .setLayoutCount = 1,
	    .pSetLayouts = &descriptor_set_layout,
	    .pushConstantRangeCount = 1,
	    .pPushConstantRanges = &push_constant_range,
	};

	VkPipelineLayout layout = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreatePipelineLayout(vk->device, &create_info, nullptr, &layout);
	REQUIRE(ret == VK_SUCCESS);
	return layout;
}

struct DistortionPipelineCacheFixture
{
	MinimalVulkanContext ctx{};
	VkShaderModule shader{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkPipelineLayout layout{VK_NULL_HANDLE};
	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};
	ComputePipelineCache<render_distortion_spec> cache{"DistortionTest"};

	void
	require_vulkan()
	{
		if (!ctx.init()) {
			SKIP("Vulkan device unavailable on this runner");
		}

		shader = create_distortion_shader_module(&ctx.vk);
		descriptor_set_layout = create_distortion_descriptor_set_layout(&ctx.vk, 2);
		layout = create_distortion_pipeline_layout(&ctx.vk, descriptor_set_layout);
		REQUIRE(vk_create_pipeline_cache(&ctx.vk, &pipeline_cache) == VK_SUCCESS);
		cache.init(shader, layout, pipeline_cache);
	}

	~DistortionPipelineCacheFixture()
	{
		cache.destroy(&ctx.vk);

		if (pipeline_cache != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyPipelineCache(ctx.vk.device, pipeline_cache, nullptr);
		}
		if (layout != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyPipelineLayout(ctx.vk.device, layout, nullptr);
		}
		if (descriptor_set_layout != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyDescriptorSetLayout(ctx.vk.device, descriptor_set_layout, nullptr);
		}
		if (shader != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyShaderModule(ctx.vk.device, shader, nullptr);
		}
	}
};

VkShaderModule
create_layer_shader_module(vk_bundle *vk)
{
	VkShaderModuleCreateInfo info = {
	    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .codeSize = sizeof(layer_comp),
	    .pCode = layer_comp,
	};

	VkShaderModule shader = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreateShaderModule(vk->device, &info, nullptr, &shader);
	REQUIRE(ret == VK_SUCCESS);
	return shader;
}

VkDescriptorSetLayout
create_layer_descriptor_set_layout(vk_bundle *vk, uint32_t sampler_array_size)
{
	VkDescriptorSetLayoutBinding bindings[3] = {
	    {
	        .binding = 0,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .descriptorCount = sampler_array_size,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	    {
	        .binding = 2,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	    {
	        .binding = 3,
	        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	};

	VkDescriptorSetLayoutCreateInfo create_info = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .bindingCount = ARRAY_SIZE(bindings),
	    .pBindings = bindings,
	};

	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreateDescriptorSetLayout(vk->device, &create_info, nullptr, &layout);
	REQUIRE(ret == VK_SUCCESS);
	return layout;
}

struct LayerPipelineCacheFixture
{
	MinimalVulkanContext ctx{};
	VkShaderModule shader{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkPipelineLayout layout{VK_NULL_HANDLE};
	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};
	ComputePipelineCache<render_layer_spec> cache{"LayerTest"};

	void
	require_vulkan()
	{
		if (!ctx.init()) {
			SKIP("Vulkan device unavailable on this runner");
		}

		shader = create_layer_shader_module(&ctx.vk);
		descriptor_set_layout = create_layer_descriptor_set_layout(&ctx.vk, 16);
		layout = create_distortion_pipeline_layout(&ctx.vk, descriptor_set_layout);
		REQUIRE(vk_create_pipeline_cache(&ctx.vk, &pipeline_cache) == VK_SUCCESS);
		cache.init(shader, layout, pipeline_cache);
	}

	~LayerPipelineCacheFixture()
	{
		cache.destroy(&ctx.vk);

		if (pipeline_cache != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyPipelineCache(ctx.vk.device, pipeline_cache, nullptr);
		}
		if (layout != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyPipelineLayout(ctx.vk.device, layout, nullptr);
		}
		if (descriptor_set_layout != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyDescriptorSetLayout(ctx.vk.device, descriptor_set_layout, nullptr);
		}
		if (shader != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyShaderModule(ctx.vk.device, shader, nullptr);
		}
	}
};

VkShaderModule
create_blit_shader_module(vk_bundle *vk)
{
	VkShaderModuleCreateInfo info = {
	    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .codeSize = sizeof(blit_comp),
	    .pCode = blit_comp,
	};

	VkShaderModule shader = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreateShaderModule(vk->device, &info, nullptr, &shader);
	REQUIRE(ret == VK_SUCCESS);
	return shader;
}

VkDescriptorSetLayout
create_blit_descriptor_set_layout(vk_bundle *vk)
{
	VkDescriptorSetLayoutBinding bindings[2] = {
	    {
	        .binding = 0,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	    {
	        .binding = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	        .pImmutableSamplers = nullptr,
	    },
	};

	VkDescriptorSetLayoutCreateInfo create_info = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .bindingCount = ARRAY_SIZE(bindings),
	    .pBindings = bindings,
	};

	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreateDescriptorSetLayout(vk->device, &create_info, nullptr, &layout);
	REQUIRE(ret == VK_SUCCESS);
	return layout;
}

struct BlitPipelineCacheFixture
{
	MinimalVulkanContext ctx{};
	VkShaderModule shader{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkPipelineLayout layout{VK_NULL_HANDLE};
	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};
	ComputePipelineCache<render_blit_spec> cache{"BlitTest"};

	void
	require_vulkan()
	{
		if (!ctx.init()) {
			SKIP("Vulkan device unavailable on this runner");
		}

		shader = create_blit_shader_module(&ctx.vk);
		descriptor_set_layout = create_blit_descriptor_set_layout(&ctx.vk);
		layout = create_blit_pipeline_layout(&ctx.vk, descriptor_set_layout);
		REQUIRE(vk_create_pipeline_cache(&ctx.vk, &pipeline_cache) == VK_SUCCESS);
		cache.init(shader, layout, pipeline_cache);
	}

	~BlitPipelineCacheFixture()
	{
		cache.destroy(&ctx.vk);

		if (pipeline_cache != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyPipelineCache(ctx.vk.device, pipeline_cache, nullptr);
		}
		if (layout != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyPipelineLayout(ctx.vk.device, layout, nullptr);
		}
		if (descriptor_set_layout != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyDescriptorSetLayout(ctx.vk.device, descriptor_set_layout, nullptr);
		}
		if (shader != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyShaderModule(ctx.vk.device, shader, nullptr);
		}
	}
};

VkShaderModule
create_blit_ms_shader_module(vk_bundle *vk)
{
	VkShaderModuleCreateInfo info = {
	    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .codeSize = sizeof(blit_ms_comp),
	    .pCode = blit_ms_comp,
	};

	VkShaderModule shader = VK_NULL_HANDLE;
	VkResult ret = vk->vkCreateShaderModule(vk->device, &info, nullptr, &shader);
	REQUIRE(ret == VK_SUCCESS);
	return shader;
}

struct BlitMsPipelineCacheFixture
{
	MinimalVulkanContext ctx{};
	VkShaderModule shader{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkPipelineLayout layout{VK_NULL_HANDLE};
	VkPipelineCache pipeline_cache{VK_NULL_HANDLE};
	ComputePipelineCache<render_blit_ms_spec> cache{"BlitMsTest"};

	void
	require_vulkan()
	{
		if (!ctx.init()) {
			SKIP("Vulkan device unavailable on this runner");
		}

		shader = create_blit_ms_shader_module(&ctx.vk);
		descriptor_set_layout = create_blit_descriptor_set_layout(&ctx.vk);
		layout = create_blit_pipeline_layout(&ctx.vk, descriptor_set_layout);
		REQUIRE(vk_create_pipeline_cache(&ctx.vk, &pipeline_cache) == VK_SUCCESS);
		cache.init(shader, layout, pipeline_cache);
	}

	~BlitMsPipelineCacheFixture()
	{
		cache.destroy(&ctx.vk);

		if (pipeline_cache != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyPipelineCache(ctx.vk.device, pipeline_cache, nullptr);
		}
		if (layout != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyPipelineLayout(ctx.vk.device, layout, nullptr);
		}
		if (descriptor_set_layout != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyDescriptorSetLayout(ctx.vk.device, descriptor_set_layout, nullptr);
		}
		if (shader != VK_NULL_HANDLE) {
			ctx.vk.vkDestroyShaderModule(ctx.vk.device, shader, nullptr);
		}
	}
};

} // namespace

TEST_CASE("render_distortion_spec is a padding-free specialization key", "[aux_render][pipeline_cache]")
{
	STATIC_REQUIRE(std::is_trivially_copyable_v<render_distortion_spec>);
	// distortion.comp has three specialization constants (k_distortion_texel_count,
	// k_do_timewarp, k_view_count); the generated POD stores each as a 4-byte field.
	STATIC_REQUIRE(sizeof(render_distortion_spec) == 3u * sizeof(int32_t));

	const render_distortion_spec key = make_test_distortion_spec(VK_FALSE);
	CHECK(key.distortion_texel_count == 2);
	CHECK(key.do_timewarp == VK_FALSE);
	CHECK(key.view_count == 2);
}

TEST_CASE("ComputePipelineCache<render_distortion_spec> caches by key", "[aux_render][pipeline_cache][gpu]")
{
	DistortionPipelineCacheFixture fixture{};
	fixture.require_vulkan();

	const render_distortion_spec non_timewarp = make_test_distortion_spec(VK_FALSE);
	const render_distortion_spec timewarp = make_test_distortion_spec(VK_TRUE);

	VkPipeline first = VK_NULL_HANDLE;
	VkPipeline second = VK_NULL_HANDLE;
	VkPipeline timewarp_pipeline = VK_NULL_HANDLE;

	REQUIRE(fixture.cache.get(&fixture.ctx.vk, non_timewarp, first) == VK_SUCCESS);
	REQUIRE(fixture.cache.get(&fixture.ctx.vk, non_timewarp, second) == VK_SUCCESS);
	REQUIRE(fixture.cache.get(&fixture.ctx.vk, timewarp, timewarp_pipeline) == VK_SUCCESS);

	CHECK(first == second);
	CHECK(first != timewarp_pipeline);
}

TEST_CASE("render_distortion C pipeline cache wrapper", "[aux_render][pipeline_cache][gpu]")
{
	DistortionPipelineCacheFixture fixture{};
	fixture.require_vulkan();

	struct render_distortion_pipeline_cache *cache = NULL;
	REQUIRE(render_distortion_pipeline_cache_create("DistortionCTest", &cache) == XRT_SUCCESS);
	REQUIRE(cache != nullptr);

	REQUIRE(render_distortion_pipeline_cache_init(cache, fixture.shader, fixture.layout, fixture.pipeline_cache) ==
	        XRT_SUCCESS);

	const render_distortion_spec key = make_test_distortion_spec(VK_FALSE);
	VkPipeline pipeline = VK_NULL_HANDLE;
	REQUIRE(render_distortion_pipeline_cache_get(cache, &fixture.ctx.vk, &key, &pipeline) == XRT_SUCCESS);
	CHECK(pipeline != VK_NULL_HANDLE);

	render_distortion_pipeline_cache_destroy(&cache, &fixture.ctx.vk);
	CHECK(cache == nullptr);
}

TEST_CASE("render_layer_spec is a padding-free specialization key", "[aux_render][pipeline_cache]")
{
	STATIC_REQUIRE(std::is_trivially_copyable_v<render_layer_spec>);
	// layer.comp has four specialization constants; the generated POD stores each as a 4-byte field.
	STATIC_REQUIRE(sizeof(render_layer_spec) == 4u * sizeof(int32_t));
	// Must match k_inset_blend_edge default in layer.comp.
	STATIC_REQUIRE(RENDER_LAYER_DEFAULT_INSET_BLEND_EDGE == 0.05f);

	const render_layer_spec key = make_test_layer_spec(VK_FALSE);
	CHECK(key.do_timewarp == VK_FALSE);
	CHECK(key.do_color_correction == VK_TRUE);
	CHECK(key.sampler_array_size == 16);
	CHECK(key.inset_blend_edge == RENDER_LAYER_DEFAULT_INSET_BLEND_EDGE);
}

TEST_CASE("ComputePipelineCache<render_layer_spec> caches by key", "[aux_render][pipeline_cache][gpu]")
{
	LayerPipelineCacheFixture fixture{};
	fixture.require_vulkan();

	const render_layer_spec non_timewarp = make_test_layer_spec(VK_FALSE);
	const render_layer_spec timewarp = make_test_layer_spec(VK_TRUE);

	VkPipeline first = VK_NULL_HANDLE;
	VkPipeline second = VK_NULL_HANDLE;
	VkPipeline timewarp_pipeline = VK_NULL_HANDLE;

	REQUIRE(fixture.cache.get(&fixture.ctx.vk, non_timewarp, first) == VK_SUCCESS);
	REQUIRE(fixture.cache.get(&fixture.ctx.vk, non_timewarp, second) == VK_SUCCESS);
	REQUIRE(fixture.cache.get(&fixture.ctx.vk, timewarp, timewarp_pipeline) == VK_SUCCESS);

	CHECK(first == second);
	CHECK(first != timewarp_pipeline);
}

TEST_CASE("render_layer C pipeline cache wrapper", "[aux_render][pipeline_cache][gpu]")
{
	LayerPipelineCacheFixture fixture{};
	fixture.require_vulkan();

	struct render_layer_pipeline_cache *cache = NULL;
	REQUIRE(render_layer_pipeline_cache_create("LayerCTest", &cache) == XRT_SUCCESS);
	REQUIRE(cache != nullptr);

	REQUIRE(render_layer_pipeline_cache_init(cache, fixture.shader, fixture.layout, fixture.pipeline_cache) ==
	        XRT_SUCCESS);

	const render_layer_spec key = make_test_layer_spec(VK_FALSE);
	VkPipeline pipeline = VK_NULL_HANDLE;
	REQUIRE(render_layer_pipeline_cache_get(cache, &fixture.ctx.vk, &key, &pipeline) == XRT_SUCCESS);
	CHECK(pipeline != VK_NULL_HANDLE);

	render_layer_pipeline_cache_destroy(&cache, &fixture.ctx.vk);
	CHECK(cache == nullptr);
}

TEST_CASE("render_blit_spec is a padding-free specialization key", "[aux_render][pipeline_cache]")
{
	STATIC_REQUIRE(std::is_trivially_copyable_v<render_blit_spec>);
	STATIC_REQUIRE(sizeof(render_blit_spec) == sizeof(int32_t));

	const render_blit_spec key = make_test_blit_spec();
	CHECK(key.color_transform_mode == 0);
}

TEST_CASE("ComputePipelineCache<render_blit_spec> caches by key", "[aux_render][pipeline_cache][gpu]")
{
	BlitPipelineCacheFixture fixture{};
	fixture.require_vulkan();

	const render_blit_spec none = make_test_blit_spec();
	const render_blit_spec gamma = make_test_blit_spec(1);

	VkPipeline first = VK_NULL_HANDLE;
	VkPipeline second = VK_NULL_HANDLE;
	VkPipeline gamma_pipeline = VK_NULL_HANDLE;

	REQUIRE(fixture.cache.get(&fixture.ctx.vk, none, first) == VK_SUCCESS);
	REQUIRE(fixture.cache.get(&fixture.ctx.vk, none, second) == VK_SUCCESS);
	REQUIRE(fixture.cache.get(&fixture.ctx.vk, gamma, gamma_pipeline) == VK_SUCCESS);

	CHECK(first == second);
	CHECK(first != gamma_pipeline);
}

TEST_CASE("render_blit C pipeline cache wrapper", "[aux_render][pipeline_cache][gpu]")
{
	BlitPipelineCacheFixture fixture{};
	fixture.require_vulkan();

	struct render_blit_pipeline_cache *cache = NULL;
	REQUIRE(render_blit_pipeline_cache_create("BlitCTest", &cache) == XRT_SUCCESS);
	REQUIRE(cache != nullptr);

	REQUIRE(render_blit_pipeline_cache_init(cache, fixture.shader, fixture.layout, fixture.pipeline_cache) ==
	        XRT_SUCCESS);

	const render_blit_spec key = make_test_blit_spec();
	VkPipeline pipeline = VK_NULL_HANDLE;
	REQUIRE(render_blit_pipeline_cache_get(cache, &fixture.ctx.vk, &key, &pipeline) == XRT_SUCCESS);
	CHECK(pipeline != VK_NULL_HANDLE);

	render_blit_pipeline_cache_destroy(&cache, &fixture.ctx.vk);
	CHECK(cache == nullptr);
}

TEST_CASE("render_blit_ms_spec is a padding-free specialization key", "[aux_render][pipeline_cache]")
{
	STATIC_REQUIRE(std::is_trivially_copyable_v<render_blit_ms_spec>);
	STATIC_REQUIRE(sizeof(render_blit_ms_spec) == sizeof(int32_t));

	const render_blit_ms_spec key = make_test_blit_ms_spec();
	CHECK(key.color_transform_mode == 0);
}

TEST_CASE("ComputePipelineCache<render_blit_ms_spec> caches by key", "[aux_render][pipeline_cache][gpu]")
{
	BlitMsPipelineCacheFixture fixture{};
	fixture.require_vulkan();

	const render_blit_ms_spec none = make_test_blit_ms_spec();
	const render_blit_ms_spec gamma = make_test_blit_ms_spec(1);

	VkPipeline first = VK_NULL_HANDLE;
	VkPipeline second = VK_NULL_HANDLE;
	VkPipeline gamma_pipeline = VK_NULL_HANDLE;

	REQUIRE(fixture.cache.get(&fixture.ctx.vk, none, first) == VK_SUCCESS);
	REQUIRE(fixture.cache.get(&fixture.ctx.vk, none, second) == VK_SUCCESS);
	REQUIRE(fixture.cache.get(&fixture.ctx.vk, gamma, gamma_pipeline) == VK_SUCCESS);

	CHECK(first == second);
	CHECK(first != gamma_pipeline);
}

TEST_CASE("render_blit_ms C pipeline cache wrapper", "[aux_render][pipeline_cache][gpu]")
{
	BlitMsPipelineCacheFixture fixture{};
	fixture.require_vulkan();

	struct render_blit_ms_pipeline_cache *cache = NULL;
	REQUIRE(render_blit_ms_pipeline_cache_create("BlitMsCTest", &cache) == XRT_SUCCESS);
	REQUIRE(cache != nullptr);

	REQUIRE(render_blit_ms_pipeline_cache_init(cache, fixture.shader, fixture.layout, fixture.pipeline_cache) ==
	        XRT_SUCCESS);

	const render_blit_ms_spec key = make_test_blit_ms_spec();
	VkPipeline pipeline = VK_NULL_HANDLE;
	REQUIRE(render_blit_ms_pipeline_cache_get(cache, &fixture.ctx.vk, &key, &pipeline) == XRT_SUCCESS);
	CHECK(pipeline != VK_NULL_HANDLE);

	render_blit_ms_pipeline_cache_destroy(&cache, &fixture.ctx.vk);
	CHECK(cache == nullptr);
}
