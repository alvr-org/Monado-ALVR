// Copyright 2023-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Compositor rendering code helpers.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_util
 */

#pragma once

#include "xrt/xrt_compositor.h"

#include "render/render_interface.h"

#include "util/comp_base.h"
#include "util/comp_render.h"


/*
 *
 * Swapchain helpers.
 *
 */

static inline VkImageView
get_image_view(const struct comp_swapchain_image *image, enum xrt_layer_composition_flags flags, uint32_t array_index)
{
	if (flags & XRT_LAYER_COMPOSITION_BLEND_TEXTURE_SOURCE_ALPHA_BIT) {
		return image->views.alpha[array_index];
	}

	return image->views.no_alpha[array_index];
}


/*
 *
 * View index helpers.
 *
 */

static inline bool
is_view_index_right(uint32_t view_index)
{
	return view_index % 2 == 1;
}

static inline void
view_index_to_projection_data(uint32_t view_index,
                              const struct xrt_layer_data *data,
                              const struct xrt_layer_projection_view_data **out_vd)
{
	const struct xrt_layer_stereo_projection_data *stereo = &data->stereo;

	if (is_view_index_right(view_index)) {
		*out_vd = &stereo->r;
	} else {
		*out_vd = &stereo->l;
	}
}

static inline void
view_index_to_depth_data(uint32_t view_index,
                         const struct xrt_layer_data *data,
                         const struct xrt_layer_projection_view_data **out_vd,
                         const struct xrt_layer_depth_data **out_dvd)
{
	const struct xrt_layer_stereo_projection_depth_data *stereo = &data->stereo_depth;

	if (is_view_index_right(view_index)) {
		*out_vd = &stereo->r;
		*out_dvd = &stereo->r_d;
	} else {
		*out_vd = &stereo->l;
		*out_dvd = &stereo->l_d;
	}
}


/*
 *
 * Layer data helpers.
 *
 */

static inline bool
is_layer_view_visible(const struct xrt_layer_data *data, uint32_t view_index)
{
	enum xrt_layer_eye_visibility visibility;
	switch (data->type) {
	case XRT_LAYER_CUBE: visibility = data->cube.visibility; break;
	case XRT_LAYER_CYLINDER: visibility = data->cylinder.visibility; break;
	case XRT_LAYER_EQUIRECT1: visibility = data->equirect1.visibility; break;
	case XRT_LAYER_EQUIRECT2: visibility = data->equirect2.visibility; break;
	case XRT_LAYER_QUAD: visibility = data->quad.visibility; break;
	case XRT_LAYER_STEREO_PROJECTION:
	case XRT_LAYER_STEREO_PROJECTION_DEPTH: return true;
	default: return false;
	};

	switch (visibility) {
	case XRT_LAYER_EYE_VISIBILITY_LEFT_BIT: return !is_view_index_right(view_index);
	case XRT_LAYER_EYE_VISIBILITY_RIGHT_BIT: return is_view_index_right(view_index);
	case XRT_LAYER_EYE_VISIBILITY_BOTH: return true;
	case XRT_LAYER_EYE_VISIBILITY_NONE:
	default: return false;
	}
}

static inline bool
is_layer_view_space(const struct xrt_layer_data *data)
{
	return (data->flags & XRT_LAYER_COMPOSITION_VIEW_SPACE_BIT) != 0;
}

static inline bool
is_layer_unpremultiplied(const struct xrt_layer_data *data)
{
	return (data->flags & XRT_LAYER_COMPOSITION_UNPREMULTIPLIED_ALPHA_BIT) != 0;
}

static inline void
set_post_transform_rect(const struct xrt_layer_data *data,
                        const struct xrt_normalized_rect *src_norm_rect,
                        bool invert_flip,
                        struct xrt_normalized_rect *out_norm_rect)
{
	struct xrt_normalized_rect rect = *src_norm_rect;

	if (data->flip_y ^ invert_flip) {
		float h = rect.h;

		rect.h = -h;
		rect.y = rect.y + h;
	}

	*out_norm_rect = rect;
}


/*
 *
 * Command helpers.
 *
 */

static inline void
cmd_barrier_view_images(struct vk_bundle *vk,
                        const struct comp_render_dispatch_data *d,
                        VkCommandBuffer cmd,
                        VkAccessFlags src_access_mask,
                        VkAccessFlags dst_access_mask,
                        VkImageLayout transition_from,
                        VkImageLayout transition_to,
                        VkPipelineStageFlags src_stage_mask,
                        VkPipelineStageFlags dst_stage_mask)
{
	VkImageSubresourceRange first_color_level_subresource_range = {
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = 1,
	    .baseArrayLayer = 0,
	    .layerCount = 1,
	};

	for (uint32_t i = 0; i < d->view_count; i++) {
		bool already_barried = false;

		VkImage image = d->views[i].image;

		uint32_t k = i;
		while (k > 0) {
			k--; // k is always greater then zero.

			if (d->views[k].image == image) {
				already_barried = true;
				break;
			}
		}

		if (already_barried) {
			continue;
		}

		vk_cmd_image_barrier_locked(              //
		    vk,                                   // vk_bundle
		    cmd,                                  // cmd_buffer
		    image,                                // image
		    src_access_mask,                      // src_access_mask
		    dst_access_mask,                      // dst_access_mask
		    transition_from,                      // old_image_layout
		    transition_to,                        // new_image_layout
		    src_stage_mask,                       // src_stage_mask
		    dst_stage_mask,                       // dst_stage_mask
		    first_color_level_subresource_range); // subresource_range
	}
}
