// Copyright 2019-2023, Collabora, Ltd.
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


/*!
 * Helper function that takes a set of layers, new device poses, a scratch
 * images with associated @ref render_gfx_target_resources and writes the needed
 * commands to the @ref render_gfx to do a full composition with distortion.
 * The scratch images are optionally used to squash layers should it not be
 * possible to do a @p fast_path. Will use the render passes of the targets
 * which set the layout.
 *
 * The render passes of @p rsi_rtrs must be created with a final_layout of
 * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL or there will be validation errors.
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
                         struct render_scratch_images *rsi,
                         struct render_gfx_target_resources rsi_rtrs[2],
                         const struct comp_layer *layers,
                         const uint32_t layer_count,
                         struct xrt_pose world_poses[2],
                         struct xrt_pose eye_poses[2],
                         struct xrt_fov fovs[2],
                         struct xrt_matrix_2x2 vertex_rots[2],
                         struct render_gfx_target_resources *rtr,
                         const struct render_viewport_data viewport_datas[2],
                         bool fast_path,
                         bool do_timewarp);


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
 * Helper function to dispatch the layer squasher, designed for stereo views.
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
comp_render_cs_stereo_layers(struct render_compute *crc,
                             const struct comp_layer *layers,
                             const uint32_t layer_count,
                             const struct xrt_normalized_rect pre_transforms[2],
                             const struct xrt_pose world_poses[2],
                             const struct xrt_pose eye_poses[2],
                             const VkImage target_images[2],
                             const VkImageView target_image_views[2],
                             const struct render_viewport_data target_views[2],
                             VkImageLayout transition_to,
                             bool do_timewarp);

/*!
 * Helper function that takes a set of layers, new device poses, a scratch
 * images and writes the needed commands to the @ref render_compute to squash
 * the given layers into the given scratch images. Will insert barriers to
 * change the scratch images to the needed layout.
 *
 * Expected layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Scratch images: Any
 *
 * After call layouts:
 * * Layer images: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * * Scratch images: @p transition_to
 *
 * @ingroup comp_util
 */
void
comp_render_cs_stereo_layers_to_scratch(struct render_compute *crc,
                                        const struct xrt_normalized_rect pre_transforms[2],
                                        struct xrt_pose world_poses[2],
                                        struct xrt_pose eye_poses[2],
                                        const struct comp_layer *layers,
                                        const uint32_t layer_count,
                                        struct render_scratch_images *rsi,
                                        VkImageLayout transition_to,
                                        bool do_timewarp);

/*!
 * Helper function that takes a set of layers, new device poses, a scratch
 * images and writes the needed commands to the @ref render_compute to do a full
 * composition with distortion. The scratch images are optionally used to squash
 * layers should it not be possible to do a fast_path. Will insert barriers to
 * change the scratch images and target images to the needed layout.
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
                        struct render_scratch_images *rsi,
                        struct xrt_pose world_poses[2],
                        struct xrt_pose eye_poses[2],
                        const struct comp_layer *layers,
                        const uint32_t layer_count,
                        VkImage target_image,
                        VkImageView target_image_view,
                        const struct render_viewport_data views[2],
                        bool fast_path,
                        bool do_timewarp);


#ifdef __cplusplus
}
#endif
