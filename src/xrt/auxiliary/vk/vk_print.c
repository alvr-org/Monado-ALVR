// Copyright 2019-2022, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Printing helper code.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_vk
 */

#include "util/u_pretty_print.h"

#include "vk/vk_helpers.h"
#include "vk/vk_print.h"


/*
 *
 * Helpers.
 *
 */

#define P(...) u_pp(dg, __VA_ARGS__)
#define PNT(...) u_pp(dg, "\n\t" __VA_ARGS__)
#define PNTT(...) u_pp(dg, "\n\t\t" __VA_ARGS__)
#define PRINT_BITS(BITS, FUNC)                                                                                         \
	do {                                                                                                           \
		for (uint32_t index = 0; index < 32; index++) {                                                        \
			uint32_t bit = (BITS) & (1u << index);                                                         \
			if (!bit) {                                                                                    \
				continue;                                                                              \
			}                                                                                              \
			const char *str = FUNC(bit, true);                                                             \
			if (str != NULL) {                                                                             \
				PNTT("%s", str);                                                                       \
			} else {                                                                                       \
				PNTT("0x%08x", bit);                                                                   \
			}                                                                                              \
		}                                                                                                      \
	} while (false)

static inline void
print_queue_non_null(u_pp_delegate_t dg, const char *prefix, const struct vk_bundle_queue *queue)
{
	assert(queue != NULL);
	PNT("%squeue.queue: %p", prefix, (const void *)queue->queue);
	PNT("%squeue.index: %d", prefix, (int32_t)queue->index);
	PNT("%squeue.family_index: %d", prefix, (int32_t)queue->family_index);
}

static inline void
print_queue(u_pp_delegate_t dg, const char *prefix, const struct vk_bundle_queue *queue)
{
	if (queue != NULL) {
		print_queue_non_null(dg, prefix, queue);
	} else {
		const struct vk_queue_pair null_queue_pair = VK_NULL_QUEUE_PAIR;

		struct vk_bundle_queue null_queue = {
		    .queue = VK_NULL_HANDLE,
		    .family_index = null_queue_pair.family_index,
		    .index = null_queue_pair.index,
		};
		print_queue_non_null(dg, prefix, &null_queue);
	}
}

/*
 *
 * 'Exported' functions.
 *
 */

void
vk_print_result(
    struct vk_bundle *vk, const char *file, int line, const char *calling_func, VkResult ret, const char *called_func)
{
	bool success = ret == VK_SUCCESS;
	enum u_logging_level level = success ? U_LOGGING_INFO : U_LOGGING_ERROR;

	// Should we be logging?
	if (level < vk->log_level) {
		return;
	}

	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	if (success) {
		u_pp(dg, "%s: ", called_func);
	} else {
		u_pp(dg, "%s failed: ", called_func);
	}

	u_pp(dg, "%s [%s:%i]", vk_result_string(ret), file, line);

	u_log(file, line, calling_func, level, "%s", sink.buffer);
}

void
vk_print_device_info(struct vk_bundle *vk,
                     enum u_logging_level log_level,
                     const VkPhysicalDeviceProperties *pdp,
                     uint32_t gpu_index,
                     const char *title)
{
	const char *device_type_string = vk_physical_device_type_string(pdp->deviceType);

	U_LOG_IFL(log_level, vk->log_level,
	          "%s"
	          "\tname: %s\n"
	          "\tvendor: 0x%04x\n"
	          "\tproduct: 0x%04x\n"
	          "\tdeviceType: %s\n"
	          "\tapiVersion: %u.%u.%u\n"
	          "\tdriverVersion: 0x%08x",
	          title,                             //
	          pdp->deviceName,                   //
	          pdp->vendorID,                     //
	          pdp->deviceID,                     //
	          device_type_string,                //
	          VK_VERSION_MAJOR(pdp->apiVersion), //
	          VK_VERSION_MINOR(pdp->apiVersion), //
	          VK_VERSION_PATCH(pdp->apiVersion), //
	          pdp->driverVersion);               // Driver specific
}

void
vk_print_opened_device_info(struct vk_bundle *vk, enum u_logging_level log_level)
{
	VkPhysicalDeviceProperties pdp;
	vk->vkGetPhysicalDeviceProperties(vk->physical_device, &pdp);

	vk_print_device_info(vk, log_level, &pdp, 0, "Device info:\n");
}

void
vk_print_features_info(struct vk_bundle *vk, enum u_logging_level log_level)
{
	U_LOG_IFL(log_level, vk->log_level,                                       //
	          "Features:"                                                     //
	          "\n\ttimestamp_compute_and_graphics: %s"                        //
	          "\n\ttimestamp_period: %f"                                      //
	          "\n\ttimestamp_valid_bits: %u"                                  //
	          "\n\ttimeline_semaphore: %s",                                   //
	          vk->features.timestamp_compute_and_graphics ? "true" : "false", //
	          vk->features.timestamp_period,                                  //
	          vk->features.timestamp_valid_bits,                              //
	          vk->features.timeline_semaphore ? "true" : "false");            //
}

void
vk_print_external_handles_info(struct vk_bundle *vk, enum u_logging_level log_level)
{

#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_WIN32_HANDLE)

	U_LOG_IFL(log_level, vk->log_level,                                               //
	          "Supported images:"                                                     //
	          "\n\t%s:\n\t\tcolor import=%s export=%s\n\t\tdepth import=%s export=%s" //
	          "\n\t%s:\n\t\tcolor import=%s export=%s\n\t\tdepth import=%s export=%s" //
	          ,                                                                       //
	          "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT",                      //
	          vk->external.color_image_import_opaque_win32 ? "true" : "false",        //
	          vk->external.color_image_export_opaque_win32 ? "true" : "false",        //
	          vk->external.depth_image_import_opaque_win32 ? "true" : "false",        //
	          vk->external.depth_image_export_opaque_win32 ? "true" : "false",        //
	          "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT",                     //
	          vk->external.color_image_import_d3d11 ? "true" : "false",               //
	          vk->external.color_image_export_d3d11 ? "true" : "false",               //
	          vk->external.depth_image_import_d3d11 ? "true" : "false",               //
	          vk->external.depth_image_export_d3d11 ? "true" : "false"                //
	);                                                                                //

#elif defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_FD)

	U_LOG_IFL(log_level, vk->log_level,                                               //
	          "Supported images:"                                                     //
	          "\n\t%s:\n\t\tcolor import=%s export=%s\n\t\tdepth import=%s export=%s" //
	          ,                                                                       //
	          "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT",                         //
	          vk->external.color_image_import_opaque_fd ? "true" : "false",           //
	          vk->external.color_image_export_opaque_fd ? "true" : "false",           //
	          vk->external.depth_image_import_opaque_fd ? "true" : "false",           //
	          vk->external.depth_image_export_opaque_fd ? "true" : "false"            //
	);                                                                                //


#elif defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER)

	U_LOG_IFL(log_level, vk->log_level,                                               //
	          "Supported images:"                                                     //
	          "\n\t%s:\n\t\tcolor import=%s export=%s\n\t\tdepth import=%s export=%s" //
	          "\n\t%s:\n\t\tcolor import=%s export=%s\n\t\tdepth import=%s export=%s" //
	          ,                                                                       //
	          "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT",                         //
	          vk->external.color_image_import_opaque_fd ? "true" : "false",           //
	          vk->external.color_image_export_opaque_fd ? "true" : "false",           //
	          vk->external.depth_image_import_opaque_fd ? "true" : "false",           //
	          vk->external.depth_image_export_opaque_fd ? "true" : "false",           //
	          "VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID",   //
	          vk->external.color_image_import_ahardwarebuffer ? "true" : "false",     //
	          vk->external.color_image_export_ahardwarebuffer ? "true" : "false",     //
	          vk->external.depth_image_import_ahardwarebuffer ? "true" : "false",     //
	          vk->external.depth_image_export_ahardwarebuffer ? "true" : "false"      //
	);                                                                                //

#endif

#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)

	U_LOG_IFL(log_level, vk->log_level,                         //
	          "Supported fences:\n\t%s: %s\n\t%s: %s",          //
	          "VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT",      //
	          vk->external.fence_sync_fd ? "true" : "false",    //
	          "VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT",    //
	          vk->external.fence_opaque_fd ? "true" : "false"); //

	U_LOG_IFL(log_level, vk->log_level,                                        //
	          "Supported semaphores:\n\t%s: %s\n\t%s: %s\n\t%s: %s\n\t%s: %s", //
	          "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT(binary)",         //
	          vk->external.binary_semaphore_sync_fd ? "true" : "false",        //
	          "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT(binary)",       //
	          vk->external.binary_semaphore_opaque_fd ? "true" : "false",      //
	          "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT(timeline)",       //
	          vk->external.timeline_semaphore_sync_fd ? "true" : "false",      //
	          "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT(timeline)",     //
	          vk->external.timeline_semaphore_opaque_fd ? "true" : "false");   //

#elif defined(XRT_GRAPHICS_SYNC_HANDLE_IS_WIN32_HANDLE)

	U_LOG_IFL(log_level, vk->log_level,                            //
	          "Supported fences:\n\t%s: %s",                       //
	          "VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT",    //
	          vk->external.fence_win32_handle ? "true" : "false"); //

	U_LOG_IFL(log_level, vk->log_level,                                         //
	          "Supported semaphores:\n\t%s: %s\n\t%s: %s\n\t%s: %s\n\t%s: %s",  //
	          "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT(binary)",      //
	          vk->external.binary_semaphore_d3d12_fence ? "true" : "false",     //
	          "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT(binary)",     //
	          vk->external.binary_semaphore_win32_handle ? "true" : "false",    //
	          "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT(timeline)",    //
	          vk->external.timeline_semaphore_d3d12_fence ? "true" : "false",   //
	          "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT(timeline)",   //
	          vk->external.timeline_semaphore_win32_handle ? "true" : "false"); //

#else
#error "Need port for fence sync handles printers"
#endif
}

void
vk_print_swapchain_create_info(struct vk_bundle *vk, VkSwapchainCreateInfoKHR *i, enum u_logging_level log_level)
{
	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);
	P("VkSwapchainCreateInfoKHR:");
	PNT("flags:");
	PRINT_BITS(i->flags, vk_swapchain_create_flag_string);
	PNT("surface: %" PRIx64, (uint64_t)i->surface);
	PNT("minImageCount: %u", i->minImageCount);
	PNT("imageFormat: %s", vk_format_string(i->imageFormat));
	PNT("imageColorSpace: %s", vk_color_space_string(i->imageColorSpace));
	PNT("imageExtent: {%u, %u}", i->imageExtent.width, i->imageExtent.height);
	PNT("imageArrayLayers: %u", i->imageArrayLayers);
	PNT("imageUsage:");
	PRINT_BITS(i->imageUsage, vk_image_usage_flag_string);
	PNT("imageSharingMode: %s", vk_sharing_mode_string(i->imageSharingMode));
	PNT("queueFamilyIndexCount: %u", i->queueFamilyIndexCount);
	PNT("preTransform: %s", vk_surface_transform_flag_string(i->preTransform, false));
	PNT("compositeAlpha: %s", vk_composite_alpha_flag_string(i->compositeAlpha, false));
	PNT("presentMode: %s", vk_present_mode_string(i->presentMode));
	PNT("clipped: %s", i->clipped ? "VK_TRUE" : "VK_FALSE");
	PNT("oldSwapchain: %" PRIx64, (uint64_t)i->oldSwapchain);

	U_LOG_IFL(log_level, vk->log_level, "%s", sink.buffer);
}

#ifdef VK_KHR_display
void
vk_print_display_surface_create_info(struct vk_bundle *vk,
                                     VkDisplaySurfaceCreateInfoKHR *i,
                                     enum u_logging_level log_level)
{
	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	P("VkDisplaySurfaceCreateInfoKHR:");
	if (i->flags == 0) {
		// No flags defined so only zero is valid.
		PNT("flags:");
	} else {
		// Field reserved for future use, just in case.
		PNT("flags: UNKNOWN FLAG(S) 0x%x", i->flags);
	}
	PNT("displayMode: %" PRIx64, (uint64_t)i->displayMode);
	PNT("planeIndex: %u", i->planeIndex);
	PNT("planeStackIndex: %u", i->planeStackIndex);
	PNT("transform: %s", vk_surface_transform_flag_string(i->transform, false));
	PNT("planeIndex: %f", i->globalAlpha);
	PNT("alphaMode: %s", vk_display_plane_alpha_flag_string(i->alphaMode, false));
	PNT("imageExtent: {%u, %u}", i->imageExtent.width, i->imageExtent.height);

	U_LOG_IFL(log_level, vk->log_level, "%s", sink.buffer);
}
#endif

void
vk_print_queues_info(const struct vk_bundle *vk, enum u_logging_level log_level)
{
	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	P("Selected Queues/Families:");
	print_queue(dg, "main_", vk->main_queue);
	if (vk->graphics_queue != vk->main_queue) {
		print_queue(dg, "graphics_", vk->graphics_queue);
	}
#if defined(VK_KHR_video_encode_queue)
	print_queue(dg, "encode_", vk->encode_queue);
#endif

	U_LOG_IFL(log_level, vk->log_level, "%s", sink.buffer);
}

/*
 *
 * String helper functions.
 *
 */

#define ENUM_TO_STR(r)                                                                                                 \
	case r: return #r

XRT_CHECK_RESULT const char *
vk_result_string(VkResult code)
{
	switch (code) {
		ENUM_TO_STR(VK_SUCCESS);
		ENUM_TO_STR(VK_NOT_READY);
		ENUM_TO_STR(VK_TIMEOUT);
		ENUM_TO_STR(VK_EVENT_SET);
		ENUM_TO_STR(VK_EVENT_RESET);
		ENUM_TO_STR(VK_INCOMPLETE);
		ENUM_TO_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
		ENUM_TO_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
		ENUM_TO_STR(VK_ERROR_INITIALIZATION_FAILED);
		ENUM_TO_STR(VK_ERROR_DEVICE_LOST);
		ENUM_TO_STR(VK_ERROR_MEMORY_MAP_FAILED);
		ENUM_TO_STR(VK_ERROR_LAYER_NOT_PRESENT);
		ENUM_TO_STR(VK_ERROR_EXTENSION_NOT_PRESENT);
		ENUM_TO_STR(VK_ERROR_FEATURE_NOT_PRESENT);
		ENUM_TO_STR(VK_ERROR_INCOMPATIBLE_DRIVER);
		ENUM_TO_STR(VK_ERROR_TOO_MANY_OBJECTS);
		ENUM_TO_STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
		ENUM_TO_STR(VK_ERROR_FRAGMENTED_POOL);
#ifdef VK_VERSION_1_1
		ENUM_TO_STR(VK_ERROR_OUT_OF_POOL_MEMORY);
		ENUM_TO_STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
#endif
#ifdef VK_VERSION_1_2
		ENUM_TO_STR(VK_ERROR_UNKNOWN); // Only defined in 1.2 and above headers.
		ENUM_TO_STR(VK_ERROR_FRAGMENTATION);
		ENUM_TO_STR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
#endif
#ifdef VK_VERSION_1_3
		ENUM_TO_STR(VK_PIPELINE_COMPILE_REQUIRED);
#endif
#ifdef VK_KHR_surface
		ENUM_TO_STR(VK_ERROR_SURFACE_LOST_KHR);
		ENUM_TO_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
#endif
#ifdef VK_KHR_swapchain
		ENUM_TO_STR(VK_SUBOPTIMAL_KHR);
		ENUM_TO_STR(VK_ERROR_OUT_OF_DATE_KHR);
#endif
#ifdef VK_KHR_display_swapchain
		ENUM_TO_STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
#endif
#ifdef VK_EXT_debug_report
		ENUM_TO_STR(VK_ERROR_VALIDATION_FAILED_EXT);
#endif
#ifdef VK_NV_glsl_shader
		ENUM_TO_STR(VK_ERROR_INVALID_SHADER_NV);
#endif
#if defined(VK_ENABLE_BETA_EXTENSIONS) && defined(VK_KHR_video_queue)
		ENUM_TO_STR(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR);
		ENUM_TO_STR(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR);
		ENUM_TO_STR(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR);
		ENUM_TO_STR(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR);
		ENUM_TO_STR(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR);
		ENUM_TO_STR(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_EXT_image_drm_format_modifier
		ENUM_TO_STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
#endif
#ifdef VK_KHR_global_priority
		ENUM_TO_STR(VK_ERROR_NOT_PERMITTED_KHR);
#endif
#ifdef VK_EXT_full_screen_exclusive
		ENUM_TO_STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
#endif
#ifdef VK_KHR_deferred_host_operations
		ENUM_TO_STR(VK_THREAD_IDLE_KHR);
#endif
#ifdef VK_KHR_deferred_host_operations
		ENUM_TO_STR(VK_THREAD_DONE_KHR);
#endif
#ifdef VK_KHR_deferred_host_operations
		ENUM_TO_STR(VK_OPERATION_DEFERRED_KHR);
#endif
#ifdef VK_KHR_deferred_host_operations
		ENUM_TO_STR(VK_OPERATION_NOT_DEFERRED_KHR);
#endif
#ifdef VK_EXT_image_compression_control
		ENUM_TO_STR(VK_ERROR_COMPRESSION_EXHAUSTED_EXT);
#endif
#if defined(VK_KHR_maintenance1) && !defined(VK_VERSION_1_1)
		ENUM_TO_STR(VK_ERROR_OUT_OF_POOL_MEMORY_KHR);
#endif
#if defined(VK_KHR_external_memory) && !defined(VK_VERSION_1_1)
		ENUM_TO_STR(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR);
#endif
#if defined(VK_EXT_descriptor_indexing) && !defined(VK_VERSION_1_2)
		ENUM_TO_STR(VK_ERROR_FRAGMENTATION_EXT);
#endif
#if defined(VK_EXT_global_priority) && !defined(VK_KHR_global_priority)
		ENUM_TO_STR(VK_ERROR_NOT_PERMITTED_EXT);
#endif
#if defined(VK_EXT_buffer_device_address) && !defined(VK_VERSION_1_2)
		ENUM_TO_STR(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT);
		// VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR = VK_ERROR_INVALID_DEVICE_ADDRESS_EXT
#endif
#if defined(VK_EXT_pipeline_creation_cache_control) && !defined(VK_VERSION_1_3)
		ENUM_TO_STR(VK_PIPELINE_COMPILE_REQUIRED_EXT);
		// VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT = VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT
#endif
	default: return "UNKNOWN RESULT";
	}
}

XRT_CHECK_RESULT const char *
vk_object_type_string(VkObjectType type)
{
	switch (type) {
		ENUM_TO_STR(VK_OBJECT_TYPE_UNKNOWN);
		ENUM_TO_STR(VK_OBJECT_TYPE_INSTANCE);
		ENUM_TO_STR(VK_OBJECT_TYPE_PHYSICAL_DEVICE);
		ENUM_TO_STR(VK_OBJECT_TYPE_DEVICE);
		ENUM_TO_STR(VK_OBJECT_TYPE_QUEUE);
		ENUM_TO_STR(VK_OBJECT_TYPE_SEMAPHORE);
		ENUM_TO_STR(VK_OBJECT_TYPE_COMMAND_BUFFER);
		ENUM_TO_STR(VK_OBJECT_TYPE_FENCE);
		ENUM_TO_STR(VK_OBJECT_TYPE_DEVICE_MEMORY);
		ENUM_TO_STR(VK_OBJECT_TYPE_BUFFER);
		ENUM_TO_STR(VK_OBJECT_TYPE_IMAGE);
		ENUM_TO_STR(VK_OBJECT_TYPE_EVENT);
		ENUM_TO_STR(VK_OBJECT_TYPE_QUERY_POOL);
		ENUM_TO_STR(VK_OBJECT_TYPE_BUFFER_VIEW);
		ENUM_TO_STR(VK_OBJECT_TYPE_IMAGE_VIEW);
		ENUM_TO_STR(VK_OBJECT_TYPE_SHADER_MODULE);
		ENUM_TO_STR(VK_OBJECT_TYPE_PIPELINE_CACHE);
		ENUM_TO_STR(VK_OBJECT_TYPE_PIPELINE_LAYOUT);
		ENUM_TO_STR(VK_OBJECT_TYPE_RENDER_PASS);
		ENUM_TO_STR(VK_OBJECT_TYPE_PIPELINE);
		ENUM_TO_STR(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT);
		ENUM_TO_STR(VK_OBJECT_TYPE_SAMPLER);
		ENUM_TO_STR(VK_OBJECT_TYPE_DESCRIPTOR_POOL);
		ENUM_TO_STR(VK_OBJECT_TYPE_DESCRIPTOR_SET);
		ENUM_TO_STR(VK_OBJECT_TYPE_FRAMEBUFFER);
		ENUM_TO_STR(VK_OBJECT_TYPE_COMMAND_POOL);
#ifdef VK_VERSION_1_1
		ENUM_TO_STR(VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE);
#elif defined(VK_KHR_descriptor_update_template)
		ENUM_TO_STR(VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR);
#endif
#ifdef VK_VERSION_1_1
		ENUM_TO_STR(VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION);
#elif defined(VK_KHR_sampler_ycbcr_conversion)
		ENUM_TO_STR(VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR);
#endif
#ifdef VK_VERSION_1_3
		ENUM_TO_STR(VK_OBJECT_TYPE_PRIVATE_DATA_SLOT);
#elif defined(VK_EXT_private_data)
		ENUM_TO_STR(VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT);
#endif
#ifdef VK_KHR_surface
		ENUM_TO_STR(VK_OBJECT_TYPE_SURFACE_KHR);
#endif
#ifdef VK_KHR_swapchain
		ENUM_TO_STR(VK_OBJECT_TYPE_SWAPCHAIN_KHR);
#endif
#ifdef VK_KHR_display
		ENUM_TO_STR(VK_OBJECT_TYPE_DISPLAY_KHR);
		ENUM_TO_STR(VK_OBJECT_TYPE_DISPLAY_MODE_KHR);
#endif
#ifdef VK_EXT_debug_report
		ENUM_TO_STR(VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT);
#endif
#ifdef VK_KHR_video_queue
		ENUM_TO_STR(VK_OBJECT_TYPE_VIDEO_SESSION_KHR);
		ENUM_TO_STR(VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR);
#endif
#if defined(VK_ENABLE_BETA_EXTENSIONS) && defined(VK_NVX_binary_import)
		ENUM_TO_STR(VK_OBJECT_TYPE_CU_MODULE_NVX);
		ENUM_TO_STR(VK_OBJECT_TYPE_CU_FUNCTION_NVX);
#endif
#ifdef VK_EXT_debug_utils
		ENUM_TO_STR(VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT);
#endif
#ifdef VK_KHR_acceleration_structure
		ENUM_TO_STR(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
#endif
#ifdef VK_EXT_validation_cache
		ENUM_TO_STR(VK_OBJECT_TYPE_VALIDATION_CACHE_EXT);
#endif
#ifdef VK_NV_ray_tracing
		ENUM_TO_STR(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV);
#endif
#ifdef VK_INTEL_performance_query
		ENUM_TO_STR(VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL);
#endif
#ifdef VK_KHR_deferred_host_operations
		ENUM_TO_STR(VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR);
#endif
#ifdef VK_NV_device_generated_commands
		ENUM_TO_STR(VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV);
#endif
#if defined(VK_ENABLE_BETA_EXTENSIONS) && defined(VK_NV_cuda_kernel_launch)
		ENUM_TO_STR(VK_OBJECT_TYPE_CUDA_MODULE_NV);
		ENUM_TO_STR(VK_OBJECT_TYPE_CUDA_FUNCTION_NV);
#endif
#ifdef VK_FUCHSIA_buffer_collection
		ENUM_TO_STR(VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA);
#endif
#ifdef VK_EXT_opacity_micromap
		ENUM_TO_STR(VK_OBJECT_TYPE_MICROMAP_EXT);
#endif
#ifdef VK_NV_optical_flow
		ENUM_TO_STR(VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV);
#endif
#ifdef VK_EXT_shader_object
		ENUM_TO_STR(VK_OBJECT_TYPE_SHADER_EXT);
#endif
	default: return "UNKNOWN OBJECT TYPE";
	}
}

XRT_CHECK_RESULT const char *
vk_physical_device_type_string(VkPhysicalDeviceType device_type)
{
	switch (device_type) {
		ENUM_TO_STR(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
		ENUM_TO_STR(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
		ENUM_TO_STR(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU);
		ENUM_TO_STR(VK_PHYSICAL_DEVICE_TYPE_CPU);
	default: return "UNKNOWN PHYSICAL DEVICE TYPE";
	}
}

XRT_CHECK_RESULT const char *
vk_format_string(VkFormat code)
{
	switch (code) {
		ENUM_TO_STR(VK_FORMAT_UNDEFINED);
		ENUM_TO_STR(VK_FORMAT_R4G4_UNORM_PACK8);
		ENUM_TO_STR(VK_FORMAT_R4G4B4A4_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_R5G6B5_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_B5G6R5_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_R5G5B5A1_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_B5G5R5A1_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_A1R5G5B5_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_R8_UNORM);
		ENUM_TO_STR(VK_FORMAT_R8_SNORM);
		ENUM_TO_STR(VK_FORMAT_R8_USCALED);
		ENUM_TO_STR(VK_FORMAT_R8_SSCALED);
		ENUM_TO_STR(VK_FORMAT_R8_UINT);
		ENUM_TO_STR(VK_FORMAT_R8_SINT);
		ENUM_TO_STR(VK_FORMAT_R8_SRGB);
		ENUM_TO_STR(VK_FORMAT_R8G8_UNORM);
		ENUM_TO_STR(VK_FORMAT_R8G8_SNORM);
		ENUM_TO_STR(VK_FORMAT_R8G8_USCALED);
		ENUM_TO_STR(VK_FORMAT_R8G8_SSCALED);
		ENUM_TO_STR(VK_FORMAT_R8G8_UINT);
		ENUM_TO_STR(VK_FORMAT_R8G8_SINT);
		ENUM_TO_STR(VK_FORMAT_R8G8_SRGB);
		ENUM_TO_STR(VK_FORMAT_R8G8B8_UNORM);
		ENUM_TO_STR(VK_FORMAT_R8G8B8_SNORM);
		ENUM_TO_STR(VK_FORMAT_R8G8B8_USCALED);
		ENUM_TO_STR(VK_FORMAT_R8G8B8_SSCALED);
		ENUM_TO_STR(VK_FORMAT_R8G8B8_UINT);
		ENUM_TO_STR(VK_FORMAT_R8G8B8_SINT);
		ENUM_TO_STR(VK_FORMAT_R8G8B8_SRGB);
		ENUM_TO_STR(VK_FORMAT_B8G8R8_UNORM);
		ENUM_TO_STR(VK_FORMAT_B8G8R8_SNORM);
		ENUM_TO_STR(VK_FORMAT_B8G8R8_USCALED);
		ENUM_TO_STR(VK_FORMAT_B8G8R8_SSCALED);
		ENUM_TO_STR(VK_FORMAT_B8G8R8_UINT);
		ENUM_TO_STR(VK_FORMAT_B8G8R8_SINT);
		ENUM_TO_STR(VK_FORMAT_B8G8R8_SRGB);
		ENUM_TO_STR(VK_FORMAT_R8G8B8A8_UNORM);
		ENUM_TO_STR(VK_FORMAT_R8G8B8A8_SNORM);
		ENUM_TO_STR(VK_FORMAT_R8G8B8A8_USCALED);
		ENUM_TO_STR(VK_FORMAT_R8G8B8A8_SSCALED);
		ENUM_TO_STR(VK_FORMAT_R8G8B8A8_UINT);
		ENUM_TO_STR(VK_FORMAT_R8G8B8A8_SINT);
		ENUM_TO_STR(VK_FORMAT_R8G8B8A8_SRGB);
		ENUM_TO_STR(VK_FORMAT_B8G8R8A8_UNORM);
		ENUM_TO_STR(VK_FORMAT_B8G8R8A8_SNORM);
		ENUM_TO_STR(VK_FORMAT_B8G8R8A8_USCALED);
		ENUM_TO_STR(VK_FORMAT_B8G8R8A8_SSCALED);
		ENUM_TO_STR(VK_FORMAT_B8G8R8A8_UINT);
		ENUM_TO_STR(VK_FORMAT_B8G8R8A8_SINT);
		ENUM_TO_STR(VK_FORMAT_B8G8R8A8_SRGB);
		ENUM_TO_STR(VK_FORMAT_A8B8G8R8_UNORM_PACK32);
		ENUM_TO_STR(VK_FORMAT_A8B8G8R8_SNORM_PACK32);
		ENUM_TO_STR(VK_FORMAT_A8B8G8R8_USCALED_PACK32);
		ENUM_TO_STR(VK_FORMAT_A8B8G8R8_SSCALED_PACK32);
		ENUM_TO_STR(VK_FORMAT_A8B8G8R8_UINT_PACK32);
		ENUM_TO_STR(VK_FORMAT_A8B8G8R8_SINT_PACK32);
		ENUM_TO_STR(VK_FORMAT_A8B8G8R8_SRGB_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2R10G10B10_UNORM_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2R10G10B10_USCALED_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2R10G10B10_SSCALED_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2R10G10B10_UINT_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2R10G10B10_SINT_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2B10G10R10_UINT_PACK32);
		ENUM_TO_STR(VK_FORMAT_A2B10G10R10_SINT_PACK32);
		ENUM_TO_STR(VK_FORMAT_R16_UNORM);
		ENUM_TO_STR(VK_FORMAT_R16_SNORM);
		ENUM_TO_STR(VK_FORMAT_R16_USCALED);
		ENUM_TO_STR(VK_FORMAT_R16_SSCALED);
		ENUM_TO_STR(VK_FORMAT_R16_UINT);
		ENUM_TO_STR(VK_FORMAT_R16_SINT);
		ENUM_TO_STR(VK_FORMAT_R16_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R16G16_UNORM);
		ENUM_TO_STR(VK_FORMAT_R16G16_SNORM);
		ENUM_TO_STR(VK_FORMAT_R16G16_USCALED);
		ENUM_TO_STR(VK_FORMAT_R16G16_SSCALED);
		ENUM_TO_STR(VK_FORMAT_R16G16_UINT);
		ENUM_TO_STR(VK_FORMAT_R16G16_SINT);
		ENUM_TO_STR(VK_FORMAT_R16G16_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R16G16B16_UNORM);
		ENUM_TO_STR(VK_FORMAT_R16G16B16_SNORM);
		ENUM_TO_STR(VK_FORMAT_R16G16B16_USCALED);
		ENUM_TO_STR(VK_FORMAT_R16G16B16_SSCALED);
		ENUM_TO_STR(VK_FORMAT_R16G16B16_UINT);
		ENUM_TO_STR(VK_FORMAT_R16G16B16_SINT);
		ENUM_TO_STR(VK_FORMAT_R16G16B16_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R16G16B16A16_UNORM);
		ENUM_TO_STR(VK_FORMAT_R16G16B16A16_SNORM);
		ENUM_TO_STR(VK_FORMAT_R16G16B16A16_USCALED);
		ENUM_TO_STR(VK_FORMAT_R16G16B16A16_SSCALED);
		ENUM_TO_STR(VK_FORMAT_R16G16B16A16_UINT);
		ENUM_TO_STR(VK_FORMAT_R16G16B16A16_SINT);
		ENUM_TO_STR(VK_FORMAT_R16G16B16A16_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R32_UINT);
		ENUM_TO_STR(VK_FORMAT_R32_SINT);
		ENUM_TO_STR(VK_FORMAT_R32_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R32G32_UINT);
		ENUM_TO_STR(VK_FORMAT_R32G32_SINT);
		ENUM_TO_STR(VK_FORMAT_R32G32_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R32G32B32_UINT);
		ENUM_TO_STR(VK_FORMAT_R32G32B32_SINT);
		ENUM_TO_STR(VK_FORMAT_R32G32B32_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R32G32B32A32_UINT);
		ENUM_TO_STR(VK_FORMAT_R32G32B32A32_SINT);
		ENUM_TO_STR(VK_FORMAT_R32G32B32A32_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R64_UINT);
		ENUM_TO_STR(VK_FORMAT_R64_SINT);
		ENUM_TO_STR(VK_FORMAT_R64_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R64G64_UINT);
		ENUM_TO_STR(VK_FORMAT_R64G64_SINT);
		ENUM_TO_STR(VK_FORMAT_R64G64_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R64G64B64_UINT);
		ENUM_TO_STR(VK_FORMAT_R64G64B64_SINT);
		ENUM_TO_STR(VK_FORMAT_R64G64B64_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_R64G64B64A64_UINT);
		ENUM_TO_STR(VK_FORMAT_R64G64B64A64_SINT);
		ENUM_TO_STR(VK_FORMAT_R64G64B64A64_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
		ENUM_TO_STR(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
		ENUM_TO_STR(VK_FORMAT_D16_UNORM);
		ENUM_TO_STR(VK_FORMAT_X8_D24_UNORM_PACK32);
		ENUM_TO_STR(VK_FORMAT_D32_SFLOAT);
		ENUM_TO_STR(VK_FORMAT_S8_UINT);
		ENUM_TO_STR(VK_FORMAT_D16_UNORM_S8_UINT);
		ENUM_TO_STR(VK_FORMAT_D24_UNORM_S8_UINT);
		ENUM_TO_STR(VK_FORMAT_D32_SFLOAT_S8_UINT);
		ENUM_TO_STR(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC1_RGB_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC2_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC2_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC3_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC3_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC4_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC4_SNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC5_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC5_SNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC6H_UFLOAT_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC6H_SFLOAT_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC7_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_BC7_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_EAC_R11_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_EAC_R11_SNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
		ENUM_TO_STR(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
#ifdef VK_VERSION_1_1
		ENUM_TO_STR(VK_FORMAT_G8B8G8R8_422_UNORM);
		ENUM_TO_STR(VK_FORMAT_B8G8R8G8_422_UNORM);
		ENUM_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM);
		ENUM_TO_STR(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
		ENUM_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM);
		ENUM_TO_STR(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM);
		ENUM_TO_STR(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM);
		ENUM_TO_STR(VK_FORMAT_R10X6_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_R10X6G10X6_UNORM_2PACK16);
		ENUM_TO_STR(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16);
		ENUM_TO_STR(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16);
		ENUM_TO_STR(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16);
		ENUM_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_R12X4_UNORM_PACK16);
		ENUM_TO_STR(VK_FORMAT_R12X4G12X4_UNORM_2PACK16);
		ENUM_TO_STR(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16);
		ENUM_TO_STR(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16);
		ENUM_TO_STR(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16);
		ENUM_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16);
		ENUM_TO_STR(VK_FORMAT_G16B16G16R16_422_UNORM);
		ENUM_TO_STR(VK_FORMAT_B16G16R16G16_422_UNORM);
		ENUM_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM);
		ENUM_TO_STR(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM);
		ENUM_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM);
		ENUM_TO_STR(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM);
		ENUM_TO_STR(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM);
#endif // VK_VERSION_1_1
#ifdef VK_IMG_format_pvrtc
		ENUM_TO_STR(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
		ENUM_TO_STR(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
		ENUM_TO_STR(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
		ENUM_TO_STR(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
		ENUM_TO_STR(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
		ENUM_TO_STR(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
		ENUM_TO_STR(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
		ENUM_TO_STR(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);
#endif // VK_IMG_format_pvrtc
#ifdef VK_EXT_texture_compression_astc_hdr
		ENUM_TO_STR(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT);
		ENUM_TO_STR(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT);
#endif // VK_EXT_texture_compression_astc_hdr
#ifdef VK_EXT_4444_formats
		ENUM_TO_STR(VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT);
		ENUM_TO_STR(VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT);
#endif // VK_EXT_4444_formats
	default: return "UNKNOWN FORMAT";
	}
}

XRT_CHECK_RESULT const char *
vk_sharing_mode_string(VkSharingMode code)
{
	switch (code) {
		ENUM_TO_STR(VK_SHARING_MODE_EXCLUSIVE);
		ENUM_TO_STR(VK_SHARING_MODE_CONCURRENT);
	default: return "UNKNOWN SHARING MODE";
	}
}

XRT_CHECK_RESULT const char *
vk_present_mode_string(VkPresentModeKHR code)
{
	switch (code) {
		ENUM_TO_STR(VK_PRESENT_MODE_FIFO_KHR);
		ENUM_TO_STR(VK_PRESENT_MODE_MAILBOX_KHR);
		ENUM_TO_STR(VK_PRESENT_MODE_IMMEDIATE_KHR);
		ENUM_TO_STR(VK_PRESENT_MODE_FIFO_RELAXED_KHR);
		ENUM_TO_STR(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR);
		ENUM_TO_STR(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR);
#if defined(VK_KHR_present_mode_fifo_latest_ready)
		ENUM_TO_STR(VK_PRESENT_MODE_FIFO_LATEST_READY_KHR);
#elif defined(VK_EXT_present_mode_fifo_latest_ready)
		ENUM_TO_STR(VK_PRESENT_MODE_FIFO_LATEST_READY_EXT);
#endif
	default: return "UNKNOWN MODE";
	}
}

XRT_CHECK_RESULT const char *
vk_color_space_string(VkColorSpaceKHR code)
{
	switch (code) {
		ENUM_TO_STR(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
#ifdef VK_EXT_swapchain_colorspace
		ENUM_TO_STR(VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_BT709_LINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_BT709_NONLINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_BT2020_LINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_HDR10_ST2084_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_DOLBYVISION_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_HDR10_HLG_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_PASS_THROUGH_EXT);
		ENUM_TO_STR(VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT);
#endif
#ifdef VK_AMD_display_native_hdr
		ENUM_TO_STR(VK_COLOR_SPACE_DISPLAY_NATIVE_AMD);
#endif
	default: return "UNKNOWN COLOR SPACE";
	}
}

XRT_CHECK_RESULT const char *
vk_power_state_string(VkDisplayPowerStateEXT code)
{
	switch (code) {
		ENUM_TO_STR(VK_DISPLAY_POWER_STATE_OFF_EXT);
		ENUM_TO_STR(VK_DISPLAY_POWER_STATE_SUSPEND_EXT);
		ENUM_TO_STR(VK_DISPLAY_POWER_STATE_ON_EXT);
	default: return "UNKNOWN MODE";
	}
}

XRT_CHECK_RESULT const char *
vk_format_feature_flag_string(VkFormatFeatureFlagBits bits, bool null_on_unknown)
{
	switch (bits) {
		ENUM_TO_STR(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		ENUM_TO_STR(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
		ENUM_TO_STR(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		ENUM_TO_STR(VK_FORMAT_FEATURE_TRANSFER_SRC_BIT);
		ENUM_TO_STR(VK_FORMAT_FEATURE_TRANSFER_DST_BIT);
	default:
		if (bits == 0) {
			return "FORMAT FEATURE: NO BITS SET";
		} else if (bits & (bits - 1)) {
			return "FORMAT FEATURE: MULTIPLE BITS SET";
		} else {
			return null_on_unknown ? NULL : "FORMAT FEATURE: UNKNOWN BIT";
		}
	}
}

XRT_CHECK_RESULT const char *
vk_swapchain_create_flag_string(VkSwapchainCreateFlagsKHR bits, bool null_on_unknown)
{
	switch (bits) {
#if (defined(VK_VERSION_1_1) || defined(VK_KHR_device_group)) && defined(VK_KHR_swapchain)
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR);
#endif
#if defined(VK_VERSION_1_1) && defined(VK_KHR_swapchain)
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR);
#endif
#ifdef VK_KHR_swapchain_mutable_format
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR);
#endif
#ifdef VK_KHR_swapchain_maintenance1
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_DEFERRED_MEMORY_ALLOCATION_BIT_KHR);
#endif
#ifdef VK_KHR_present_id2
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_PRESENT_ID_2_BIT_KHR);
#endif
#ifdef VK_KHR_present_wait2
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_PRESENT_WAIT_2_BIT_KHR);
#endif
#ifdef VK_EXT_multisampled_render_to_swapchain
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_BIT_EXT);
#endif
#ifdef VK_EXT_present_timing
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_PRESENT_TIMING_BIT_EXT);
#endif
#if defined(VK_EXT_swapchain_maintenance1) && !defined(VK_KHR_swapchain_maintenance1)
		ENUM_TO_STR(VK_SWAPCHAIN_CREATE_DEFERRED_MEMORY_ALLOCATION_BIT_EXT);
#endif
	default:
		if (bits == 0) {
			return "SWAPCHAIN CREATE: NO BITS SET";
		} else if (bits & (bits - 1)) {
			return "SWAPCHAIN CREATE: MULTIPLE BITS SET";
		} else {
			return null_on_unknown ? NULL : "SWAPCHAIN CREATE: UNKNOWN BIT";
		}
	}
}

XRT_CHECK_RESULT const char *
vk_image_usage_flag_string(VkImageUsageFlagBits bits, bool null_on_unknown)
{
	switch (bits) {
		ENUM_TO_STR(VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		ENUM_TO_STR(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		ENUM_TO_STR(VK_IMAGE_USAGE_SAMPLED_BIT);
		ENUM_TO_STR(VK_IMAGE_USAGE_STORAGE_BIT);
		ENUM_TO_STR(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		ENUM_TO_STR(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		ENUM_TO_STR(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		ENUM_TO_STR(VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
#ifdef VK_KHR_video_decode_queue
		ENUM_TO_STR(VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR);
		ENUM_TO_STR(VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR);
		ENUM_TO_STR(VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR);
#endif
#ifdef VK_EXT_fragment_density_map
		ENUM_TO_STR(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT);
#endif
#ifdef VK_KHR_fragment_shading_rate
		ENUM_TO_STR(VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR);
#endif
#ifdef VK_EXT_host_image_copy
		ENUM_TO_STR(VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT);
#endif
#ifdef VK_EXT_attachment_feedback_loop_layout
		ENUM_TO_STR(VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT);
#endif
#ifdef VK_HUAWEI_invocation_mask
		ENUM_TO_STR(VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI);
#endif
#ifdef VK_QCOM_image_processing
		ENUM_TO_STR(VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM);
#endif
#ifdef VK_QCOM_image_processing
		ENUM_TO_STR(VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM);
#endif
#if defined(VK_NV_shading_rate_image) && !defined(VK_KHR_fragment_shading_rate)
		ENUM_TO_STR(VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV);
#endif
	default:
		if (bits == 0) {
			return "IMAGE USAGE: NO BITS SET";
		} else if (bits & (bits - 1)) {
			return "IMAGE USAGE: MULTIPLE BITS SET";
		} else {
			return null_on_unknown ? NULL : "IMAGE USAGE: UNKNOWN BIT";
		}
	}
}

XRT_CHECK_RESULT const char *
vk_composite_alpha_flag_string(VkCompositeAlphaFlagBitsKHR bits, bool null_on_unknown)
{
	switch (bits) {
		ENUM_TO_STR(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
		ENUM_TO_STR(VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR);
		ENUM_TO_STR(VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR);
		ENUM_TO_STR(VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);
	default:
		if (bits == 0) {
			return "COMPOSITE ALPHA: NO BITS SET";
		} else if (bits & (bits - 1)) {
			return "COMPOSITE ALPHA: MULTIPLE BITS SET";
		} else {
			return null_on_unknown ? NULL : "COMPOSITE ALPHA: UNKNOWN BIT";
		}
	}
}

XRT_CHECK_RESULT const char *
vk_surface_transform_flag_string(VkSurfaceTransformFlagBitsKHR bits, bool null_on_unknown)
{
	switch (bits) {
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR);
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR);
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR);
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR);
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR);
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR);
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR);
		ENUM_TO_STR(VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR);
	default:
		if (bits == 0) {
			return "SURFACE TRANSFORM: NO BITS SET";
		} else if (bits & (bits - 1)) {
			return "SURFACE TRANSFORM: MULTIPLE BITS SET";
		} else {
			return null_on_unknown ? NULL : "SURFACE TRANSFORM: UNKNOWN BIT";
		}
	}
}

#ifdef VK_KHR_display
XRT_CHECK_RESULT const char *
vk_display_plane_alpha_flag_string(VkDisplayPlaneAlphaFlagBitsKHR bits, bool null_on_unknown)
{
	switch (bits) {
		ENUM_TO_STR(VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR);
		ENUM_TO_STR(VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR);
		ENUM_TO_STR(VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR);
		ENUM_TO_STR(VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR);
	default:
		if (bits == 0) {
			return "DISPLAY PLANE ALPHA: NO BITS SET";
		} else if (bits & (bits - 1)) {
			return "DISPLAY PLANE ALPHA: MULTIPLE BITS SET";
		} else {
			return null_on_unknown ? NULL : "DISPLAY PLANE ALPHA: UNKNOWN BIT";
		}
	}
}
#endif

XRT_CHECK_RESULT const char *
xrt_swapchain_usage_flag_string(enum xrt_swapchain_usage_bits bits, bool null_on_unknown)
{
	switch (bits) {
		ENUM_TO_STR(XRT_SWAPCHAIN_USAGE_COLOR);
		ENUM_TO_STR(XRT_SWAPCHAIN_USAGE_DEPTH_STENCIL);
		ENUM_TO_STR(XRT_SWAPCHAIN_USAGE_UNORDERED_ACCESS);
		ENUM_TO_STR(XRT_SWAPCHAIN_USAGE_TRANSFER_SRC);
		ENUM_TO_STR(XRT_SWAPCHAIN_USAGE_TRANSFER_DST);
		ENUM_TO_STR(XRT_SWAPCHAIN_USAGE_SAMPLED);
		ENUM_TO_STR(XRT_SWAPCHAIN_USAGE_MUTABLE_FORMAT);
		ENUM_TO_STR(XRT_SWAPCHAIN_USAGE_INPUT_ATTACHMENT);
	default:
		if (bits == 0) {
			return "XRT SWAPCHAIN USAGE: NO BITS SET";
		} else if (bits & (bits - 1)) {
			return "XRT SWAPCHAIN USAGE: MULTIPLE BITS SET";
		} else {
			return null_on_unknown ? NULL : "XRT SWAPCHAIN USAGE: UNKNOWN BIT";
		}
	}
}
