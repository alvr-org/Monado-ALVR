// Copyright 2019-2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Printing helper code.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @ingroup aux_vk
 */

#include "util/u_pretty_print.h"
#include "vk/vk_helpers.h"


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
	PNT("surface: %p", (void *)i->surface);
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
	PNT("oldSwapchain: %p", (void *)i->oldSwapchain);

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
	PNT("displayMode: %p", (void *)i->displayMode);
	PNT("planeIndex: %u", i->planeIndex);
	PNT("planeStackIndex: %u", i->planeStackIndex);
	PNT("transform: %s", vk_surface_transform_flag_string(i->transform, false));
	PNT("planeIndex: %f", i->globalAlpha);
	PNT("alphaMode: %s", vk_display_plane_alpha_flag_string(i->alphaMode, false));
	PNT("imageExtent: {%u, %u}", i->imageExtent.width, i->imageExtent.height);

	U_LOG_IFL(log_level, vk->log_level, "%s", sink.buffer);
}
#endif
