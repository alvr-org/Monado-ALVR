// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Sub allocation functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_render
 */

#include "vk/vk_mini_helpers.h"
#include "render/render_interface.h"


// Align a size with a power of two value.
static VkDeviceSize
align_padding_pot(VkDeviceSize size, VkDeviceSize alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}


/*
 *
 * 'Exported' functions.
 *
 */

void
render_sub_alloc_tracker_init(struct render_sub_alloc_tracker *rsat, struct render_buffer *buffer)
{
	rsat->buffer = buffer->buffer;
	rsat->used = 0;
	rsat->total_size = buffer->size;
	rsat->mapped = buffer->mapped;
}

XRT_CHECK_RESULT VkResult
render_sub_alloc_ubo_alloc_and_get_ptr(struct vk_bundle *vk,
                                       struct render_sub_alloc_tracker *rsat,
                                       VkDeviceSize size,
                                       void **out_ptr,
                                       struct render_sub_alloc *out_rsa)
{
	assert(rsat->total_size >= rsat->used);
	VkDeviceSize space_left = rsat->total_size - rsat->used;

	if (space_left < size) {
		VK_ERROR(vk, "Can not fit %u in left %u of total %u", (uint32_t)size, (uint32_t)space_left,
		         (uint32_t)rsat->total_size);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	// Make sure we align from start of memory.
	VkDeviceSize padded_used = align_padding_pot(rsat->used + size, RENDER_ALWAYS_SAFE_UBO_ALIGNMENT);

	// Save the current used as offset.
	VkDeviceSize offset = rsat->used;

	// Ensure used never gets larger then total_size.
	if (padded_used > rsat->total_size) {
		rsat->used = rsat->total_size;
	} else {
		rsat->used = padded_used;
	}

	void *ptr = rsat->mapped == NULL ? NULL : (void *)((uint8_t *)rsat->mapped + offset);


	/*
	 * All done.
	 */

	*out_ptr = ptr;
	*out_rsa = (struct render_sub_alloc){
	    .buffer = rsat->buffer,
	    .size = size,
	    .offset = offset,
	};

	return VK_SUCCESS;
}

XRT_CHECK_RESULT VkResult
render_sub_alloc_ubo_alloc_and_write(struct vk_bundle *vk,
                                     struct render_sub_alloc_tracker *rsat,
                                     const void *src,
                                     VkDeviceSize size,
                                     struct render_sub_alloc *out_rsa)
{
	VkResult ret;

	if (rsat->mapped == NULL) {
		VK_ERROR(vk, "Sub allocation not mapped");
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	void *dst;
	ret = render_sub_alloc_ubo_alloc_and_get_ptr(vk, rsat, size, &dst, out_rsa);
	VK_CHK_AND_RET(ret, "render_sub_alloc_ubo_alloc_and_get_ptr");

	memcpy(dst, src, size);

	return VK_SUCCESS;
}
