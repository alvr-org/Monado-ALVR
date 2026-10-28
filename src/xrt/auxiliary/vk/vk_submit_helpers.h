// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper utilities for Vulkan queue submission with mixed binary and timeline semaphores.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_vk
 *
 * ## Usage Example
 *
 * @code{.c}
 * // Initialize semaphore lists
 * struct vk_semaphore_list_wait wait_sems = XRT_STRUCT_INIT;
 * struct vk_semaphore_list_signal signal_sems = XRT_STRUCT_INIT;
 *
 * // Add semaphores (mix of binary and timeline)
 * if (present_complete != VK_NULL_HANDLE) {
 *     vk_semaphore_list_wait_add_binary(&wait_sems, present_complete, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
 * }
 * vk_semaphore_list_signal_add_timeline(&signal_sems, render_complete, frame_id);
 *
 * // Prepare submit info
 * struct vk_submit_info_builder builder;
 * vk_submit_info_builder_prepare(&builder, &wait_sems, &cmd, 1, &signal_sems, NULL);
 *
 * // Submit to queue
 * ret = vk->vkQueueSubmit(queue, 1, &builder.submit_info, fence);
 * @endcode
 */

#pragma once

#include "util/u_logging.h"
#include "vk/vk_helpers.h"

#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Maximum number of semaphores that can be accumulated in semaphore
 * lists. Should be enough for typical compositor use cases.
 */
#define VK_SEMAPHORE_LIST_MAX_COUNT 4


/*!
 * Accumulator for wait semaphores to be used in VkSubmitInfo.
 * Handles both binary and timeline semaphores, tracking their handles,
 * values, and pipeline stage flags.
 *
 * Must be zero-initialized using XRT_STRUCT_INIT before use.
 *
 * @ingroup aux_vk
 */
struct vk_semaphore_list_wait
{
	//! Array of semaphore handles
	VkSemaphore semaphores[VK_SEMAPHORE_LIST_MAX_COUNT];

	//! Array of semaphore values (0 for binary, value for timeline)
	uint64_t values[VK_SEMAPHORE_LIST_MAX_COUNT];

	//! Array of pipeline stage flags for each semaphore
	VkPipelineStageFlags stages[VK_SEMAPHORE_LIST_MAX_COUNT];

	//! Number of semaphores currently in the list
	uint32_t count;
};

/*!
 * Accumulator for signal semaphores to be used in VkSubmitInfo.
 * Handles both binary and timeline semaphores, tracking their handles
 * and values.
 *
 * Must be zero-initialized using XRT_STRUCT_INIT before use.
 *
 * @ingroup aux_vk
 */
struct vk_semaphore_list_signal
{
	//! Array of semaphore handles
	VkSemaphore semaphores[VK_SEMAPHORE_LIST_MAX_COUNT];

	//! Array of semaphore values (0 for binary, value for timeline)
	uint64_t values[VK_SEMAPHORE_LIST_MAX_COUNT];

	//! Number of semaphores currently in the list
	uint32_t count;
};

/*!
 * Builder for VkSubmitInfo with optional timeline semaphore support.
 * Simplifies creation of submit infos with mixed binary and timeline
 * semaphores.
 *
 * @ingroup aux_vk
 */
struct vk_submit_info_builder
{
	//! The main submit info structure
	VkSubmitInfo submit_info;

#ifdef VK_KHR_timeline_semaphore
	//! Timeline semaphore info (chained to submit_info if needed)
	VkTimelineSemaphoreSubmitInfoKHR timeline_info;
#endif
};

/*!
 * Add a binary wait semaphore to the list.
 * The value will be set to 0 (ignored for binary semaphores).
 *
 * @param list      Wait semaphore list to add to
 * @param semaphore Binary semaphore (must not be VK_NULL_HANDLE)
 * @param stage     Pipeline stage to wait at
 *
 * @ingroup aux_vk
 */
static inline void
vk_semaphore_list_wait_add_binary(struct vk_semaphore_list_wait *list,
                                  VkSemaphore semaphore,
                                  VkPipelineStageFlags stage)
{
	assert(semaphore != VK_NULL_HANDLE);
	assert(list->count < VK_SEMAPHORE_LIST_MAX_COUNT);

	if (list->count >= VK_SEMAPHORE_LIST_MAX_COUNT) {
		U_LOG_E("vk_semaphore_list_wait_add_binary: list is full");
		return;
	}

	list->semaphores[list->count] = semaphore;
	list->values[list->count] = 0; // Ignored for binary
	list->stages[list->count] = stage;
	list->count++;
}

/*!
 * Add a timeline wait semaphore to the list with a specific value.
 *
 * @param list      Wait semaphore list to add to
 * @param semaphore Timeline semaphore (must not be VK_NULL_HANDLE)
 * @param value     Timeline value to wait for
 * @param stage     Pipeline stage to wait at
 *
 * @ingroup aux_vk
 */
static inline void
vk_semaphore_list_wait_add_timeline(struct vk_semaphore_list_wait *list,
                                    VkSemaphore semaphore,
                                    uint64_t value,
                                    VkPipelineStageFlags stage)
{
	assert(semaphore != VK_NULL_HANDLE);
	assert(list->count < VK_SEMAPHORE_LIST_MAX_COUNT);

	if (list->count >= VK_SEMAPHORE_LIST_MAX_COUNT) {
		U_LOG_E("vk_semaphore_list_wait_add_timeline: list is full");
		return;
	}

	list->semaphores[list->count] = semaphore;
	list->values[list->count] = value;
	list->stages[list->count] = stage;
	list->count++;
}

/*!
 * Add a binary signal semaphore to the list.
 * The value will be set to 0 (ignored for binary semaphores).
 *
 * @param list      Signal semaphore list to add to
 * @param semaphore Binary semaphore (must not be VK_NULL_HANDLE)
 *
 * @ingroup aux_vk
 */
static inline void
vk_semaphore_list_signal_add_binary(struct vk_semaphore_list_signal *list, VkSemaphore semaphore)
{
	assert(semaphore != VK_NULL_HANDLE);
	assert(list->count < VK_SEMAPHORE_LIST_MAX_COUNT);

	if (list->count >= VK_SEMAPHORE_LIST_MAX_COUNT) {
		U_LOG_E("vk_semaphore_list_signal_add_binary: list is full");
		return;
	}

	list->semaphores[list->count] = semaphore;
	list->values[list->count] = 0; // Ignored for binary
	list->count++;
}

/*!
 * Add a timeline signal semaphore to the list with a specific value.
 *
 * @param list      Signal semaphore list to add to
 * @param semaphore Timeline semaphore (must not be VK_NULL_HANDLE)
 * @param value     Timeline value to signal
 *
 * @ingroup aux_vk
 */
static inline void
vk_semaphore_list_signal_add_timeline(struct vk_semaphore_list_signal *list, VkSemaphore semaphore, uint64_t value)
{
	assert(semaphore != VK_NULL_HANDLE);
	assert(list->count < VK_SEMAPHORE_LIST_MAX_COUNT);

	if (list->count >= VK_SEMAPHORE_LIST_MAX_COUNT) {
		U_LOG_E("vk_semaphore_list_signal_add_timeline: list is full");
		return;
	}

	list->semaphores[list->count] = semaphore;
	list->values[list->count] = value;
	list->count++;
}

/*!
 * Prepare a VkSubmitInfo from wait and signal semaphore lists.
 * Automatically sets up VkTimelineSemaphoreSubmitInfoKHR if any
 * timeline semaphores are present.
 *
 * @param builder              Submit info builder to initialize
 * @param wait_semaphores      List of wait semaphores (can be NULL)
 * @param command_buffers      Array of command buffers to execute
 * @param command_buffer_count Number of command buffers
 * @param signal_semaphores    List of signal semaphores (can be NULL)
 * @param next                 Optional pNext chain to prepend to
 *                             (builder will add timeline info if
 *                             needed)
 *
 * @ingroup aux_vk
 */
void
vk_submit_info_builder_prepare(struct vk_submit_info_builder *builder,
                               const struct vk_semaphore_list_wait *wait_semaphores,
                               const VkCommandBuffer *command_buffers,
                               uint32_t command_buffer_count,
                               const struct vk_semaphore_list_signal *signal_semaphores,
                               const void *next);


#ifdef __cplusplus
}
#endif
