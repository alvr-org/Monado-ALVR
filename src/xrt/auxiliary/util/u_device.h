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

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_tracking.h"

#ifdef __cplusplus
extern "C" {
#endif


extern const struct xrt_matrix_2x2 u_device_rotation_right;
extern const struct xrt_matrix_2x2 u_device_rotation_left;
extern const struct xrt_matrix_2x2 u_device_rotation_ident;
extern const struct xrt_matrix_2x2 u_device_rotation_180;

enum u_device_alloc_flags
{
	// clang-format off
	U_DEVICE_ALLOC_NO_FLAGS      = 0,
	U_DEVICE_ALLOC_HMD           = 1u << 0u,
	U_DEVICE_ALLOC_TRACKING_NONE = 1u << 1u,
	// clang-format on
};

/*!
 *
 * Info to describe 2D extents of a device's screen
 *
 */
struct u_extents_2d
{
	uint32_t w_pixels; // Width of entire screen in pixels
	uint32_t h_pixels; // Height of entire screen
};

/*!
 *
 * Info to describe a very simple headset with diffractive lens optics.
 *
 */
struct u_device_simple_info
{
	struct
	{
		uint32_t w_pixels;
		uint32_t h_pixels;
		float w_meters;
		float h_meters;
	} display;

	float lens_horizontal_separation_meters;
	float lens_vertical_position_meters;

	float fov[XRT_MAX_VIEWS];
};

/*!
 * Setup the device information given a very simple info struct.
 *
 * @return true on success.
 * @ingroup aux_util
 */
bool
u_device_setup_one_eye(struct xrt_device *xdev, const struct u_device_simple_info *info);

/*!
 * Setup the device information given a very simple info struct.
 *
 * @return true on success.
 * @ingroup aux_util
 */
bool
u_device_setup_split_side_by_side(struct xrt_device *xdev, const struct u_device_simple_info *info);

/*!
 * Setup the device's display's 2D extents.
 * Good for headsets without traditional VR optics.
 *
 * @return true on success.
 * @ingroup aux_util
 */
bool
u_extents_2d_split_side_by_side(struct xrt_device *xdev, const struct u_extents_2d *extents);


/*!
 * Dump the device config to stderr.
 *
 * @ingroup aux_util
 */
void
u_device_dump_config(struct xrt_device *xdev, const char *prefix, const char *prod);

#define U_DEVICE_ALLOCATE(type, flags, input_count, output_count)                                                      \
	((type *)u_device_allocate(flags, sizeof(type), input_count, output_count))


/*!
 * Helper function to allocate a device plus inputs in the same allocation
 * placed after the device in memory.
 *
 * Will setup any pointers and num values.
 *
 * @ingroup aux_util
 */
void *
u_device_allocate(enum u_device_alloc_flags flags, size_t size, size_t input_count, size_t output_count);

/*!
 * Helper function to free a device and any data hanging of it.
 *
 * @ingroup aux_util
 */
void
u_device_free(struct xrt_device *xdev);


#define XRT_DEVICE_ROLE_UNASSIGNED (-1)

/*!
 * Helper function to assign head, left hand and right hand roles.
 *
 * @ingroup aux_util
 */
void
u_device_assign_xdev_roles(struct xrt_device **xdevs, size_t xdev_count, int *head, int *left, int *right);

/*!
 * Helper function for `get_view_pose` in an HMD driver.
 *
 * Takes in a translation from the left to right eye, and returns a center to left or right eye transform that assumes
 * the eye relation is symmetrical around the tracked point ("center eye"). Knowing IPD is a subset of this: If you know
 * IPD better than the overall Monado system, copy @p eye_relation and put your known IPD in @p real_eye_relation->x
 *
 * If you have rotation, apply it after calling this function.
 *
 * @param eye_relation 3D translation from left eye to right eye.
 * @param view_index 0 for left, 1 for right.
 * @param out_pose The output pose to populate. Will receive translation, with an identity rotation.
 */
void
u_device_get_view_pose(const struct xrt_vec3 *eye_relation, uint32_t view_index, struct xrt_pose *out_pose);


/*
 *
 * Default implementation of functions.
 *
 */

/*!
 * Helper function to implement @ref xrt_device::get_view_poses in a HMD driver.
 *
 * The field @ref xrt_device::hmd needs to be set and valid.
 */
void
u_device_get_view_poses(struct xrt_device *xdev,
                        const struct xrt_vec3 *default_eye_relation,
                        uint64_t at_timestamp_ns,
                        uint32_t view_count,
                        struct xrt_space_relation *out_head_relation,
                        struct xrt_fov *out_fovs,
                        struct xrt_pose *out_poses);


/*
 *
 * No-op implementation of functions.
 *
 */

/*!
 * Noop function for @ref xrt_device::update_inputs,
 * should only be used from a device with any inputs.
 *
 * @ingroup aux_util
 */
void
u_device_noop_update_inputs(struct xrt_device *xdev);


/*
 *
 * Not implemented function helpers.
 *
 */

/*!
 * Not implemented function for @ref xrt_device::get_hand_tracking.
 *
 * @ingroup aux_util
 */
void
u_device_ni_get_hand_tracking(struct xrt_device *xdev,
                              enum xrt_input_name name,
                              uint64_t desired_timestamp_ns,
                              struct xrt_hand_joint_set *out_value,
                              uint64_t *out_timestamp_ns);

/*!
 * Not implemented function for @ref xrt_device::set_output.
 *
 * @ingroup aux_util
 */
void
u_device_ni_set_output(struct xrt_device *xdev, enum xrt_output_name name, const union xrt_output_value *value);

/*!
 * Not implemented function for @ref xrt_device::get_view_poses.
 *
 * @ingroup aux_util
 */
void
u_device_ni_get_view_poses(struct xrt_device *xdev,
                           const struct xrt_vec3 *default_eye_relation,
                           uint64_t at_timestamp_ns,
                           uint32_t view_count,
                           struct xrt_space_relation *out_head_relation,
                           struct xrt_fov *out_fovs,
                           struct xrt_pose *out_poses);

/*!
 * Not implemented function for @ref xrt_device::compute_distortion.
 *
 * @ingroup aux_util
 */
bool
u_device_ni_compute_distortion(
    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result);

/*!
 * Not implemented function for @ref xrt_device::get_visibility_mask.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_visibility_mask(struct xrt_device *xdev,
                                enum xrt_visibility_mask_type type,
                                uint32_t view_index,
                                struct xrt_visibility_mask **out_mask);

/*!
 * Not implemented function for @ref xrt_device::is_form_factor_available.
 *
 * @ingroup aux_util
 */
bool
u_device_ni_is_form_factor_available(struct xrt_device *xdev, enum xrt_form_factor form_factor);


#ifdef __cplusplus
}
#endif
