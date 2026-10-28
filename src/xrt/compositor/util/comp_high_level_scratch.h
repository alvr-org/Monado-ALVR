// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Higher level interface for scratch images.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup comp_util
 */

#pragma once


#include "render/render_interface.h"

#include "comp_scratch.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Scratch images that can be used for staging buffers.
 *
 * @ingroup comp_util
 */
struct chl_scratch
{
	//! Shared render passed for the views.
	struct render_gfx_render_pass render_pass;

	struct
	{
		//! Per-view scratch images.
		struct comp_scratch_single_images cssi;

		//! Targets for rendering to the scratch buffer.
		struct render_gfx_target_resources targets[COMP_SCRATCH_NUM_IMAGES];
	} views[XRT_MAX_VIEWS];

	/*!
	 * Number of views that has been ensured and have Vulkan resources,
	 * all comp_scratch_single_images are always inited.
	 */
	uint32_t view_count;

	//! The extent used to create the images.
	VkExtent2D extent;

	//! Format requested.
	VkFormat format;

	//! Has the render pass been initialized.
	bool render_pass_initialized;
};

/*!
 * Must becalled before used and before the scratch images are registered with
 * the u_var system.
 *
 * @memberof chl_scratch
 */
void
chl_scratch_init(struct chl_scratch *scratch);

/*!
 * Resources must be manually called before calling this functions, and the
 * scratch images unregistered from the u_var system.
 *
 * @memberof chl_scratch
 */
void
chl_scratch_fini(struct chl_scratch *scratch);

/*!
 * Ensure the scratch images and the render target resources are created.
 *
 * @memberof chl_scratch
 */
bool
chl_scratch_ensure(struct chl_scratch *scratch,
                   struct render_resources *rr,
                   uint32_t view_count,
                   VkExtent2D extent,
                   const VkFormat format);

/*!
 * Free all Vulkan resources that this scratch has created.
 *
 * @memberof chl_scratch
 */
void
chl_scratch_free_resources(struct chl_scratch *scratch, struct render_resources *rr);

/*!
 * Get the image, see @ref comp_scratch_single_images_get_image.
 *
 * @memberof chl_scratch
 */
static inline VkImage
chl_scratch_get_image(struct chl_scratch *scratch, uint32_t view_index, uint32_t image_index)
{
	return comp_scratch_single_images_get_image(&scratch->views[view_index].cssi, image_index);
}

/*!
 * Get the sample view, see @ref comp_scratch_single_images_get_sample_view.
 *
 * @memberof chl_scratch
 */
static inline VkImageView
chl_scratch_get_sample_view(struct chl_scratch *scratch, uint32_t view_index, uint32_t image_index)
{
	return comp_scratch_single_images_get_sample_view(&scratch->views[view_index].cssi, image_index);
}

/*!
 * Get the storage view, see @ref comp_scratch_single_images_get_storage_view.
 *
 * @memberof chl_scratch
 */
static inline VkImageView
chl_scratch_get_storage_view(struct chl_scratch *scratch, uint32_t view_index, uint32_t image_index)
{
	return comp_scratch_single_images_get_storage_view(&scratch->views[view_index].cssi, image_index);
}


/*
 *
 * State
 *
 */

/*!
 * Per view frame state tracking which index was gotten and if it was used.
 *
 * @ingroup comp_util
 */
struct chl_scratch_state_view
{
	uint32_t index;

	bool used;
};

/*!
 * Used to track the index of images gotten for the images, and if it has been
 * used. The user will need to mark images as used.
 *
 * @ingroup comp_util
 */
struct chl_scratch_state
{
	struct chl_scratch_state_view views[XRT_MAX_VIEWS];
};

/*!
 * Zeros out the struct and calls get on all the images, setting the @p index
 * field on the state for each view.
 *
 * @memberof chl_scratch_state
 */
static inline void
chl_scratch_state_init_and_get(struct chl_scratch_state *scratch_state, struct chl_scratch *scratch)
{
	U_ZERO(scratch_state);

	// Loop over all the of the images in the scratch view.
	for (uint32_t i = 0; i < scratch->view_count; i++) {
		comp_scratch_single_images_get(&scratch->views[i].cssi, &scratch_state->views[i].index);
	}
}

/*!
 * Calls discard or done on all the scratch images, it calls done if the @p used
 * field is set to true.
 *
 * @memberof chl_scratch_state
 */
static inline void
chl_scratch_state_discard_or_done(struct chl_scratch_state *scratch_state, struct chl_scratch *scratch)
{
	// Loop over all the of the images in the scratch view.
	for (uint32_t i = 0; i < scratch->view_count; i++) {
		if (scratch_state->views[i].used) {
			comp_scratch_single_images_done(&scratch->views[i].cssi);
		} else {
			comp_scratch_single_images_discard(&scratch->views[i].cssi);
		}
	}

	U_ZERO(scratch_state);
}


#ifdef __cplusplus
}
#endif
