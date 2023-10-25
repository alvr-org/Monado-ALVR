// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Compositor rendering code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Fernando Velazquez Innella <finnella@magicleap.com>
 * @ingroup comp_main
 */

#include "xrt/xrt_compositor.h"

#include "os/os_time.h"

#include "math/m_api.h"
#include "math/m_mathinclude.h"

#include "util/u_misc.h"
#include "util/u_trace_marker.h"

#include "vk/vk_helpers.h"

#include "render/render_interface.h"

#include "util/comp_render.h"
#include "util/comp_render_helpers.h"
#include "util/comp_base.h"


/*
 *
 * Compute layer data builders.
 *
 */

static inline void
do_cs_equirect2_layer(const struct xrt_layer_data *data,
                      const struct comp_layer *layer,
                      const struct xrt_matrix_4x4 *eye_view_mat,
                      const struct xrt_matrix_4x4 *world_view_mat,
                      uint32_t view_index,
                      uint32_t cur_layer,
                      uint32_t cur_image,
                      VkSampler clamp_to_edge,
                      VkSampler clamp_to_border_black,
                      VkSampler src_samplers[RENDER_MAX_IMAGES],
                      VkImageView src_image_views[RENDER_MAX_IMAGES],
                      struct render_compute_layer_ubo_data *ubo_data,
                      uint32_t *out_cur_image)
{
	const struct xrt_layer_equirect2_data *eq2 = &data->equirect2;

	const struct comp_swapchain_image *image = &layer->sc_array[0]->images[eq2->sub.image_index];
	uint32_t array_index = eq2->sub.array_index;

	// Image to use.
	src_samplers[cur_image] = clamp_to_edge;
	src_image_views[cur_image] = get_image_view(image, data->flags, array_index);

	// Used for Subimage and OpenGL flip.
	set_post_transform_rect(                    //
	    data,                                   // data
	    &eq2->sub.norm_rect,                    // src_norm_rect
	    false,                                  // invert_flip
	    &ubo_data->post_transforms[cur_layer]); // out_norm_rect

	struct xrt_vec3 scale = {1.f, 1.f, 1.f};

	struct xrt_matrix_4x4 model;
	math_matrix_4x4_model(&eq2->pose, &scale, &model);

	struct xrt_matrix_4x4 model_inv;
	math_matrix_4x4_inverse(&model, &model_inv);

	const struct xrt_matrix_4x4 *v = is_layer_view_space(data) ? eye_view_mat : world_view_mat;

	struct xrt_matrix_4x4 v_inv;
	math_matrix_4x4_inverse(v, &v_inv);

	math_matrix_4x4_multiply(&model_inv, &v_inv, &ubo_data->mv_inverse[cur_layer]);

	// Simplifies the shader.
	if (eq2->radius >= INFINITY) {
		ubo_data->eq2_data[cur_layer].radius = 0.f;
	} else {
		ubo_data->eq2_data[cur_layer].radius = eq2->radius;
	}

	ubo_data->eq2_data[cur_layer].central_horizontal_angle = eq2->central_horizontal_angle;
	ubo_data->eq2_data[cur_layer].upper_vertical_angle = eq2->upper_vertical_angle;
	ubo_data->eq2_data[cur_layer].lower_vertical_angle = eq2->lower_vertical_angle;

	ubo_data->images_samplers[cur_layer].images[0] = cur_image;
	cur_image++;

	*out_cur_image = cur_image;
}

static inline void
do_cs_projection_layer(const struct xrt_layer_data *data,
                       const struct comp_layer *layer,
                       const struct xrt_pose *world_pose,
                       uint32_t view_index,
                       uint32_t cur_layer,
                       uint32_t cur_image,
                       VkSampler clamp_to_edge,
                       VkSampler clamp_to_border_black,
                       VkSampler src_samplers[RENDER_MAX_IMAGES],
                       VkImageView src_image_views[RENDER_MAX_IMAGES],
                       struct render_compute_layer_ubo_data *ubo_data,
                       bool do_timewarp,
                       uint32_t *out_cur_image)
{
	const struct xrt_layer_projection_view_data *vd = NULL;
	const struct xrt_layer_depth_data *dvd = NULL;

	if (data->type == XRT_LAYER_STEREO_PROJECTION) {
		view_index_to_projection_data(view_index, data, &vd);
	} else {
		view_index_to_depth_data(view_index, data, &vd, &dvd);
	}

	uint32_t sc_array_index = is_view_index_right(view_index) ? 1 : 0;
	uint32_t array_index = vd->sub.array_index;
	const struct comp_swapchain_image *image = &layer->sc_array[sc_array_index]->images[vd->sub.image_index];

	// Color
	src_samplers[cur_image] = clamp_to_border_black;
	src_image_views[cur_image] = get_image_view(image, data->flags, array_index);
	ubo_data->images_samplers[cur_layer + 0].images[0] = cur_image++;

	// Depth
	if (data->type == XRT_LAYER_STEREO_PROJECTION_DEPTH) {
		uint32_t d_array_index = dvd->sub.array_index;
		const struct comp_swapchain_image *d_image =
		    &layer->sc_array[sc_array_index + 2]->images[dvd->sub.image_index];

		src_samplers[cur_image] = clamp_to_edge; // Edge to keep depth stable at edges.
		src_image_views[cur_image] = get_image_view(d_image, data->flags, d_array_index);
		ubo_data->images_samplers[cur_layer + 0].images[1] = cur_image++;
	}

	set_post_transform_rect(                    //
	    data,                                   // data
	    &vd->sub.norm_rect,                     // src_norm_rect
	    false,                                  // invert_flip
	    &ubo_data->post_transforms[cur_layer]); // out_norm_rect

	// unused if timewarp is off
	if (do_timewarp) {
		render_calc_time_warp_matrix(          //
		    &vd->pose,                         //
		    &vd->fov,                          //
		    world_pose,                        //
		    &ubo_data->transforms[cur_layer]); //
	}

	*out_cur_image = cur_image;
}

static inline void
do_cs_quad_layer(const struct xrt_layer_data *data,
                 const struct comp_layer *layer,
                 const struct xrt_matrix_4x4 *eye_view_mat,
                 const struct xrt_matrix_4x4 *world_view_mat,
                 uint32_t view_index,
                 uint32_t cur_layer,
                 uint32_t cur_image,
                 VkSampler clamp_to_edge,
                 VkSampler clamp_to_border_black,
                 VkSampler src_samplers[RENDER_MAX_IMAGES],
                 VkImageView src_image_views[RENDER_MAX_IMAGES],
                 struct render_compute_layer_ubo_data *ubo_data,
                 uint32_t *out_cur_image)
{
	const struct xrt_layer_quad_data *q = &data->quad;

	const struct comp_swapchain_image *image = &layer->sc_array[0]->images[q->sub.image_index];
	uint32_t array_index = q->sub.array_index;

	// Image to use.
	src_samplers[cur_image] = clamp_to_edge;
	src_image_views[cur_image] = get_image_view(image, data->flags, array_index);

	// Set the normalized post transform values.
	struct xrt_normalized_rect post_transform = XRT_STRUCT_INIT;
	set_post_transform_rect( //
	    data,                // data
	    &q->sub.norm_rect,   // src_norm_rect
	    true,                // invert_flip
	    &post_transform);    // out_norm_rect

	// Is this layer viewspace or not.
	const struct xrt_matrix_4x4 *view_mat = is_layer_view_space(data) ? eye_view_mat : world_view_mat;

	// Transform quad pose into view space.
	struct xrt_vec3 quad_position = XRT_STRUCT_INIT;
	math_matrix_4x4_transform_vec3(view_mat, &data->quad.pose.position, &quad_position);

	// neutral quad layer faces +z, towards the user
	struct xrt_vec3 normal = (struct xrt_vec3){.x = 0, .y = 0, .z = 1};

	// rotation of the quad normal in world space
	struct xrt_quat rotation = data->quad.pose.orientation;
	math_quat_rotate_vec3(&rotation, &normal, &normal);

	/*
	 * normal is a vector that originates on the plane, not on the origin.
	 * Instead of using the inverse quad transform to transform it into view space we can
	 * simply add up vectors:
	 *
	 * combined_normal [in world space] = plane_origin [in world space] + normal [in plane
	 * space] [with plane in world space]
	 *
	 * Then combined_normal can be transformed to view space via view matrix and a new
	 * normal_view_space retrieved:
	 *
	 * normal_view_space = combined_normal [in view space] - plane_origin [in view space]
	 */
	struct xrt_vec3 normal_view_space = normal;
	math_vec3_accum(&data->quad.pose.position, &normal_view_space);
	math_matrix_4x4_transform_vec3(view_mat, &normal_view_space, &normal_view_space);
	math_vec3_subtract(&quad_position, &normal_view_space);

	struct xrt_vec3 scale = {1.f, 1.f, 1.f};
	struct xrt_matrix_4x4 plane_transform_view_space, inverse_quad_transform;
	math_matrix_4x4_model(&data->quad.pose, &scale, &plane_transform_view_space);
	math_matrix_4x4_multiply(view_mat, &plane_transform_view_space, &plane_transform_view_space);
	math_matrix_4x4_inverse(&plane_transform_view_space, &inverse_quad_transform);

	// Write all of the UBO data.
	ubo_data->post_transforms[cur_layer] = post_transform;
	ubo_data->quad_extent[cur_layer].val = data->quad.size;
	ubo_data->quad_position[cur_layer].val = quad_position;
	ubo_data->quad_normal[cur_layer].val = normal_view_space;
	ubo_data->inverse_quad_transform[cur_layer] = inverse_quad_transform;
	ubo_data->images_samplers[cur_layer].images[0] = cur_image;
	cur_image++;

	*out_cur_image = cur_image;
}


static inline void
do_cs_cylinder_layer(const struct xrt_layer_data *data,
                     const struct comp_layer *layer,
                     const struct xrt_matrix_4x4 *eye_view_mat,
                     const struct xrt_matrix_4x4 *world_view_mat,
                     uint32_t view_index,
                     uint32_t cur_layer,
                     uint32_t cur_image,
                     VkSampler clamp_to_edge,
                     VkSampler clamp_to_border_black,
                     VkSampler src_samplers[RENDER_MAX_IMAGES],
                     VkImageView src_image_views[RENDER_MAX_IMAGES],
                     struct render_compute_layer_ubo_data *ubo_data,
                     uint32_t *out_cur_image)
{
	const struct xrt_layer_cylinder_data *c = &data->cylinder;

	const struct comp_swapchain_image *image = &layer->sc_array[0]->images[c->sub.image_index];
	uint32_t array_index = c->sub.array_index;

	// Image to use.
	src_samplers[cur_image] = clamp_to_edge;
	src_image_views[cur_image] = get_image_view(image, data->flags, array_index);

	// Used for Subimage and OpenGL flip.
	set_post_transform_rect(                    //
	    data,                                   // data
	    &c->sub.norm_rect,                      // src_norm_rect
	    false,                                  // invert_flip
	    &ubo_data->post_transforms[cur_layer]); // out_norm_rect

	ubo_data->cylinder_data[cur_layer].central_angle = c->central_angle;
	ubo_data->cylinder_data[cur_layer].aspect_ratio = c->aspect_ratio;

	struct xrt_vec3 scale = {1.f, 1.f, 1.f};

	struct xrt_matrix_4x4 model;
	math_matrix_4x4_model(&c->pose, &scale, &model);

	struct xrt_matrix_4x4 model_inv;
	math_matrix_4x4_inverse(&model, &model_inv);

	const struct xrt_matrix_4x4 *v = is_layer_view_space(data) ? eye_view_mat : world_view_mat;

	struct xrt_matrix_4x4 v_inv;
	math_matrix_4x4_inverse(v, &v_inv);

	math_matrix_4x4_multiply(&model_inv, &v_inv, &ubo_data->mv_inverse[cur_layer]);

	// Simplifies the shader.
	if (c->radius >= INFINITY) {
		ubo_data->cylinder_data[cur_layer].radius = 0.f;
	} else {
		ubo_data->cylinder_data[cur_layer].radius = c->radius;
	}

	ubo_data->cylinder_data[cur_layer].central_angle = c->central_angle;
	ubo_data->cylinder_data[cur_layer].aspect_ratio = c->aspect_ratio;

	ubo_data->images_samplers[cur_layer].images[0] = cur_image;
	cur_image++;

	*out_cur_image = cur_image;
}

/*
 *
 * Compute distortion helpers.
 *
 */

static void
do_cs_distortion_for_scratch(struct render_compute *crc,
                             struct render_scratch_images *rsi,
                             VkImage target_image,
                             VkImageView target_image_view,
                             const struct render_viewport_data views[2])
{
	VkSampler sampler = crc->r->samplers.clamp_to_border_black;

	VkImageView src_image_views[2] = {
	    rsi->color[0].srgb_view, // Read with gamma curve.
	    rsi->color[1].srgb_view,
	};
	VkSampler src_samplers[2] = {sampler, sampler};

	struct xrt_normalized_rect src_norm_rects[2] = {
	    {.x = 0.0f, .y = 0.0f, .w = 1.0f, .h = 1.0f},
	    {.x = 0.0f, .y = 0.0f, .w = 1.0f, .h = 1.0f},
	};

	render_compute_projection( //
	    crc,                   //
	    src_samplers,          //
	    src_image_views,       //
	    src_norm_rects,        //
	    target_image,          //
	    target_image_view,     //
	    views                  //
	);
}

static void
do_cs_distortion_for_layer(struct render_compute *crc,
                           const struct xrt_pose world_poses[2],
                           const struct comp_layer *layer,
                           const struct xrt_layer_projection_view_data *lvd,
                           const struct xrt_layer_projection_view_data *rvd,
                           VkImage target_image,
                           VkImageView target_image_view,
                           const struct render_viewport_data views[2],
                           bool do_timewarp)
{
	const struct xrt_layer_data *data = &layer->data;
	uint32_t left_array_index = lvd->sub.array_index;
	uint32_t right_array_index = rvd->sub.array_index;
	const struct comp_swapchain_image *left = &layer->sc_array[0]->images[lvd->sub.image_index];
	const struct comp_swapchain_image *right = &layer->sc_array[1]->images[rvd->sub.image_index];

	VkSampler clamp_to_border_black = crc->r->samplers.clamp_to_border_black;
	VkSampler src_samplers[2] = {
	    clamp_to_border_black,
	    clamp_to_border_black,
	};

	VkImageView src_image_views[2] = {
	    get_image_view(left, data->flags, left_array_index),
	    get_image_view(right, data->flags, right_array_index),
	};

	struct xrt_normalized_rect src_norm_rects[2] = {lvd->sub.norm_rect, rvd->sub.norm_rect};
	if (data->flip_y) {
		src_norm_rects[0].h = -src_norm_rects[0].h;
		src_norm_rects[0].y = 1 + src_norm_rects[0].y;
		src_norm_rects[1].h = -src_norm_rects[1].h;
		src_norm_rects[1].y = 1 + src_norm_rects[1].y;
	}

	if (!do_timewarp) {
		render_compute_projection( //
		    crc,                   //
		    src_samplers,          //
		    src_image_views,       //
		    src_norm_rects,        //
		    target_image,          //
		    target_image_view,     //
		    views);                //
	} else {
		struct xrt_pose src_poses[2] = {
		    lvd->pose,
		    rvd->pose,
		};

		struct xrt_fov src_fovs[2] = {
		    lvd->fov,
		    rvd->fov,
		};

		render_compute_projection_timewarp( //
		    crc,                            //
		    src_samplers,                   //
		    src_image_views,                //
		    src_norm_rects,                 //
		    src_poses,                      //
		    src_fovs,                       //
		    world_poses,                    //
		    target_image,                   //
		    target_image_view,              //
		    views);                         //
	}
}


/*
 *
 * 'Exported' compute helpers.
 *
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
                     bool do_timewarp)
{
	VkSampler clamp_to_edge = crc->r->samplers.clamp_to_edge;
	VkSampler clamp_to_border_black = crc->r->samplers.clamp_to_border_black;

	// Not the transform of the views, but the inverse: actual view matrices.
	struct xrt_matrix_4x4 world_view_mat, eye_view_mat;
	math_matrix_4x4_view_from_pose(world_pose, &world_view_mat);
	math_matrix_4x4_view_from_pose(eye_pose, &eye_view_mat);

	struct render_buffer *ubo = &crc->r->compute.layer.ubos[view_index];
	struct render_compute_layer_ubo_data *ubo_data = ubo->mapped;

	// Tightly pack layers in data struct.
	uint32_t cur_layer = 0;

	// Tightly pack color and optional depth images.
	uint32_t cur_image = 0;
	VkSampler src_samplers[RENDER_MAX_IMAGES];
	VkImageView src_image_views[RENDER_MAX_IMAGES];

	ubo_data->view = *target_view;
	ubo_data->pre_transform = *pre_transform;

	for (uint32_t c_layer_i = 0; c_layer_i < layer_count; c_layer_i++) {
		const struct comp_layer *layer = &layers[c_layer_i];
		const struct xrt_layer_data *data = &layer->data;

		if (!is_layer_view_visible(data, view_index)) {
			continue;
		}

		/*!
		 * Stop compositing layers if device's sampled image limit is
		 * reached. For most hardware this isn't a problem, most have
		 * well over 32 max samplers. But notably the RPi4 only have 16
		 * which is a limit we may run into. But if you got 16+ layers
		 * on a RPi4 you have more problems then max samplers.
		 */
		uint32_t required_image_samplers;
		switch (data->type) {
		case XRT_LAYER_CYLINDER: required_image_samplers = 1; break;
		case XRT_LAYER_EQUIRECT2: required_image_samplers = 1; break;
		case XRT_LAYER_STEREO_PROJECTION: required_image_samplers = 1; break;
		case XRT_LAYER_STEREO_PROJECTION_DEPTH: required_image_samplers = 2; break;
		case XRT_LAYER_QUAD: required_image_samplers = 1; break;
		default:
			VK_ERROR(crc->r->vk, "Skipping layer #%u, unknown type: %u", c_layer_i, data->type);
			continue; // Skip this layer if don't know about it.
		}

		//! Exit loop if shader cannot receive more image samplers
		if (cur_image + required_image_samplers > crc->r->compute.layer.image_array_size) {
			break;
		}

		switch (data->type) {
		case XRT_LAYER_CYLINDER:
			do_cs_cylinder_layer(      //
			    data,                  // data
			    layer,                 // layer
			    &eye_view_mat,         // eye_view_mat
			    &world_view_mat,       // world_view_mat
			    view_index,            // view_index
			    cur_layer,             // cur_layer
			    cur_image,             // cur_image
			    clamp_to_edge,         // clamp_to_edge
			    clamp_to_border_black, // clamp_to_border_black
			    src_samplers,          // src_samplers
			    src_image_views,       // src_image_views
			    ubo_data,              // ubo_data
			    &cur_image);           // out_cur_image
			break;
		case XRT_LAYER_EQUIRECT2:
			do_cs_equirect2_layer(     //
			    data,                  // data
			    layer,                 // layer
			    &eye_view_mat,         // eye_view_mat
			    &world_view_mat,       // world_view_mat
			    view_index,            // view_index
			    cur_layer,             // cur_layer
			    cur_image,             // cur_image
			    clamp_to_edge,         // clamp_to_edge
			    clamp_to_border_black, // clamp_to_border_black
			    src_samplers,          // src_samplers
			    src_image_views,       // src_image_views
			    ubo_data,              // ubo_data
			    &cur_image);           // out_cur_image
			break;
		case XRT_LAYER_STEREO_PROJECTION_DEPTH:
		case XRT_LAYER_STEREO_PROJECTION: {
			do_cs_projection_layer(    //
			    data,                  // data
			    layer,                 // layer
			    world_pose,            // world_pose
			    view_index,            // view_index
			    cur_layer,             // cur_layer
			    cur_image,             // cur_image
			    clamp_to_edge,         // clamp_to_edge
			    clamp_to_border_black, // clamp_to_border_black
			    src_samplers,          // src_samplers
			    src_image_views,       // src_image_views
			    ubo_data,              // ubo_data
			    do_timewarp,           // do_timewarp
			    &cur_image);           // out_cur_image
		} break;
		case XRT_LAYER_QUAD: {
			do_cs_quad_layer(          //
			    data,                  // data
			    layer,                 // layer
			    &eye_view_mat,         // eye_view_mat
			    &world_view_mat,       // world_view_mat
			    view_index,            // view_index
			    cur_layer,             // cur_layer
			    cur_image,             // cur_image
			    clamp_to_edge,         // clamp_to_edge
			    clamp_to_border_black, // clamp_to_border_black
			    src_samplers,          // src_samplers
			    src_image_views,       // src_image_views
			    ubo_data,              // ubo_data
			    &cur_image);           // out_cur_image
		} break;
		default:
			// Should not get here!
			assert(false);
			VK_ERROR(crc->r->vk, "Should not get here!");
			continue;
		}

		ubo_data->layer_type[cur_layer].val = data->type;
		ubo_data->layer_type[cur_layer].unpremultiplied = is_layer_unpremultiplied(data);

		// Finally okay to increment the current layer.
		cur_layer++;
	}

	// Set the number of layers.
	ubo_data->layer_count.value = cur_layer;

	for (uint32_t i = cur_layer; i < RENDER_MAX_LAYERS; i++) {
		ubo_data->layer_type[i].val = UINT32_MAX;
	}

	//! @todo: If Vulkan 1.2, use VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT and skip this
	while (cur_image < crc->r->compute.layer.image_array_size) {
		src_samplers[cur_image] = clamp_to_edge;
		src_image_views[cur_image] = crc->r->mock.color.image_view;
		cur_image++;
	}

	VkDescriptorSet descriptor_set = crc->layer_descriptor_sets[view_index];

	render_compute_layers( //
	    crc,               //
	    descriptor_set,    //
	    ubo->buffer,       //
	    src_samplers,      //
	    src_image_views,   //
	    cur_image,         //
	    target_image_view, //
	    target_view,       //
	    do_timewarp);      //
}

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
                             bool do_timewarp)
{
	VkImageSubresourceRange first_color_level_subresource_range = {
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = 1,
	    .baseArrayLayer = 0,
	    .layerCount = 1,
	};

	vk_cmd_image_barrier_gpu_locked(          //
	    crc->r->vk,                           //
	    crc->r->cmd,                          //
	    target_images[0],                     //
	    0,                                    //
	    VK_ACCESS_SHADER_WRITE_BIT,           //
	    VK_IMAGE_LAYOUT_UNDEFINED,            //
	    VK_IMAGE_LAYOUT_GENERAL,              //
	    first_color_level_subresource_range); //

	if (target_images[0] != target_images[1]) {
		vk_cmd_image_barrier_gpu_locked(          //
		    crc->r->vk,                           //
		    crc->r->cmd,                          //
		    target_images[1],                     //
		    0,                                    //
		    VK_ACCESS_SHADER_WRITE_BIT,           //
		    VK_IMAGE_LAYOUT_UNDEFINED,            //
		    VK_IMAGE_LAYOUT_GENERAL,              //
		    first_color_level_subresource_range); //
	}

	for (uint32_t view_index = 0; view_index < 2; view_index++) {
		comp_render_cs_layer(               //
		    crc,                            //
		    view_index,                     //
		    layers,                         //
		    layer_count,                    //
		    &pre_transforms[view_index],    //
		    &world_poses[view_index],       //
		    &eye_poses[view_index],         //
		    target_images[view_index],      //
		    target_image_views[view_index], //
		    &target_views[view_index],      //
		    do_timewarp);                   //
	}

	vk_cmd_image_barrier_locked(              //
	    crc->r->vk,                           //
	    crc->r->cmd,                          //
	    target_images[0],                     //
	    VK_ACCESS_SHADER_WRITE_BIT,           //
	    VK_ACCESS_MEMORY_READ_BIT,            //
	    VK_IMAGE_LAYOUT_GENERAL,              //
	    transition_to,                        //
	    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, //
	    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,    //
	    first_color_level_subresource_range); //

	if (target_images[0] != target_images[1]) {
		vk_cmd_image_barrier_locked(              //
		    crc->r->vk,                           //
		    crc->r->cmd,                          //
		    target_images[1],                     //
		    VK_ACCESS_SHADER_WRITE_BIT,           //
		    VK_ACCESS_MEMORY_READ_BIT,            //
		    VK_IMAGE_LAYOUT_GENERAL,              //
		    transition_to,                        //
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, //
		    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,    //
		    first_color_level_subresource_range); //
	}
}

void
comp_render_cs_stereo_layers_to_scratch(struct render_compute *crc,
                                        const struct xrt_normalized_rect pre_transforms[2],
                                        struct xrt_pose world_poses[2],
                                        struct xrt_pose eye_poses[2],
                                        const struct comp_layer *layers,
                                        const uint32_t layer_count,
                                        struct render_scratch_images *rsi,
                                        VkImageLayout transition_to,
                                        bool do_timewarp)
{
	struct render_viewport_data target_views[2] = {
	    {.w = rsi->extent.width, .h = rsi->extent.height},
	    {.w = rsi->extent.width, .h = rsi->extent.height},
	};

	VkImage target_images[2] = {
	    rsi->color[0].image,
	    rsi->color[1].image,
	};

	VkImageView target_image_views[2] = {
	    rsi->color[0].unorm_view, // Have to write in linear
	    rsi->color[1].unorm_view,
	};

	comp_render_cs_stereo_layers( //
	    crc,                      // crc
	    layers,                   // layers
	    layer_count,              // layer_count
	    pre_transforms,           // pre_transforms
	    world_poses,              // world_poses
	    eye_poses,                // eye_poses
	    target_images,            // target_images
	    target_image_views,       // target_image_views
	    target_views,             // target_views
	    transition_to,            // transition_to
	    do_timewarp);             // do_timewarp
}

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
                        bool do_timewarp)
{
	assert(!fast_path || layer_count > 0);

	// We make an assumption that we will be using the distortion code.
	const struct xrt_normalized_rect pre_transforms[2] = {
	    crc->r->distortion.uv_to_tanangle[0],
	    crc->r->distortion.uv_to_tanangle[1],
	};

	// We want to read from the images afterwards.
	VkImageLayout transition_to = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (fast_path && layers[0].data.type == XRT_LAYER_STEREO_PROJECTION) {
		int i = 0;
		const struct comp_layer *layer = &layers[i];
		const struct xrt_layer_stereo_projection_data *stereo = &layer->data.stereo;
		const struct xrt_layer_projection_view_data *lvd = &stereo->l;
		const struct xrt_layer_projection_view_data *rvd = &stereo->r;

		do_cs_distortion_for_layer( //
		    crc,                    // crc
		    world_poses,            // world_poses
		    layer,                  // layer
		    lvd,                    // lvd
		    rvd,                    // rvd
		    target_image,           // target_image
		    target_image_view,      // target_image_view
		    views,                  // views
		    do_timewarp);           // do_timewarp
	} else if (fast_path && layers[0].data.type == XRT_LAYER_STEREO_PROJECTION_DEPTH) {
		int i = 0;
		const struct comp_layer *layer = &layers[i];
		const struct xrt_layer_stereo_projection_depth_data *stereo = &layer->data.stereo_depth;
		const struct xrt_layer_projection_view_data *lvd = &stereo->l;
		const struct xrt_layer_projection_view_data *rvd = &stereo->r;

		do_cs_distortion_for_layer( //
		    crc,                    // crc
		    world_poses,            // world_poses
		    layer,                  // layer
		    lvd,                    // lvd
		    rvd,                    // rvd
		    target_image,           // target_image
		    target_image_view,      // target_image_view
		    views,                  // views
		    do_timewarp);           // do_timewarp
	} else if (layer_count > 0) {
		comp_render_cs_stereo_layers_to_scratch( //
		    crc,                                 //
		    pre_transforms,                      //
		    world_poses,                         //
		    eye_poses,                           //
		    layers,                              //
		    layer_count,                         //
		    rsi,                                 //
		    transition_to,                       //
		    do_timewarp);                        //

		do_cs_distortion_for_scratch( //
		    crc,                      //
		    rsi,                      //
		    target_image,             //
		    target_image_view,        //
		    views);                   //
	} else {
		render_compute_clear(  //
		    crc,               //
		    target_image,      //
		    target_image_view, //
		    views);            //
	}
}
