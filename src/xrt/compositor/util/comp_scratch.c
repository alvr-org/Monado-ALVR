// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper implementation for native compositors.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @ingroup comp_util
 */


#include "comp_scratch.h"

#include "util/u_handles.h"
#include "util/u_limited_unique_id.h"

#include "vk/vk_mini_helpers.h"
#include "vk/vk_image_allocator.h"


/*
 *
 * Helpers.
 *
 */

static inline void
fill_info(VkExtent2D extent, struct xrt_swapchain_create_info *out_info)
{
	enum xrt_swapchain_create_flags create = 0;

	enum xrt_swapchain_usage_bits bits =       //
	    XRT_SWAPCHAIN_USAGE_COLOR |            //
	    XRT_SWAPCHAIN_USAGE_SAMPLED |          //
	    XRT_SWAPCHAIN_USAGE_TRANSFER_SRC |     //
	    XRT_SWAPCHAIN_USAGE_TRANSFER_DST |     //
	    XRT_SWAPCHAIN_USAGE_UNORDERED_ACCESS | //
	    XRT_SWAPCHAIN_USAGE_MUTABLE_FORMAT;    //

	struct xrt_swapchain_create_info info = {
	    .create = create,
	    .bits = bits,
	    .format = VK_FORMAT_R8G8B8A8_UNORM,
	    .sample_count = 1,
	    .width = extent.width,
	    .height = extent.height,
	    .face_count = 1,
	    .array_size = 1,
	    .mip_count = 1,
	};

	// Use format list to get good performance everywhere.
	info.formats[info.format_count++] = VK_FORMAT_R8G8B8A8_UNORM;
	info.formats[info.format_count++] = VK_FORMAT_R8G8B8A8_SRGB;

	*out_info = info;
}


/*
 *
 * Indices functions
 *
 */

#define INVALID_INDEX ((uint32_t)(-1))

static inline void
indices_init(struct comp_scratch_indices *i)
{
	i->current = INVALID_INDEX;
	i->last = INVALID_INDEX;
}

static inline void
indices_get(struct comp_scratch_indices *i, uint32_t *out_index)
{
	assert(i->current == INVALID_INDEX);

	uint32_t current = i->last;
	if (current == INVALID_INDEX) {
		current = 0;
	} else {
		if (++current >= COMP_SCRATCH_NUM_IMAGES) {
			current = 0;
		}
	}

	i->current = current;
	*out_index = current;
}

static inline uint32_t
indices_done(struct comp_scratch_indices *i)
{
	assert(i->current != INVALID_INDEX);
	assert(i->current < COMP_SCRATCH_NUM_IMAGES);

	i->last = i->current;
	i->current = INVALID_INDEX;

	return i->last;
}

static inline void
indices_discard(struct comp_scratch_indices *i)
{
	assert(i->current != INVALID_INDEX);
	assert(i->current < COMP_SCRATCH_NUM_IMAGES);

	i->current = INVALID_INDEX;
}


/*
 *
 * Temp struct helpers.
 *
 */

struct tmp
{
	//! Images created.
	struct vk_image_collection vkic;

	//! Handles retrieved.
	xrt_graphics_buffer_handle_t handles[COMP_SCRATCH_NUM_IMAGES];

	//! For automatic conversion to linear.
	VkImageView srgb_views[COMP_SCRATCH_NUM_IMAGES];

	//! For storage operations in compute shaders.
	VkImageView unorm_views[COMP_SCRATCH_NUM_IMAGES];
};

static inline bool
tmp_init_and_create(struct tmp *t, struct vk_bundle *vk, const struct xrt_swapchain_create_info *info)
{
	VkResult ret;

	// Completely init the struct so it's safe to destroy on failure.
	U_ZERO(t);
	for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
		t->handles[i] = XRT_GRAPHICS_BUFFER_HANDLE_INVALID;
	}

	// Do the allocation.
	ret = vk_ic_allocate(vk, info, COMP_SCRATCH_NUM_IMAGES, &t->vkic);
	VK_CHK_WITH_RET(ret, "vk_ic_allocate", false);

	ret = vk_ic_get_handles(vk, &t->vkic, COMP_SCRATCH_NUM_IMAGES, t->handles);
	VK_CHK_WITH_GOTO(ret, "vk_ic_get_handles", err_destroy_vkic);


	/*
	 * Create the image views.
	 */

	// Base info.
	const VkFormat srgb_format = VK_FORMAT_R8G8B8A8_SRGB;
	const VkFormat unorm_format = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;

	// Both usages are common.
	const VkImageUsageFlags unorm_usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	// Very few cards support SRGB storage.
	const VkImageUsageFlags srgb_usage = VK_IMAGE_USAGE_SAMPLED_BIT;

	const VkImageSubresourceRange subresource_range = {
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = VK_REMAINING_MIP_LEVELS,
	    .baseArrayLayer = 0,
	    .layerCount = VK_REMAINING_ARRAY_LAYERS,
	};

	for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
		VkImage image = t->vkic.images[i].handle;

		ret = vk_create_view_usage( //
		    vk,                     // vk_bundle
		    image,                  // image
		    view_type,              // type
		    srgb_format,            // format
		    srgb_usage,             // image_usage
		    subresource_range,      // subresource_range
		    &t->srgb_views[i]);     // out_image_view
		VK_CHK_WITH_GOTO(ret, "vk_create_view_usage(srgb)", err_destroy_views);

		VK_NAME_IMAGE_VIEW(vk, t->srgb_views[i], "comp_scratch_image_view(srgb)");

		ret = vk_create_view_usage( //
		    vk,                     // vk_bundle
		    image,                  // image
		    view_type,              // type
		    unorm_format,           // format
		    unorm_usage,            // image_usage
		    subresource_range,      // subresource_range
		    &t->unorm_views[i]);    // out_image_view
		VK_CHK_WITH_GOTO(ret, "vk_create_view_usage(unorm)", err_destroy_views);

		VK_NAME_IMAGE_VIEW(vk, t->unorm_views[i], "comp_scratch_image_view(unorm)");
	}

	return true;

err_destroy_views:
	for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
		D(ImageView, t->srgb_views[i]);
		D(ImageView, t->unorm_views[i]);
	}

err_destroy_vkic:
	vk_ic_destroy(vk, &t->vkic);

	return false;
}

static inline void
tmp_take(struct tmp *t,
         struct xrt_image_native native_images[COMP_SCRATCH_NUM_IMAGES],
         struct render_scratch_color_image images[COMP_SCRATCH_NUM_IMAGES])
{
	for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
		images[i].image = t->vkic.images[i].handle;
		images[i].device_memory = t->vkic.images[i].memory;
		images[i].srgb_view = t->srgb_views[i];
		images[i].unorm_view = t->unorm_views[i];

		native_images[i].size = t->vkic.images[i].size;
		native_images[i].use_dedicated_allocation = t->vkic.images[i].use_dedicated_allocation;
		native_images[i].handle = t->handles[i]; // Move

		t->srgb_views[i] = VK_NULL_HANDLE;
		t->unorm_views[i] = VK_NULL_HANDLE;
		t->handles[i] = XRT_GRAPHICS_BUFFER_HANDLE_INVALID;
	}

	// We now own everything.
	U_ZERO(&t->vkic);
}

static inline void
tmp_destroy(struct tmp *t, struct vk_bundle *vk)
{
	vk_ic_destroy(vk, &t->vkic);
	for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
		u_graphics_buffer_unref(&t->handles[i]);
		D(ImageView, t->srgb_views[i]);
		D(ImageView, t->unorm_views[i]);
	}
}


/*
 *
 * 'Exported' single functions.
 *
 */

void
comp_scratch_single_images_init(struct comp_scratch_single_images *cssi)
{
	// Just to be sure.
	U_ZERO(cssi);

	indices_init(&cssi->indices);

	u_native_images_debug_init(&cssi->unid);

	// Invalid handle may be different to zero.
	for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
		cssi->native_images[i].handle = XRT_GRAPHICS_BUFFER_HANDLE_INVALID;
	}
}

bool
comp_scratch_single_images_ensure(struct comp_scratch_single_images *cssi, struct vk_bundle *vk, VkExtent2D extent)
{
	if (cssi->info.width == extent.width && cssi->info.height == extent.height) {
		// Our work here is done!
		return true;
	}

	struct xrt_swapchain_create_info info = XRT_STRUCT_INIT;
	fill_info(extent, &info);

	struct tmp t; // Is initialized in function.
	if (!tmp_init_and_create(&t, vk, &info)) {
		VK_ERROR(vk, "Failed to allocate images");
		return false;
	}

	// Clear old information, we haven't touched this struct yet.
	comp_scratch_single_images_free(cssi, vk);

	// Copy out images and information.
	tmp_take(&t, cssi->native_images, cssi->images);

	// Generate new unique id for caching and set info.
	cssi->limited_unique_id = u_limited_unique_id_get();
	cssi->info = info;

	return true;
}

void
comp_scratch_single_images_free(struct comp_scratch_single_images *cssi, struct vk_bundle *vk)
{
	// Make sure nothing refers to the images.
	u_native_images_debug_clear(&cssi->unid);

	for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
		u_graphics_buffer_unref(&cssi->native_images[i].handle);

		D(ImageView, cssi->images[i].srgb_view);
		D(ImageView, cssi->images[i].unorm_view);
		D(Image, cssi->images[i].image);
		DF(Memory, cssi->images[i].device_memory);
	}

	// Clear info, so ensure will recreate.
	U_ZERO(&cssi->info);

	// Clear unique id so to force recreate.
	cssi->limited_unique_id.data = 0;

	// Clear indices.
	indices_init(&cssi->indices);
}

void
comp_scratch_single_images_get(struct comp_scratch_single_images *cssi, uint32_t *out_index)
{
	indices_get(&cssi->indices, out_index);
}

void
comp_scratch_single_images_done(struct comp_scratch_single_images *cssi)
{
	uint32_t last = indices_done(&cssi->indices);

	assert(cssi->info.width > 0);
	assert(cssi->info.height > 0);

	u_native_images_debug_set(   //
	    &cssi->unid,             //
	    cssi->limited_unique_id, //
	    cssi->native_images,     //
	    COMP_SCRATCH_NUM_IMAGES, //
	    &cssi->info,             //
	    last,                    //
	    false);                  //
}

void
comp_scratch_single_images_discard(struct comp_scratch_single_images *cssi)
{
	indices_discard(&cssi->indices);

	u_native_images_debug_clear(&cssi->unid);
}

void
comp_scratch_single_images_clear_debug(struct comp_scratch_single_images *cssi)
{
	u_native_images_debug_clear(&cssi->unid);
}

void
comp_scratch_single_images_destroy(struct comp_scratch_single_images *cssi)
{
	u_native_images_debug_destroy(&cssi->unid);
}


/*
 *
 * 'Exported' stereo functions.
 *
 */

void
comp_scratch_stereo_images_init(struct comp_scratch_stereo_images *cssi)
{
	// Just to be sure.
	U_ZERO(cssi);

	indices_init(&cssi->indices);

	u_native_images_debug_init(&cssi->views[0].unid);
	u_native_images_debug_init(&cssi->views[1].unid);

	for (uint32_t view = 0; view < 2; view++) {
		for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
			cssi->views[view].native_images[i].handle = XRT_GRAPHICS_BUFFER_HANDLE_INVALID;
		}
	}
}

bool
comp_scratch_stereo_images_ensure(struct comp_scratch_stereo_images *cssi, struct vk_bundle *vk, VkExtent2D extent)
{
	if (cssi->info.width == extent.width && cssi->info.height == extent.height) {
		// Our work here is done!
		return true;
	}

	// Get info we need to share with.
	struct xrt_swapchain_create_info info = XRT_STRUCT_INIT;
	fill_info(extent, &info);

	struct tmp ts[2]; // Is initialized in function.
	if (!tmp_init_and_create(&ts[0], vk, &info)) {
		VK_ERROR(vk, "Failed to allocate images for view 0");
		return false;
	}

	if (!tmp_init_and_create(&ts[1], vk, &info)) {
		VK_ERROR(vk, "Failed to allocate images for view 1");
		goto err_destroy;
	}

	// Clear old information, we haven't touched this struct yet.
	comp_scratch_stereo_images_free(cssi, vk);

	for (uint32_t view = 0; view < 2; view++) {
		struct render_scratch_color_image images[COMP_SCRATCH_NUM_IMAGES];

		tmp_take(&ts[view], cssi->views[view].native_images, images);

		// Deal with SoA vs AoS.
		for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
			cssi->rsis[i].extent = extent;
			cssi->rsis[i].color[view] = images[i];
		}
	}

	// Generate new unique id for caching and set info.
	cssi->limited_unique_id = u_limited_unique_id_get();
	cssi->info = info;

	return true;

err_destroy:
	tmp_destroy(&ts[0], vk);
	tmp_destroy(&ts[1], vk);

	return false;
}

void
comp_scratch_stereo_images_free(struct comp_scratch_stereo_images *cssi, struct vk_bundle *vk)
{
	// Make sure nothing refers to the images.
	u_native_images_debug_clear(&cssi->views[0].unid);
	u_native_images_debug_clear(&cssi->views[1].unid);

	for (uint32_t view = 0; view < 2; view++) {
		for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
			// Organised into views, then native images.
			u_graphics_buffer_unref(&cssi->views[view].native_images[i].handle);

			// Organised into scratch images, then views.
			D(ImageView, cssi->rsis[i].color[view].srgb_view);
			D(ImageView, cssi->rsis[i].color[view].unorm_view);
			D(Image, cssi->rsis[i].color[view].image);
			DF(Memory, cssi->rsis[i].color[view].device_memory);
		}
	}

	// Clear info, so ensure will recreate.
	U_ZERO(&cssi->info);

	// Clear unique id so to force recreate.
	cssi->limited_unique_id.data = 0;

	// Clear indices.
	indices_init(&cssi->indices);
}

void
comp_scratch_stereo_images_get(struct comp_scratch_stereo_images *cssi, uint32_t *out_index)
{
	indices_get(&cssi->indices, out_index);
}

void
comp_scratch_stereo_images_done(struct comp_scratch_stereo_images *cssi)
{
	uint32_t last = indices_done(&cssi->indices);

	assert(cssi->info.width > 0);
	assert(cssi->info.height > 0);

	for (uint32_t view = 0; view < 2; view++) {
		u_native_images_debug_set(           //
		    &cssi->views[view].unid,         //
		    cssi->limited_unique_id,         //
		    cssi->views[view].native_images, //
		    COMP_SCRATCH_NUM_IMAGES,         //
		    &cssi->info,                     //
		    last,                            //
		    false);                          //
	}
}

void
comp_scratch_stereo_images_discard(struct comp_scratch_stereo_images *cssi)
{
	indices_discard(&cssi->indices);

	u_native_images_debug_clear(&cssi->views[0].unid);
	u_native_images_debug_clear(&cssi->views[1].unid);
}

void
comp_scratch_stereo_images_clear_debug(struct comp_scratch_stereo_images *cssi)
{
	u_native_images_debug_clear(&cssi->views[0].unid);
	u_native_images_debug_clear(&cssi->views[1].unid);
}

void
comp_scratch_stereo_images_destroy(struct comp_scratch_stereo_images *cssi)
{
	// Make sure nothing refers to the images.
	u_native_images_debug_destroy(&cssi->views[0].unid);
	u_native_images_debug_destroy(&cssi->views[1].unid);
}
