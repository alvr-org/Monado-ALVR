// Copyright 2019-2025, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vulkan queue family helpers.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_vk
 */

#include "vk/vk_queue_family.h"
#include "vk/vk_helpers.h"

#include <stdlib.h>


/*
 *
 * 'Exported' functions.
 *
 */

VkResult
vk_queue_family_find_graphics(struct vk_bundle *vk, struct vk_queue_family *out_graphics_queue_family)
{
	/* Find the first graphics queue */
	VkQueueFamilyProperties *queue_family_props = NULL;
	uint32_t queue_family_count = 0;
	uint32_t i = 0;

	vk_get_physical_device_queue_family_properties( //
	    vk,                                         //
	    vk->physical_device,                        //
	    &queue_family_count,                        //
	    &queue_family_props);                       //
	if (queue_family_count == 0) {
		VK_DEBUG(vk, "Failed to get queue properties");
		goto err_free;
	}

	for (i = 0; i < queue_family_count; i++) {
		if (queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			break;
		}
	}

	if (i >= queue_family_count) {
		VK_DEBUG(vk, "No graphics queue found");
		goto err_free;
	}

	*out_graphics_queue_family = (struct vk_queue_family){
	    .queue_family = queue_family_props[i],
	    .family_index = i,
	};

	free(queue_family_props);

	return VK_SUCCESS;

err_free:
	free(queue_family_props);
	return VK_ERROR_INITIALIZATION_FAILED;
}

VkResult
vk_queue_family_find_and_avoid_graphics(struct vk_bundle *vk,
                                        VkQueueFlags required_flags,
                                        struct vk_queue_family *out_queue_family)
{
	/* Find the "best" queue with the requested flags (prefer queues without graphics) */
	VkQueueFamilyProperties *queue_family_props = NULL;
	uint32_t queue_family_count = 0;
	uint32_t i = 0;

	vk_get_physical_device_queue_family_properties( //
	    vk,                                         //
	    vk->physical_device,                        //
	    &queue_family_count,                        //
	    &queue_family_props);                       //
	if (queue_family_count == 0) {
		VK_DEBUG(vk, "Failed to get queue properties");
		goto err_free;
	}

	for (i = 0; i < queue_family_count; i++) {
		if ((queue_family_props[i].queueFlags & required_flags) != required_flags) {
			continue;
		}

		if (~queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			break;
		}
	}

	if (i >= queue_family_count) {
		/* If there's no suitable queue without graphics, just find any suitable one*/
		for (i = 0; i < queue_family_count; i++) {
			if ((queue_family_props[i].queueFlags & required_flags) == required_flags) {
				break;
			}
		}

		if (i >= queue_family_count) {
			VK_DEBUG(vk, "No compatible queue family found (flags: 0x%xd)", required_flags);
			goto err_free;
		}
	}

	*out_queue_family = (struct vk_queue_family){
	    .queue_family = queue_family_props[i],
	    .family_index = i,
	};

	free(queue_family_props);

	return VK_SUCCESS;

err_free:
	free(queue_family_props);

	return VK_ERROR_INITIALIZATION_FAILED;
}
