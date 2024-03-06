// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Misc helpers for device drivers.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Moses Turner <moses@collabora.com>
 * @ingroup aux_util
 */

#include "util/u_logging.h"
#include "util/u_device.h"
#include "util/u_misc.h"

#include "math/m_mathinclude.h"
#include "math/m_api.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>


/*
 *
 * Matricies.
 *
 */

const struct xrt_matrix_2x2 u_device_rotation_right = {{
    .vecs =
        {
            {0, 1},
            {-1, 0},
        },
}};


const struct xrt_matrix_2x2 u_device_rotation_left = {{
    .vecs =
        {
            {0, -1},
            {1, 0},
        },
}};

const struct xrt_matrix_2x2 u_device_rotation_ident = {{
    .vecs =
        {
            {1, 0},
            {0, 1},
        },
}};

const struct xrt_matrix_2x2 u_device_rotation_180 = {{
    .vecs =
        {
            {-1, 0},
            {0, -1},
        },
}};


/*
 *
 * Print helpers.
 *
 */

#define PRINT_STR(name, val) U_LOG_RAW("\t%s = %s", name, val)

#define PRINT_INT(name, val) U_LOG_RAW("\t%s = %u", name, val)

#define PRINT_MM(name, val)                                                                                            \
	U_LOG_RAW("\t%s = %f (%i.%02imm)", name, val, (int32_t)(val * 1000.f), abs((int32_t)(val * 100000.f)) % 100)

#define PRINT_ANGLE(name, val) U_LOG_RAW("\t%s = %f (%i°)", name, val, (int32_t)(val * (180 / M_PI)))

#define PRINT_MAT2X2(name, rot) U_LOG_RAW("\t%s = {%f, %f} {%f, %f}", name, rot.v[0], rot.v[1], rot.v[2], rot.v[3])

/*!
 * Dump the device config to stderr.
 */
void
u_device_dump_config(struct xrt_device *xdev, const char *prefix, const char *prod)
{
	U_LOG_RAW("%s - device_setup", prefix);
	PRINT_STR("prod", prod);
	if (xdev->hmd != NULL) {
		PRINT_INT("screens[0].w_pixels ", xdev->hmd->screens[0].w_pixels);
		PRINT_INT("screens[0].h_pixels ", xdev->hmd->screens[0].h_pixels);
		//		PRINT_MM(    "info.display.w_meters", info.display.w_meters);
		//		PRINT_MM(    "info.display.h_meters", info.display.h_meters);

		uint32_t view_count = xdev->hmd->view_count;
		PRINT_INT("view_count", view_count);
		for (uint32_t i = 0; i < view_count; ++i) {
			struct xrt_view *view = &xdev->hmd->views[i];
			struct xrt_fov *fov = &xdev->hmd->distortion.fov[i];
			U_LOG_RAW("\tview index = %u", i);
			U_LOG_RAW("\tviews[%d].viewport.x_pixels = %u", i, view->viewport.x_pixels);
			U_LOG_RAW("\tviews[%d].viewport.y_pixels = %u", i, view->viewport.y_pixels);
			U_LOG_RAW("\tviews[%d].viewport.w_pixels = %u", i, view->viewport.w_pixels);
			U_LOG_RAW("\tviews[%d].viewport.h_pixels = %u", i, view->viewport.h_pixels);
			U_LOG_RAW("\tviews[%d].display.w_pixels = %u", i, view->display.w_pixels);
			U_LOG_RAW("\tviews[%d].display.h_pixels = %u", i, view->display.h_pixels);
			U_LOG_RAW("\tviews[%d].rot = {%f, %f} {%f, %f}", i, view->rot.v[0], view->rot.v[1],
			          view->rot.v[2], view->rot.v[3]);
			U_LOG_RAW("\tdistortion.fov[%d].angle_left = %f (%i°)", i, fov->angle_left,
			          (int32_t)(fov->angle_left * (180 / M_PI)));
			U_LOG_RAW("\tdistortion.fov[%d].angle_right = %f (%i°)", i, fov->angle_right,
			          (int32_t)(fov->angle_right * (180 / M_PI)));
			U_LOG_RAW("\tdistortion.fov[%d].angle_up = %f (%i°)", i, fov->angle_up,
			          (int32_t)(fov->angle_up * (180 / M_PI)));
			U_LOG_RAW("\tdistortion.fov[%d].angle_down = %f (%i°)", i, fov->angle_down,
			          (int32_t)(fov->angle_down * (180 / M_PI)));
		}
	}
}


/*
 *
 * Helper setup functions.
 *
 */

bool
u_extents_2d_split_side_by_side(struct xrt_device *xdev, const struct u_extents_2d *extents)
{
	uint32_t eye_w_pixels = extents->w_pixels / 2;
	uint32_t eye_h_pixels = extents->h_pixels;

	xdev->hmd->screens[0].w_pixels = extents->w_pixels;
	xdev->hmd->screens[0].h_pixels = extents->h_pixels;

	// Left
	xdev->hmd->views[0].display.w_pixels = eye_w_pixels;
	xdev->hmd->views[0].display.h_pixels = eye_h_pixels;
	xdev->hmd->views[0].viewport.x_pixels = 0;
	xdev->hmd->views[0].viewport.y_pixels = 0;
	xdev->hmd->views[0].viewport.w_pixels = eye_w_pixels;
	xdev->hmd->views[0].viewport.h_pixels = eye_h_pixels;
	xdev->hmd->views[0].rot = u_device_rotation_ident;

	// Right
	xdev->hmd->views[1].display.w_pixels = eye_w_pixels;
	xdev->hmd->views[1].display.h_pixels = eye_h_pixels;
	xdev->hmd->views[1].viewport.x_pixels = eye_w_pixels;
	xdev->hmd->views[1].viewport.y_pixels = 0;
	xdev->hmd->views[1].viewport.w_pixels = eye_w_pixels;
	xdev->hmd->views[1].viewport.h_pixels = eye_h_pixels;
	xdev->hmd->views[1].rot = u_device_rotation_ident;
	return true;
}

bool
u_device_setup_one_eye(struct xrt_device *xdev, const struct u_device_simple_info *info)
{
	uint32_t w_pixels = info->display.w_pixels;
	uint32_t h_pixels = info->display.h_pixels;
	float w_meters = info->display.w_meters;
	float h_meters = info->display.h_meters;

	float lens_center_x_meters = w_meters / 2.0;

	float lens_center_y_meters = info->lens_vertical_position_meters;

	// Common
	size_t idx = 0;
	xdev->hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	xdev->hmd->blend_mode_count = idx;

	if (xdev->hmd->distortion.models == 0) {
		xdev->hmd->distortion.models = XRT_DISTORTION_MODEL_NONE;
		xdev->hmd->distortion.preferred = XRT_DISTORTION_MODEL_NONE;
	}
	xdev->hmd->screens[0].w_pixels = info->display.w_pixels;
	xdev->hmd->screens[0].h_pixels = info->display.h_pixels;

	// Left
	xdev->hmd->views[0].display.w_pixels = w_pixels;
	xdev->hmd->views[0].display.h_pixels = h_pixels;
	xdev->hmd->views[0].viewport.x_pixels = 0;
	xdev->hmd->views[0].viewport.y_pixels = 0;
	xdev->hmd->views[0].viewport.w_pixels = w_pixels;
	xdev->hmd->views[0].viewport.h_pixels = h_pixels;
	xdev->hmd->views[0].rot = u_device_rotation_ident;

	{
		/* left eye */
		if (!math_compute_fovs(w_meters, lens_center_x_meters, info->fov[0], h_meters, lens_center_y_meters, 0,
		                       &xdev->hmd->distortion.fov[0])) {
			return false;
		}
	}

	return true;
}

bool
u_device_setup_split_side_by_side(struct xrt_device *xdev, const struct u_device_simple_info *info)
{

	uint32_t view_count = xdev->hmd->view_count;

	uint32_t w_pixels = info->display.w_pixels / view_count;
	uint32_t h_pixels = info->display.h_pixels;
	float w_meters = info->display.w_meters / view_count;
	float h_meters = info->display.h_meters;

	float lens_center_x_meters[2] = {
	    w_meters - info->lens_horizontal_separation_meters / 2.0f,
	    info->lens_horizontal_separation_meters / 2.0f,
	};

	float lens_center_y_meters[2] = {
	    info->lens_vertical_position_meters,
	    info->lens_vertical_position_meters,
	};

	// Common
	size_t idx = 0;
	xdev->hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	xdev->hmd->blend_mode_count = idx;

	if (xdev->hmd->distortion.models == 0) {
		xdev->hmd->distortion.models = XRT_DISTORTION_MODEL_NONE;
		xdev->hmd->distortion.preferred = XRT_DISTORTION_MODEL_NONE;
	}
	xdev->hmd->screens[0].w_pixels = info->display.w_pixels;
	xdev->hmd->screens[0].h_pixels = info->display.h_pixels;

	// Left
	for (uint32_t i = 0; i < view_count; ++i) {
		xdev->hmd->views[i].display.w_pixels = w_pixels;
		xdev->hmd->views[i].display.h_pixels = h_pixels;
		xdev->hmd->views[i].viewport.x_pixels = w_pixels * i;
		xdev->hmd->views[i].viewport.y_pixels = 0;
		xdev->hmd->views[i].viewport.w_pixels = w_pixels;
		xdev->hmd->views[i].viewport.h_pixels = h_pixels;
		xdev->hmd->views[i].rot = u_device_rotation_ident;
	}

	{
		/* right eye */
		if (!math_compute_fovs(w_meters, lens_center_x_meters[view_count - 1], info->fov[view_count - 1],
		                       h_meters, lens_center_y_meters[view_count - 1], 0,
		                       &xdev->hmd->distortion.fov[view_count - 1])) {
			return false;
		}
	}
	if (view_count == 2) {
		/* left eye - mirroring right eye */
		xdev->hmd->distortion.fov[0].angle_up = xdev->hmd->distortion.fov[1].angle_up;
		xdev->hmd->distortion.fov[0].angle_down = xdev->hmd->distortion.fov[1].angle_down;

		xdev->hmd->distortion.fov[0].angle_left = -xdev->hmd->distortion.fov[1].angle_right;
		xdev->hmd->distortion.fov[0].angle_right = -xdev->hmd->distortion.fov[1].angle_left;
	}

	return true;
}

void *
u_device_allocate(enum u_device_alloc_flags flags, size_t size, size_t input_count, size_t output_count)
{
	bool alloc_hmd = (flags & U_DEVICE_ALLOC_HMD) != 0;
	bool alloc_tracking = (flags & U_DEVICE_ALLOC_TRACKING_NONE) != 0;

	size_t total_size = size;

	// Inputs
	size_t offset_inputs = total_size;
	total_size += input_count * sizeof(struct xrt_input);

	// Outputs
	size_t offset_outputs = total_size;
	total_size += output_count * sizeof(struct xrt_output);

	// HMD
	size_t offset_hmd = total_size;
	total_size += alloc_hmd ? sizeof(struct xrt_hmd_parts) : 0;

	// Tracking
	size_t offset_tracking = total_size;
	total_size += alloc_tracking ? sizeof(struct xrt_tracking_origin) : 0;

	// Do the allocation
	char *ptr = U_TYPED_ARRAY_CALLOC(char, total_size);
	struct xrt_device *xdev = (struct xrt_device *)ptr;

	if (input_count > 0) {
		xdev->input_count = input_count;
		xdev->inputs = (struct xrt_input *)(ptr + offset_inputs);

		// Set inputs to active initially, easier for drivers.
		for (size_t i = 0; i < input_count; i++) {
			xdev->inputs[i].active = true;
		}
	}

	if (output_count > 0) {
		xdev->output_count = output_count;
		xdev->outputs = (struct xrt_output *)(ptr + offset_outputs);
	}

	if (alloc_hmd) {
		xdev->hmd = (struct xrt_hmd_parts *)(ptr + offset_hmd);
		// set default view count
		xdev->hmd->view_count = 2;
	}

	if (alloc_tracking) {
		xdev->tracking_origin = (struct xrt_tracking_origin *)(ptr + offset_tracking);
		xdev->tracking_origin->type = XRT_TRACKING_TYPE_NONE;
		xdev->tracking_origin->offset.orientation.w = 1.0f;
		snprintf(xdev->tracking_origin->name, XRT_TRACKING_NAME_LEN, "%s", "No tracking");
	}

	return xdev;
}

void
u_device_free(struct xrt_device *xdev)
{
	if (xdev->hmd != NULL) {
		free(xdev->hmd->distortion.mesh.vertices);
		xdev->hmd->distortion.mesh.vertices = NULL;

		free(xdev->hmd->distortion.mesh.indices);
		xdev->hmd->distortion.mesh.indices = NULL;
	}

	free(xdev);
}

/*
 * move the assigned xdev from hand to other_hand if:
 * - controller of type "any hand" is assigned to hand
 * - other_hand is unassiged
 */
static void
try_move_assignment(struct xrt_device **xdevs, int *hand, int *other_hand)
{
	if (*hand != XRT_DEVICE_ROLE_UNASSIGNED && xdevs[*hand]->device_type == XRT_DEVICE_TYPE_ANY_HAND_CONTROLLER &&
	    *other_hand == XRT_DEVICE_ROLE_UNASSIGNED) {

		*other_hand = *hand;
		*hand = XRT_DEVICE_ROLE_UNASSIGNED;
	}
}

void
u_device_assign_xdev_roles(struct xrt_device **xdevs, size_t xdev_count, int *head, int *left, int *right)
{
	*head = XRT_DEVICE_ROLE_UNASSIGNED;
	*left = XRT_DEVICE_ROLE_UNASSIGNED;
	*right = XRT_DEVICE_ROLE_UNASSIGNED;
	assert(xdev_count < INT_MAX);

	for (size_t i = 0; i < xdev_count; i++) {
		if (xdevs[i] == NULL) {
			continue;
		}

		switch (xdevs[i]->device_type) {
		case XRT_DEVICE_TYPE_HMD:
			if (*head == XRT_DEVICE_ROLE_UNASSIGNED) {
				*head = (int)i;
			}
			break;
		case XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER:
			try_move_assignment(xdevs, left, right);
			if (*left == XRT_DEVICE_ROLE_UNASSIGNED) {
				*left = (int)i;
			}
			break;
		case XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER:
			try_move_assignment(xdevs, right, left);
			if (*right == XRT_DEVICE_ROLE_UNASSIGNED) {
				*right = (int)i;
			}
			break;
		case XRT_DEVICE_TYPE_ANY_HAND_CONTROLLER:
			if (*left == XRT_DEVICE_ROLE_UNASSIGNED) {
				*left = (int)i;
			} else if (*right == XRT_DEVICE_ROLE_UNASSIGNED) {
				*right = (int)i;
			} else {
				//! @todo: do something with unassigned devices?
			}
			break;
		default: break;
		}
	}

	// fill unassigned left/right with hand trackers if available
	for (size_t i = 0; i < xdev_count; i++) {
		if (xdevs[i] == NULL) {
			continue;
		}
		if (xdevs[i]->device_type == XRT_DEVICE_TYPE_HAND_TRACKER) {
			if (*left == XRT_DEVICE_ROLE_UNASSIGNED) {
				*left = (int)i;
			}
			if (*right == XRT_DEVICE_ROLE_UNASSIGNED) {
				*right = (int)i;
			}
			break;
		}
	}
}

void
u_device_get_view_pose(const struct xrt_vec3 *eye_relation, uint32_t view_index, struct xrt_pose *out_pose)
{
	struct xrt_pose pose = XRT_POSE_IDENTITY;
	bool adjust = view_index == 0;

	pose.position.x = eye_relation->x / 2.0f;
	pose.position.y = eye_relation->y / 2.0f;
	pose.position.z = eye_relation->z / 2.0f;

	// Adjust for left/right while also making sure there aren't any -0.f.
	if (pose.position.x > 0.0f && adjust) {
		pose.position.x = -pose.position.x;
	}
	if (pose.position.y > 0.0f && adjust) {
		pose.position.y = -pose.position.y;
	}
	if (pose.position.z > 0.0f && adjust) {
		pose.position.z = -pose.position.z;
	}

	*out_pose = pose;
}


/*
 *
 * Default implementation of functions.
 *
 */

void
u_device_get_view_poses(struct xrt_device *xdev,
                        const struct xrt_vec3 *default_eye_relation,
                        uint64_t at_timestamp_ns,
                        uint32_t view_count,
                        struct xrt_space_relation *out_head_relation,
                        struct xrt_fov *out_fovs,
                        struct xrt_pose *out_poses)
{
	xrt_device_get_tracked_pose(xdev, XRT_INPUT_GENERIC_HEAD_POSE, at_timestamp_ns, out_head_relation);

	for (uint32_t i = 0; i < view_count && i < ARRAY_SIZE(xdev->hmd->views); i++) {
		out_fovs[i] = xdev->hmd->distortion.fov[i];
	}

	for (uint32_t i = 0; i < view_count; i++) {
		u_device_get_view_pose(default_eye_relation, i, &out_poses[i]);
	}
}


/*
 *
 * No-op implementation of functions.
 *
 */

void
u_device_noop_update_inputs(struct xrt_device *xdev)
{
	// Empty, should only be used from a device without any inputs.
}


/*
 *
 * Not implemented function helpers.
 *
 */

#define E(FN) U_LOG_E("Function " #FN " is not implemented for '%s'", xdev->str)

void
u_device_ni_get_hand_tracking(struct xrt_device *xdev,
                              enum xrt_input_name name,
                              uint64_t desired_timestamp_ns,
                              struct xrt_hand_joint_set *out_value,
                              uint64_t *out_timestamp_ns)
{
	E(get_hand_tracking);
}

void
u_device_ni_set_output(struct xrt_device *xdev, enum xrt_output_name name, const union xrt_output_value *value)
{
	E(get_hand_tracking);
}

void
u_device_ni_get_view_poses(struct xrt_device *xdev,
                           const struct xrt_vec3 *default_eye_relation,
                           uint64_t at_timestamp_ns,
                           uint32_t view_count,
                           struct xrt_space_relation *out_head_relation,
                           struct xrt_fov *out_fovs,
                           struct xrt_pose *out_poses)
{
	E(get_hand_tracking);
}

bool
u_device_ni_compute_distortion(
    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
{
	E(compute_distortion);
	return false;
}

xrt_result_t
u_device_ni_get_visibility_mask(struct xrt_device *xdev,
                                enum xrt_visibility_mask_type type,
                                uint32_t view_index,
                                struct xrt_visibility_mask **out_mask)
{
	E(get_visibility_mask);
	return XRT_ERROR_DEVICE_FUNCTION_NOT_IMPLEMENTED;
}

bool
u_device_ni_is_form_factor_available(struct xrt_device *xdev, enum xrt_form_factor form_factor)
{
	E(is_form_factor_available);
	return false;
}
