// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vulkan queue builder helper.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_vk
 */

#pragma once

#include "vk/vk_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * A builder for creating a list of queue create infos, allocate on the stack
 * and then pass into vkCreateDevice.
 *
 * @ingroup aux_vk
 */
struct vk_queue_builder
{
	//! Array of queue create infos to fill in.
	VkDeviceQueueCreateInfo create_infos[VK_BUNDLE_MAX_QUEUES];

	/*!
	 * Array of queue priorities to fill in. While VK_BUNDLE_MAX_QUEUES is
	 * the maximum total number of queues, there can either be that many
	 * queues from one family, or one queue from each of that many families,
	 * or any combination in between. Since we don't know ahead of time how
	 * many families will be used, we allocate the maximum in each dimension
	 * for simplicity.
	 */
	float priorities[VK_BUNDLE_MAX_QUEUES][VK_BUNDLE_MAX_QUEUES];

	//! Count of queue create infos and priorities added.
	uint32_t create_info_count;

	/*!
	 * Can create multiple queues in the same family, this is the total
	 * number of queues that are created, must be less than or equal to
	 * @ref VK_BUNDLE_MAX_QUEUES.
	 */
	uint32_t total_count;
};

/*!
 * Add a queue to the builder, will assert if it runs out of space, assumes that
 * the user can share the queue with other users.
 *
 * @param builder The builder to add the queue to.
 * @param family_index The family index of the queue to add.
 * @return The queue pair of the added queue.
 */
struct vk_queue_pair
vk_queue_builder_add(struct vk_queue_builder *builder, uint32_t family_index);


#ifdef __cplusplus
}
#endif
