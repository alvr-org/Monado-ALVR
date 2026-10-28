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

#include "util/u_device_ni.h"
#include "util/u_logging.h"


/*
 *
 * Not implemented function helpers.
 *
 */

#define E(FN) U_LOG_E("Function " #FN " is not implemented for '%s'", xdev->str)

xrt_result_t
u_device_ni_get_hand_tracking(struct xrt_device *xdev,
                              enum xrt_input_name name,
                              int64_t desired_timestamp_ns,
                              struct xrt_hand_joint_set *out_value,
                              int64_t *out_timestamp_ns)
{
	E(get_hand_tracking);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_face_tracking(struct xrt_device *xdev,
                              enum xrt_input_name facial_expression_type,
                              int64_t at_timestamp_ns,
                              struct xrt_facial_expression_set *out_value)
{
	E(get_face_tracking);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_face_calibration_state_android(struct xrt_device *xdev, bool *out_face_is_calibrated)
{
	E(get_face_calibration_state_android);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_body_skeleton(struct xrt_device *xdev,
                              enum xrt_input_name body_tracking_type,
                              struct xrt_body_skeleton *out_value)
{
	E(get_body_skeleton);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_body_joints(struct xrt_device *xdev,
                            enum xrt_input_name body_tracking_type,
                            int64_t desired_timestamp_ns,
                            struct xrt_body_joint_set *out_value)
{
	E(get_body_joints);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_reset_body_tracking_calibration_meta(struct xrt_device *xdev)
{
	E(reset_body_tracking_calibration_meta);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_set_body_tracking_calibration_override_meta(struct xrt_device *xdev, float new_body_height)
{
	E(set_body_tracking_calibration_override_meta);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_set_output(struct xrt_device *xdev, enum xrt_output_name name, const struct xrt_output_value *value)
{
	E(set_output);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_output_limits(struct xrt_device *xdev, struct xrt_output_limits *limits)
{
	E(get_output_limits);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_begin_plane_detection_ext(struct xrt_device *xdev,
                                      const struct xrt_plane_detector_begin_info_ext *begin_info,
                                      uint64_t plane_detection_id,
                                      uint64_t *out_plane_detection_id)
{
	E(begin_plane_detection_ext);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_destroy_plane_detection_ext(struct xrt_device *xdev, uint64_t plane_detection_id)
{
	E(destroy_plane_detection_ext);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_plane_detection_state_ext(struct xrt_device *xdev,
                                          uint64_t plane_detection_id,
                                          enum xrt_plane_detector_state_ext *out_state)
{
	E(get_plane_detection_state_ext);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_plane_detections_ext(struct xrt_device *xdev,
                                     uint64_t plane_detection_id,
                                     struct xrt_plane_detections_ext *out_detections)
{
	E(get_plane_detections_ext);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_view_poses(struct xrt_device *xdev,
                           const struct xrt_vec3 *default_eye_relation,
                           int64_t at_timestamp_ns,
                           enum xrt_view_type view_type,
                           uint32_t view_count,
                           struct xrt_space_relation *out_head_relation,
                           struct xrt_fov *out_fovs,
                           struct xrt_pose *out_poses)
{
	E(get_view_poses);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_compute_distortion(
    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
{
	E(compute_distortion);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_visibility_mask(struct xrt_device *xdev,
                                enum xrt_visibility_mask_type type,
                                uint32_t view_index,
                                struct xrt_visibility_mask **out_mask)
{
	E(get_visibility_mask);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_ref_space_usage(struct xrt_device *xdev,
                            enum xrt_reference_space_type type,
                            enum xrt_input_name name,
                            bool used)
{
	E(ref_space_usage);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_is_form_factor_available(struct xrt_device *xdev, enum xrt_form_factor form_factor, bool *out_available)
{
	E(is_form_factor_available);
	*out_available = false;
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_battery_status(struct xrt_device *xdev, bool *out_present, bool *out_charging, float *out_charge)
{
	E(get_battery_status);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_brightness(struct xrt_device *xdev, float *out_brightness)
{
	E(get_brightness);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_set_brightness(struct xrt_device *xdev, float brightness, bool relative)
{
	E(set_brightness);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_get_compositor_info(struct xrt_device *xdev,
                                const struct xrt_device_compositor_mode *mode,
                                struct xrt_device_compositor_info *out_info)
{
	E(get_compositor_info);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_begin_feature(struct xrt_device *xdev, enum xrt_device_feature_type type)
{
	E(begin_feature);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_end_feature(struct xrt_device *xdev, enum xrt_device_feature_type type)
{
	E(end_feature);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_device_ni_notify_chirality(struct xrt_device *xdev, bool has_chirality, enum xrt_hand chirality)
{
	E(notify_chirality);
	return XRT_ERROR_NOT_IMPLEMENTED;
}
