// Copyright 2023-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Compositor gfx rendering code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_util
 */

#include "xrt/xrt_compositor.h"

#include "math/m_api.h"
#include "math/m_mathinclude.h"

#include "util/u_misc.h"
#include "util/u_trace_marker.h"

#include "vk/vk_helpers.h"

#include "render/render_interface.h"

#include "util/comp_render.h"
#include "util/comp_render_helpers.h"


/*
 *
 * Internal structs.
 *
 */

/*
 * Internal per-view for the layer shashing render step.
 */
struct gfx_layer_view_state
{
	// Filled out descriptor sets.
	VkDescriptorSet descriptor_sets[RENDER_MAX_LAYERS];

	// The type of layer.
	enum xrt_layer_type types[RENDER_MAX_LAYERS];
	// Is the alpha premultipled, false means unpremultiplied.
	bool premultiplied_alphas[RENDER_MAX_LAYERS];

	// To go to this view's tangent lengths.
	struct xrt_normalized_rect to_tangent;

	// Number of layers filed in.
	uint32_t layer_count;

	// Full rotation and translation VP matrix, in world space.
	struct xrt_matrix_4x4 world_vp_full;
	// Full rotation and translation VP matrix, in view space.
	struct xrt_matrix_4x4 eye_vp_full;

	// Full rotation and translation inverse V matrix, in world space.
	struct xrt_matrix_4x4 world_v_inv_full;
	// Full rotation and translation inverse V matrix, in view space.
	struct xrt_matrix_4x4 eye_v_inv_full;

	// Only rotation and translation VP matrix, in world space.
	struct xrt_matrix_4x4 world_vp_rot_only;
	// Only rotation and translation VP matrix, in view space.
	struct xrt_matrix_4x4 eye_vp_rot_only;
};

/*
 * Internal state for the layer squashing render step.
 */
struct gfx_layer_state
{
	struct gfx_layer_view_state views[2];
};

/*
 * Internal state for the mesh rendering step.
 */
struct gfx_mesh_state
{
	VkDescriptorSet descriptor_sets[2];
};

/*
 * Per-view input data for the mesh rendering step.
 */
struct gfx_mesh_view_data
{
	struct xrt_pose src_pose;
	struct xrt_fov src_fov;
	struct xrt_normalized_rect src_norm_rect;
	VkSampler src_sampler;
	VkImageView src_image_view;
};

/*
 * Input data for the mesh rendering step,
 * combined with comp_render_dispatch_data.
 */
struct gfx_mesh_data
{
	struct gfx_mesh_view_data views[2];
};


/*
 *
 * Static data.
 *
 */

static const VkClearColorValue background_color_idle = {
    .float32 = {0.1f, 0.1f, 0.1f, 1.0f},
};

static const VkClearColorValue background_color_active = {
    .float32 = {0.0f, 0.0f, 0.0f, 1.0f},
};


/*
 *
 * Input builder functions.
 *
 */

inline static void
gfx_mesh_add_view(struct gfx_mesh_data *md,
                  uint32_t view_index,
                  const struct xrt_pose *src_pose,
                  const struct xrt_fov *src_fov,
                  const struct xrt_normalized_rect *src_norm_rect,
                  VkSampler src_sampler,
                  VkImageView src_image_view)
{
	md->views[view_index].src_pose = *src_pose;
	md->views[view_index].src_fov = *src_fov;
	md->views[view_index].src_norm_rect = *src_norm_rect;
	md->views[view_index].src_sampler = src_sampler;
	md->views[view_index].src_image_view = src_image_view;
}



/*
 *
 * Model view projection helper functions.
 *
 */

static inline void
calc_mvp_full(struct gfx_layer_view_state *state,
              const struct xrt_layer_data *layer_data,
              const struct xrt_pose *pose,
              const struct xrt_vec3 *scale,
              struct xrt_matrix_4x4 *result)
{
	struct xrt_matrix_4x4 model;
	math_matrix_4x4_model(pose, scale, &model);

	if (is_layer_view_space(layer_data)) {
		math_matrix_4x4_multiply(&state->eye_vp_full, &model, result);
	} else {
		math_matrix_4x4_multiply(&state->world_vp_full, &model, result);
	}
}

static inline void
calc_mv_inv_full(struct gfx_layer_view_state *state,
                 const struct xrt_layer_data *layer_data,
                 const struct xrt_pose *pose,
                 const struct xrt_vec3 *scale,
                 struct xrt_matrix_4x4 *result)
{
	struct xrt_matrix_4x4 model;
	math_matrix_4x4_model(pose, scale, &model);

	struct xrt_matrix_4x4 model_inv;
	math_matrix_4x4_inverse(&model, &model_inv);

	struct xrt_matrix_4x4 *v;
	if (is_layer_view_space(layer_data)) {
		v = &state->eye_v_inv_full;
	} else {
		v = &state->world_v_inv_full;
	}

	math_matrix_4x4_multiply(&model_inv, v, result);
}

static inline void
calc_mvp_rot_only(struct gfx_layer_view_state *state,
                  const struct xrt_layer_data *data,
                  const struct xrt_pose *pose,
                  const struct xrt_vec3 *scale,
                  struct xrt_matrix_4x4 *result)
{
	struct xrt_matrix_4x4 model;
	struct xrt_pose rot_only = {
	    .orientation = pose->orientation,
	    .position = XRT_VEC3_ZERO,
	};
	math_matrix_4x4_model(&rot_only, scale, &model);

	if (is_layer_view_space(data)) {
		math_matrix_4x4_multiply(&state->eye_vp_rot_only, &model, result);
	} else {
		math_matrix_4x4_multiply(&state->world_vp_rot_only, &model, result);
	}
}


/*
 *
 * Graphics layer data builders.
 *
 */

static inline void
add_layer(struct gfx_layer_view_state *state, const struct xrt_layer_data *data, VkDescriptorSet descriptor_set)
{
	uint32_t cur_layer = state->layer_count++;
	state->descriptor_sets[cur_layer] = descriptor_set;
	state->types[cur_layer] = data->type;
	state->premultiplied_alphas[cur_layer] = !is_layer_unpremultiplied(data);
}

static VkResult
do_cylinder_layer(struct render_gfx *rr,
                  const struct comp_layer *layer,
                  uint32_t view_index,
                  VkSampler clamp_to_edge,
                  VkSampler clamp_to_border_black,
                  struct gfx_layer_view_state *state)
{
	const struct xrt_layer_data *layer_data = &layer->data;
	const struct xrt_layer_cylinder_data *c = &layer_data->cylinder;
	struct vk_bundle *vk = rr->r->vk;
	VkResult ret;

	const uint32_t array_index = c->sub.array_index;
	const struct comp_swapchain_image *image = &layer->sc_array[0]->images[c->sub.image_index];

	// Color
	VkSampler src_sampler = clamp_to_edge; // WIP: Is this correct?
	VkImageView src_image_view = get_image_view(image, layer_data->flags, array_index);

	// Fully initialised below.
	struct render_gfx_layer_cylinder_data data;

	// Used for Subimage and OpenGL flip.
	set_post_transform_rect(   //
	    layer_data,            // data
	    &c->sub.norm_rect,     // src_norm_rect
	    false,                 // invert_flip
	    &data.post_transform); // out_norm_rect

	// Shared scale for all paths.
	struct xrt_vec3 scale = {1, 1, 1};

	// Handle infinite radius.
	if (c->radius == 0 || c->radius == INFINITY) {
		// Use rotation only to center the cylinder on the eye.
		calc_mvp_rot_only(state, layer_data, &c->pose, &scale, &data.mvp);
		data.radius = 1.0; // Fixed radius at one.
		data.central_angle = c->central_angle;
		data.aspect_ratio = c->aspect_ratio;
	} else {
		calc_mvp_full(state, layer_data, &c->pose, &scale, &data.mvp);
		data.radius = c->radius;
		data.central_angle = c->central_angle;
		data.aspect_ratio = c->aspect_ratio;
	}

	// Can fail if we have too many layers.
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	ret = render_gfx_layer_cylinder_alloc_and_write( //
	    rr,                                          // rr
	    &data,                                       // data
	    src_sampler,                                 // src_sampler
	    src_image_view,                              // src_image_view
	    &descriptor_set);                            // out_descriptor_set
	VK_CHK_AND_RET(ret, "render_gfx_layer_quad_alloc_and_write");

	VK_NAME_DESCRIPTOR_SET(vk, descriptor_set, "render_gfx layer quad descriptor set");

	add_layer(state, layer_data, descriptor_set);

	return VK_SUCCESS;
}

static VkResult
do_equirect2_layer(struct render_gfx *rr,
                   const struct comp_layer *layer,
                   uint32_t view_index,
                   VkSampler clamp_to_edge,
                   VkSampler clamp_to_border_black,
                   struct gfx_layer_view_state *state)
{
	const struct xrt_layer_data *layer_data = &layer->data;
	const struct xrt_layer_equirect2_data *eq2 = &layer_data->equirect2;
	struct vk_bundle *vk = rr->r->vk;
	VkResult ret;

	const uint32_t array_index = eq2->sub.array_index;
	const struct comp_swapchain_image *image = &layer->sc_array[0]->images[eq2->sub.image_index];

	// Color
	VkSampler src_sampler = clamp_to_edge;
	VkImageView src_image_view = get_image_view(image, layer_data->flags, array_index);

	// Fully initialised below.
	struct render_gfx_layer_equirect2_data data;

	// Used for Subimage and OpenGL flip.
	set_post_transform_rect(   //
	    layer_data,            // data
	    &eq2->sub.norm_rect,   // src_norm_rect
	    false,                 // invert_flip
	    &data.post_transform); // out_norm_rect

	struct xrt_vec3 scale = {1.f, 1.f, 1.f};
	calc_mv_inv_full(state, layer_data, &eq2->pose, &scale, &data.mv_inverse);

	// Make it possible to go tangent lengths.
	data.to_tangent = state->to_tangent;

	// Simplifies the shader.
	if (eq2->radius >= INFINITY) {
		data.radius = 0.0;
	} else {
		data.radius = eq2->radius;
	}

	data.central_horizontal_angle = eq2->central_horizontal_angle;
	data.upper_vertical_angle = eq2->upper_vertical_angle;
	data.lower_vertical_angle = eq2->lower_vertical_angle;

	// Can fail if we have too many layers.
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	ret = render_gfx_layer_equirect2_alloc_and_write( //
	    rr,                                           // rr
	    &data,                                        // data
	    src_sampler,                                  // src_sampler
	    src_image_view,                               // src_image_view
	    &descriptor_set);                             // out_descriptor_set
	VK_CHK_AND_RET(ret, "render_gfx_layer_quad_alloc_and_write");

	VK_NAME_DESCRIPTOR_SET(vk, descriptor_set, "render_gfx layer quad descriptor set");

	add_layer(state, layer_data, descriptor_set);

	return VK_SUCCESS;
}

static VkResult
do_projection_layer(struct render_gfx *rr,
                    const struct comp_layer *layer,
                    uint32_t view_index,
                    VkSampler clamp_to_edge,
                    VkSampler clamp_to_border_black,
                    struct gfx_layer_view_state *state)
{
	const struct xrt_layer_data *layer_data = &layer->data;
	const struct xrt_layer_projection_view_data *vd = NULL;
	const struct xrt_layer_depth_data *dvd = NULL;
	struct vk_bundle *vk = rr->r->vk;
	VkResult ret;

	if (layer_data->type == XRT_LAYER_STEREO_PROJECTION) {
		view_index_to_projection_data(view_index, layer_data, &vd);
	} else {
		view_index_to_depth_data(view_index, layer_data, &vd, &dvd);
	}

	uint32_t sc_array_index = is_view_index_right(view_index) ? 1 : 0;
	uint32_t array_index = vd->sub.array_index;
	const struct comp_swapchain_image *image = &layer->sc_array[sc_array_index]->images[vd->sub.image_index];

	// Color
	VkSampler src_sampler = clamp_to_border_black;
	VkImageView src_image_view = get_image_view(image, layer_data->flags, array_index);

	// Fully initialised below.
	struct render_gfx_layer_projection_data data;

	// Used for Subimage and OpenGL flip.
	set_post_transform_rect(   //
	    layer_data,            // data
	    &vd->sub.norm_rect,    // src_norm_rect
	    false,                 // invert_flip
	    &data.post_transform); // out_norm_rect

	// Used to go from UV to tangent space.
	render_calc_uv_to_tangent_lengths_rect(&vd->fov, &data.to_tanget);

	// Create MVP matrix, rotation only so we get 3dof timewarp.
	struct xrt_vec3 scale = {1, 1, 1};
	calc_mvp_rot_only(state, layer_data, &vd->pose, &scale, &data.mvp);

	// Can fail if we have too many layers.
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	ret = render_gfx_layer_projection_alloc_and_write( //
	    rr,                                            // rr
	    &data,                                         // data
	    src_sampler,                                   // src_sampler
	    src_image_view,                                // src_image_view
	    &descriptor_set);                              // out_descriptor_set
	VK_CHK_AND_RET(ret, "render_gfx_layer_projection_alloc_and_write");

	VK_NAME_DESCRIPTOR_SET(vk, descriptor_set, "render_gfx layer quad descriptor set");

	add_layer(state, layer_data, descriptor_set);

	return VK_SUCCESS;
}

static VkResult
do_quad_layer(struct render_gfx *rr,
              const struct comp_layer *layer,
              uint32_t view_index,
              VkSampler clamp_to_edge,
              VkSampler clamp_to_border_black,
              struct gfx_layer_view_state *state)
{
	const struct xrt_layer_data *layer_data = &layer->data;
	const struct xrt_layer_quad_data *q = &layer_data->quad;
	struct vk_bundle *vk = rr->r->vk;
	VkResult ret;

	const uint32_t array_index = q->sub.array_index;
	const struct comp_swapchain_image *image = &layer->sc_array[0]->images[q->sub.image_index];

	// Color
	VkSampler src_sampler = clamp_to_edge;
	VkImageView src_image_view = get_image_view(image, layer_data->flags, array_index);

	// Fully initialised below.
	struct render_gfx_layer_quad_data data;

	// Used for Subimage and OpenGL flip.
	set_post_transform_rect(   //
	    layer_data,            // data
	    &q->sub.norm_rect,     // src_norm_rect
	    false,                 // invert_flip
	    &data.post_transform); // out_norm_rect

	// Create MVP matrix, full 6dof mvp needed.
	struct xrt_vec3 scale = {q->size.x, q->size.y, 1};
	calc_mvp_full(state, layer_data, &q->pose, &scale, &data.mvp);

	// Can fail if we have too many layers.
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	ret = render_gfx_layer_quad_alloc_and_write( //
	    rr,                                      // rr
	    &data,                                   // data
	    src_sampler,                             // src_sampler
	    src_image_view,                          // src_image_view
	    &descriptor_set);                        // out_descriptor_set
	VK_CHK_AND_RET(ret, "render_gfx_layer_quad_alloc_and_write");

	VK_NAME_DESCRIPTOR_SET(vk, descriptor_set, "render_gfx layer quad descriptor set");

	add_layer(state, layer_data, descriptor_set);

	return VK_SUCCESS;
}

static void
do_layers(struct render_gfx *rr,
          const struct comp_layer *layers,
          uint32_t layer_count,
          const struct comp_render_dispatch_data *d)
{
	COMP_TRACE_MARKER();

	struct vk_bundle *vk = rr->r->vk;
	VkResult ret;

	// Hardcoded to stereo.
	struct gfx_layer_state ls = XRT_STRUCT_INIT;

	for (uint32_t view = 0; view < d->view_count; view++) {

		// Data for this view, convenience.
		const struct xrt_pose world_pose = d->views[view].world_pose;
		const struct xrt_pose eye_pose = d->views[view].eye_pose;
		const struct xrt_fov new_fov = d->views[view].fov;

		// Current state we are writing to.
		struct gfx_layer_view_state *state = &ls.views[view];

		// Used to go from UV to tangent space.
		render_calc_uv_to_tangent_lengths_rect(&new_fov, &state->to_tangent);

		// Projection
		struct xrt_matrix_4x4 p;
		math_matrix_4x4_projection_vulkan_infinite_reverse(&new_fov, 0.1, &p);

		// Reused view matrix.
		struct xrt_matrix_4x4 v;

		// World
		math_matrix_4x4_view_from_pose(&world_pose, &v);
		math_matrix_4x4_multiply(&p, &v, &state->world_vp_full);
		math_matrix_4x4_inverse(&v, &state->world_v_inv_full);

		struct xrt_pose world_rot_only = {world_pose.orientation, XRT_VEC3_ZERO};
		math_matrix_4x4_view_from_pose(&world_rot_only, &v);
		math_matrix_4x4_multiply(&p, &v, &state->world_vp_rot_only);

		// Eye
		math_matrix_4x4_view_from_pose(&eye_pose, &v);
		math_matrix_4x4_multiply(&p, &v, &state->eye_vp_full);
		math_matrix_4x4_inverse(&v, &state->eye_v_inv_full);

		struct xrt_pose eye_rot_only = {eye_pose.orientation, XRT_VEC3_ZERO};
		math_matrix_4x4_view_from_pose(&eye_rot_only, &v);
		math_matrix_4x4_multiply(&p, &v, &state->eye_vp_rot_only);
	}

	/*
	 * Reserve UBOs, create descriptor sets, and fill in any data a head of
	 * time, if we ever want to copy UBO data this lets us do that easily
	 * write a copy command before the other gfx commands.
	 */

	assert(layer_count <= RENDER_MAX_LAYERS && "Too many layers");

	VkSampler clamp_to_edge = rr->r->samplers.clamp_to_edge;
	VkSampler clamp_to_border_black = rr->r->samplers.clamp_to_border_black;

	for (uint32_t view = 0; view < d->view_count; view++) {

		// Source for data and written to as well, read and write.
		struct gfx_layer_view_state *state = &ls.views[view];

		for (uint32_t i = 0; i < layer_count; i++) {
			const struct xrt_layer_data *data = &layers[i].data;
			if (!is_layer_view_visible(data, view)) {
				continue;
			}

			switch (data->type) {
			case XRT_LAYER_CYLINDER:
				ret = do_cylinder_layer(   //
				    rr,                    // rr
				    &layers[i],            // layer
				    view,                  // view_index
				    clamp_to_edge,         // clamp_to_edge
				    clamp_to_border_black, // clamp_to_border_black
				    state);                // state
				VK_CHK_WITH_GOTO(ret, "do_cylinder_layer", err_layer);
				break;
			case XRT_LAYER_EQUIRECT2:
				ret = do_equirect2_layer(  //
				    rr,                    // rr
				    &layers[i],            // layer
				    view,                  // view_index
				    clamp_to_edge,         // clamp_to_edge
				    clamp_to_border_black, // clamp_to_border_black
				    state);                // state
				VK_CHK_WITH_GOTO(ret, "do_equirect2_layer", err_layer);
				break;
			case XRT_LAYER_STEREO_PROJECTION:
			case XRT_LAYER_STEREO_PROJECTION_DEPTH:
				ret = do_projection_layer( //
				    rr,                    // rr
				    &layers[i],            // layer
				    view,                  // view_index
				    clamp_to_edge,         // clamp_to_edge
				    clamp_to_border_black, // clamp_to_border_black
				    state);                // state
				VK_CHK_WITH_GOTO(ret, "do_projection_layer", err_layer);
				break;
			case XRT_LAYER_QUAD:
				ret = do_quad_layer(       //
				    rr,                    // rr
				    &layers[i],            // layer
				    view,                  // view_index
				    clamp_to_edge,         // clamp_to_edge
				    clamp_to_border_black, // clamp_to_border_black
				    state);                // state
				VK_CHK_WITH_GOTO(ret, "do_quad_layer", err_layer);
				break;
			default: break;
			}
		}
	}


	/*
	 * Do command writing here.
	 */

	const VkClearColorValue *color = layer_count == 0 ? &background_color_idle : &background_color_active;

	for (uint32_t view = 0; view < d->view_count; view++) {

		// Convenience.
		const struct render_viewport_data *viewport_data = &d->views[view].layer_viewport_data;

		render_gfx_begin_target(    //
		    rr,                     //
		    d->views[view].gfx.rtr, //
		    color);                 //

		render_gfx_begin_view( //
		    rr,                //
		    view,              // view_index
		    viewport_data);    // viewport_data

		// Only source for data here, read only.
		const struct gfx_layer_view_state *state = &ls.views[view];

		for (uint32_t i = 0; i < state->layer_count; i++) {
			switch (state->types[i]) {
			case XRT_LAYER_CYLINDER:
				render_gfx_layer_cylinder(          //
				    rr,                             //
				    state->premultiplied_alphas[i], //
				    state->descriptor_sets[i]);     //
				break;
			case XRT_LAYER_EQUIRECT2:
				render_gfx_layer_equirect2(         //
				    rr,                             //
				    state->premultiplied_alphas[i], //
				    state->descriptor_sets[i]);     //
				break;
			case XRT_LAYER_STEREO_PROJECTION:
			case XRT_LAYER_STEREO_PROJECTION_DEPTH:
				render_gfx_layer_projection(        //
				    rr,                             //
				    state->premultiplied_alphas[i], //
				    state->descriptor_sets[i]);     //
				break;
			case XRT_LAYER_QUAD:
				render_gfx_layer_quad(              //
				    rr,                             //
				    state->premultiplied_alphas[i], //
				    state->descriptor_sets[i]);     //
				break;
			default: break;
			}
		}

		render_gfx_end_view(rr);

		render_gfx_end_target(rr);
	}


	return;

err_layer:
	// Allocator reset at end of frame, nothing to clean up.
	VK_ERROR(vk, "Layer processing failed, that shouldn't happen!");
}


/*
 *
 * Graphics distortion helpers.
 *
 */

static void
do_mesh(struct render_gfx *rr,
        bool do_timewarp,
        const struct gfx_mesh_data *md,
        const struct comp_render_dispatch_data *d)
{
	struct vk_bundle *vk = rr->r->vk;
	VkResult ret;

	/*
	 * Reserve UBOs, create descriptor sets, and fill in any data a head of
	 * time, if we ever want to copy UBO data this lets us do that easily
	 * write a copy command before the other gfx commands.
	 */

	struct gfx_mesh_state ms = XRT_STRUCT_INIT;

	for (uint32_t i = 0; i < d->view_count; i++) {

		struct render_gfx_mesh_ubo_data data = {
		    .vertex_rot = d->views[i].gfx.vertex_rot,
		    .post_transform = md->views[i].src_norm_rect,
		};

		// Extra arguments for timewarp.
		if (do_timewarp) {
			data.pre_transform = d->views[i].target_pre_transform;

			render_calc_time_warp_matrix( //
			    &md->views[i].src_pose,   //
			    &md->views[i].src_fov,    //
			    &d->views[i].world_pose,  //
			    &data.transform);         //
		}

		ret = render_gfx_mesh_alloc_and_write( //
		    rr,                                //
		    &data,                             //
		    md->views[i].src_sampler,          //
		    md->views[i].src_image_view,       //
		    &ms.descriptor_sets[i]);           //
		VK_CHK_WITH_GOTO(ret, "render_gfx_mesh_alloc", err_no_memory);

		VK_NAME_DESCRIPTOR_SET(vk, ms.descriptor_sets[i], "render_gfx mesh descriptor sets");
	}


	/*
	 * Do command writing here.
	 */

	render_gfx_begin_target(       //
	    rr,                        //
	    d->gfx.rtr,                //
	    &background_color_active); //

	for (uint32_t i = 0; i < d->view_count; i++) {
		// Convenience.
		const struct render_viewport_data *viewport_data = &d->views[i].target_viewport_data;

		render_gfx_begin_view( //
		    rr,                //
		    i,                 // view_index
		    viewport_data);    // viewport_data

		render_gfx_mesh_draw(      //
		    rr,                    // rr
		    i,                     // mesh_index
		    ms.descriptor_sets[i], // descriptor_set
		    do_timewarp);          // do_timewarp

		render_gfx_end_view(rr);
	}

	render_gfx_end_target(rr);

	return;

err_no_memory:
	// Allocator reset at end of frame, nothing to clean up.
	VK_ERROR(vk, "Could not allocate all UBOs for frame, that's really strange and shouldn't happen!");
}

static void
do_mesh_from_proj(struct render_gfx *rr,
                  const struct comp_render_dispatch_data *d,
                  const struct comp_layer *layer,
                  const struct xrt_layer_projection_view_data *lvd,
                  const struct xrt_layer_projection_view_data *rvd)
{
	const struct xrt_layer_data *data = &layer->data;
	const uint32_t left_array_index = lvd->sub.array_index;
	const uint32_t right_array_index = rvd->sub.array_index;
	const struct comp_swapchain_image *left = &layer->sc_array[0]->images[lvd->sub.image_index];
	const struct comp_swapchain_image *right = &layer->sc_array[1]->images[rvd->sub.image_index];

	VkSampler clamp_to_border_black = rr->r->samplers.clamp_to_border_black;

	struct gfx_mesh_data md = XRT_STRUCT_INIT;
	for (uint32_t i = 0; i < d->view_count; i++) {

		struct xrt_pose src_pose;
		struct xrt_fov src_fov;
		struct xrt_normalized_rect src_norm_rect;
		VkImageView src_image_view;

		if (!is_view_index_right(i)) {
			// Left, aka not right.
			src_pose = lvd->pose;
			src_fov = lvd->fov;
			src_norm_rect = lvd->sub.norm_rect;
			src_image_view = get_image_view(left, data->flags, left_array_index);
		} else {
			// Right
			src_pose = rvd->pose;
			src_fov = rvd->fov;
			src_norm_rect = rvd->sub.norm_rect;
			src_image_view = get_image_view(right, data->flags, right_array_index);
		}

		if (data->flip_y) {
			src_norm_rect.h = -src_norm_rect.h;
			src_norm_rect.y = 1 + src_norm_rect.y;
		}

		gfx_mesh_add_view(         //
		    &md,                   // md
		    i,                     // view_index
		    &src_pose,             // src_pose
		    &src_fov,              // src_fov
		    &src_norm_rect,        // src_norm_rect
		    clamp_to_border_black, // src_sampler
		    src_image_view);       // src_image_view
	}

	do_mesh(            //
	    rr,             //
	    d->do_timewarp, //
	    &md,            //
	    d);             //
}


/*
 *
 * 'Exported' function(s).
 *
 */

void
comp_render_gfx_dispatch(struct render_gfx *rr,
                         const struct comp_layer *layers,
                         const uint32_t layer_count,
                         const struct comp_render_dispatch_data *d)
{
	// Convenience.
	bool fast_path = d->fast_path;

	// Only used if fast_path is true.
	const struct comp_layer *layer = &layers[0];

	// Sanity check.
	assert(!fast_path || layer_count >= 1);

	if (fast_path && layer->data.type == XRT_LAYER_STEREO_PROJECTION) {
		// Fast path.
		const struct xrt_layer_stereo_projection_data *stereo = &layer->data.stereo;
		const struct xrt_layer_projection_view_data *lvd = &stereo->l;
		const struct xrt_layer_projection_view_data *rvd = &stereo->r;

		do_mesh_from_proj( //
		    rr,            //
		    d,             //
		    layer,         //
		    lvd,           //
		    rvd);          //

	} else if (fast_path && layer->data.type == XRT_LAYER_STEREO_PROJECTION_DEPTH) {
		// Fast path.
		const struct xrt_layer_stereo_projection_depth_data *stereo = &layer->data.stereo_depth;
		const struct xrt_layer_projection_view_data *lvd = &stereo->l;
		const struct xrt_layer_projection_view_data *rvd = &stereo->r;

		do_mesh_from_proj( //
		    rr,            //
		    d,             //
		    layer,         //
		    lvd,           //
		    rvd);          //

	} else {
		if (fast_path) {
			U_LOG_W("Wanted fast path but no projection layer, falling back to layer squasher.");
		}


		/*
		 * Layer squashing.
		 */

		do_layers(       //
		    rr,          // rr
		    layers,      // layers
		    layer_count, // layer_count
		    d);          // d


		/*
		 * Distortion.
		 */

		VkImageLayout transition_from = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkImageLayout transition_to = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		cmd_barrier_view_images(                           //
		    rr->r->vk,                                     //
		    d,                                             //
		    rr->r->cmd,                                    // cmd
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,          // src_access_mask
		    VK_ACCESS_SHADER_READ_BIT,                     // dst_access_mask
		    transition_from,                               // transition_from
		    transition_to,                                 // transition_to
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // src_stage_mask
		    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);        // dst_stage_mask

		// Shared between all views.
		VkSampler clamp_to_border_black = rr->r->samplers.clamp_to_border_black;

		struct gfx_mesh_data md = XRT_STRUCT_INIT;
		for (uint32_t i = 0; i < d->view_count; i++) {
			struct xrt_pose src_pose = d->views[i].world_pose;
			struct xrt_fov src_fov = d->views[i].fov;
			VkImageView src_image_view = d->views[i].srgb_view;
			struct xrt_normalized_rect src_norm_rect = d->views[i].layer_norm_rect;

			gfx_mesh_add_view(         //
			    &md,                   // md
			    i,                     // view_index
			    &src_pose,             // src_pose
			    &src_fov,              // src_fov
			    &src_norm_rect,        // src_norm_rect
			    clamp_to_border_black, // src_sampler
			    src_image_view);       // src_image_view
		}

		// We are passing in the same old and new poses.
		do_mesh(   //
		    rr,    //
		    false, // do_timewarp
		    &md,   // md
		    d);    // d
	}
}
