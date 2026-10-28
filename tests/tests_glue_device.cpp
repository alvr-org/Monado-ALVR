// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Tests for glue/g_device.hpp
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 */

#include "glue/g_device.hpp"
#include "util/u_device.h"
#include "util/u_device_ni.h"

#include "catch_amalgamated.hpp"

#include <cstring>


//! Test device with minimal functions
constexpr xrt::util::DeviceFunctions kMinimalFunctions = {};

//! Test device with hand tracking enabled
constexpr xrt::util::DeviceFunctions kHandTrackingFunctions = {
    .hand_tracking = true,
};

//! Test device with HMD functions enabled
constexpr xrt::util::DeviceFunctions kHmdFunctions = {
    .hmd = true,
};

//! Test device with body tracking enabled
constexpr xrt::util::DeviceFunctions kBodyTrackingFunctions = {
    .body_tracking = true,
};

//! Test device with battery enabled
constexpr xrt::util::DeviceFunctions kBatteryFunctions = {
    .battery = true,
};

//! Test device with brightness enabled
constexpr xrt::util::DeviceFunctions kBrightnessFunctions = {
    .brightness = true,
};

//! Test device with all features enabled
constexpr xrt::util::DeviceFunctions kAllFunctions = {
    .hmd = true,
    .distortion = true,
    .visibility_mask = true,
    .update_inputs = true,
    .output = true,
    .output_limits = true,
    .hand_tracking = true,
    .face_tracking = true,
    .face_calibration_android = true,
    .body_tracking = true,
    .plane_detection = true,
    .reference_space = true,
    .battery = true,
    .brightness = true,
    .compositor_info = true,
    .features = true,
};


/*!
 * Minimal test device
 */
class MinimalTestDevice : public xrt::util::DeviceBase<MinimalTestDevice, kMinimalFunctions>
{
public:
	bool destroyed = false;

	MinimalTestDevice() : DeviceBase() {}

	xrt_result_t
	getTrackedPose(enum xrt_input_name name, int64_t at_timestamp_ns, struct xrt_space_relation *out_relation)
	{
		out_relation->relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
		return XRT_SUCCESS;
	}

	static void
	destroyDevice(struct xrt_device *xdev)
	{
		auto *dev = MinimalTestDevice::fromXDev(xdev);
		dev->destroyed = true;
	}
};

/*!
 * Hand tracking test device
 */
class HandTrackingTestDevice : public xrt::util::DeviceBase<HandTrackingTestDevice, kHandTrackingFunctions>
{
public:
	bool destroyed = false;
	int hand_tracking_call_count = 0;

	HandTrackingTestDevice() : DeviceBase() {}

	xrt_result_t
	getTrackedPose(enum xrt_input_name name, int64_t at_timestamp_ns, struct xrt_space_relation *out_relation)
	{
		out_relation->relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getHandTracking(enum xrt_input_name name,
	                int64_t desired_timestamp_ns,
	                struct xrt_hand_joint_set *out_value,
	                int64_t *out_timestamp_ns)
	{
		hand_tracking_call_count++;
		return XRT_SUCCESS;
	}

	static void
	destroyDevice(struct xrt_device *xdev)
	{
		auto *dev = HandTrackingTestDevice::fromXDev(xdev);
		dev->destroyed = true;
	}
};

/*!
 * HMD test device
 */
class HmdTestDevice : public xrt::util::DeviceBase<HmdTestDevice, kHmdFunctions>
{
public:
	bool destroyed = false;
	int get_view_poses_call_count = 0;
	int form_factor_call_count = 0;

	HmdTestDevice() : DeviceBase() {}

	xrt_result_t
	getTrackedPose(enum xrt_input_name name, int64_t at_timestamp_ns, struct xrt_space_relation *out_relation)
	{
		out_relation->relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getViewPoses(const struct xrt_vec3 *default_eye_relation,
	             int64_t at_timestamp_ns,
	             enum xrt_view_type view_type,
	             uint32_t view_count,
	             struct xrt_space_relation *out_head_relation,
	             struct xrt_fov *out_fovs,
	             struct xrt_pose *out_poses)
	{
		get_view_poses_call_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	isFormFactorAvailable(enum xrt_form_factor form_factor, bool *out_available)
	{
		form_factor_call_count++;
		*out_available = false;
		return XRT_SUCCESS;
	}

	static void
	destroyDevice(struct xrt_device *xdev)
	{
		auto *dev = HmdTestDevice::fromXDev(xdev);
		dev->destroyed = true;
	}
};

/*!
 * Body tracking test device
 */
class BodyTrackingTestDevice : public xrt::util::DeviceBase<BodyTrackingTestDevice, kBodyTrackingFunctions>
{
public:
	bool destroyed = false;
	int get_body_skeleton_call_count = 0;
	int get_body_joints_call_count = 0;
	int reset_calibration_call_count = 0;
	int set_calibration_call_count = 0;

	BodyTrackingTestDevice() : DeviceBase() {}

	xrt_result_t
	getTrackedPose(enum xrt_input_name name, int64_t at_timestamp_ns, struct xrt_space_relation *out_relation)
	{
		out_relation->relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getBodySkeleton(enum xrt_input_name body_tracking_type, struct xrt_body_skeleton *out_value)
	{
		get_body_skeleton_call_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getBodyJoints(enum xrt_input_name body_tracking_type,
	              int64_t desired_timestamp_ns,
	              struct xrt_body_joint_set *out_value)
	{
		get_body_joints_call_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	resetBodyTrackingCalibrationMeta()
	{
		reset_calibration_call_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	setBodyTrackingCalibrationOverrideMeta(float new_body_height)
	{
		set_calibration_call_count++;
		return XRT_SUCCESS;
	}

	static void
	destroyDevice(struct xrt_device *xdev)
	{
		auto *dev = BodyTrackingTestDevice::fromXDev(xdev);
		dev->destroyed = true;
	}
};

/*!
 * Battery test device
 */
class BatteryTestDevice : public xrt::util::DeviceBase<BatteryTestDevice, kBatteryFunctions>
{
public:
	bool destroyed = false;
	int get_battery_call_count = 0;

	BatteryTestDevice() : DeviceBase() {}

	xrt_result_t
	getTrackedPose(enum xrt_input_name name, int64_t at_timestamp_ns, struct xrt_space_relation *out_relation)
	{
		out_relation->relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getBatteryStatus(bool *out_present, bool *out_charging, float *out_charge)
	{
		get_battery_call_count++;
		*out_present = true;
		*out_charging = false;
		*out_charge = 0.75f;
		return XRT_SUCCESS;
	}

	static void
	destroyDevice(struct xrt_device *xdev)
	{
		auto *dev = BatteryTestDevice::fromXDev(xdev);
		dev->destroyed = true;
	}
};

/*!
 * Brightness test device
 */
class BrightnessTestDevice : public xrt::util::DeviceBase<BrightnessTestDevice, kBrightnessFunctions>
{
public:
	bool destroyed = false;
	int get_brightness_call_count = 0;
	int set_brightness_call_count = 0;

	BrightnessTestDevice() : DeviceBase() {}

	xrt_result_t
	getTrackedPose(enum xrt_input_name name, int64_t at_timestamp_ns, struct xrt_space_relation *out_relation)
	{
		out_relation->relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getBrightness(float *out_brightness)
	{
		get_brightness_call_count++;
		*out_brightness = 0.5f;
		return XRT_SUCCESS;
	}

	xrt_result_t
	setBrightness(float brightness, bool relative)
	{
		set_brightness_call_count++;
		return XRT_SUCCESS;
	}

	static void
	destroyDevice(struct xrt_device *xdev)
	{
		auto *dev = BrightnessTestDevice::fromXDev(xdev);
		dev->destroyed = true;
	}
};


TEST_CASE("DeviceBase_MinimalDevice")
{
	MinimalTestDevice dev;
	auto *xdev = dev.getXDev();

	SECTION("Basic properties")
	{
		REQUIRE(xdev != nullptr);
		REQUIRE_FALSE(dev.destroyed);
	}

	SECTION("Function pointers are set")
	{
		REQUIRE(xdev->get_tracked_pose != nullptr);
		REQUIRE(xdev->destroy != nullptr);
	}

	SECTION("Stub functions are set for disabled features")
	{
		REQUIRE(xdev->get_view_poses == u_device_ni_get_view_poses);
		REQUIRE(xdev->is_form_factor_available == u_device_ni_is_form_factor_available);
		REQUIRE(xdev->compute_distortion == u_device_ni_compute_distortion);
		// @todo Fix when u_device function is fixed.
		// REQUIRE(xdev->get_visibility_mask == u_device_ni_get_visibility_mask);
		REQUIRE(xdev->update_inputs == u_device_noop_update_inputs);
		REQUIRE(xdev->set_output == u_device_ni_set_output);
		REQUIRE(xdev->get_output_limits == u_device_ni_get_output_limits);
		REQUIRE(xdev->ref_space_usage == u_device_ni_ref_space_usage);
		REQUIRE(xdev->get_hand_tracking == u_device_ni_get_hand_tracking);
		REQUIRE(xdev->get_face_tracking == u_device_ni_get_face_tracking);
		REQUIRE(xdev->get_body_skeleton == u_device_ni_get_body_skeleton);
		REQUIRE(xdev->get_body_joints == u_device_ni_get_body_joints);
		REQUIRE(xdev->reset_body_tracking_calibration_meta == u_device_ni_reset_body_tracking_calibration_meta);
		REQUIRE(xdev->set_body_tracking_calibration_override_meta ==
		        u_device_ni_set_body_tracking_calibration_override_meta);
		REQUIRE(xdev->begin_plane_detection_ext == u_device_ni_begin_plane_detection_ext);
		REQUIRE(xdev->destroy_plane_detection_ext == u_device_ni_destroy_plane_detection_ext);
		REQUIRE(xdev->get_plane_detection_state_ext == u_device_ni_get_plane_detection_state_ext);
		REQUIRE(xdev->get_plane_detections_ext == u_device_ni_get_plane_detections_ext);
		REQUIRE(xdev->get_battery_status == u_device_ni_get_battery_status);
		REQUIRE(xdev->get_brightness == u_device_ni_get_brightness);
		REQUIRE(xdev->set_brightness == u_device_ni_set_brightness);
		REQUIRE(xdev->begin_feature == u_device_ni_begin_feature);
		REQUIRE(xdev->end_feature == u_device_ni_end_feature);
	}

	SECTION("fromXDev conversion works")
	{
		auto *retrieved = MinimalTestDevice::fromXDev(xdev);
		REQUIRE(retrieved == &dev);
	}

	SECTION("getTrackedPose can be called")
	{
		xrt_space_relation relation;
		memset(&relation, 0, sizeof(relation));

		xrt_result_t result = xdev->get_tracked_pose(xdev, XRT_INPUT_GENERIC_HEAD_POSE, 0, &relation);
		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(relation.relation_flags == XRT_SPACE_RELATION_BITMASK_NONE);
	}

	SECTION("destroy calls destroyDevice")
	{
		REQUIRE_FALSE(dev.destroyed);
		xdev->destroy(xdev);
		REQUIRE(dev.destroyed);
	}
}

TEST_CASE("DeviceBase_HandTrackingDevice")
{
	HandTrackingTestDevice dev;
	auto *xdev = dev.getXDev();

	SECTION("Hand tracking function is set")
	{
		REQUIRE(xdev->get_hand_tracking != nullptr);
		REQUIRE(xdev->get_hand_tracking != u_device_ni_get_hand_tracking);
	}

	SECTION("Other disabled features still use stubs")
	{
		REQUIRE(xdev->get_view_poses == u_device_ni_get_view_poses);
		REQUIRE(xdev->is_form_factor_available == u_device_ni_is_form_factor_available);
		REQUIRE(xdev->get_face_tracking == u_device_ni_get_face_tracking);
	}

	SECTION("Hand tracking can be called")
	{
		xrt_hand_joint_set joint_set;
		int64_t timestamp;

		REQUIRE(dev.hand_tracking_call_count == 0);

		xrt_result_t result =
		    xdev->get_hand_tracking(xdev, XRT_INPUT_HT_UNOBSTRUCTED_LEFT, 0, &joint_set, &timestamp);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(dev.hand_tracking_call_count == 1);
	}

	SECTION("fromXDev conversion works")
	{
		auto *retrieved = HandTrackingTestDevice::fromXDev(xdev);
		REQUIRE(retrieved == &dev);
	}
}

TEST_CASE("DeviceBase_HmdDevice")
{
	HmdTestDevice dev;
	auto *xdev = dev.getXDev();

	SECTION("HMD functions are set")
	{
		REQUIRE(xdev->get_view_poses != nullptr);
		REQUIRE(xdev->get_view_poses != u_device_ni_get_view_poses);
		REQUIRE(xdev->is_form_factor_available != nullptr);
		REQUIRE(xdev->is_form_factor_available != u_device_ni_is_form_factor_available);
	}

	SECTION("Other disabled features still use stubs")
	{
		REQUIRE(xdev->compute_distortion == u_device_ni_compute_distortion);
		REQUIRE(xdev->ref_space_usage == u_device_ni_ref_space_usage);
		REQUIRE(xdev->get_hand_tracking == u_device_ni_get_hand_tracking);
		REQUIRE(xdev->get_face_tracking == u_device_ni_get_face_tracking);
		REQUIRE(xdev->get_body_skeleton == u_device_ni_get_body_skeleton);
		REQUIRE(xdev->get_body_joints == u_device_ni_get_body_joints);
		REQUIRE(xdev->get_battery_status == u_device_ni_get_battery_status);
		REQUIRE(xdev->get_output_limits == u_device_ni_get_output_limits);
	}

	SECTION("get_view_poses can be called")
	{
		xrt_vec3 default_eye_relation = {0.0f, 0.0f, 0.0f};
		xrt_space_relation head_relation;
		xrt_fov fovs[2];
		xrt_pose poses[2];

		REQUIRE(dev.get_view_poses_call_count == 0);

		xrt_result_t result = xdev->get_view_poses(xdev, &default_eye_relation, 0, XRT_VIEW_TYPE_STEREO, 2,
		                                           &head_relation, fovs, poses);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(dev.get_view_poses_call_count == 1);
	}

	SECTION("is_form_factor_available can be called")
	{
		REQUIRE(dev.form_factor_call_count == 0);

		bool available = false;
		xrt_result_t result = xdev->is_form_factor_available(xdev, XRT_FORM_FACTOR_HMD, &available);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE_FALSE(available);
		REQUIRE(dev.form_factor_call_count == 1);
	}

	SECTION("fromXDev conversion works")
	{
		auto *retrieved = HmdTestDevice::fromXDev(xdev);
		REQUIRE(retrieved == &dev);
	}
}

TEST_CASE("DeviceBase_MultipleInstances")
{
	MinimalTestDevice dev1;
	MinimalTestDevice dev2;

	SECTION("Each device has its own xrt_device")
	{
		REQUIRE(dev1.getXDev() != dev2.getXDev());
	}

	SECTION("fromXDev returns correct instance")
	{
		auto *retrieved1 = MinimalTestDevice::fromXDev(dev1.getXDev());
		auto *retrieved2 = MinimalTestDevice::fromXDev(dev2.getXDev());

		REQUIRE(retrieved1 == &dev1);
		REQUIRE(retrieved2 == &dev2);
		REQUIRE(retrieved1 != retrieved2);
	}

	SECTION("Destroying one doesn't affect the other")
	{
		dev1.getXDev()->destroy(dev1.getXDev());

		REQUIRE(dev1.destroyed);
		REQUIRE_FALSE(dev2.destroyed);
	}
}

TEST_CASE("DeviceBase_BodyTrackingDevice")
{
	BodyTrackingTestDevice dev;
	auto *xdev = dev.getXDev();

	SECTION("Body tracking functions are set")
	{
		REQUIRE(xdev->get_body_skeleton != nullptr);
		REQUIRE(xdev->get_body_joints != nullptr);
		REQUIRE(xdev->reset_body_tracking_calibration_meta != nullptr);
		REQUIRE(xdev->set_body_tracking_calibration_override_meta != nullptr);
	}

	SECTION("Other disabled features still use stubs")
	{
		REQUIRE(xdev->get_view_poses == u_device_ni_get_view_poses);
		REQUIRE(xdev->get_hand_tracking == u_device_ni_get_hand_tracking);
		REQUIRE(xdev->get_face_tracking == u_device_ni_get_face_tracking);
		REQUIRE(xdev->get_battery_status == u_device_ni_get_battery_status);
	}

	SECTION("get_body_skeleton can be called")
	{
		xrt_body_skeleton skeleton;

		REQUIRE(dev.get_body_skeleton_call_count == 0);

		xrt_result_t result = xdev->get_body_skeleton(xdev, XRT_INPUT_GENERIC_BODY_TRACKING, &skeleton);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(dev.get_body_skeleton_call_count == 1);
	}

	SECTION("get_body_joints can be called")
	{
		xrt_body_joint_set joint_set;

		REQUIRE(dev.get_body_joints_call_count == 0);

		xrt_result_t result = xdev->get_body_joints(xdev, XRT_INPUT_GENERIC_BODY_TRACKING, 0, &joint_set);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(dev.get_body_joints_call_count == 1);
	}

	SECTION("reset_body_tracking_calibration_meta can be called")
	{
		REQUIRE(dev.reset_calibration_call_count == 0);

		xrt_result_t result = xdev->reset_body_tracking_calibration_meta(xdev);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(dev.reset_calibration_call_count == 1);
	}

	SECTION("set_body_tracking_calibration_override_meta can be called")
	{
		REQUIRE(dev.set_calibration_call_count == 0);

		xrt_result_t result = xdev->set_body_tracking_calibration_override_meta(xdev, 1.75f);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(dev.set_calibration_call_count == 1);
	}

	SECTION("fromXDev conversion works")
	{
		auto *retrieved = BodyTrackingTestDevice::fromXDev(xdev);
		REQUIRE(retrieved == &dev);
	}
}

TEST_CASE("DeviceBase_BatteryDevice")
{
	BatteryTestDevice dev;
	auto *xdev = dev.getXDev();

	SECTION("Battery function is set")
	{
		REQUIRE(xdev->get_battery_status != nullptr);
	}

	SECTION("Other disabled features still use stubs")
	{
		REQUIRE(xdev->get_view_poses == u_device_ni_get_view_poses);
		REQUIRE(xdev->get_hand_tracking == u_device_ni_get_hand_tracking);
		REQUIRE(xdev->get_face_tracking == u_device_ni_get_face_tracking);
		REQUIRE(xdev->get_body_skeleton == u_device_ni_get_body_skeleton);
		REQUIRE(xdev->get_brightness == u_device_ni_get_brightness);
		REQUIRE(xdev->set_brightness == u_device_ni_set_brightness);
	}

	SECTION("get_battery_status can be called")
	{
		bool present = false;
		bool charging = false;
		float charge = 0.0f;

		REQUIRE(dev.get_battery_call_count == 0);

		xrt_result_t result = xdev->get_battery_status(xdev, &present, &charging, &charge);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(present == true);
		REQUIRE(charging == false);
		REQUIRE(charge == 0.75f);
		REQUIRE(dev.get_battery_call_count == 1);
	}

	SECTION("fromXDev conversion works")
	{
		auto *retrieved = BatteryTestDevice::fromXDev(xdev);
		REQUIRE(retrieved == &dev);
	}
}

TEST_CASE("DeviceBase_BrightnessDevice")
{
	BrightnessTestDevice dev;
	auto *xdev = dev.getXDev();

	SECTION("Brightness functions are set")
	{
		REQUIRE(xdev->get_brightness != nullptr);
		REQUIRE(xdev->set_brightness != nullptr);
	}

	SECTION("Other disabled features still use stubs")
	{
		REQUIRE(xdev->get_view_poses == u_device_ni_get_view_poses);
		REQUIRE(xdev->get_hand_tracking == u_device_ni_get_hand_tracking);
		REQUIRE(xdev->get_face_tracking == u_device_ni_get_face_tracking);
		REQUIRE(xdev->get_body_skeleton == u_device_ni_get_body_skeleton);
		REQUIRE(xdev->get_battery_status == u_device_ni_get_battery_status);
	}

	SECTION("get_brightness can be called")
	{
		float brightness = 0.0f;

		REQUIRE(dev.get_brightness_call_count == 0);

		xrt_result_t result = xdev->get_brightness(xdev, &brightness);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(brightness == 0.5f);
		REQUIRE(dev.get_brightness_call_count == 1);
	}

	SECTION("set_brightness can be called")
	{
		REQUIRE(dev.set_brightness_call_count == 0);

		xrt_result_t result = xdev->set_brightness(xdev, 0.8f, false);

		REQUIRE(result == XRT_SUCCESS);
		REQUIRE(dev.set_brightness_call_count == 1);
	}

	SECTION("fromXDev conversion works")
	{
		auto *retrieved = BrightnessTestDevice::fromXDev(xdev);
		REQUIRE(retrieved == &dev);
	}
}

/*!
 * Comprehensive test device with all features enabled
 */
class AllFeaturesTestDevice : public xrt::util::DeviceBase<AllFeaturesTestDevice, kAllFunctions>
{
public:
	bool destroyed = false;
	int update_inputs_count = 0;
	int tracked_pose_count = 0;
	int hand_tracking_count = 0;
	int face_tracking_count = 0;
	int face_calibration_android_count = 0;
	int body_skeleton_count = 0;
	int body_joints_count = 0;
	int reset_body_calibration_count = 0;
	int set_body_calibration_count = 0;
	int set_output_count = 0;
	int get_output_limits_count = 0;
	int begin_plane_detection_count = 0;
	int destroy_plane_detection_count = 0;
	int get_plane_detection_state_count = 0;
	int get_plane_detections_count = 0;
	int get_view_poses_count = 0;
	int compute_distortion_count = 0;
	int get_visibility_mask_count = 0;
	int ref_space_usage_count = 0;
	int is_form_factor_available_count = 0;
	int get_battery_status_count = 0;
	int get_brightness_count = 0;
	int set_brightness_count = 0;
	int get_compositor_info_count = 0;
	int begin_feature_count = 0;
	int end_feature_count = 0;

	AllFeaturesTestDevice() : DeviceBase() {}

	xrt_result_t
	updateInputs()
	{
		update_inputs_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getTrackedPose(enum xrt_input_name name, int64_t at_timestamp_ns, struct xrt_space_relation *out_relation)
	{
		tracked_pose_count++;
		out_relation->relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getHandTracking(enum xrt_input_name name,
	                int64_t desired_timestamp_ns,
	                struct xrt_hand_joint_set *out_value,
	                int64_t *out_timestamp_ns)
	{
		hand_tracking_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getFaceTracking(enum xrt_input_name facial_expression_type,
	                int64_t at_timestamp_ns,
	                struct xrt_facial_expression_set *out_value)
	{
		face_tracking_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getFaceCalibrationStateAndroid(bool *out_face_is_calibrated)
	{
		face_calibration_android_count++;
		*out_face_is_calibrated = true;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getBodySkeleton(enum xrt_input_name body_tracking_type, struct xrt_body_skeleton *out_value)
	{
		body_skeleton_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getBodyJoints(enum xrt_input_name body_tracking_type,
	              int64_t desired_timestamp_ns,
	              struct xrt_body_joint_set *out_value)
	{
		body_joints_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	resetBodyTrackingCalibrationMeta()
	{
		reset_body_calibration_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	setBodyTrackingCalibrationOverrideMeta(float new_body_height)
	{
		set_body_calibration_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	setOutput(enum xrt_output_name name, const struct xrt_output_value *value)
	{
		set_output_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getOutputLimits(struct xrt_output_limits *limits)
	{
		get_output_limits_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	beginPlaneDetectionExt(const struct xrt_plane_detector_begin_info_ext *begin_info,
	                       uint64_t plane_detection_id,
	                       uint64_t *out_plane_detection_id)
	{
		begin_plane_detection_count++;
		*out_plane_detection_id = 42;
		return XRT_SUCCESS;
	}

	xrt_result_t
	destroyPlaneDetectionExt(uint64_t plane_detection_id)
	{
		destroy_plane_detection_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getPlaneDetectionStateExt(uint64_t plane_detection_id, enum xrt_plane_detector_state_ext *out_state)
	{
		get_plane_detection_state_count++;
		*out_state = XRT_PLANE_DETECTOR_STATE_DONE_EXT;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getPlaneDetectionsExt(uint64_t plane_detection_id, struct xrt_plane_detections_ext *out_detections)
	{
		get_plane_detections_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getViewPoses(const struct xrt_vec3 *default_eye_relation,
	             int64_t at_timestamp_ns,
	             enum xrt_view_type view_type,
	             uint32_t view_count,
	             struct xrt_space_relation *out_head_relation,
	             struct xrt_fov *out_fovs,
	             struct xrt_pose *out_poses)
	{
		get_view_poses_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	computeDistortion(uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
	{
		compute_distortion_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getVisibilityMask(enum xrt_visibility_mask_type type,
	                  uint32_t view_index,
	                  struct xrt_visibility_mask **out_mask)
	{
		get_visibility_mask_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	refSpaceUsage(enum xrt_reference_space_type type, enum xrt_input_name name, bool used)
	{
		ref_space_usage_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	isFormFactorAvailable(enum xrt_form_factor form_factor, bool *out_available)
	{
		is_form_factor_available_count++;
		*out_available = true;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getBatteryStatus(bool *out_present, bool *out_charging, float *out_charge)
	{
		get_battery_status_count++;
		*out_present = true;
		*out_charging = false;
		*out_charge = 0.85f;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getBrightness(float *out_brightness)
	{
		get_brightness_count++;
		*out_brightness = 0.7f;
		return XRT_SUCCESS;
	}

	xrt_result_t
	setBrightness(float brightness, bool relative)
	{
		set_brightness_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	getCompositorInfo(const struct xrt_device_compositor_mode *mode, struct xrt_device_compositor_info *out_info)
	{
		get_compositor_info_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	beginFeature(enum xrt_device_feature_type type)
	{
		begin_feature_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	endFeature(enum xrt_device_feature_type type)
	{
		end_feature_count++;
		return XRT_SUCCESS;
	}

	static void
	destroyDevice(struct xrt_device *xdev)
	{
		auto *dev = AllFeaturesTestDevice::fromXDev(xdev);
		dev->destroyed = true;
	}
};

TEST_CASE("DeviceBase_AllFeaturesDevice")
{
	AllFeaturesTestDevice dev;
	auto *xdev = dev.getXDev();

	SECTION("All function pointers are set")
	{
		REQUIRE(xdev->update_inputs != nullptr);
		REQUIRE(xdev->get_tracked_pose != nullptr);
		REQUIRE(xdev->get_hand_tracking != nullptr);
		REQUIRE(xdev->get_face_tracking != nullptr);
		REQUIRE(xdev->get_face_calibration_state_android != nullptr);
		REQUIRE(xdev->get_body_skeleton != nullptr);
		REQUIRE(xdev->get_body_joints != nullptr);
		REQUIRE(xdev->reset_body_tracking_calibration_meta != nullptr);
		REQUIRE(xdev->set_body_tracking_calibration_override_meta != nullptr);
		REQUIRE(xdev->set_output != nullptr);
		REQUIRE(xdev->get_output_limits != nullptr);
		REQUIRE(xdev->begin_plane_detection_ext != nullptr);
		REQUIRE(xdev->destroy_plane_detection_ext != nullptr);
		REQUIRE(xdev->get_plane_detection_state_ext != nullptr);
		REQUIRE(xdev->get_plane_detections_ext != nullptr);
		REQUIRE(xdev->get_view_poses != nullptr);
		REQUIRE(xdev->compute_distortion != nullptr);
		REQUIRE(xdev->get_visibility_mask != nullptr);
		REQUIRE(xdev->ref_space_usage != nullptr);
		REQUIRE(xdev->is_form_factor_available != nullptr);
		REQUIRE(xdev->get_battery_status != nullptr);
		REQUIRE(xdev->get_brightness != nullptr);
		REQUIRE(xdev->set_brightness != nullptr);
		REQUIRE(xdev->get_compositor_info != nullptr);
		REQUIRE(xdev->begin_feature != nullptr);
		REQUIRE(xdev->end_feature != nullptr);
		REQUIRE(xdev->destroy != nullptr);
	}

	SECTION("All functions can be called and track call counts")
	{
		// Test update_inputs
		REQUIRE(dev.update_inputs_count == 0);
		xdev->update_inputs(xdev);
		REQUIRE(dev.update_inputs_count == 1);

		// Test get_tracked_pose
		REQUIRE(dev.tracked_pose_count == 0);
		xrt_space_relation relation;
		xdev->get_tracked_pose(xdev, XRT_INPUT_GENERIC_HEAD_POSE, 0, &relation);
		REQUIRE(dev.tracked_pose_count == 1);

		// Test get_hand_tracking
		REQUIRE(dev.hand_tracking_count == 0);
		xrt_hand_joint_set hand_joints;
		int64_t timestamp;
		xdev->get_hand_tracking(xdev, XRT_INPUT_HT_UNOBSTRUCTED_LEFT, 0, &hand_joints, &timestamp);
		REQUIRE(dev.hand_tracking_count == 1);

		// Test get_face_tracking
		REQUIRE(dev.face_tracking_count == 0);
		xrt_facial_expression_set face_set;
		xdev->get_face_tracking(xdev, XRT_INPUT_GENERIC_FACE_TRACKING, 0, &face_set);
		REQUIRE(dev.face_tracking_count == 1);

		// Test get_face_calibration_state_android
		REQUIRE(dev.face_calibration_android_count == 0);
		bool face_calibrated;
		xdev->get_face_calibration_state_android(xdev, &face_calibrated);
		REQUIRE(dev.face_calibration_android_count == 1);
		REQUIRE(face_calibrated == true);

		// Test get_body_skeleton
		REQUIRE(dev.body_skeleton_count == 0);
		xrt_body_skeleton skeleton;
		xdev->get_body_skeleton(xdev, XRT_INPUT_GENERIC_BODY_TRACKING, &skeleton);
		REQUIRE(dev.body_skeleton_count == 1);

		// Test get_body_joints
		REQUIRE(dev.body_joints_count == 0);
		xrt_body_joint_set body_joints;
		xdev->get_body_joints(xdev, XRT_INPUT_GENERIC_BODY_TRACKING, 0, &body_joints);
		REQUIRE(dev.body_joints_count == 1);

		// Test reset_body_tracking_calibration_meta
		REQUIRE(dev.reset_body_calibration_count == 0);
		xdev->reset_body_tracking_calibration_meta(xdev);
		REQUIRE(dev.reset_body_calibration_count == 1);

		// Test set_body_tracking_calibration_override_meta
		REQUIRE(dev.set_body_calibration_count == 0);
		xdev->set_body_tracking_calibration_override_meta(xdev, 1.8f);
		REQUIRE(dev.set_body_calibration_count == 1);

		// Test set_output
		REQUIRE(dev.set_output_count == 0);
		xrt_output_value output_value;
		xdev->set_output(xdev, XRT_OUTPUT_NAME_SIMPLE_VIBRATION, &output_value);
		REQUIRE(dev.set_output_count == 1);

		// Test get_output_limits
		REQUIRE(dev.get_output_limits_count == 0);
		xrt_output_limits limits;
		xdev->get_output_limits(xdev, &limits);
		REQUIRE(dev.get_output_limits_count == 1);

		// Test begin_plane_detection_ext
		REQUIRE(dev.begin_plane_detection_count == 0);
		xrt_plane_detector_begin_info_ext begin_info = {};
		uint64_t plane_detection_id;
		xdev->begin_plane_detection_ext(xdev, &begin_info, 0, &plane_detection_id);
		REQUIRE(dev.begin_plane_detection_count == 1);
		REQUIRE(plane_detection_id == 42);

		// Test get_plane_detection_state_ext
		REQUIRE(dev.get_plane_detection_state_count == 0);
		xrt_plane_detector_state_ext state;
		xdev->get_plane_detection_state_ext(xdev, 42, &state);
		REQUIRE(dev.get_plane_detection_state_count == 1);
		REQUIRE(state == XRT_PLANE_DETECTOR_STATE_DONE_EXT);

		// Test get_plane_detections_ext
		REQUIRE(dev.get_plane_detections_count == 0);
		xrt_plane_detections_ext detections;
		xdev->get_plane_detections_ext(xdev, 42, &detections);
		REQUIRE(dev.get_plane_detections_count == 1);

		// Test destroy_plane_detection_ext
		REQUIRE(dev.destroy_plane_detection_count == 0);
		xdev->destroy_plane_detection_ext(xdev, 42);
		REQUIRE(dev.destroy_plane_detection_count == 1);

		// Test get_view_poses
		REQUIRE(dev.get_view_poses_count == 0);
		xrt_vec3 eye_relation = {0.0f, 0.0f, 0.0f};
		xrt_space_relation head_relation;
		xrt_fov fovs[2];
		xrt_pose poses[2];
		xdev->get_view_poses(xdev, &eye_relation, 0, XRT_VIEW_TYPE_STEREO, 2, &head_relation, fovs, poses);
		REQUIRE(dev.get_view_poses_count == 1);

		// Test compute_distortion
		REQUIRE(dev.compute_distortion_count == 0);
		xrt_uv_triplet triplet;
		xdev->compute_distortion(xdev, 0, 0.5f, 0.5f, &triplet);
		REQUIRE(dev.compute_distortion_count == 1);

		// Test get_visibility_mask
		REQUIRE(dev.get_visibility_mask_count == 0);
		xrt_visibility_mask *mask;
		xdev->get_visibility_mask(xdev, XRT_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH, 0, &mask);
		REQUIRE(dev.get_visibility_mask_count == 1);

		// Test ref_space_usage
		REQUIRE(dev.ref_space_usage_count == 0);
		xdev->ref_space_usage(xdev, XRT_SPACE_REFERENCE_TYPE_STAGE, XRT_INPUT_GENERIC_HEAD_POSE, true);
		REQUIRE(dev.ref_space_usage_count == 1);

		// Test is_form_factor_available
		REQUIRE(dev.is_form_factor_available_count == 0);
		bool form_factor_available = false;
		xrt_result_t form_factor_result =
		    xdev->is_form_factor_available(xdev, XRT_FORM_FACTOR_HMD, &form_factor_available);
		REQUIRE(form_factor_result == XRT_SUCCESS);
		REQUIRE(dev.is_form_factor_available_count == 1);
		REQUIRE(form_factor_available == true);

		// Test get_battery_status
		REQUIRE(dev.get_battery_status_count == 0);
		bool battery_present, battery_charging;
		float battery_charge;
		xdev->get_battery_status(xdev, &battery_present, &battery_charging, &battery_charge);
		REQUIRE(dev.get_battery_status_count == 1);
		REQUIRE(battery_present == true);
		REQUIRE(battery_charging == false);
		REQUIRE(battery_charge == 0.85f);

		// Test get_brightness
		REQUIRE(dev.get_brightness_count == 0);
		float brightness;
		xdev->get_brightness(xdev, &brightness);
		REQUIRE(dev.get_brightness_count == 1);
		REQUIRE(brightness == 0.7f);

		// Test set_brightness
		REQUIRE(dev.set_brightness_count == 0);
		xdev->set_brightness(xdev, 0.9f, false);
		REQUIRE(dev.set_brightness_count == 1);

		// Test get_compositor_info
		REQUIRE(dev.get_compositor_info_count == 0);
		xrt_device_compositor_mode mode = {};
		xrt_device_compositor_info info;
		xdev->get_compositor_info(xdev, &mode, &info);
		REQUIRE(dev.get_compositor_info_count == 1);

		// Test begin_feature
		REQUIRE(dev.begin_feature_count == 0);
		xdev->begin_feature(xdev, XRT_DEVICE_FEATURE_FACE_TRACKING);
		REQUIRE(dev.begin_feature_count == 1);

		// Test end_feature
		REQUIRE(dev.end_feature_count == 0);
		xdev->end_feature(xdev, XRT_DEVICE_FEATURE_FACE_TRACKING);
		REQUIRE(dev.end_feature_count == 1);

		// Test destroy
		REQUIRE_FALSE(dev.destroyed);
		xdev->destroy(xdev);
		REQUIRE(dev.destroyed);
	}

	SECTION("fromXDev conversion works")
	{
		auto *retrieved = AllFeaturesTestDevice::fromXDev(xdev);
		REQUIRE(retrieved == &dev);
	}
}
