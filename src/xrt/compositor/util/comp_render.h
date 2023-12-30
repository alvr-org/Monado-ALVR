// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Independent semaphore implementation.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_util
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_vulkan_includes.h"

#include "render/render_interface.h"


#ifdef __cplusplus
extern "C" {
#endif

struct comp_layer;


/*
 *
 * Input data struct.
 *
 */

/*!
 * The input data needed for a single view, it shared between both GFX and CS
 * paths. To fully render a single view two "rendering" might be needed, the
 * first being the layer squashing, and the second is the distortion step. The
 * target for the layer squashing is referred to as "layer" or "scratch" and
 * prefixed with `layer` if needs be. The other final step is referred to as
 * "distortion target" or just "target", and is prefixed with `target`.
 */
struct comp_render_view_data
{
	//! New world pose of this view.
	struct xrt_pose world_pose;

	//! New eye pose of this view.
	struct xrt_pose eye_pose;

	/*!
	 * New fov of this view, used for the layer scratch image. Needs to
	 * match distortion parameters if distortion is used.
	 */
	struct xrt_fov fov;

	/*!
	 * The layer image for this view (aka scratch image),
	 * used for barrier operations.
	 */
	VkImage image;

	/*!
	 * View into layer image (aka scratch image),
	 * used for both GFX (read/write) and CS (read) paths.
	 */
	VkImageView srgb_view;

	/*!
	 * Pre-view layer target viewport_data (where in the image we should
	 * render the view).
	 */
	struct render_viewport_data layer_viewport_data;

	/*!
	 * When sampling from the layer image (aka scratch image), how should we
	 * transform it to get to the pixels correctly.
	 */
	struct xrt_normalized_rect layer_norm_rect;

	//! Go from UV to tanangle for the target, this needs to match @p fov.
	struct xrt_normalized_rect target_pre_transform;

	// Distortion target viewport data (aka target).
	struct render_viewport_data target_viewport_data;

	struct
	{
		//! Per-view layer target resources.
		struct render_gfx_target_resources *rtr;

		//! Distortion target vertex rotation information.
		struct xrt_matrix_2x2 vertex_rot;
	} gfx;

	struct
	{
		//! Only used on compute path.
		VkImageView unorm_view;
	} cs;
};

/*!
 * The input data needed for a complete layer squashing distortion rendering
 * to a target. This struct is shared between GFX and CS paths.
 */
struct comp_render_dispatch_data
{
	struct comp_render_view_data views[2];

	//! The number of views currently in this dispatch data.
	uint32_t view_count;

	//! Fast path can be disabled for mirroing so needs to be an argument.
	bool fast_path;

	//! Very often true, can be disabled for debugging.
	bool do_timewarp;

	struct
	{
		// The resources needed for the target.
		struct render_gfx_target_resources *rtr;
	} gfx;

	struct
	{
		// Target image for distortion, used for barrier.
		VkImage target_image;

		// Target image view for distortion.
		VkImageView target_unorm_view;
	} cs;
};


/*
 *
 * Gfx functions.
 *
 */

static inline void
comp_render_gfx_initial_init(struct comp_render_dispatch_data *data,
                             struct render_gfx_target_resources *rtr,
                             bool fast_path,
                             bool do_timewarp)
{
	U_ZERO(data);

	data->fast_path = fast_path;
	data->do_timewarp = do_timewarp;
	data->gfx.rtr = rtr;
}

static inline void
comp_render_gfx_add_view(struct comp_render_dispatch_data *data,
                         const struct xrt_pose *world_pose,
                         const struct xrt_pose *eye_pose,
                         const struct xrt_fov *fov,
                         struct render_gfx_target_resources *rtr,
                         const struct render_viewport_data *layer_viewport_data,
                         const struct xrt_normalized_rect *layer_norm_rect,
                         VkImage image,
                         VkImageView srgb_view,
                         const struct xrt_matrix_2x2 *vertex_rot,
                         const struct render_viewport_data *target_viewport_data)
{
	uint32_t i = data->view_count++;

	assert(i < ARRAY_SIZE(data->views));

	struct comp_render_view_data *view = &data->views[i];

	render_calc_uv_to_tangent_lengths_rect(fov, &view->target_pre_transform);

	view->world_pose = *world_pose;
	view->eye_pose = *eye_pose;
	view->fov = *fov;
	view->image = image;
	view->srgb_view = srgb_view;
	view->layer_viewport_data = *layer_viewport_data;
	view->layer_norm_rect = *layer_norm_rect;
	view->target_viewport_data = *target_viewport_data;

	view->gfx.rtr = rtr;
	view->gfx.vertex_rot = *vertex_rot;
}

/*!
 * Helper function that takes a set of layers, new device poses, a scratch
 * images with associated @ref render_gfx_target_resources and writes the needed
 * commands to the @ref render_gfx to do a full composition with distortion.
 * The scratch images are optionally used to squash layers should it not be
 * possible to do a @p comp_render_dispatch_data::fast_path. Will use the render
 * passes of the targets which set the layout.
 *
 * The render passes of @p comp_render_dispatch_data::views::rtr must be created
 * with a final_layout of VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL or there will
 * be validation errors.
 *
 * Expected layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Scratch images: Any (as per the @ref render_gfx_render_pass)
 * * Target image: Any (as per the @ref render_gfx_render_pass)
 *
 * After call layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Scratch images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Target image: What the render pass of @p rtr specifies.
 *
 * @ingroup comp_util
 */
void
comp_render_gfx_dispatch(struct render_gfx *rr,
                         const struct comp_layer *layers,
                         const uint32_t layer_count,
                         const struct comp_render_dispatch_data *d);


/*
 *
 * CS functions.
 *
 */

static inline void
comp_render_cs_initial_init(struct comp_render_dispatch_data *data,
                            VkImage target_image,
                            VkImageView target_unorm_view,
                            bool fast_path,
                            bool do_timewarp)
{
	U_ZERO(data);

	data->fast_path = fast_path;
	data->do_timewarp = do_timewarp;

	data->cs.target_image = target_image;
	data->cs.target_unorm_view = target_unorm_view;
}

static inline void
comp_render_cs_add_view(struct comp_render_dispatch_data *data,
                        const struct xrt_pose *world_pose,
                        const struct xrt_pose *eye_pose,
                        const struct xrt_fov *fov,
                        const struct render_viewport_data *layer_viewport_data,
                        const struct xrt_normalized_rect *layer_norm_rect,
                        VkImage image,
                        VkImageView srgb_view,
                        VkImageView unorm_view,
                        const struct render_viewport_data *target_viewport_data)
{
	uint32_t i = data->view_count++;

	assert(i < ARRAY_SIZE(data->views));

	struct comp_render_view_data *view = &data->views[i];

	render_calc_uv_to_tangent_lengths_rect(fov, &view->target_pre_transform);

	view->world_pose = *world_pose;
	view->eye_pose = *eye_pose;
	view->fov = *fov;
	view->layer_viewport_data = *layer_viewport_data;
	view->layer_norm_rect = *layer_norm_rect;
	view->image = image;
	view->srgb_view = srgb_view;
	view->target_viewport_data = *target_viewport_data;

	view->cs.unorm_view = unorm_view;
}

/*!
 * Helper to dispatch the layer squasher for a single view.
 *
 * All source layer images and target image needs to be in the correct image
 * layout, no barrier is inserted at all. The @p view_index argument is needed
 * to grab a pre-allocated UBO from the @ref render_resources and to correctly
 * select left/right data from various layers.
 *
 * Expected layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Target images: VK_IMAGE_LAYOUT_GENERAL
 */
void
comp_render_cs_layer(struct render_compute *crc,
                     uint32_t view_index,
                     const struct comp_layer *layers,
                     const uint32_t layer_count,
                     const struct xrt_normalized_rect *pre_transform,
                     const struct xrt_pose *world_pose,
                     const struct xrt_pose *eye_pose,
                     const VkImage target_image,
                     const VkImageView target_image_view,
                     const struct render_viewport_data *target_view,
                     bool do_timewarp);

/*!
 * Helper function to dispatch the layer squasher, works on any number of views.
 *
 * All source layer images needs to be in the correct image layout, no barrier
 * is inserted for them. The target images are barried from undefined to general
 * so they can be written to, then to the laying defined by @p transition_to.
 *
 * Expected layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Target images: Any
 *
 * After call layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Target images: @p transition_to
 *
 * @ingroup comp_util
 */
void
comp_render_cs_layers(struct render_compute *crc,
                      const struct comp_layer *layers,
                      const uint32_t layer_count,
                      const struct comp_render_dispatch_data *d,
                      VkImageLayout transition_to);

/*!
 * Helper function that takes a set of layers, new device poses, a scratch
 * images and writes the needed commands to the @ref render_compute to do a full
 * composition with distortion. The scratch images are optionally used to squash
 * layers should it not be possible to do a fast_path. Will insert barriers to
 * change the scratch images and target images to the needed layout.
 *
 * Currently limited to exactly two views.
 *
 * Expected layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Sratch images: Any
 * * Target image: Any
 *
 * After call layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Sratch images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Target image: VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
 *
 * @ingroup comp_util
 */
void
comp_render_cs_dispatch(struct render_compute *crc,
                        const struct comp_layer *layers,
                        const uint32_t layer_count,
                        const struct comp_render_dispatch_data *d);


#ifdef __cplusplus
}
#endif
