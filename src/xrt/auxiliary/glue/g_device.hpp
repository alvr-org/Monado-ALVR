// Copyright 2024-2026, NVIDIA CORPORATION.
// Copyright 2026, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for glue classes to wrap XRT device interfaces.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_device.h"
#include "util/u_device.h"
#include "util/u_device_id.h"
#include "g_catch_guard.hpp"
#include "g_traits.hpp"

#include <type_traits>


namespace xrt::util {

/*!
 * Selects which functions that the @ref DeviceBase wrapper should set.
 */
struct DeviceFunctions
{
	/*!
	 * @ref xrt_device::get_view_poses
	 * @ref xrt_device::is_form_factor_available
	 */
	bool hmd{false};

	//! @ref xrt_device::compute_distortion
	bool distortion{false};

	//! @ref xrt_device::get_visibility_mask
	bool visibility_mask{false};

	//! @ref xrt_device::update_inputs
	bool update_inputs{false};

	//! @ref xrt_device::set_output
	bool output{false};

	//! @ref xrt_device::get_output_limits
	bool output_limits{false};

	//! @ref xrt_device::get_hand_tracking
	bool hand_tracking{false};

	//! @ref xrt_device::get_face_tracking
	bool face_tracking{false};

	//! @ref xrt_device::get_face_calibration_state_android
	bool face_calibration_android{false};

	/*!
	 * @ref xrt_device::get_body_skeleton
	 * @ref xrt_device::get_body_joints
	 * @ref xrt_device::reset_body_tracking_calibration_meta
	 * @ref xrt_device::set_body_tracking_calibration_override_meta
	 */
	bool body_tracking{false};

	/*!
	 * @ref xrt_device::begin_plane_detection_ext
	 * @ref xrt_device::destroy_plane_detection_ext
	 * @ref xrt_device::get_plane_detection_state_ext
	 * @ref xrt_device::get_plane_detections_ext
	 */
	bool plane_detection{false};

	//! @ref xrt_device::ref_space_usage
	bool reference_space{false};

	//! @ref xrt_device::get_battery_status
	bool battery{false};

	/*!
	 * @ref xrt_device::get_brightness
	 * @ref xrt_device::set_brightness
	 */
	bool brightness{false};

	//! @ref xrt_device::get_compositor_info
	bool compositor_info{false};

	/*!
	 * @ref xrt_device::begin_feature
	 * @ref xrt_device::end_feature
	 */
	bool features{false};

	/*!
	 * @ref xrt_device::notify_chirality
	 */
	bool notify_chirality{false};
};

/*!
 * CRTP glue wrapper for @ref xrt_device. Relies on standard layout to recover
 * the derived object from the C struct, and has some requirements and
 * limitations because of that. See @ref cpp-glue-wrappers for the guide and
 * conventions for these wrappers.
 *
 * Unlike the other wrappers it wires function pointers selectively: the
 * `functions` @ref DeviceFunctions bitmask decides which C function pointers
 * are installed, so `T` only has to implement C++ methods for the features it
 * turns on.
 */
template <class T, DeviceFunctions functions> class DeviceBase
{
public: // Members
	/*!
	 * Fully resets and sets function pointers on @ref xrt_device.
	 */
	DeviceBase() noexcept
	{
		static_assert(std::is_standard_layout_v<DeviceBase>,
		              "glue base must be standard layout for pointer recovery");
		static_assert(is_non_virtual_base_v<DeviceBase, T>,
		              "glue base must be a non-virtual base of T for pointer recovery");

		// Setup function for the device.
		auto &xdev = *getXDev();

		u_device_id_assign(&xdev);

		// Inits all functions, some are replaced below.
		u_device_populate_function_pointers(&xdev, getTrackedPoseWrap, destroyDeviceWrap);

		if constexpr (functions.hmd) {
			xdev.get_view_poses = getViewPosesWrap;
			xdev.is_form_factor_available = isFormFactorAvailableWrap;
		}

		if constexpr (functions.distortion) {
			xdev.compute_distortion = computeDistortionWrap;
		}

		if constexpr (functions.visibility_mask) {
			xdev.get_visibility_mask = getVisibilityMaskWrap;
		}

		if constexpr (functions.update_inputs) {
			xdev.update_inputs = updateInputsWrap;
		}

		if constexpr (functions.output) {
			xdev.set_output = setOutputWrap;
		}

		if constexpr (functions.output_limits) {
			xdev.get_output_limits = getOutputLimitsWrap;
		}

		if constexpr (functions.hand_tracking) {
			xdev.get_hand_tracking = getHandTrackingWrap;
		}

		if constexpr (functions.face_tracking) {
			xdev.get_face_tracking = getFaceTrackingWrap;
		}

		if constexpr (functions.face_calibration_android) {
			xdev.get_face_calibration_state_android = getFaceCalibrationStateAndroidWrap;
		}

		if constexpr (functions.body_tracking) {
			xdev.get_body_skeleton = getBodySkeletonWrap;
			xdev.get_body_joints = getBodyJointsWrap;
			xdev.reset_body_tracking_calibration_meta = resetBodyTrackingCalibrationMetaWrap;
			xdev.set_body_tracking_calibration_override_meta = setBodyTrackingCalibrationOverrideMetaWrap;
		}

		if constexpr (functions.plane_detection) {
			xdev.begin_plane_detection_ext = beginPlaneDetectionExtWrap;
			xdev.destroy_plane_detection_ext = destroyPlaneDetectionExtWrap;
			xdev.get_plane_detection_state_ext = getPlaneDetectionStateExtWrap;
			xdev.get_plane_detections_ext = getPlaneDetectionsExtWrap;
		}

		if constexpr (functions.reference_space) {
			xdev.ref_space_usage = refSpaceUsageWrap;
		}

		if constexpr (functions.battery) {
			xdev.get_battery_status = getBatteryStatusWrap;
		}

		if constexpr (functions.brightness) {
			xdev.get_brightness = getBrightnessWrap;
			xdev.set_brightness = setBrightnessWrap;
		}

		if constexpr (functions.compositor_info) {
			xdev.get_compositor_info = getCompositorInfoWrap;
		}

		if constexpr (functions.features) {
			xdev.begin_feature = beginFeatureWrap;
			xdev.end_feature = endFeatureWrap;
		}

		if constexpr (functions.notify_chirality) {
			xdev.notify_chirality = notifyChiralityWrap;
		}
	}

	/*!
	 * Destructor
	 */
	~DeviceBase() noexcept = default;

	const T &
	derived() const noexcept
	{
		return static_cast<const T &>(*this);
	}

	T &
	derived() noexcept
	{
		return static_cast<T &>(*this);
	}

	//! Gets the pointer to the derived class from a @ref xrt_device.
	static const T *
	fromXDev(const xrt_device *xdev) noexcept
	{
		return &(reinterpret_cast<const DeviceBase *>(xdev)->derived());
	}

	//! Gets the pointer to the derived class from a @ref xrt_device.
	static T *
	fromXDev(xrt_device *xdev) noexcept
	{
		return &(reinterpret_cast<DeviceBase *>(xdev)->derived());
	}

	//! Gets the underlying xrt_device pointer.
	const xrt_device *
	getXDev() const noexcept
	{
		return &mDevice;
	}

	//! Gets the underlying xrt_device pointer.
	xrt_device *
	getXDev() noexcept
	{
		return &mDevice;
	}


private: // Fields
	/*!
	 * Wrapped @ref xrt_device. Must be the first data member: a pointer to it is
	 * then interconvertible with a pointer to this standard-layout base, which
	 * lets the glue cast a C pointer back to the derived C++ class. See
	 * @ref cpp-glue-wrappers.
	 */
	xrt_device mDevice = {};


private: // Functions
#define GET(xdev) (fromXDev(xdev)->derived())

	static xrt_result_t
	updateInputsWrap(struct xrt_device *xdev) noexcept
	try {
		return GET(xdev).updateInputs();
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getTrackedPoseWrap(struct xrt_device *xdev,
	                   enum xrt_input_name name,
	                   int64_t at_timestamp_ns,
	                   struct xrt_space_relation *out_relation) noexcept
	try {
		return GET(xdev).getTrackedPose(name, at_timestamp_ns, out_relation);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getHandTrackingWrap(struct xrt_device *xdev,
	                    enum xrt_input_name name,
	                    int64_t desired_timestamp_ns,
	                    struct xrt_hand_joint_set *out_value,
	                    int64_t *out_timestamp_ns) noexcept
	try {
		return GET(xdev).getHandTracking(name, desired_timestamp_ns, out_value, out_timestamp_ns);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getFaceTrackingWrap(struct xrt_device *xdev,
	                    enum xrt_input_name facial_expression_type,
	                    int64_t at_timestamp_ns,
	                    struct xrt_facial_expression_set *out_value) noexcept
	try {
		return GET(xdev).getFaceTracking(facial_expression_type, at_timestamp_ns, out_value);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getFaceCalibrationStateAndroidWrap(struct xrt_device *xdev, bool *out_face_is_calibrated) noexcept
	try {
		return GET(xdev).getFaceCalibrationStateAndroid(out_face_is_calibrated);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getBodySkeletonWrap(struct xrt_device *xdev,
	                    enum xrt_input_name body_tracking_type,
	                    struct xrt_body_skeleton *out_value) noexcept
	try {
		return GET(xdev).getBodySkeleton(body_tracking_type, out_value);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getBodyJointsWrap(struct xrt_device *xdev,
	                  enum xrt_input_name body_tracking_type,
	                  int64_t desired_timestamp_ns,
	                  struct xrt_body_joint_set *out_value) noexcept
	try {
		return GET(xdev).getBodyJoints(body_tracking_type, desired_timestamp_ns, out_value);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	resetBodyTrackingCalibrationMetaWrap(struct xrt_device *xdev) noexcept
	try {
		return GET(xdev).resetBodyTrackingCalibrationMeta();
	}
	G_CATCH_GUARDS

	static xrt_result_t
	setBodyTrackingCalibrationOverrideMetaWrap(struct xrt_device *xdev, float new_body_height) noexcept
	try {
		return GET(xdev).setBodyTrackingCalibrationOverrideMeta(new_body_height);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	setOutputWrap(struct xrt_device *xdev, enum xrt_output_name name, const struct xrt_output_value *value) noexcept
	try {
		return GET(xdev).setOutput(name, value);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getOutputLimitsWrap(struct xrt_device *xdev, struct xrt_output_limits *limits) noexcept
	try {
		return GET(xdev).getOutputLimits(limits);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	beginPlaneDetectionExtWrap(struct xrt_device *xdev,
	                           const struct xrt_plane_detector_begin_info_ext *begin_info,
	                           uint64_t plane_detection_id,
	                           uint64_t *out_plane_detection_id) noexcept
	try {
		return GET(xdev).beginPlaneDetectionExt(begin_info, plane_detection_id, out_plane_detection_id);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	destroyPlaneDetectionExtWrap(struct xrt_device *xdev, uint64_t plane_detection_id) noexcept
	try {
		return GET(xdev).destroyPlaneDetectionExt(plane_detection_id);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getPlaneDetectionStateExtWrap(struct xrt_device *xdev,
	                              uint64_t plane_detection_id,
	                              enum xrt_plane_detector_state_ext *out_state) noexcept
	try {
		return GET(xdev).getPlaneDetectionStateExt(plane_detection_id, out_state);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getPlaneDetectionsExtWrap(struct xrt_device *xdev,
	                          uint64_t plane_detection_id,
	                          struct xrt_plane_detections_ext *out_detections) noexcept
	try {
		return GET(xdev).getPlaneDetectionsExt(plane_detection_id, out_detections);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getViewPosesWrap(struct xrt_device *xdev,
	                 const struct xrt_vec3 *default_eye_relation,
	                 int64_t at_timestamp_ns,
	                 enum xrt_view_type view_type,
	                 uint32_t view_count,
	                 struct xrt_space_relation *out_head_relation,
	                 struct xrt_fov *out_fovs,
	                 struct xrt_pose *out_poses) noexcept
	try {
		return GET(xdev).getViewPoses(default_eye_relation, at_timestamp_ns, view_type, view_count,
		                              out_head_relation, out_fovs, out_poses);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	computeDistortionWrap(
	    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result) noexcept
	try {
		return GET(xdev).computeDistortion(view, u, v, out_result);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getVisibilityMaskWrap(struct xrt_device *xdev,
	                      enum xrt_visibility_mask_type type,
	                      uint32_t view_index,
	                      struct xrt_visibility_mask **out_mask) noexcept
	try {
		return GET(xdev).getVisibilityMask(type, view_index, out_mask);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	refSpaceUsageWrap(struct xrt_device *xdev,
	                  enum xrt_reference_space_type type,
	                  enum xrt_input_name name,
	                  bool used) noexcept
	try {
		return GET(xdev).refSpaceUsage(type, name, used);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	isFormFactorAvailableWrap(struct xrt_device *xdev,
	                          enum xrt_form_factor form_factor,
	                          bool *out_available) noexcept
	try {
		return GET(xdev).isFormFactorAvailable(form_factor, out_available);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getBatteryStatusWrap(struct xrt_device *xdev, bool *out_present, bool *out_charging, float *out_charge) noexcept
	try {
		return GET(xdev).getBatteryStatus(out_present, out_charging, out_charge);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getBrightnessWrap(struct xrt_device *xdev, float *out_brightness) noexcept
	try {
		return GET(xdev).getBrightness(out_brightness);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	setBrightnessWrap(struct xrt_device *xdev, float brightness, bool relative) noexcept
	try {
		return GET(xdev).setBrightness(brightness, relative);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getCompositorInfoWrap(struct xrt_device *xdev,
	                      const struct xrt_device_compositor_mode *mode,
	                      struct xrt_device_compositor_info *out_info) noexcept
	try {
		return GET(xdev).getCompositorInfo(mode, out_info);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	beginFeatureWrap(struct xrt_device *xdev, enum xrt_device_feature_type type) noexcept
	try {
		return GET(xdev).beginFeature(type);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	endFeatureWrap(struct xrt_device *xdev, enum xrt_device_feature_type type) noexcept
	try {
		return GET(xdev).endFeature(type);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	notifyChiralityWrap(struct xrt_device *xdev, bool has_chirality, enum xrt_hand chirality) noexcept
	try {
		return GET(xdev).notifyChirality(has_chirality, chirality);
	}
	G_CATCH_GUARDS

	static void
	destroyDeviceWrap(struct xrt_device *xdev) noexcept
	try {
		T::destroyDevice(xdev);
	}
	G_CATCH_GUARDS_VOID


#undef GET
};

} // namespace xrt::util
