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

#pragma once

#include "vk/vk_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * A queue family with its properties and index.
 *
 * @ingroup aux_vk
 */
struct vk_queue_family
{
	VkQueueFamilyProperties queue_family;
	uint32_t family_index;
};

/*!
 * Find the first graphics queue family.
 *
 * @param vk The Vulkan bundle to find the queue family for.
 * @param out_graphics_queue_family The queue family to return.
 * @return VK_SUCCESS if the queue family was found, otherwise an error code.
 */
VkResult
vk_queue_family_find_graphics(struct vk_bundle *vk, struct vk_queue_family *out_graphics_queue_family);

/*!
 * Find a queue family with the given flags, preferring queues without graphics.
 *
 * @param vk The Vulkan bundle to find the queue family for.
 * @param required_flags The flags that must be supported by the queue family.
 * @param out_queue_family The queue family to return.
 * @return VK_SUCCESS if the queue family was found, otherwise an error code.
 */
VkResult
vk_queue_family_find_and_avoid_graphics(struct vk_bundle *vk,
                                        VkQueueFlags required_flags,
                                        struct vk_queue_family *out_queue_family);


#ifdef __cplusplus
}
#endif
