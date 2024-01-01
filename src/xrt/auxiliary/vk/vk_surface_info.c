// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper for getting information from a VkSurfaceKHR.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vk
 */

#include "util/u_pretty_print.h"
#include "vk_surface_info.h"


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
vk_surface_info_destroy(struct vk_surface_info *info)
{
	if (info->present_modes != NULL) {
		free(info->present_modes);
		info->present_mode_count = 0;
		info->present_modes = NULL;
	}

	if (info->formats != NULL) {
		free(info->formats);
		info->format_count = 0;
		info->formats = NULL;
	}

	U_ZERO(info);
}

XRT_CHECK_RESULT VkResult
vk_surface_info_fill_in(struct vk_bundle *vk, struct vk_surface_info *info, VkSurfaceKHR surface)
{
	VkResult ret;

	assert(info->formats == NULL);
	assert(info->format_count == 0);
	assert(info->present_modes == NULL);
	assert(info->present_mode_count == 0);

	ret = vk_enumerate_surface_formats( //
	    vk,                             //
	    surface,                        //
	    &info->format_count,            //
	    &info->formats);                //
	VK_CHK_WITH_GOTO(ret, "vk_enumerate_surface_formats", error);

	ret = vk_enumerate_surface_present_modes( //
	    vk,                                   //
	    surface,                              //
	    &info->present_mode_count,            //
	    &info->present_modes);                //
	VK_CHK_WITH_GOTO(ret, "vk_enumerate_surface_present_modes", error);

	ret = vk->vkGetPhysicalDeviceSurfaceCapabilitiesKHR( //
	    vk->physical_device,                             //
	    surface,                                         //
	    &info->caps);                                    //
	VK_CHK_WITH_GOTO(ret, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR", error);

#ifdef VK_EXT_display_surface_counter
	if (vk->has_EXT_display_control) {
		info->caps2.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_EXT;
		ret = vk->vkGetPhysicalDeviceSurfaceCapabilities2EXT( //
		    vk->physical_device,                              //
		    surface,                                          //
		    &info->caps2);                                    //
		VK_CHK_WITH_GOTO(ret, "vkGetPhysicalDeviceSurfaceCapabilities2EXT", error);
	}
#endif

	return VK_SUCCESS;

error:
	vk_surface_info_destroy(info);

	return ret;
}

void
vk_print_surface_info(struct vk_bundle *vk, struct vk_surface_info *info, enum u_logging_level log_level)
{
	if (vk->log_level > log_level) {
		return;
	}

	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	P("VkSurfaceKHR info:");
	PNT("caps.minImageCount: %u", info->caps.minImageCount);
	PNT("caps.maxImageCount: %u", info->caps.maxImageCount);
	PNT("caps.currentExtent: %ux%u", info->caps.currentExtent.width, info->caps.currentExtent.height);
	PNT("caps.minImageExtent: %ux%u", info->caps.minImageExtent.width, info->caps.minImageExtent.height);
	PNT("caps.maxImageExtent: %ux%u", info->caps.maxImageExtent.width, info->caps.maxImageExtent.height);
	PNT("caps.maxImageArrayLayers: %u", info->caps.maxImageArrayLayers);
	PNT("caps.supportedTransforms:");
	PRINT_BITS(info->caps.supportedTransforms, vk_surface_transform_flag_string);
	PNT("caps.currentTransform: %s", vk_surface_transform_flag_string(info->caps.currentTransform, false));
	PNT("caps.supportedCompositeAlpha:");
	PRINT_BITS(info->caps.supportedCompositeAlpha, vk_composite_alpha_flag_string);
	PNT("caps.supportedUsageFlags:");
	PRINT_BITS(info->caps.supportedUsageFlags, vk_image_usage_flag_string);

	PNT("present_modes(%u):", info->present_mode_count);
	for (uint32_t i = 0; i < info->present_mode_count; i++) {
		PNTT("%s", vk_present_mode_string(info->present_modes[i]));
	}

	PNT("formats(%u):", info->format_count);
	for (uint32_t i = 0; i < info->format_count; i++) {
		VkSurfaceFormatKHR *f = &info->formats[i];
		PNTT("[format = %s, colorSpace = %s]", vk_format_string(f->format),
		     vk_color_space_string(f->colorSpace));
	}

	U_LOG_IFL(log_level, vk->log_level, "%s", sink.buffer);
}
