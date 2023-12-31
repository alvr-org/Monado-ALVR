// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper implementation for native compositors.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @ingroup comp_util
 */

#pragma once

#include "util/u_native_images_debug.h"

#include "vk/vk_helpers.h"

#include "render/render_interface.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Defines.
 *
 */

//! The number of images for each view, works like a swapchain.
#define COMP_SCRATCH_NUM_IMAGES (4)


/*
 *
 * Common for handling indices.
 *
 */

/*!
 * Small helper struct to deal with indices.
 *
 * @ingroup comp_util
 */
struct comp_scratch_indices
{
	uint32_t current;

	uint32_t last;
};


/*
 *
 * Single view images.
 *
 */

/*!
 * Holds scratch images for a single view, designed to work with render code.
 * Also manages @ref xrt_image_native and @ref u_native_images_debug to
 * facilitate easy debugging.
 *
 * @ingroup comp_util
 */
struct comp_scratch_single_images
{
	//! Images used when rendering.
	struct render_scratch_color_image images[COMP_SCRATCH_NUM_IMAGES];

	//! To connect to the debug UI.
	struct u_native_images_debug unid;

	//! Exposed via @ref unid.
	struct xrt_swapchain_create_info info;

	//! Exposed via @ref unid.
	struct xrt_image_native native_images[COMP_SCRATCH_NUM_IMAGES];

	//! Keeping track of indices.
	struct comp_scratch_indices indices;

	//! Process unique id, used for caching.
	xrt_limited_unique_id_t limited_unique_id;
};

/*!
 * Init the struct, this function must be called before calling any other
 * function on this struct, or variable tracking setup on @p unid. Zero init is
 * not enough as it has a mutex in it and has native handles which on some
 * platforms zero is a valid handle.
 *
 * @ingroup comp_util
 */
void
comp_scratch_single_images_init(struct comp_scratch_single_images *cssi);

/*!
 * Ensure that the scratch images are allocated and match @p extent size.
 *
 * @ingroup comp_util
 */
bool
comp_scratch_single_images_ensure(struct comp_scratch_single_images *cssi, struct vk_bundle *vk, VkExtent2D extent);

/*!
 * Free all images allocated, @p init must be called before calling this
 * function, is safe to call without any image allocated.
 *
 * @ingroup comp_util
 */
void
comp_scratch_single_images_free(struct comp_scratch_single_images *cssi, struct vk_bundle *vk);

/*!
 * Get the next free image, after this function has been called you must call
 * either @p done or @p discard before calling any other function.
 *
 * @ingroup comp_util
 */
void
comp_scratch_single_images_get(struct comp_scratch_single_images *cssi, uint32_t *out_index);

/*!
 * After calling @p get and rendering to the image you call this function to
 * signal that you are done with this function, the GPU work needs to be fully
 * completed before calling done.
 *
 * @ingroup comp_util
 */
void
comp_scratch_single_images_done(struct comp_scratch_single_images *cssi);

/*!
 * Discard a @g get call, this clears the image debug part causing no image to
 * be shown in the debug UI.
 *
 * @ingroup comp_util
 */
void
comp_scratch_single_images_discard(struct comp_scratch_single_images *cssi);

/*!
 * Clears the debug output, this causes nothing to be shown in the debug UI.
 *
 * @ingroup comp_util
 */
void
comp_scratch_single_images_clear_debug(struct comp_scratch_single_images *cssi);

/*!
 * Destroys scratch image struct, if any images has been allocated must call
 * @p free before as this function only destroys the mutex, and the @p unid must
 * no longer be tracked.
 *
 * @ingroup comp_util
 */
void
comp_scratch_single_images_destroy(struct comp_scratch_single_images *cssi);


/*
 *
 * Stereo.
 *
 */

/*!
 * Holds scartch images for a stereo views, designed to work with render code.
 * Also manages @ref xrt_image_native and @ref u_native_images_debug to
 * facilitate easy debugging.
 *
 * @ingroup comp_util
 */
struct comp_scratch_stereo_images
{
	struct render_scratch_images rsis[COMP_SCRATCH_NUM_IMAGES];

	struct xrt_swapchain_create_info info;

	//! Keeping track of indices.
	struct comp_scratch_indices indices;

	struct
	{
		//! Debug output for each view.
		struct u_native_images_debug unid;

		//! Count always equals to the number of rsis.
		struct xrt_image_native native_images[COMP_SCRATCH_NUM_IMAGES];
	} views[2];

	//! Process unique id, used for caching.
	xrt_limited_unique_id_t limited_unique_id;
};

void
comp_scratch_stereo_images_init(struct comp_scratch_stereo_images *cssi);

bool
comp_scratch_stereo_images_ensure(struct comp_scratch_stereo_images *cssi, struct vk_bundle *vk, VkExtent2D extent);

void
comp_scratch_stereo_images_free(struct comp_scratch_stereo_images *cssi, struct vk_bundle *vk);

void
comp_scratch_stereo_images_get(struct comp_scratch_stereo_images *cssi, uint32_t *out_index);

void
comp_scratch_stereo_images_done(struct comp_scratch_stereo_images *cssi);

void
comp_scratch_stereo_images_discard(struct comp_scratch_stereo_images *cssi);

void
comp_scratch_stereo_images_clear_debug(struct comp_scratch_stereo_images *cssi);

void
comp_scratch_stereo_images_destroy(struct comp_scratch_stereo_images *cssi);


#ifdef __cplusplus
}
#endif
