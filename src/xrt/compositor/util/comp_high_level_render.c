// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Higher level interface for rendering a frame.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Fernando Velazquez Innella <finnella@magicleap.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup comp_util
 */

#include "comp_high_level_render.h"


/*
 *
 * Common
 *
 */

void
chl_frame_state_init(struct chl_frame_state *frame_state,
                     struct render_resources *rr,
                     uint32_t view_count,
                     bool do_timewarp,
                     bool fast_path,
                     struct chl_scratch *scratch)
{
	assert(rr != NULL);
	assert(rr->vk != NULL);
	assert(view_count <= rr->view_count);

	U_ZERO(frame_state);
	frame_state->view_count = view_count;
	frame_state->scratch = scratch;

	comp_render_initial_init( //
	    &frame_state->data,   // data
	    fast_path,            // fast_path
	    do_timewarp);         // do_timewarp

	chl_scratch_state_init_and_get(&frame_state->scratch_state, scratch);
}

void
chl_frame_state_fini(struct chl_frame_state *frame_state)
{
	if (frame_state->scratch == NULL) {
		return;
	}

	chl_scratch_state_discard_or_done(&frame_state->scratch_state, frame_state->scratch);

	U_ZERO(frame_state);
}


/*
 *
 * Graphics
 *
 */

void
chl_frame_state_gfx_set_views(struct chl_frame_state *frame_state,
                              const struct xrt_pose world_poses_scanout_begin[XRT_MAX_VIEWS],
                              const struct xrt_pose world_poses_scanout_end[XRT_MAX_VIEWS],
                              const struct xrt_pose eye_poses[XRT_MAX_VIEWS],
                              const struct xrt_fov fovs[XRT_MAX_VIEWS],
                              uint32_t layer_count)
{
	for (uint32_t i = 0; i < frame_state->view_count; i++) {
		// Which image of the scratch images for this view are we using.
		uint32_t scratch_index = frame_state->scratch_state.views[i].index;

		// The set of scratch images we are using for this view.
		struct comp_scratch_single_images *scratch_view = &frame_state->scratch->views[i].cssi;

		// The render target resources for the scratch images.
		struct render_gfx_target_resources *rsci_rtr = &frame_state->scratch->views[i].targets[scratch_index];

		// Scratch color image.
		struct render_scratch_color_image *rsci = &scratch_view->images[scratch_index];

		// Use the whole scratch image.
		struct render_viewport_data layer_viewport_data = {
		    .x = 0,
		    .y = 0,
		    .w = scratch_view->info.width,
		    .h = scratch_view->info.height,
		};

		comp_render_gfx_add_squash_view(   //
		    &frame_state->data,            //
		    &world_poses_scanout_begin[i], //
		    &world_poses_scanout_end[i],   //
		    &eye_poses[i],                 //
		    &fovs[i],                      //
		    rsci->image,                   // squash_image
		    rsci_rtr,                      // squash_rtr
		    &layer_viewport_data);         // squash_viewport_data

		if (layer_count == 0) {
			frame_state->scratch_state.views[i].used = false;
		} else {
			frame_state->scratch_state.views[i].used = !frame_state->data.fast_path;
		}
	}
}

void
chl_frame_state_gfx_set_target(struct chl_frame_state *frame_state,
                               struct render_gfx_target_resources *target_rtr,
                               const struct render_viewport_data target_viewport_datas[XRT_MAX_VIEWS],
                               const render_scissor_data_t target_scissor_datas[XRT_MAX_VIEWS],
                               const struct xrt_matrix_2x2 vertex_rots[XRT_MAX_VIEWS])
{
	// Add the target info.
	comp_render_gfx_add_target(&frame_state->data, target_rtr);

	for (uint32_t i = 0; i < frame_state->view_count; i++) {
		// Which image of the scratch images for this view are we using.
		uint32_t scratch_index = frame_state->scratch_state.views[i].index;

		// The set of scratch images we are using for this view.
		struct comp_scratch_single_images *scratch_view = &frame_state->scratch->views[i].cssi;

		// Scratch image covers the whole image.
		struct xrt_normalized_rect layer_norm_rect = {.x = 0.0f, .y = 0.0f, .w = 1.0f, .h = 1.0f};

		VkImageView sample_view = comp_scratch_single_images_get_sample_view(scratch_view, scratch_index);

		comp_render_gfx_add_target_view( //
		    &frame_state->data,          //
		    sample_view,                 // squash_as_src_sample_view
		    &layer_norm_rect,            // squash_as_src_norm_rect
		    &vertex_rots[i],             // target_vertex_rot
		    &target_viewport_datas[i],   // target_viewport_data
		    &target_scissor_datas[i]);   // target_scissor_data
	}
}


/*
 *
 * Compute
 *
 */

void
chl_frame_state_cs_set_views(struct chl_frame_state *frame_state,
                             const struct xrt_pose world_poses_scanout_begin[XRT_MAX_VIEWS],
                             const struct xrt_pose world_poses_scanout_end[XRT_MAX_VIEWS],
                             const struct xrt_pose eye_poses[XRT_MAX_VIEWS],
                             const struct xrt_fov fovs[XRT_MAX_VIEWS],
                             uint32_t layer_count)
{
	for (uint32_t i = 0; i < frame_state->view_count; i++) {
		// Which image of the scratch images for this view are we using.
		uint32_t scratch_index = frame_state->scratch_state.views[i].index;

		// The set of scratch images we are using for this view.
		struct comp_scratch_single_images *scratch_view = &frame_state->scratch->views[i].cssi;

		// Scratch color image.
		struct render_scratch_color_image *rsci = &scratch_view->images[scratch_index];

		// Use the whole scratch image.
		struct render_viewport_data layer_viewport_data = {
		    .x = 0,
		    .y = 0,
		    .w = scratch_view->info.width,
		    .h = scratch_view->info.height,
		};

		VkImageView storage_view = comp_scratch_single_images_get_storage_view(scratch_view, scratch_index);

		comp_render_cs_add_squash_view(    //
		    &frame_state->data,            //
		    &world_poses_scanout_begin[i], //
		    &world_poses_scanout_end[i],   //
		    &eye_poses[i],                 //
		    &fovs[i],                      //
		    rsci->image,                   // squash_image
		    storage_view,                  // squash_storage_view
		    &layer_viewport_data);         // squash_viewport_data

		if (layer_count == 0) {
			frame_state->scratch_state.views[i].used = false;
		} else {
			frame_state->scratch_state.views[i].used = !frame_state->data.fast_path;
		}
	}
}

void
chl_frame_state_cs_set_target(struct chl_frame_state *frame_state,
                              VkImage target_image,
                              VkImageView target_storage_view,
                              VkImageLayout target_final_layout,
                              const struct render_viewport_data views[XRT_MAX_VIEWS])
{
	// Add the target info.
	comp_render_cs_add_target( //
	    &frame_state->data,    // data
	    target_image,          // target_image
	    target_storage_view,   // target_unorm_view
	    target_final_layout);  // target_final_layout

	for (uint32_t i = 0; i < frame_state->view_count; i++) {
		// Which image of the scratch images for this view are we using.
		uint32_t scratch_index = frame_state->scratch_state.views[i].index;

		// The set of scratch images we are using for this view.
		struct comp_scratch_single_images *scratch_view = &frame_state->scratch->views[i].cssi;

		// Scratch image covers the whole image.
		struct xrt_normalized_rect layer_norm_rect = {.x = 0.0f, .y = 0.0f, .w = 1.0f, .h = 1.0f};

		VkImageView sample_view = comp_scratch_single_images_get_sample_view(scratch_view, scratch_index);

		comp_render_cs_add_target_view( //
		    &frame_state->data,         //
		    sample_view,                // squash_as_src_sample_view
		    &layer_norm_rect,           // squash_as_src_norm_rect
		    &views[i],                  // target_viewport_data
		    &views[i]);                 // target_scissor_data
	}
}
