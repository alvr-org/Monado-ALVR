// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Compositor render implementation.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup comp_util
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_vulkan_includes.h" // IWYU pragma: keep

#include "render/render_interface.h"
#include "util/u_misc.h"

#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif

struct comp_layer;
struct render_compute;
struct render_gfx;
struct render_gfx_target_resources;


/*!
 * @defgroup comp_render
 * @brief Renders, aka "layer squashers" and distortion application.
 *
 * Two parallel implementations of the render module exist:
 *
 * - one uses graphics shaders (aka GFX, @ref comp_render_gfx, @ref comp_render_gfx.c)
 * - the other uses compute shaders (aka CS, @ref comp_render_cs, @ref comp_render_cs.c)
 *
 * Their abilities are effectively equivalent, although the graphics version disregards depth
 * data, while the compute shader does use it somewhat.
 *
 * @note In general this module requires that swapchains in your supplied @ref comp_layer layers
 * implement @ref comp_swapchain in addition to just @ref xrt_swapchain.
 */

/*
 *
 * Input data struct.
 *
 */

/*!
 * @name Input data structs
 * @{
 */

/*!
 * The input data needed for a single view, shared between both GFX and CS
 * paths.
 *
 * To fully render a single view two "rendering" might be needed, the
 * first being the layer squashing, and the second is the distortion step. The
 * target for the layer squashing is referred to as "layer" or "scratch" and
 * prefixed with `layer` if needs be. The other final step is referred to as
 * "distortion target" or just "target", and is prefixed with `target`.
 *
 * @ingroup comp_render
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
 *
 * @ingroup comp_render
 */
struct comp_render_dispatch_data
{
	struct comp_render_view_data views[XRT_MAX_VIEWS];

	//! The number of views currently in this dispatch data.
	uint32_t view_count;

	//! Fast path can be disabled for mirroing so needs to be an argument.
	bool fast_path;

	//! Very often true, can be disabled for debugging.
	bool do_timewarp;

	//! Members used only by GFX @ref comp_render_gfx
	struct
	{
		//! The resources needed for the target.
		struct render_gfx_target_resources *rtr;
	} gfx;

	//! Members used only by CS @ref comp_render_cs
	struct
	{
		//! Target image for distortion, used for barrier.
		VkImage target_image;

		//! Target image view for distortion.
		VkImageView target_unorm_view;
	} cs;
};

/*!
 * Shared implementation setting up common view params between GFX and CS.
 *
 * Private implementation method, do not use outside of more-specific add_view calls!
 *
 * @param data Common render dispatch data, will be updated
 * @param world_pose New world pose of this view.
 *        Populates @ref comp_render_view_data::world_pose
 * @param eye_pose New eye pose of this view
 *        Populates @ref comp_render_view_data::eye_pose
 * @param fov Assigned to fov in the view data, and used to compute @ref comp_render_view_data::target_pre_transform
 *        Populates @ref comp_render_view_data::fov
 * @param layer_viewport_data Where in the image to render the view
 *        Populates @ref comp_render_view_data::layer_viewport_data
 * @param layer_norm_rect How to transform when sampling from the scratch image.
 *        Populates @ref comp_render_view_data::layer_norm_rect
 * @param image Scratch image for this view
 *        Populates @ref comp_render_view_data::image
 * @param srgb_view SRGB image view into the scratch image
 *        Populates @ref comp_render_view_data::srgb_view
 * @param target_viewport_data Distortion target viewport data (aka target)
 *        Populates @ref comp_render_view_data::target_viewport_data

 * @return Pointer to the @ref comp_render_view_data we have been populating, for additional setup.
 */
static inline struct comp_render_view_data *
comp_render_dispatch_add_view(struct comp_render_dispatch_data *data,
                              const struct xrt_pose *world_pose,
                              const struct xrt_pose *eye_pose,
                              const struct xrt_fov *fov,
                              const struct render_viewport_data *layer_viewport_data,
                              const struct xrt_normalized_rect *layer_norm_rect,
                              VkImage image,
                              VkImageView srgb_view,
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

	return view;
}

/*! @} */

/*
 *
 * Gfx functions.
 *
 */

/*!
 *
 * @defgroup comp_render_gfx
 *
 * GFX renderer control and dispatch - uses graphics shaders.
 *
 * Depends on the common @ref comp_render_dispatch_data, as well as the resources
 * @ref render_gfx_target_resources (often called `rtr`), and @ref render_gfx.
 *
 * @ingroup comp_render
 * @{
 */

/*!
 * Initialize structure for use of the GFX renderer.
 *
 * @param[out] data Common render dispatch data. Will be zeroed and initialized.
 * @param rtr GFX-specific resources for the entire frameedg. Must be populated before call.
 * @param fast_path Whether we will use the "fast path" avoiding layer squashing.
 * @param do_timewarp Whether timewarp (reprojection) will be performed.
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

/*!
 * Add view to the common data, as required by the GFX renderer.
 *
 * @param[in,out] data Common render dispatch data, will be updated
 * @param world_pose New world pose of this view.
 *        Populates @ref comp_render_view_data::world_pose
 * @param eye_pose New eye pose of this view
 *        Populates @ref comp_render_view_data::eye_pose
 * @param fov Assigned to fov in the view data, and used to
 *        compute @ref comp_render_view_data::target_pre_transform - also
 *        populates @ref comp_render_view_data::fov
 * @param rtr Will be associated with this view. GFX-specific
 * @param layer_viewport_data Where in the image to render the view
 *        Populates @ref comp_render_view_data::layer_viewport_data
 * @param layer_norm_rect How to transform when sampling from the scratch image.
 *        Populates @ref comp_render_view_data::layer_norm_rect
 * @param image Scratch image for this view
 *        Populates @ref comp_render_view_data::image
 * @param srgb_view SRGB image view into the scratch image
 *        Populates @ref comp_render_view_data::srgb_view
 * @param vertex_rot
 * @param target_viewport_data Distortion target viewport data (aka target)
 *        Populates @ref comp_render_view_data::target_viewport_data
 */
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
	struct comp_render_view_data *view = comp_render_dispatch_add_view( //
	    data,                                                           //
	    world_pose,                                                     //
	    eye_pose,                                                       //
	    fov,                                                            //
	    layer_viewport_data,                                            //
	    layer_norm_rect,                                                //
	    image,                                                          //
	    srgb_view,                                                      //
	    target_viewport_data);

	// TODO why is the one in data not used instead
	view->gfx.rtr = rtr;
	view->gfx.vertex_rot = *vertex_rot;
}

/*!
 * Writes the needed commands to the @ref render_gfx to do a full composition with distortion.
 *
 * Takes a set of layers, new device poses, scratch
 * images with associated @ref render_gfx_target_resources and writes the needed
 * commands to the @ref render_gfx to do a full composition with distortion.
 * The scratch images are optionally used to squash layers should it not be
 * possible to do a @p comp_render_dispatch_data::fast_path. Will use the render
 * passes of the targets which set the layout.
 *
 * The render passes of @p comp_render_dispatch_data::views::rtr must be created
 * with a final_layout of `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` or there will
 * be validation errors.
 *
 * Expected layouts:
 *
 * - Layer images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Scratch images: Any (as per the @ref render_gfx_render_pass)
 * - Target image: Any (as per the @ref render_gfx_render_pass)
 *
 * After call layouts:
 *
 * - Layer images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Scratch images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Target image: What the render pass of @p rtr specifies.
 *
 * @note Swapchains in the @p layers must implement @ref comp_swapchain in
 * addition to just @ref xrt_swapchain, as this function downcasts to @ref comp_swapchain !
 *
 * @param rr GFX render object
 * @param[in] layers Layers to render, see note.
 * @param[in] layer_count Number of elements in @p layers array.
 * @param[in] d Common render dispatch data
 */
void
comp_render_gfx_dispatch(struct render_gfx *rr,
                         const struct comp_layer *layers,
                         const uint32_t layer_count,
                         const struct comp_render_dispatch_data *d);

/* end of comp_render_gfx group */

/*! @} */


/*
 *
 * CS functions.
 *
 */

/*!
 *
 * @defgroup comp_render_cs
 *
 * CS renderer control and dispatch - uses compute shaders
 *
 * Depends on @ref render_compute (often called `crc`)
 *
 * @ingroup comp_render
 * @{
 */

/*!
 * Initialize structure for use of the CS renderer.
 *
 * @param data Common render dispatch data. Will be zeroed and initialized.
 * @param target_image Image to render into
 * @param target_unorm_view Corresponding image view
 * @param fast_path Whether we will use the "fast path" avoiding layer squashing.
 * @param do_timewarp Whether timewarp (reprojection) will be performed.
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

/*!
 * Add view to the common data, as required by the CS renderer.
 *
 * @param[in,out] data Common render dispatch data, will be updated
 * @param world_pose New world pose of this view.
 *        Populates @ref comp_render_view_data::world_pose
 * @param eye_pose New eye pose of this view
 *        Populates @ref comp_render_view_data::eye_pose
 * @param fov Assigned to fov in the view data, and used to compute @ref comp_render_view_data::target_pre_transform
 *        Populates @ref comp_render_view_data::fov
 * @param layer_viewport_data Where in the image to render the view
 *        Populates @ref comp_render_view_data::layer_viewport_data
 * @param layer_norm_rect How to transform when sampling from the scratch image.
 *        Populates @ref comp_render_view_data::layer_norm_rect
 * @param image Scratch image for this view
 *        Populates @ref comp_render_view_data::image
 * @param srgb_view SRGB image view into the scratch image
 *        Populates @ref comp_render_view_data::srgb_view
 * @param unorm_view UNORM image view into the scratch image, CS specific
 * @param target_viewport_data Distortion target viewport data (aka target)
 *        Populates @ref comp_render_view_data::target_viewport_data
 */
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
	struct comp_render_view_data *view = comp_render_dispatch_add_view( //
	    data,                                                           //
	    world_pose,                                                     //
	    eye_pose,                                                       //
	    fov,                                                            //
	    layer_viewport_data,                                            //
	    layer_norm_rect,                                                //
	    image,                                                          //
	    srgb_view,                                                      //
	    target_viewport_data);

	view->cs.unorm_view = unorm_view;
}

/*!
 * Dispatch the layer squasher for a single view.
 *
 * All source layer images and target image needs to be in the correct image
 * layout, no barrier is inserted at all. The @p view_index argument is needed
 * to grab a pre-allocated UBO from the @ref render_resources and to correctly
 * select left/right data from various layers.
 *
 * Expected layouts:
 *
 * - Layer images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Target images: `VK_IMAGE_LAYOUT_GENERAL`
 *
 * @note Swapchains in the @p layers must implement @ref comp_swapchain in
 * addition to just @ref xrt_swapchain, as this function downcasts to @ref comp_swapchain !
 *
 * @param crc Compute renderer object
 * @param view_index Index of the view
 * @param layers Layers to render, see note.
 * @param layer_count Number of elements in @p layers array.
 * @param pre_transform
 * @param world_pose
 * @param eye_pose
 * @param target_image
 * @param target_image_view
 * @param target_view
 * @param do_timewarp
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
 * Dispatch the layer squasher, on any number of views.
 *
 * All source layer images needs to be in the correct image layout, no barrier
 * is inserted for them. The target images are barriered from undefined to general
 * so they can be written to, then to the laying defined by @p transition_to.
 *
 * Expected layouts:
 *
 * - Layer images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Target images: Any
 *
 * After call layouts:
 *
 * - Layer images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Target images: @p transition_to
 *
 * @note Swapchains in the @p layers must implement @ref comp_swapchain in
 * addition to just @ref xrt_swapchain, as this function downcasts to @ref comp_swapchain !
 *
 * @param crc Compute renderer object
 * @param[in] layers Layers to render, see note.
 * @param[in] layer_count Number of elements in @p layers array.
 * @param[in] d Common render dispatch data
 * @param[in] transition_to Desired image layout for target images
 */
void
comp_render_cs_layers(struct render_compute *crc,
                      const struct comp_layer *layers,
                      const uint32_t layer_count,
                      const struct comp_render_dispatch_data *d,
                      VkImageLayout transition_to);

/*!
 * Write commands to @p crc to do a full composition with distortion.
 *
 * Helper function that takes a set of layers, new device poses, a scratch
 * images and writes the needed commands to the @ref render_compute to do a full
 * composition with distortion. The scratch images are optionally used to squash
 * layers should it not be possible to do a fast_path. Will insert barriers to
 * change the scratch images and target images to the needed layout.
 *
 *
 * Expected layouts:
 *
 * - Layer images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Scratch images: Any
 * - Target image: Any
 *
 * After call layouts:
 *
 * - Layer images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Scratch images: `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
 * - Target image: `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`
 *
 * @note Swapchains in the @p layers must implement @ref comp_swapchain in
 * addition to just @ref xrt_swapchain, as this function downcasts to @ref comp_swapchain !
 *
 * @param crc Compute renderer object
 * @param[in] layers Layers to render, see note.
 * @param[in] layer_count Number of elements in @p layers array.
 * @param[in] d Common render dispatch data
 *
 */
void
comp_render_cs_dispatch(struct render_compute *crc,
                        const struct comp_layer *layers,
                        const uint32_t layer_count,
                        const struct comp_render_dispatch_data *d);

/* end of comp_render_cs group */
/*! @} */

#ifdef __cplusplus
}
#endif
