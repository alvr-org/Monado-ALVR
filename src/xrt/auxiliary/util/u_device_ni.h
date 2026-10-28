// Copyright 2019-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Not implemented function helpers for device drivers.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_device.h"

#ifdef __cplusplus
extern "C" {
#endif


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
xrt_result_t
u_device_ni_get_hand_tracking(struct xrt_device *xdev,
                              enum xrt_input_name name,
                              int64_t desired_timestamp_ns,
                              struct xrt_hand_joint_set *out_value,
                              int64_t *out_timestamp_ns);

/*!
 * Not implemented function for @ref xrt_device::get_face_tracking.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_face_tracking(struct xrt_device *xdev,
                              enum xrt_input_name facial_expression_type,
                              int64_t at_timestamp_ns,
                              struct xrt_facial_expression_set *out_value);

/*!
 * Not implemented function for @ref xrt_device::get_face_tracking.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_face_calibration_state_android(struct xrt_device *xdev, bool *out_face_is_calibrated);

/*!
 * Not implemented function for @ref xrt_device::get_body_skeleton.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_body_skeleton(struct xrt_device *xdev,
                              enum xrt_input_name body_tracking_type,
                              struct xrt_body_skeleton *out_value);

/*!
 * Not implemented function for @ref xrt_device::get_body_joints.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_body_joints(struct xrt_device *xdev,
                            enum xrt_input_name body_tracking_type,
                            int64_t desired_timestamp_ns,
                            struct xrt_body_joint_set *out_value);

/*!
 * Not implemented function for @ref xrt_device::reset_body_tracking_calibration_meta.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_reset_body_tracking_calibration_meta(struct xrt_device *xdev);

/*!
 * Not implemented function for @ref xrt_device::set_body_tracking_calibration_override_meta.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_set_body_tracking_calibration_override_meta(struct xrt_device *xdev, float new_body_height);

/*!
 * Not implemented function for @ref xrt_device::set_output.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_set_output(struct xrt_device *xdev, enum xrt_output_name name, const struct xrt_output_value *value);

/*!
 * Not implemented function for @ref xrt_device::get_output_limits.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_output_limits(struct xrt_device *xdev, struct xrt_output_limits *limits);

/*!
 * Not implemented function for @ref xrt_device::begin_plane_detection_ext.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_begin_plane_detection_ext(struct xrt_device *xdev,
                                      const struct xrt_plane_detector_begin_info_ext *begin_info,
                                      uint64_t plane_detection_id,
                                      uint64_t *out_plane_detection_id);

/*!
 * Not implemented function for @ref xrt_device::destroy_plane_detection_ext.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_destroy_plane_detection_ext(struct xrt_device *xdev, uint64_t plane_detection_id);

/*!
 * Not implemented function for @ref xrt_device::get_plane_detection_state_ext.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_plane_detection_state_ext(struct xrt_device *xdev,
                                          uint64_t plane_detection_id,
                                          enum xrt_plane_detector_state_ext *out_state);

/*!
 * Not implemented function for @ref xrt_device::get_plane_detections_ext.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_plane_detections_ext(struct xrt_device *xdev,
                                     uint64_t plane_detection_id,
                                     struct xrt_plane_detections_ext *out_detections);

/*!
 * Not implemented function for @ref xrt_device::get_view_poses.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_view_poses(struct xrt_device *xdev,
                           const struct xrt_vec3 *default_eye_relation,
                           int64_t at_timestamp_ns,
                           enum xrt_view_type view_type,
                           uint32_t view_count,
                           struct xrt_space_relation *out_head_relation,
                           struct xrt_fov *out_fovs,
                           struct xrt_pose *out_poses);

/*!
 * Not implemented function for @ref xrt_device::compute_distortion.
 *
 * @ingroup aux_util
 */
xrt_result_t
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
 * Not implemented function for @ref xrt_device::ref_space_usage.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_ref_space_usage(struct xrt_device *xdev,
                            enum xrt_reference_space_type type,
                            enum xrt_input_name name,
                            bool used);

/*!
 * Not implemented function for @ref xrt_device::is_form_factor_available.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_is_form_factor_available(struct xrt_device *xdev, enum xrt_form_factor form_factor, bool *out_available);

/*!
 * Not implemented function for @ref xrt_device::get_battery_status.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_battery_status(struct xrt_device *xdev, bool *out_present, bool *out_charging, float *out_charge);

/*!
 * Not implemented function for @ref xrt_device::get_brightness.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_brightness(struct xrt_device *xdev, float *out_brightness);

/*!
 * Not implemented function for @ref xrt_device::set_brightness.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_set_brightness(struct xrt_device *xdev, float brightness, bool relative);

/*!
 * Not implemented function for @ref xrt_device::get_compositor_info.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_get_compositor_info(struct xrt_device *xdev,
                                const struct xrt_device_compositor_mode *mode,
                                struct xrt_device_compositor_info *out_info);

/*!
 * Not implemented function for @ref xrt_device::begin_feature.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_begin_feature(struct xrt_device *xdev, enum xrt_device_feature_type type);

/*!
 * Not implemented function for @ref xrt_device::end_feature.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_end_feature(struct xrt_device *xdev, enum xrt_device_feature_type type);

/*!
 * Not implemented function for @ref xrt_device::notify_chirality
 *
 * @ingroup aux_util
 */
xrt_result_t
u_device_ni_notify_chirality(struct xrt_device *xdev, bool has_chirality, enum xrt_hand chirality);


#ifdef __cplusplus
}
#endif
