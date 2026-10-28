// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper utilities for Vulkan queue submission with mixed binary and timeline semaphores.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_vk
 */

#include "vk/vk_submit_helpers.h"
#include "util/u_misc.h"


void
vk_submit_info_builder_prepare(struct vk_submit_info_builder *builder,
                               const struct vk_semaphore_list_wait *wait_semaphores,
                               const VkCommandBuffer *command_buffers,
                               uint32_t command_buffer_count,
                               const struct vk_semaphore_list_signal *signal_semaphores,
                               const void *next)
{
	// Zero out the builder
	U_ZERO(builder);

	// Setup basic submit info
	builder->submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	builder->submit_info.pNext = next; // Can't use vk_append_to_pnext_chain because next->pNext might be non-NULL.

	// Wait semaphores
	if (wait_semaphores != NULL && wait_semaphores->count > 0) {
		builder->submit_info.waitSemaphoreCount = wait_semaphores->count;
		builder->submit_info.pWaitSemaphores = wait_semaphores->semaphores;
		builder->submit_info.pWaitDstStageMask = wait_semaphores->stages;
	}

	// Command buffers
	builder->submit_info.commandBufferCount = command_buffer_count;
	builder->submit_info.pCommandBuffers = command_buffers;

	// Signal semaphores
	if (signal_semaphores != NULL && signal_semaphores->count > 0) {
		builder->submit_info.signalSemaphoreCount = signal_semaphores->count;
		builder->submit_info.pSignalSemaphores = signal_semaphores->semaphores;
	}

#ifdef VK_KHR_timeline_semaphore
	// Check if we need timeline semaphore support
	// We need it if any semaphore has a non-zero value
	bool needs_timeline = false;

	if (wait_semaphores != NULL) {
		for (uint32_t i = 0; i < wait_semaphores->count; i++) {
			if (wait_semaphores->values[i] != 0) {
				needs_timeline = true;
				break;
			}
		}
	}

	if (!needs_timeline && signal_semaphores != NULL) {
		for (uint32_t i = 0; i < signal_semaphores->count; i++) {
			if (signal_semaphores->values[i] != 0) {
				needs_timeline = true;
				break;
			}
		}
	}

	if (needs_timeline) {
		builder->timeline_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR;

		if (wait_semaphores != NULL && wait_semaphores->count > 0) {
			builder->timeline_info.waitSemaphoreValueCount = wait_semaphores->count;
			builder->timeline_info.pWaitSemaphoreValues = wait_semaphores->values;
		}

		if (signal_semaphores != NULL && signal_semaphores->count > 0) {
			builder->timeline_info.signalSemaphoreValueCount = signal_semaphores->count;
			builder->timeline_info.pSignalSemaphoreValues = signal_semaphores->values;
		}

		// Chain the timeline info to the submit info
		vk_append_to_pnext_chain((VkBaseInStructure *)&builder->submit_info,
		                         (VkBaseInStructure *)&builder->timeline_info);
	}
#endif
}
