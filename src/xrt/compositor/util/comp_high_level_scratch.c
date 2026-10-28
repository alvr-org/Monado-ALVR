// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Higher level interface for scratch images.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @author Andrei Aristarkhov <aaristarkhov@nvidia.com>
 * @author Gareth Morgan <gmorgan@nvidia.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup comp_main
 */


#include "comp_high_level_scratch.h"


void
chl_scratch_init(struct chl_scratch *scratch)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(scratch->views); i++) {
		comp_scratch_single_images_init(&scratch->views[i].cssi);
	}
}

void
chl_scratch_fini(struct chl_scratch *scratch)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(scratch->views); i++) {
		comp_scratch_single_images_destroy(&scratch->views[i].cssi);
	}
}

bool
chl_scratch_ensure(struct chl_scratch *scratch,
                   struct render_resources *rr,
                   uint32_t view_count,
                   VkExtent2D extent,
                   const VkFormat format)
{
	struct vk_bundle *vk = rr->vk;
	bool bret = false;

	// Is everything already correct?
	if (scratch->view_count == view_count &&       //
	    scratch->extent.width == extent.width &&   //
	    scratch->extent.height == extent.height && //
	    scratch->format == format) {
		return true;
	}

	// Free all old resources.
	chl_scratch_free_resources(scratch, rr);

	// Shared render pass between all scratch images.
	bret = render_gfx_render_pass_init(            //
	    &scratch->render_pass,                     // rgrp
	    rr,                                        // struct render_resources
	    format,                                    // format
	    VK_ATTACHMENT_LOAD_OP_CLEAR,               // load_op
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL); // final_layout
	if (!bret) {
		VK_ERROR(vk, "render_gfx_render_pass_init: false");
		return false;
	}

	// Need to track if the render pass has been initialized.
	scratch->render_pass_initialized = true;

	for (uint32_t i = 0; i < view_count; i++) {
		// Helper.
		struct comp_scratch_single_images *cssi = &scratch->views[i].cssi;

		if (format == VK_FORMAT_R8G8B8A8_SRGB) {
			// Special creation function for the mutable format.
			bret = comp_scratch_single_images_ensure_mutable(cssi, vk, extent);
		} else {
			bret = comp_scratch_single_images_ensure(cssi, vk, extent, format);
		}

		if (!bret) {
			VK_ERROR(vk, "comp_scratch_single_images_ensure[_mutable]: false");
			// Free any that has already been allocated.
			chl_scratch_free_resources(scratch, rr);
			return false;
		}

		for (uint32_t k = 0; k < COMP_SCRATCH_NUM_IMAGES; k++) {

			/*
			 * For graphics parts we use the same image view as the
			 * source. In other words the sRGB image view for the
			 * non-linear formats.
			 */
			VkImageView target_image_view = chl_scratch_get_sample_view(scratch, i, k);

			render_gfx_target_resources_init(  //
			    &scratch->views[i].targets[k], // rtr
			    rr,                            // struct render_resources
			    &scratch->render_pass,         // struct render_gfx_render_pass
			    target_image_view,             // target
			    extent);                       // extent
		}

		/*
		 * Update the count, doing it this way means free_resources
		 * will free the allocated images correctly. The count is one
		 * more then the index.
		 */
		scratch->view_count = i + 1;
	}

	// Update the cached values.
	scratch->extent = extent;
	scratch->format = format;

	return true;
}

void
chl_scratch_free_resources(struct chl_scratch *scratch, struct render_resources *rr)
{
	struct vk_bundle *vk = rr->vk;

	for (uint32_t i = 0; i < scratch->view_count; i++) {
		for (uint32_t k = 0; k < COMP_SCRATCH_NUM_IMAGES; k++) {
			render_gfx_target_resources_fini(&scratch->views[i].targets[k]);
		}

		comp_scratch_single_images_free(&scratch->views[i].cssi, vk);
	}

	// Nothing allocated.
	scratch->view_count = 0;
	scratch->extent.width = 0;
	scratch->extent.height = 0;
	scratch->format = VK_FORMAT_UNDEFINED;

	// Do this after the image targets as they reference the render pass.
	if (scratch->render_pass_initialized) {
		render_gfx_render_pass_fini(&scratch->render_pass);
		scratch->render_pass_initialized = false;
	}
}
