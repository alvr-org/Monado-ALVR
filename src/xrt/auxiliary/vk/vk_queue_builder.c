// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vulkan queue builder helper.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_vk
 */

#include "vk/vk_queue_builder.h"
#include "vk/vk_helpers.h"

#include <assert.h>


/*
 *
 * 'Exported' functions.
 *
 */

struct vk_queue_pair
vk_queue_builder_add(struct vk_queue_builder *builder, uint32_t family_index)
{
	assert(builder != NULL);

	for (uint32_t i = 0; i < builder->create_info_count; ++i) {
		if (builder->create_infos[i].queueFamilyIndex != family_index) {
			continue;
		}

		// Assume that the user can share the queue with other users.
		return (struct vk_queue_pair){
		    .family_index = family_index,
		    .index = 0,
		};
	}

	uint32_t index = builder->create_info_count;
	VkDeviceQueueCreateInfo *info = &builder->create_infos[index];

	// Default to 0.f as this function is simple.
	builder->priorities[index][0] = 0.f;

	info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	info->pNext = NULL;
	info->queueFamilyIndex = family_index;
	info->queueCount = 1;
	info->pQueuePriorities = builder->priorities[index];

	builder->create_info_count++;

	// Start at index 0 for the first queue in the family.
	return (struct vk_queue_pair){
	    .family_index = family_index,
	    .index = 0,
	};
}
