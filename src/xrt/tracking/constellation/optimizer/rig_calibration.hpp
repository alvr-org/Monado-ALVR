// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Constellation tracker rig calibration.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

// Included for XRT_FEATURE_RERUN, which the layout of RigCalibration depends on.
#include "xrt/xrt_config_build.h"

#include "os/os_threading.h"

#include "util/u_var.h"

#include "tracking/t_constellation.h"

#include "pose_optimize.hpp"
#include "math.hpp"

#include <vector>
#include <array>
#include <map>
#include <memory>


namespace xrt::tracking::constellation {

// Declared here rather than included, since the tracker's internal header is the one that includes this file.
struct CameraSample;
struct ConstellationTracker;

}; // namespace xrt::tracking::constellation

namespace xrt::tracking::constellation::optimizer::rig_calibration {

using namespace xrt::tracking::constellation;
namespace os = xrt::auxiliary::os;

/*!
 * One camera's view of one device at one exposure, as the LED correspondences the tracker labelled.
 */
struct DeviceCameraSample
{
public: // Fields
	uint32_t mosaic_idx;
	uint32_t camera_idx;

	timepoint_ns timestamp_ns;

	/*!
	 * The seed pose that the tracker recovered. This may not have any actual measurement taken into account.
	 */
	xrt_pose Tcv_cam_device_seed;

	uint32_t num_points;
	//! The 2D blob points
	std::array<xrt_vec2, XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE> points2d;
	//! The 3D blob points, in the device's frame. Same order as @ref points2d.
	std::array<xrt_vec3, XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE> points3d;
};

struct DeviceSamples
{
public: // Fields
	std::vector<DeviceCameraSample> camera_samples;
	std::vector<xrt_imu_sample> imu_samples;

public: // Methods
	DeviceSamples();
};

struct CameraDescription
{
public: // Fields
	size_t mosaic_idx;
	size_t camera_idx;

	//! Intrinsics and distortion, which is what turns a LED in the device's frame into a pixel.
	camera_model params;

	//! Where the camera sits in the world, in OpenCV space. Only read when @ref has_concrete_pose is set.
	xrt_pose Tcv_world_cam;

	/*!
	 * Whether the camera has a concrete pose, in which case @ref Tcv_world_cam is where it is held for the whole
	 * solve. If false, the camera's pose will be optimized for.
	 */
	bool has_concrete_pose;
};

//! The IMU intrinsics the calibration solved for.
struct ImuCalibration
{
public: // Fields
	//! Accelerometer bias, in meters per second squared.
	xrt_vec3_f64 accel_bias;
	//! Gyroscope bias, in radians per second.
	xrt_vec3_f64 gyro_bias;
	//! Per-axis accelerometer scale, unitless.
	xrt_vec3_f64 accel_scale;

	xrt_quat Qcv_imu_model;
};

//! The pose the calibration ended up with for a single camera.
struct CameraCalibration
{
public: // Fields
	size_t mosaic_idx;
	size_t camera_idx;

	//! The pose of the camera, in OpenCV space.
	xrt_pose Tcv_world_cam;

	//! Carried through from the description, so a consumer can reproject the observations this pose was fit to.
	camera_model params;

	//! False when the pose was handed to the optimizer as a constraint instead of being solved for.
	bool optimized;
	//! True when the camera was pinned at the origin with only its pitch and roll free.
	bool is_2dof;
};

//! The device state the calibration solved for at a single keyframe.
struct KeyframeCalibration
{
public: // Fields
	timepoint_ns timestamp_ns;

	//! The pose of the device, in OpenCV space.
	xrt_pose Tcv_world_device;
	//! The linear velocity of the device in world space, in meters per second, in OpenCV space.
	xrt_vec3 velocity_m_s;

	//! The camera observations which constrained this keyframe.
	std::vector<DeviceCameraSample> camera_samples;
};

//! The output of a successful @ref RigCalibration::optimize call.
struct CalibrationResult
{
public: // Fields
	//! The device whose samples the solve ran on.
	t_constellation_device_id_t device_id;

	ImuCalibration imu;
	//! The magnitude of gravity, in meters per second squared. Shared by every device the solve covered.
	double gravity_mag;
	std::vector<CameraCalibration> cameras;
	std::vector<KeyframeCalibration> keyframes;

	//! How well each kind of factor was fit at the solution.
	FactorFitness imu_fitness;
	FactorFitness camera_fitness;

	//! Solver cost before the solve, `1/2 ||f(x_0)||^2`.
	double initial_cost;
	//! Solver cost after the solve, `1/2 ||f(x)||^2`.
	double final_cost;
	//! How many iterations the solver took.
	int iterations;
};

struct RigCalibration
{
private: // Fields
	u_logging_level log_level;
	std::map<t_constellation_device_id_t, DeviceSamples> device_samples;
	std::vector<CameraDescription> cameras;
	mutable os::Mutex lock;
	bool collecting_data{false};

#ifdef XRT_FEATURE_RERUN
	std::unique_ptr<struct RerunContext> rerun_context{nullptr};
#endif

	u_var_button u_var_button_begin_collecting_data{};
	u_var_button u_var_button_end_collecting_data{};
	u_var_button u_var_button_optimize{};

public: // Methods
	RigCalibration();
	~RigCalibration();

	//! Adds a camera to the rig with an optional anchored world pose.
	void
	addCamera(const CameraDescription &camera_description);

	//! Begins collecting data for the rig optimization.
	void
	beginCollectingData();

	//! Ends collecting data for the rig optimization.
	void
	endCollectingData();

	//! Returns whether the rig calibration is currently collecting data.
	bool
	isCollectingData();

	//! Pushes a complete camera sample. Must be called with `tracker->device_lock` held.
	void
	pushCameraSample(ConstellationTracker *tracker, const CameraSample &sample);
	//! Pushes an IMU sample for this particular device, IMU samples should be in OpenXR coordinate space.
	void
	pushImuSample(t_constellation_device_id_t device_id, const xrt_imu_sample &sample);

	//! Runs the optimizer on the data that has been passed
	bool
	optimize(CalibrationResult &out_result);

	bool
	numSamplesForBestDevice(size_t &out_num_samples) const;

private: // Methods
	//! Must be called with `lock` already held.
	bool
	bestDeviceLocked(t_constellation_device_id_t &out_device) const;

	DeviceSamples &
	getDeviceSamplesLocked(t_constellation_device_id_t device_id);

	/*!
	 * Runs one full solve over the given samples, with every camera timestamp shifted by @p camera_time_offset_ns.
	 *
	 * Shifting is what makes the offset sweep possible: a constant error in the camera timestamps shows up as
	 * camera factors that cannot be reconciled with the IMU, so the offset which minimizes the camera fitness is
	 * the negative of that error.
	 *
	 * @param device_id             The device whose samples these are, only carried through into the result.
	 * @param camera_samples        The camera samples to solve against. Taken by value because the offset is
	 *                              applied to the copy and the ordering is reworked in place.
	 * @param imu_samples           The IMU samples to solve against, must already be sorted by timestamp.
	 * @param camera_time_offset_ns Added to every camera sample timestamp before anything else happens.
	 * @param[out] out_result       Written only when this returns true.
	 *
	 * @return True when the solve produced a usable solution.
	 */
	bool
	solveWithCameraTimeOffset(t_constellation_device_id_t device_id,
	                          const std::vector<CameraDescription> &cameras,
	                          std::vector<DeviceCameraSample> camera_samples,
	                          const std::vector<xrt_imu_sample> &imu_samples,
	                          time_duration_ns camera_time_offset_ns,
	                          CalibrationResult &out_result);
};

}; // namespace xrt::tracking::constellation::optimizer::rig_calibration
