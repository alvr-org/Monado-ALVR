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
comp_render_layer(struct render_compute *crc,
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
comp_render_stereo_layers(struct render_compute *crc,
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


#ifdef __cplusplus
}
#endif
