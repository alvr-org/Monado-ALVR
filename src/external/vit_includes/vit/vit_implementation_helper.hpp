// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Visual-Intertial Tracking tracker helper.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Simon Zeni <simon.zeni@collabora.com>
 */

#pragma once

#include "vit_interface.h"


namespace vit {

typedef vit_result_t Result;
typedef vit_image_format_t ImageFormat;
typedef vit_camera_distortion_t CameraDistortion;
typedef vit_tracker_capability_t TrackerCapability;
typedef vit_tracker_pose_capability_t TrackerPoseCapability;
typedef vit_tracker_timing_titles TrackerTimingTitles;
typedef vit_tracker_t Tracker;
typedef vit_config_t Config;
typedef vit_pose_t Pose;
typedef vit_config_t Config;
typedef vit_imu_sample_t ImuSample;
typedef vit_mask_t Mask;
typedef vit_img_sample_t ImgSample;
typedef vit_pose_data_t PoseData;
typedef vit_pose_timing_t PoseTiming;
typedef vit_pose_feature_t PoseFeature;
typedef vit_pose_features_t PoseFeatures;
typedef vit_camera_calibration_t CameraCalibration;
typedef vit_inertial_calibration_t InertialCalibration;
typedef vit_imu_calibration_t ImuCalibration;

} // namespace vit

extern "C" {

struct vit_tracker
{
	virtual ~vit_tracker() = default;

	virtual vit_result_t has_image_format(const vit_image_format_t fmt, bool *out_supported) const = 0;
	virtual vit_result_t get_capabilities(vit_tracker_capability_t *out_capabilities) const = 0;
	virtual vit_result_t get_pose_capabilities(vit_tracker_pose_capability_t *out_capabilities) const = 0;
	virtual vit_result_t set_pose_capabilities(const vit_tracker_pose_capability_t capabilities, bool value) = 0;
	virtual vit_result_t start() = 0;
	virtual vit_result_t stop() = 0;
	virtual vit_result_t reset() = 0;
	virtual vit_result_t is_running(bool *out_running) const = 0;
	virtual vit_result_t add_imu_calibration(const vit_imu_calibration_t *calibration) = 0;
	virtual vit_result_t add_camera_calibration(const vit_camera_calibration_t *calibration) = 0;
	virtual vit_result_t push_imu_sample(const vit_imu_sample_t *sample) = 0;
	virtual vit_result_t push_img_sample(const vit_img_sample_t *sample) = 0;
	virtual vit_result_t pop_pose(vit_pose_t **pose) = 0;
	virtual vit_result_t get_timing_titles(vit_tracker_timing_titles *out_titles) const = 0;
};

struct vit_pose
{
	virtual ~vit_pose() = default;

	virtual vit_result_t get_data(vit_pose_data *out_data) const = 0;
	virtual vit_result_t get_timing(vit_pose_timing *out_timing) const = 0;
	virtual vit_result_t get_features(uint32_t camera_index, vit_pose_features_t *out_features) const = 0;
};

#ifdef VIT_INTERFACE_IMPLEMENTATION

vit_result_t
vit_api_get_version(uint32_t *out_major, uint32_t *out_minor, uint32_t *out_patch)
{
	*out_major = VIT_HEADER_VERSION_MAJOR;
	*out_minor = VIT_HEADER_VERSION_MINOR;
	*out_patch = VIT_HEADER_VERSION_PATCH;
	return VIT_SUCCESS;
}

void
vit_tracker_destroy(vit_tracker_t *tracker)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	delete t;
}

vit_result_t
vit_tracker_has_image_format(const vit_tracker_t *tracker, const vit_image_format_t image_format, bool *out_supported)
{
	const vit::Tracker *t = static_cast<const vit::Tracker *>(tracker);
	return t->has_image_format(image_format, out_supported);
}

vit_result_t
vit_tracker_get_capabilities(const vit_tracker_t *tracker, vit_tracker_capability_t *out_caps)
{
	const vit::Tracker *t = static_cast<const vit::Tracker *>(tracker);
	return t->get_capabilities(out_caps);
}

vit_result_t
vit_tracker_get_pose_capabilities(const vit_tracker_t *tracker, vit_tracker_pose_capability_t *out_caps)
{
	const vit::Tracker *t = static_cast<const vit::Tracker *>(tracker);
	return t->get_pose_capabilities(out_caps);
}

vit_result_t
vit_tracker_set_pose_capabilities(vit_tracker_t *tracker, vit_tracker_pose_capability_t caps, bool value)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->set_pose_capabilities(caps, value);
}

vit_result_t
vit_tracker_start(vit_tracker_t *tracker)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->start();
}

vit_result_t
vit_tracker_stop(vit_tracker_t *tracker)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->stop();
}

vit_result_t
vit_tracker_reset(vit_tracker_t *tracker)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->reset();
}

vit_result_t
vit_tracker_is_running(const vit_tracker *tracker, bool *out_running)
{
	const vit::Tracker *t = static_cast<const vit::Tracker *>(tracker);
	return t->is_running(out_running);
}

vit_result_t
vit_tracker_push_imu_sample(vit_tracker *tracker, const vit_imu_sample_t *sample)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->push_imu_sample(sample);
}

vit_result_t
vit_tracker_push_img_sample(vit_tracker *tracker, const vit_img_sample_t *sample)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->push_img_sample(sample);
}

vit_result_t
vit_tracker_add_imu_calibration(vit_tracker_t *tracker, const vit_imu_calibration_t *calibration)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->add_imu_calibration(calibration);
}

vit_result_t
vit_tracker_add_camera_calibration(vit_tracker_t *tracker, const vit_camera_calibration_t *calibration)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->add_camera_calibration(calibration);
}

vit_result_t
vit_tracker_pop_pose(vit_tracker_t *tracker, vit_pose_t **out_pose)
{
	vit::Tracker *t = static_cast<vit::Tracker *>(tracker);
	return t->pop_pose(out_pose);
}

vit_result_t
vit_tracker_get_timing_titles(const vit_tracker_t *tracker, vit_tracker_timing_titles *out_titles)
{
	const vit::Tracker *t = static_cast<const vit::Tracker *>(tracker);
	return t->get_timing_titles(out_titles);
}

void
vit_pose_destroy(vit_pose_t *pose)
{
	vit::Pose *p = static_cast<vit::Pose *>(pose);
	delete p;
}

vit_result_t
vit_pose_get_data(const vit_pose_t *pose, vit_pose_data_t *out_data)
{
	const vit::Pose *p = static_cast<const vit::Pose *>(pose);
	return p->get_data(out_data);
}

vit_result_t
vit_pose_get_timing(const vit_pose_t *pose, vit_pose_timing_t *out_timing)
{
	const vit::Pose *p = static_cast<const vit::Pose *>(pose);
	return p->get_timing(out_timing);
}

vit_result_t
vit_pose_get_features(const vit_pose_t *pose, uint32_t camera_index, vit_pose_features_t *out_features)
{
	const vit::Pose *p = static_cast<const vit::Pose *>(pose);
	return p->get_features(camera_index, out_features);
}

#endif
}
