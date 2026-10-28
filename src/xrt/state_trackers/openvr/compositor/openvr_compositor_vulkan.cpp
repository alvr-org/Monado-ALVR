// Copyright 2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of Vulkan-specific compositor functionality for OpenVR.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#include "xrt/xrt_config_build.h"
#include "xrt/xrt_gfx_vk.h"

#include "util/u_pretty_print.h"
#include "util/u_debug.h"

#include "vk/vk_format.h"
#include "vk/vk_cmd.h"

#include "client/comp_vk_client.h"

#include "render/render_interface.h"

#include "cache/render_compute_pipeline_cache.h"
#include "cache/render_shader_specialization_helpers.h"

#include "common/openvr_error.hpp"

#include "openvr_compositor.hpp"

#include <vector>


#ifndef XRT_HAVE_VULKAN
#error "This file should not be compiled without Vulkan support!"
#endif

namespace xrt::state_trackers::openvr {

DEBUG_GET_ONCE_LOG_OPTION(compositor_log, "XRT_COMPOSITOR_LOG", U_LOGGING_WARN)

/*
 *
 * Helper functions
 *
 */

/*!
 * When dealing with an Auto colour space, pick the correct one to use based on the format. According to the OpenVR
 * documentation, all 8-bit per component formats are sRGB/Gamma
 */
static vr::EColorSpace
determineColorSpace(VkFormat format)
{
	switch (format) {
	case VK_FORMAT_R8_UNORM:
	case VK_FORMAT_R8_SNORM:
	case VK_FORMAT_R8_USCALED:
	case VK_FORMAT_R8_SSCALED:
	case VK_FORMAT_R8_UINT:
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_SRGB:
	case VK_FORMAT_R8G8_UNORM:
	case VK_FORMAT_R8G8_SNORM:
	case VK_FORMAT_R8G8_USCALED:
	case VK_FORMAT_R8G8_SSCALED:
	case VK_FORMAT_R8G8_UINT:
	case VK_FORMAT_R8G8_SINT:
	case VK_FORMAT_R8G8_SRGB:
	case VK_FORMAT_R8G8B8_UNORM:
	case VK_FORMAT_R8G8B8_SNORM:
	case VK_FORMAT_R8G8B8_USCALED:
	case VK_FORMAT_R8G8B8_SSCALED:
	case VK_FORMAT_R8G8B8_UINT:
	case VK_FORMAT_R8G8B8_SINT:
	case VK_FORMAT_R8G8B8_SRGB:
	case VK_FORMAT_B8G8R8_UNORM:
	case VK_FORMAT_B8G8R8_SNORM:
	case VK_FORMAT_B8G8R8_USCALED:
	case VK_FORMAT_B8G8R8_SSCALED:
	case VK_FORMAT_B8G8R8_UINT:
	case VK_FORMAT_B8G8R8_SINT:
	case VK_FORMAT_B8G8R8_SRGB:
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SNORM:
	case VK_FORMAT_R8G8B8A8_USCALED:
	case VK_FORMAT_R8G8B8A8_SSCALED:
	case VK_FORMAT_R8G8B8A8_UINT:
	case VK_FORMAT_R8G8B8A8_SINT:
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_SNORM:
	case VK_FORMAT_B8G8R8A8_USCALED:
	case VK_FORMAT_B8G8R8A8_SSCALED:
	case VK_FORMAT_B8G8R8A8_UINT:
	case VK_FORMAT_B8G8R8A8_SINT:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_UINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return vr::EColorSpace::ColorSpace_Gamma;
	default: return vr::EColorSpace::ColorSpace_Linear;
	}
}

static vr::EColorSpace
resolveColorSpace(VkFormat format, vr::EColorSpace color_space)
{
	if (color_space == vr::EColorSpace::ColorSpace_Auto) {
		return determineColorSpace(format);
	}

	return color_space;
}

static VkFormat
resolveSwapchainSampleFormat(VkFormat format, vr::EColorSpace color_space)
{
	color_space = resolveColorSpace(format, color_space);

	VkFormat srgb_to_unorm = vk_format_convert_srgb_to_unorm(format);

	if (srgb_to_unorm != VK_FORMAT_UNDEFINED) {
		// If the format is an sRGB format, we need to return the sRGB version of it for sampling, since the
		// runtime expects that if it's marked as sRGB, the bytes are gamma-encoded.
		return color_space == vr::EColorSpace::ColorSpace_Gamma ? format : srgb_to_unorm;
	} else {
		// Otherwise, we can just return the format for sampling as well.
		return color_space == vr::EColorSpace::ColorSpace_Gamma ? vk_format_convert_unorm_to_srgb(format)
		                                                        : format;
	}
}

/*!
 * Given the format that the compositor will later sample with, return the storage-compatible format to write into.
 */
static VkFormat
resolveSwapchainStorageFormat(VkFormat source_format, VkFormat sample_format)
{
	if (sample_format == source_format) {
		return sample_format;
	}

	VkFormat srgb_storage_format = vk_format_convert_srgb_to_unorm(sample_format);
	if (srgb_storage_format != VK_FORMAT_UNDEFINED) {
		// If the sample format is sRGB, we need to return the UNORM version of it for storage, since we'll be
		// doing a manual sRGB->linear conversion in the shader.
		return srgb_storage_format;
	}

	// Otherwise, we can just return the sample format for storage as well.
	return sample_format;
}

static std::optional<render_compute_blit_resolve_color_mode>
determineBlitResolveColorMode(VkFormat format, vr::EColorSpace color_space)
{
	color_space = resolveColorSpace(format, color_space);

	VkFormat srgb_to_unorm = vk_format_convert_srgb_to_unorm(format);

	if (srgb_to_unorm != VK_FORMAT_UNDEFINED) {
		// If the format is an sRGB format, we need to check if the runtime expects gamma bytes in a linear
		// format, or if it expects linear bytes in an sRGB format, since that determines which shader variant
		// we need to use for blitting.
		if (color_space == vr::EColorSpace::ColorSpace_Linear) {
			return std::optional<render_compute_blit_resolve_color_mode>{
			    RENDER_BLIT_RESOLVE_COLOR_MODE_LINEAR_IN_SRGB_FORMAT};
		}

		// If it's sRGB and the color space is also sRGB, then no conversion is needed
		return std::nullopt;
	}

	// If the format is not an sRGB format, but the color space is sRGB, then we need to use the shader variant that
	// expects gamma bytes in a linear format.
	if (color_space == vr::EColorSpace::ColorSpace_Gamma) {
		return std::optional<render_compute_blit_resolve_color_mode>{
		    RENDER_BLIT_RESOLVE_COLOR_MODE_GAMMA_IN_LINEAR_FORMAT};
	}

	return std::nullopt;
}

/*
 * For dispatching compute to the blit target, calculate the number of groups.
 */
static void
calculateDispatchDimensions(const VkExtent2D extent, uint32_t *out_w, uint32_t *out_h)
{
	// Power of two divide and round up.
#define P2_DIVIDE_ROUND_UP(v, div) ((v + (div - 1)) / div)
	uint32_t w = P2_DIVIDE_ROUND_UP(extent.width, 8);
	uint32_t h = P2_DIVIDE_ROUND_UP(extent.height, 8);
#undef P2_DIVIDE_ROUND_UP

	*out_w = w;
	*out_h = h;
}

static uint32_t
getBlitPipelineId(render_compute_blit_resolve_color_mode color_mode)
{
	return static_cast<uint32_t>(color_mode) - 1;
}

/*
 *
 * Compositor implementations
 *
 */

xrt_result_t
Compositor::setupBlitPipelines(openvr_logger &logger)
{
	VkResult vk_result;

	render_shaders_load(&this->shaders, this->vk);

	struct vk_descriptor_pool_info blit_pool_info = {
	    .uniform_per_descriptor_count = 0,
	    .sampler_per_descriptor_count = 1,
	    .storage_image_per_descriptor_count = 1,
	    .storage_buffer_per_descriptor_count = 0,
	    .descriptor_count = 1,
	    .freeable = false,
	};

	vk_result = vk_create_descriptor_pool( //
	    vk,                                // vk_bundle
	    &blit_pool_info,                   // info
	    &this->blit.descriptor_pool);      // out_descriptor_pool
	if (vk_result != VK_SUCCESS) {
		VK_ERROR(vk, "Failed to create blit descriptor pool for Vulkan compositor, error code: %d", vk_result);
		return XRT_ERROR_VULKAN;
	}
	VK_NAME_DESCRIPTOR_POOL(vk, this->blit.descriptor_pool, "openvr_compositor_vulkan blit descriptor pool");

	vk_result = vk_create_pipeline_cache(vk, &this->blit.driver_pipeline_cache);
	if (vk_result != VK_SUCCESS) {
		VK_ERROR(vk, "Failed to create blit pipeline cache for Vulkan compositor, error code: %d", vk_result);
		return XRT_ERROR_VULKAN;
	}
	VK_NAME_PIPELINE_CACHE(vk, this->blit.driver_pipeline_cache, "openvr_compositor_vulkan blit pipeline cache");

	VkDescriptorSetLayoutBinding set_layout_bindings[2] = {
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

	VkDescriptorSetLayoutCreateInfo set_layout_info = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .bindingCount = ARRAY_SIZE(set_layout_bindings),
	    .pBindings = set_layout_bindings,
	};

	vk_result = vk->vkCreateDescriptorSetLayout( //
	    vk->device,                              //
	    &set_layout_info,                        //
	    NULL,                                    //
	    &this->blit.descriptor_set_layout);      //
	if (vk_result != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreateDescriptorSetLayout failed: %s", vk_result_string(vk_result));
		return XRT_ERROR_VULKAN;
	}
	VK_NAME_DESCRIPTOR_SET_LAYOUT(vk, this->blit.descriptor_set_layout,
	                              "openvr_compositor_vulkan blit descriptor set layout");

	VkPushConstantRange push_constant_range = {
	    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	    .offset = 0,
	    .size = sizeof(render_compute_blit_push_data),
	};
	VkPipelineLayoutCreateInfo pipeline_layout_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .setLayoutCount = 1,
	    .pSetLayouts = &this->blit.descriptor_set_layout,
	    .pushConstantRangeCount = 1,
	    .pPushConstantRanges = &push_constant_range,
	};

	vk_result = vk->vkCreatePipelineLayout( //
	    vk->device,                         // device
	    &pipeline_layout_info,              // pCreateInfo
	    NULL,                               // pAllocator
	    &this->blit.pipeline_layout);       // pPipelineLayout
	if (vk_result != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreatePipelineLayout failed: %s", vk_result_string(vk_result));
		return XRT_ERROR_VULKAN;
	}
	VK_NAME_PIPELINE_LAYOUT(vk, this->blit.pipeline_layout, "openvr_compositor_vulkan blit pipeline layout");

	xrt_result_t xret =
	    render_blit_ms_pipeline_cache_create("openvr_compositor_vulkan blit", &this->blit.pipeline_cache);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	xret = render_blit_ms_pipeline_cache_init( //
	    this->blit.pipeline_cache,             //
	    this->shaders.blit_ms_comp,            //
	    this->blit.pipeline_layout,            //
	    this->blit.driver_pipeline_cache);     //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	constexpr std::array<render_compute_blit_resolve_color_mode, RENDER_BLIT_RESOLVE_COLOR_MODE_COUNT> kColorModes =
	    {
	        RENDER_BLIT_RESOLVE_COLOR_MODE_GAMMA_IN_LINEAR_FORMAT,
	        RENDER_BLIT_RESOLVE_COLOR_MODE_LINEAR_IN_SRGB_FORMAT,
	    };

	const struct render_blit_ms_spec blit_variants[] = {
	    render_make_blit_ms_spec(RENDER_BLIT_RESOLVE_COLOR_MODE_GAMMA_IN_LINEAR_FORMAT),
	    render_make_blit_ms_spec(RENDER_BLIT_RESOLVE_COLOR_MODE_LINEAR_IN_SRGB_FORMAT),
	};

	xret = render_blit_ms_pipeline_cache_prewarm( //
	    this->blit.pipeline_cache,                //
	    this->vk,                                 //
	    blit_variants,                            //
	    std::size(blit_variants));                //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	for (render_compute_blit_resolve_color_mode color_mode : kColorModes) {
		uint32_t pipeline_id = getBlitPipelineId(color_mode);
		assert(pipeline_id < std::size(this->blit.pipelines));

		const render_blit_ms_spec key = render_make_blit_ms_spec(color_mode);

		xret = render_blit_ms_pipeline_cache_get( //
		    this->blit.pipeline_cache,            //
		    this->vk,                             //
		    &key,                                 //
		    &this->blit.pipelines[pipeline_id]);  //
		if (xret != XRT_SUCCESS) {
			return xret;
		}
	}

	// Create the sampler used for the blit operation
	vk_result = vk_create_sampler(this->vk, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, &this->blit.sampler);
	if (vk_result != VK_SUCCESS) {
		OPENVR_LOG_ERROR(logger, "vk_create_sampler: %s", vk_result_string(vk_result));
		return XRT_ERROR_VULKAN;
	}
	VK_NAME_SAMPLER(vk, this->blit.sampler, "OpenVR compositor blit source sampler");

	vk_result = vk_create_descriptor_set(vk, this->blit.descriptor_pool, this->blit.descriptor_set_layout,
	                                     &this->blit.descriptor_set);
	if (vk_result != VK_SUCCESS) {
		OPENVR_LOG_ERROR(logger, "vk_create_descriptor_set: %s", vk_result_string(vk_result));
		return XRT_ERROR_VULKAN;
	}
	VK_NAME_DESCRIPTOR_SET(vk, this->blit.descriptor_set, "OpenVR compositor blit descriptor set");

	OPENVR_LOG_DEBUG(logger, "Successfully set up Vulkan blit pipelines for OpenVR compositor");

	return XRT_SUCCESS;
}

xrt_result_t
Compositor::setupVulkanCompositor(openvr_logger &logger, vr::VRVulkanTextureData_t &vulkan_data)
{
	VkInstance vkInstance = vulkan_data.m_pInstance;
	VkPhysicalDevice vkPhysicalDevice = vulkan_data.m_pPhysicalDevice;
	VkDevice vkDevice = vulkan_data.m_pDevice;
	uint32_t queueFamilyIndex = vulkan_data.m_nQueueFamilyIndex;

	auto vulkanGetInstanceProcAddr = vkGetInstanceProcAddr;

#define GET_PROC(INST, NAME) PFN_vk##NAME loaded_##NAME = (PFN_vk##NAME)vulkanGetInstanceProcAddr(INST, "vk" #NAME)
	GET_PROC(vkInstance, GetDeviceQueue);
	GET_PROC(vkInstance, GetPhysicalDeviceQueueFamilyProperties);
#undef GET_PROC

	uint32_t queueFamilyCount;
	loaded_GetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> properties(queueFamilyCount);
	loaded_GetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, properties.data());

	uint32_t queueCount = properties[queueFamilyIndex].queueCount;
	uint32_t queueIndex = 0;
	for (; queueIndex < queueCount; queueIndex++) {
		VkQueue queue;
		// @todo this is very very dumb, OpenVR doesn't give us queue index, only a VkQueue, so we need this
		//       sillyness to find the queue index to pass into Monado. Perhaps there's a cleaner way?
		loaded_GetDeviceQueue(vkDevice, queueFamilyIndex, queueIndex, &queue);
		if (queue == vulkan_data.m_pQueue) {
			break;
		}
	}

	this->xc_vk = xrt_gfx_vk_provider_create( //
	    this->xcn,                            //
	    vkInstance,                           //
	    vkGetInstanceProcAddr,                //
	    vkPhysicalDevice,                     //
	    vkDevice,                             //
	    false,                                //
	    false,                                //
	    false,                                //
	    false,                                //
	    false,                                //
	    false,                                //
	    queueFamilyIndex,                     //
	    queueIndex);                          //

	if (this->xc_vk == nullptr) {
		OPENVR_LOG_ERROR(logger, "Failed to create Vulkan compositor client");
		return XRT_ERROR_VULKAN;
	}

	xrt_begin_session_info bsi = {
	    .view_type = XRT_VIEW_TYPE_STEREO,
	    .ext_hand_tracking_enabled = false,
	    .ext_hand_tracking_data_source_enabled = false,
	    .ext_eye_gaze_interaction_enabled = false,
	    .ext_future_enabled = false,
	    .ext_hand_interaction_enabled = false,
	    .htc_facial_tracking_enabled = false,
	    .fb_body_tracking_enabled = false,
	    .fb_face_tracking2_enabled = false,
	    .meta_body_tracking_full_body_enabled = false,
	    .meta_body_tracking_calibration_enabled = false,
	    .meta_body_tracking_fidelity_enabled = false,
	    .android_face_tracking_enabled = false,
	};
	xrt_result_t xret = xrt_comp_begin_session(this->xc_vk, &bsi);

	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to begin Vulkan compositor session", xret);

		// Destroy the compositor we created, since it's not usable.
		struct xrt_compositor *xc = this->xc_vk;
		xrt_comp_destroy(&xc);
		this->xc_vk = nullptr;

		return xret;
	}

	// We've created an active compositor, and begun the session.
	this->active_compositor = this->xc_vk;

	client_vk_compositor *c = (client_vk_compositor *)this->xc_vk;

	// Slurp up the vulkan data so we can use it elsewhere in the compositor.
	this->vk = &c->vk;
	this->cmd_pool = &c->pool;

	this->setupBlitPipelines(logger);

	return XRT_SUCCESS;
}

void
Compositor::destroyVulkanResources()
{
	if (this->vk != nullptr) {
		if (this->blit.sampler != VK_NULL_HANDLE) {
			vk->vkDestroySampler(vk->device, this->blit.sampler, NULL);
			this->blit.sampler = VK_NULL_HANDLE;
		}

		for (uint32_t i = 0; i < std::size(this->blit.pipelines); i++) {
			this->blit.pipelines[i] = VK_NULL_HANDLE;
		}

		if (this->blit.pipeline_cache != nullptr) {
			render_blit_ms_pipeline_cache_destroy(&this->blit.pipeline_cache, this->vk);
		}

		if (this->blit.pipeline_layout != VK_NULL_HANDLE) {
			vk->vkDestroyPipelineLayout(vk->device, this->blit.pipeline_layout, NULL);
			this->blit.pipeline_layout = VK_NULL_HANDLE;
		}

		if (this->blit.descriptor_set_layout != VK_NULL_HANDLE) {
			vk->vkDestroyDescriptorSetLayout(vk->device, this->blit.descriptor_set_layout, NULL);
			this->blit.descriptor_set_layout = VK_NULL_HANDLE;
		}

		if (this->blit.driver_pipeline_cache != VK_NULL_HANDLE) {
			vk->vkDestroyPipelineCache(vk->device, this->blit.driver_pipeline_cache, NULL);
			this->blit.driver_pipeline_cache = VK_NULL_HANDLE;
		}

		if (this->blit.descriptor_pool != VK_NULL_HANDLE) {
			vk->vkDestroyDescriptorPool(vk->device, this->blit.descriptor_pool, NULL);
			this->blit.descriptor_pool = VK_NULL_HANDLE;
		}

		render_shaders_fini(&this->shaders, this->vk);

		// Don't free this->vk or this->cmd_pool itself since it is owned by the xrt_compositor_vk
	}
}

xrt_result_t
Compositor::transferAppImageToSwapchainImage(openvr_logger &logger,
                                             xrt_swapchain *xsc,
                                             uint32_t dst_index,
                                             vr::VRVulkanTextureData_t &texture_data,
                                             uint32_t src_array_index,
                                             VkFormat storage_format,
                                             vr::EColorSpace color_space,
                                             const xrt_rect &rect)
{
	client_vk_swapchain *sc = (client_vk_swapchain *)xsc;

	VkResult vk_result;
	const VkFormat src_format = static_cast<VkFormat>(texture_data.m_nFormat);
	const bool source_is_multisampled = texture_data.m_nSampleCount > 1;
	const std::optional<render_compute_blit_resolve_color_mode> resolve_color_mode =
	    determineBlitResolveColorMode(src_format, color_space);
	const bool use_compute_resolve = source_is_multisampled && resolve_color_mode.has_value();

	VkImageView src_view = VK_NULL_HANDLE;
	VkImageView target_view = VK_NULL_HANDLE;

	auto pool = this->cmd_pool;
	bool pool_locked = false;

	// Cleans up any resources that were created
	auto cleanup = [&]() {
		if (use_compute_resolve) {
			vk->vkResetDescriptorPool(vk->device, this->blit.descriptor_pool, 0);
		}

		if (target_view != VK_NULL_HANDLE) {
			vk->vkDestroyImageView(vk->device, target_view, NULL);
			target_view = VK_NULL_HANDLE;
		}

		if (src_view != VK_NULL_HANDLE) {
			vk->vkDestroyImageView(vk->device, src_view, NULL);
			src_view = VK_NULL_HANDLE;
		}

		if (pool_locked) {
			vk_cmd_pool_unlock(pool);
			pool_locked = false;
		}
	};

	VkImage src_image = (VkImage)(texture_data.m_nImage);
	VkImage dst_image = sc->base.images[dst_index];

	// For writing and submitting commands.
	vk_cmd_pool_lock(pool);
	pool_locked = true;

	VkCommandBuffer cmd;
	vk_result = vk_cmd_pool_create_and_begin_cmd_buffer_locked(vk, pool, 0, &cmd);
	if (vk_result != VK_SUCCESS) {
		cleanup();
		return XRT_ERROR_VULKAN;
	}
	VK_NAME_COMMAND_BUFFER(vk, cmd, "BlitAppImageToSwapchainImage command buffer");

	vk_cmd_image_transfer_info transfer_info = {
	    .src =
	        {
	            .params =
	                {
	                    .layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                    .access_mask = VK_ACCESS_TRANSFER_READ_BIT,
	                    .stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT,
	                    .rect = rect,
	                },
	            .fm_image =
	                {
	                    .base_array_layer = src_array_index,
	                    .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
	                    .image = src_image,
	                },
	        },
	    .dst =
	        {
	            .params =
	                {
	                    .layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                    .access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
	                    .stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT,
	                    // Always copy to offset {0,0}, since we're creating the swapchain as submit size and
	                    // putting the app's frame at the origin.
	                    .rect = {.offset = {}, .extent = rect.extent},
	                },
	            .fm_image =
	                {
	                    .base_array_layer = 0,
	                    .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
	                    .image = dst_image,
	                },
	        },
	};

	if (!source_is_multisampled) {
		// If the image isn't multisampled, then we can always just do a flat image copy
		vk_cmd_copy_image_locked(vk, cmd, &transfer_info);
	} else if (!use_compute_resolve) {
		// If we don't need a more custom resolve pipeline, then do a normal image resolve
		vk_cmd_resolve_image_locked(vk, cmd, &transfer_info);
	} else {
		// We need to do a custom image resolve through a compute shader

		VkImageSubresourceRange first_color_level_subresource_range = {
		    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		    .baseMipLevel = 0,
		    .levelCount = 1,
		    .baseArrayLayer = 0,
		    .layerCount = 1,
		};

		vk_result = vk_create_view(vk, src_image, VK_IMAGE_VIEW_TYPE_2D, src_format,
		                           first_color_level_subresource_range, &src_view);
		if (vk_result != VK_SUCCESS) {
			OPENVR_LOG_ERROR(logger, "vk_create_view(src): %s", vk_result_string(vk_result));
			cleanup();
			return XRT_ERROR_VULKAN;
		}
		VK_NAME_IMAGE_VIEW(vk, src_view, "BlitAppImageToSwapchainImage source image view");

		// @todo cache target image views, since we know out own swapchain only has a finite set of images that
		//       will be used, and we know their lifetimes.
		vk_result =
		    vk_create_view_usage(vk, dst_image, VK_IMAGE_VIEW_TYPE_2D, storage_format,
		                         VK_IMAGE_USAGE_STORAGE_BIT, first_color_level_subresource_range, &target_view);
		if (vk_result != VK_SUCCESS) {
			OPENVR_LOG_ERROR(logger, "vk_create_view(dst): %s", vk_result_string(vk_result));
			cleanup();
			return XRT_ERROR_VULKAN;
		}
		VK_NAME_IMAGE_VIEW(vk, target_view, "BlitAppImageToSwapchainImage target image view");

		// Barrier the source image
		vk_cmd_image_barrier_locked(                  //
		    vk,                                       // vk_bundle
		    cmd,                                      // cmdbuffer
		    src_image,                                // image
		    VK_ACCESS_TRANSFER_READ_BIT,              // srcAccessMask
		    VK_ACCESS_SHADER_READ_BIT,                // dstAccessMask
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,     // oldImageLayout
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // newImageLayout
		    VK_PIPELINE_STAGE_TRANSFER_BIT,           // srcStageMask
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,     // dstStageMask
		    first_color_level_subresource_range);     // subresourceRange

		// Barrier the destination image
		vk_cmd_image_barrier_locked(                       //
		    vk,                                            // vk_bundle
		    cmd,                                           // cmdbuffer
		    dst_image,                                     // image
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,          // srcAccessMask
		    VK_ACCESS_SHADER_WRITE_BIT,                    // dstAccessMask
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,      // oldImageLayout
		    VK_IMAGE_LAYOUT_GENERAL,                       // newImageLayout
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,          // dstStageMask
		    first_color_level_subresource_range);          // subresourceRange

		VkDescriptorImageInfo src_image_info = {
		    .sampler = this->blit.sampler,
		    .imageView = src_view,
		    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		VkDescriptorImageInfo target_image_info = {
		    .sampler = VK_NULL_HANDLE,
		    .imageView = target_view,
		    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		};

		VkWriteDescriptorSet write_descriptor_sets[2] = {
		    {
		        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		        .pNext = nullptr,
		        .dstSet = this->blit.descriptor_set,
		        .dstBinding = 0,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		        .pImageInfo = &src_image_info,
		        .pBufferInfo = nullptr,
		        .pTexelBufferView = nullptr,
		    },
		    {
		        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		        .pNext = nullptr,
		        .dstSet = this->blit.descriptor_set,
		        .dstBinding = 1,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		        .pImageInfo = &target_image_info,
		        .pBufferInfo = nullptr,
		        .pTexelBufferView = nullptr,
		    },
		};

		vk->vkUpdateDescriptorSets(vk->device, ARRAY_SIZE(write_descriptor_sets), write_descriptor_sets, 0,
		                           NULL);

		uint32_t pipeline_id = getBlitPipelineId(*resolve_color_mode);
		assert(pipeline_id < std::size(this->blit.pipelines));

		VkPipeline pipeline = this->blit.pipelines[pipeline_id];
		assert(pipeline != VK_NULL_HANDLE);

		vk->vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

		vk->vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->blit.pipeline_layout, 0, 1,
		                            &this->blit.descriptor_set, 0, NULL);

		struct xrt_normalized_rect from_rect = {
		    .x = rect.offset.w / (float)texture_data.m_nWidth,
		    .y = rect.offset.h / (float)texture_data.m_nHeight,
		    .w = rect.extent.w / (float)texture_data.m_nWidth,
		    .h = rect.extent.h / (float)texture_data.m_nHeight,
		};

		render_compute_blit_push_data constants = {
		    .source_rect = from_rect,
		    .target_rect =
		        {
		            .offset = {0, 0},
		            .extent = {rect.extent.w, rect.extent.h},
		        },
		};

		vk->vkCmdPushConstants(cmd, this->blit.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
		                       sizeof(constants), &constants);

		VkExtent2D target_extent = {
		    .width = (uint32_t)rect.extent.w,
		    .height = (uint32_t)rect.extent.h,
		};

		uint32_t w = 0, h = 0;
		calculateDispatchDimensions(target_extent, &w, &h);
		assert(w != 0 && h != 0);

		vk->vkCmdDispatch(cmd, w, h, 1);

		// Barrier the source image back to the game
		vk_cmd_image_barrier_locked(                  //
		    vk,                                       // vk_bundle
		    cmd,                                      // cmdbuffer
		    src_image,                                // image
		    VK_ACCESS_SHADER_READ_BIT,                // srcAccessMask
		    VK_ACCESS_TRANSFER_READ_BIT,              // dstAccessMask
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // oldImageLayout
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,     // newImageLayout
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,     // srcStageMask
		    VK_PIPELINE_STAGE_TRANSFER_BIT,           // dstStageMask
		    first_color_level_subresource_range);     // subresourceRange

		// Barrier the destination image back to the compositor
		vk_cmd_image_barrier_locked(                       //
		    vk,                                            // vk_bundle
		    cmd,                                           // cmdbuffer
		    dst_image,                                     // image
		    VK_ACCESS_SHADER_WRITE_BIT,                    // srcAccessMask
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,          // dstAccessMask
		    VK_IMAGE_LAYOUT_GENERAL,                       // oldImageLayout
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,      // newImageLayout
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,          // srcStageMask
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
		    first_color_level_subresource_range);          // subresourceRange
	}

	vk_result = vk_cmd_pool_end_submit_wait_and_free_cmd_buffer_locked(vk, pool, cmd);

	if (vk_result != VK_SUCCESS) {
		OPENVR_LOG_ERROR(logger, "vk_cmd_pool_end_submit_wait_and_free_cmd_buffer_locked: %s",
		                 vk_result_string(vk_result));
		cleanup();
		return XRT_ERROR_VULKAN;
	}

	cleanup();
	return XRT_SUCCESS;
}

vr::EVRCompositorError
Compositor::submitVulkan(openvr_logger &logger,
                         vr::EVREye eye,
                         vr::VRVulkanTextureData_t &texture_data,
                         uint32_t array_index,
                         vr::EColorSpace color_space,
                         const vr::VRTextureBounds_t &bounds)
{
	// @todo validate what parts of `texture_data` we need to validate and return InvalidTexture when invalid

	OPENVR_LOG_TRACE(logger,
	                 "Submitting Vulkan texture for eye %d, image: %p, format: %d, color space: %d, extents: "
	                 "%dx%d, bounds: %fx%f,%fx%f",
	                 eye, (void *)texture_data.m_nImage, texture_data.m_nFormat, color_space, texture_data.m_nWidth,
	                 texture_data.m_nHeight, bounds.uMin, bounds.vMin, bounds.uMax, bounds.vMax);

	if (this->active_compositor == nullptr) {
		xrt_result_t xret = this->setupVulkanCompositor(logger, texture_data);

		if (xret != XRT_SUCCESS) {
			OPENVR_LOG_ERROR(logger, "Failed to setup Vulkan compositor for submission");
			return xretToCompositorError(xret);
		}
	}

	assert(this->active_compositor != nullptr);

	SwapchainCache &cache = this->swapchain_caches[eye];

	// Resolve the format's colour space to the correct one
	const vr::EColorSpace resolved_color_space =
	    resolveColorSpace(static_cast<VkFormat>(texture_data.m_nFormat), color_space);

	const VkFormat swapchain_sample_format =
	    resolveSwapchainSampleFormat(static_cast<VkFormat>(texture_data.m_nFormat), resolved_color_space);

	xrt_rect source_rect = {
	    .offset =
	        {
	            .w = (int)(bounds.uMin * texture_data.m_nWidth),
	            .h = (int)(bounds.vMin * texture_data.m_nHeight),
	        },
	    .extent =
	        {
	            .w = (int)((bounds.uMax - bounds.uMin) * texture_data.m_nWidth),
	            .h = (int)((bounds.vMax - bounds.vMin) * texture_data.m_nHeight),
	        },
	};

	// Don't let 0 width submits cause problems
	if (source_rect.extent.w == 0 || source_rect.extent.h == 0) {
		return vr::EVRCompositorError::VRCompositorError_None;
	}

	// The storage format
	const VkFormat storage_format =
	    resolveSwapchainStorageFormat(static_cast<VkFormat>(texture_data.m_nFormat), swapchain_sample_format);

	xrt_swapchain_create_flags swapchain_flags = (xrt_swapchain_create_flags)(0);
	xrt_swapchain_usage_bits swapchain_bits =
	    (xrt_swapchain_usage_bits)(XRT_SWAPCHAIN_USAGE_SAMPLED | XRT_SWAPCHAIN_USAGE_TRANSFER_DST);

	if (storage_format != swapchain_sample_format) {
		// Add the "sample as sRGB" flag, since the only time we have a mismatch is when we're using a linear
		// storage buffer, but we need to sample as sRGB.
		swapchain_flags = (xrt_swapchain_create_flags)(swapchain_flags | XRT_SWAPCHAIN_CREATE_SAMPLE_AS_SRGB);
		// This mismatch only happens when we're using our compute shader pipeline for handling sRGB<->linear
		// conversion with multisampled swapchains. In this case, we need unordered access instead of transfer
		// since we're writing to it from a compute shader.
		swapchain_bits =
		    (xrt_swapchain_usage_bits)(XRT_SWAPCHAIN_USAGE_SAMPLED | XRT_SWAPCHAIN_USAGE_UNORDERED_ACCESS |
		                               XRT_SWAPCHAIN_USAGE_MUTABLE_FORMAT);
	}

	xrt_result_t xret =
	    cache.ensureSwapchain(this->active_compositor, storage_format, swapchain_sample_format,
	                          texture_data.m_nWidth, texture_data.m_nHeight, swapchain_bits, swapchain_flags);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to get swapchain for submitted texture", xret);
		return xretToCompositorError(xret);
	}

	xrt_swapchain *xsc = cache.xsc;

	uint32_t image_index;

	// Acquire image
	xret = xrt_swapchain_acquire_image(xsc, &image_index);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to acquire swapchain image", xret);
		return xretToCompositorError(xret);
	}

	// Wait for it to become available
	xret = xrt_swapchain_wait_image(xsc, XRT_INFINITE_DURATION, image_index);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to wait for image to be available", xret);
		return xretToCompositorError(xret);
	}

	// Barrier it for our own use
	xret = xrt_swapchain_barrier_image(xsc, XRT_BARRIER_TO_APP, image_index);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to barrier image for app use", xret);
		return xretToCompositorError(xret);
	}

	// Copy the app texture into the swapchain image
	xret = transferAppImageToSwapchainImage(logger, xsc, image_index, texture_data, array_index, storage_format,
	                                        resolved_color_space, source_rect);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to blit app image to swapchain image", xret);
		return xretToCompositorError(xret);
	}

	// Barrier it for compositor use
	xret = xrt_swapchain_barrier_image(xsc, XRT_BARRIER_TO_COMP, image_index);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to barrier image for compositor use", xret);
		return xretToCompositorError(xret);
	}

	xrt_rect full_swapchain_rect = {
	    .offset = {0, 0},
	    .extent = source_rect.extent,
	};

	this->frame_eye_states[eye] = {
	    .texture =
	        {
	            .xsc = nullptr,
	            .swapchain_index = image_index,

	            .width = cache.width,
	            .height = cache.height,

	            .bounds = full_swapchain_rect,
	        },
	};
	xrt_swapchain_reference(&this->frame_eye_states[eye]->texture.xsc, xsc);

	return vr::EVRCompositorError::VRCompositorError_None;
}

#define UUID_STR_SIZE (XRT_UUID_SIZE * 3 + 1)

// @todo de-dup this all with oxr_vulkan
static void
snprint_uuid(char *str, size_t size, const xrt_uuid_t *uuid)
{
	for (size_t i = 0, offset = 0; i < ARRAY_SIZE(uuid->data) && offset < size; i++, offset += 3) {
		snprintf(str + offset, size - offset, "%02x ", uuid->data[i]);
	}
}

static void
snprint_luid(char *str, size_t size, xrt_luid_t *luid)
{
	for (size_t i = 0, offset = 0; i < ARRAY_SIZE(luid->data) && offset < size; i++, offset += 3) {
		snprintf(str + offset, size - offset, "%02x ", luid->data[i]);
	}
}

void
Compositor::getVulkanOutputDevice(openvr_logger &logger, uint64_t *out_device, VkInstance pInstance)
{
	auto vulkanGetInstanceProcAddr = vkGetInstanceProcAddr;

	// We have no way to give a suggestion in this case.
	// @todo Should we create our own instance here?
	if (pInstance == VK_NULL_HANDLE) {
		*out_device = 0;
		return;
	}

	// @todo TODO: dedup this whole function with oxr_vulkan
#define GET_PROC(INST, NAME) PFN_vk##NAME loaded_##NAME = (PFN_vk##NAME)vulkanGetInstanceProcAddr(INST, "vk" #NAME)
	GET_PROC(pInstance, EnumeratePhysicalDevices);
	GET_PROC(pInstance, GetPhysicalDeviceProperties2KHR);
#undef GET_PROC

	VkResult vk_ret;
	uint32_t count;

	vk_ret = loaded_EnumeratePhysicalDevices(pInstance, &count, NULL);
	if (vk_ret != VK_SUCCESS) {
		OPENVR_LOG_ERROR(logger, "Failed to enumerate physical devices to get output device");
		*out_device = 0;
		return;
	}

	if (count == 0) {
		OPENVR_LOG_WARN(logger, "No Vulkan physical devices found");
		*out_device = 0;
		return;
	}

	std::vector<VkPhysicalDevice> phys(count);
	vk_ret = loaded_EnumeratePhysicalDevices(pInstance, &count, phys.data());
	if (vk_ret != VK_SUCCESS) {
		OPENVR_LOG_ERROR(logger, "Failed to enumerate physical devices to get output device");
		*out_device = 0;
		return;
	}

	char suggested_uuid_str[UUID_STR_SIZE] = {0};
	snprint_uuid(suggested_uuid_str, ARRAY_SIZE(suggested_uuid_str), &this->xsysc->info.client_vk_deviceUUID);

	u_logging_level log_level = debug_get_log_option_compositor_log();
	int gpu_index = -1;
	for (uint32_t i = 0; i < count; i++) {
		VkPhysicalDeviceIDProperties pdidp = {
		    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
		    .pNext = nullptr,
		    .deviceUUID = {},
		    .driverUUID = {},
		    .deviceLUID = {},
		    .deviceNodeMask = 0,
		    .deviceLUIDValid = VK_FALSE,
		};

		VkPhysicalDeviceProperties2 pdp2 = {
		    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		    .pNext = &pdidp,
		    .properties = {},
		};

		loaded_GetPhysicalDeviceProperties2KHR(phys[i], &pdp2);

		// These should always be true
		static_assert(VK_UUID_SIZE == XRT_UUID_SIZE, "uuid sizes mismatch");
		static_assert(ARRAY_SIZE(pdidp.deviceUUID) == XRT_UUID_SIZE, "array size mismatch");

		char buffer[UUID_STR_SIZE] = {0};
		if (log_level <= U_LOGGING_DEBUG) {
			snprint_uuid(buffer, ARRAY_SIZE(buffer), (xrt_uuid_t *)pdidp.deviceUUID);
			// oxr_log(log, "GPU: #%d, uuid: %s", i, buffer);
			if (pdidp.deviceLUIDValid == VK_TRUE) {
				snprint_luid(buffer, ARRAY_SIZE(buffer), (xrt_luid_t *)pdidp.deviceLUID);
				// oxr_log(log, "  LUID: %s", buffer);
			}
		}

		if (memcmp(pdidp.deviceUUID, this->xsysc->info.client_vk_deviceUUID.data, XRT_UUID_SIZE) == 0) {
			gpu_index = i;
			if (log_level <= U_LOGGING_DEBUG) {
				OPENVR_LOG_DEBUG(logger, "Using GPU #%d with uuid %s suggested by runtime", gpu_index,
				                 buffer);
			}
			break;
		}
	}

	if (gpu_index == -1) {
		OPENVR_LOG_WARN(logger, "Did not find runtime suggested GPU, fall back to GPU 0\n\tuuid: %s",
		                suggested_uuid_str);
		gpu_index = 0;
	}

	*out_device = (uint64_t)phys[gpu_index];
}

}; // namespace xrt::state_trackers::openvr
