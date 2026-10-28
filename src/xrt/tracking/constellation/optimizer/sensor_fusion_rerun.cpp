// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the rerun recorder logic for the sensor fusion.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "sensor_fusion_rerun.hpp"

#include "math/m_api.h"
#include "math/m_vec3.h"

#include "tracking/t_camera_models.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <string>
#include <vector>


using namespace xrt::auxiliary::tracking;
using namespace xrt::tracking::constellation;
using namespace xrt::tracking::constellation::optimizer;
using namespace xrt::tracking::constellation::optimizer::sensor_fusion;

// Anonymous namespace for internal functions.
namespace {

/*
 *
 * Common entity names
 *
 */

constexpr std::string kSummaryEntityName = "summary";
constexpr std::string kSolverMetricsEntityName = "metrics/solver";
constexpr std::string kSolveTimelineName = "solves";
constexpr std::string kTimeTimelineName = "fusion_time";

std::string
getCameraObservationEntityName(size_t mosaic_idx, size_t camera_idx)
{
	return std::format("{}/observed", getWorldCameraEntityName(mosaic_idx, camera_idx));
}

//! Lives in world space rather than under the camera, since the LEDs are placed by the device's solved pose.
std::string
getMatchedLedsEntityName(size_t mosaic_idx, size_t camera_idx)
{
	return std::format("{}/matched_leds/{}/{}", getWorldEntityName(), mosaic_idx, camera_idx);
}

std::string
getDeviceEntityName()
{
	return std::format("{}/device", getWorldEntityName());
}

//! Sibling of the device entity, so that it is not moved around by the device's per-solve transform.
std::string
getDeviceWindowEntityName()
{
	return std::format("{}/window", getWorldEntityName());
}

//! Child of the device entity, which is the solved IMU pose, so this carries only the IMU -> LED model rotation.
std::string
getDeviceModelEntityName()
{
	return std::format("{}/model", getDeviceEntityName());
}

std::string
getDeviceMetricsEntityName(t_constellation_device_id_t device_id)
{
	return std::format("metrics/devices/{}", device_id);
}

std::string
getCameraMetricsEntityName(size_t mosaic_idx, size_t camera_idx, t_constellation_device_id_t device_id)
{
	return std::format("metrics/cameras/{}/{}/{}", mosaic_idx, camera_idx, device_id);
}


/*
 *
 * Helper functions
 *
 */

//! The IMU -> LED model rotation as a rotation vector about the IMU's own axes, in degrees.
xrt_vec3
imuModelRotationVectorDegrees(const xrt_quat &Qcv_imu_model)
{
	// The log of a negative-w quaternion is the long way round, so flip it onto the short one first.
	xrt_quat q = Qcv_imu_model;
	if (q.w < 0.0f) {
		q = xrt_quat{-q.x, -q.y, -q.z, -q.w};
	}

	xrt_vec3 rotation_vector;
	math_quat_ln_so3(&q, &rotation_vector);

	return m_vec3_mul_scalar(rotation_vector, static_cast<float>(180.0 / M_PI));
}

}; // namespace

namespace xrt::tracking::constellation::optimizer::sensor_fusion {

/*
 *
 * Private methods
 *
 */

void
RerunContext::logStaticScene()
{
	// The whole solve happens in OpenCV space.
	this->stream->log_static("/", rerun::ViewCoordinates::RIGHT_HAND_Y_DOWN);
	this->stream->log_static(getWorldEntityName(), rerun::TransformAxes3D(kAxisLength));
}

void
RerunContext::logSolveSummary(const SolveSnapshot &snapshot)
{
	std::string text = std::format(        //
	    "# Sensor fusion solve {}\n"       //
	    "\n"                               //
	    "* Keyframes in window: {}\n"      //
	    "* Parameter blocks: {}\n"         //
	    "* Residuals: {}\n"                //
	    "* Iterations: {}\n"               //
	    "* Termination: {}\n"              //
	    "* Solve time: {:.1f} ms\n"        //
	    "* Gravity: {:.4f} m/s^2\n"        //
	    "* Late samples dropped: {}\n"     //
	    "\n"                               //
	    "## Fit\n"                         //
	    "\n"                               //
	    "* Initial RMS: {:.3f} sigma\n"    //
	    "* Final RMS: {:.3f} sigma\n"      //
	    "\n"                               //
	    "## Device\n"                      //
	    "\n",                              //
	    snapshot.solve_index,              //
	    snapshot.num_keyframes,            //
	    snapshot.num_parameter_blocks,     //
	    snapshot.num_residuals,            //
	    snapshot.num_iterations,           //
	    snapshot.termination,              //
	    snapshot.solve_time_s * 1000.0,    //
	    snapshot.world_gravity_mag,        //
	    snapshot.num_late_samples_dropped, //
	    snapshot.initial_rms_sigma,        //
	    snapshot.final_rms_sigma);         //

	text += std::format(                               //
	    "* Device {}: {} keyframes in solve\n",        //
	    snapshot.device.id,                            //
	    std::popcount(snapshot.device.keyframe_mask)); //

	const SnapshotDevice &device = snapshot.device;
	const xrt_vec3 imu_model_rotation_deg = imuModelRotationVectorDegrees(device.Qcv_imu_model);

	text += std::format(                                                                                 //
	    "* IMU -> model offset: {:.3f} deg, {:.3f} deg from calibration\n"                               //
	    "* IMU -> model rotation vector: {:.3f} {:.3f} {:.3f} deg\n"                                     //
	    "* Q_imu_model: {:.5f} {:.5f} {:.5f} {:.5f}\n",                                                  //
	    orientationErrorDegrees(XRT_QUAT_IDENTITY, device.Qcv_imu_model),                                //
	    orientationErrorDegrees(device.calibrated_Qcv_imu_model, device.Qcv_imu_model),                  //
	    imu_model_rotation_deg.x, imu_model_rotation_deg.y, imu_model_rotation_deg.z,                    //
	    device.Qcv_imu_model.x, device.Qcv_imu_model.y, device.Qcv_imu_model.z, device.Qcv_imu_model.w); //

	this->stream->log(kSummaryEntityName, rerun::TextDocument(text).with_media_type(rerun::MediaType::markdown()));
}

void
RerunContext::logSolverMetrics(const SolveSnapshot &snapshot)
{
	const std::string metrics = kSolverMetricsEntityName;

	/*
	 * The two RMS numbers are the whole story of a solve: where the seeds put it and where it got to. Both are in
	 * standard deviations, so they stay comparable as the window's residual count moves around.
	 */
	this->stream->log(metrics + "/initial_rms_sigma", rerun::Scalars(snapshot.initial_rms_sigma));
	this->stream->log(metrics + "/final_rms_sigma", rerun::Scalars(snapshot.final_rms_sigma));
	this->stream->log(metrics + "/iterations", rerun::Scalars(static_cast<double>(snapshot.num_iterations)));
	this->stream->log(metrics + "/solve_time_ms", rerun::Scalars(snapshot.solve_time_s * 1000.0));
	this->stream->log(metrics + "/residuals", rerun::Scalars(static_cast<double>(snapshot.num_residuals)));
	this->stream->log(metrics + "/parameter_blocks",
	                  rerun::Scalars(static_cast<double>(snapshot.num_parameter_blocks)));
	this->stream->log(metrics + "/keyframes", rerun::Scalars(static_cast<double>(snapshot.num_keyframes)));
	this->stream->log(metrics + "/gravity_m_s2", rerun::Scalars(snapshot.world_gravity_mag));
	this->stream->log(metrics + "/late_samples_dropped",
	                  rerun::Scalars(static_cast<double>(snapshot.num_late_samples_dropped)));
}

void
RerunContext::logCameras(const SolveSnapshot &snapshot)
{
	/*
	 * Camera poses arrive on the observations rather than from a fixed table, they might move if they're attached
	 * to a device, and so we pull the keyframe's observations and log the cameras in the solve snapshot. Newer data
	 * overwrites older data, so we read keyframes in order of their age (oldest first).
	 */
	for (int64_t j = (int64_t)snapshot.num_keyframes - 1L; j >= 0; j--) {
		for (uint32_t i = 0; i < snapshot.keyframes[j].num_observations; i++) {
			const SnapshotObservation &observation = snapshot.keyframes[j].observations[i];
			const std::string camera_entity =
			    getWorldCameraEntityName(observation.mosaic_idx, observation.camera_idx);

			this->stream->log(camera_entity, toRerunTransform(observation.Tcv_world_cam));
			this->stream->log(camera_entity + "/axes", rerun::TransformAxes3D(kAxisLength));
		}
	}
}

void
RerunContext::logDevice(const SolveSnapshot &snapshot)
{
	const SnapshotDevice &device = snapshot.device;
	const SolvedDeviceKeyframe &newest = snapshot.keyframes[0].device;

	if (!(device.keyframe_mask & 1)) {
		return;
	}

	const std::string device_entity = getDeviceEntityName();

	this->stream->log(device_entity, toRerunTransform(newest.Tcv_world_device));
	this->stream->log(device_entity + "/axes", rerun::TransformAxes3D(kAxisLength));

	/*
	 * The device entity is the solved IMU pose, and the camera factors see the LED model through this rotation on
	 * top of it, so the angle between these axes and the device's is the offset the solve is holding. Drawn longer
	 * so the two stay tellable apart when the offset is small.
	 */
	xrt_pose T_imu_model = XRT_POSE_IDENTITY;
	T_imu_model.orientation = device.Qcv_imu_model;

	const std::string model_entity = getDeviceModelEntityName();

	this->stream->log(model_entity, toRerunTransform(T_imu_model));
	this->stream->log(model_entity + "/axes", rerun::TransformAxes3D(kAxisLength * 1.5f));

	/*
	 * The velocity is in world space but the device entity carries the device's own transform, so the arrow is
	 * drawn from the window entity, which does not move.
	 */
	this->stream->log(                             //
	    getDeviceWindowEntityName() + "/velocity", //
	    rerun::Arrows3D::from_vectors(             //
	        {
	            rerun::Vector3D(static_cast<float>(newest.velocity.x), //
	                            static_cast<float>(newest.velocity.y), //
	                            static_cast<float>(newest.velocity.z)),
	        }) //
	        .with_origins({
	            rerun::Position3D(newest.Tcv_world_device.position.x, //
	                              newest.Tcv_world_device.position.y, //
	                              newest.Tcv_world_device.position.z),
	        })                                           //
	        .with_colors({deviceColor(device.id, 1.0f)}) //
	);                                                   //
}

void
RerunContext::logDeviceWindow(const SolveSnapshot &snapshot)
{
	if (!kLogOldKeyframes) {
		return;
	}

	const SnapshotDevice &device = snapshot.device;

	std::vector<rerun::Position3D> positions;
	std::vector<rerun::Position3D> strip_points;

	// Oldest first, so the strip runs the way the device actually travelled.
	for (size_t i = snapshot.keyframes.size(); i-- > 0;) {
		const SolvedDeviceKeyframe &solved = snapshot.keyframes[i].device;

		// Skip keyframes that aren't in the solve
		if (!(snapshot.device.keyframe_mask & (1u << i))) {
			continue;
		}

		const xrt_vec3 &position = solved.Tcv_world_device.position;
		positions.emplace_back(position.x, position.y, position.z);
		strip_points.emplace_back(position.x, position.y, position.z);
	}

	const std::string window_entity = getDeviceWindowEntityName();

	if (positions.empty()) {
		this->stream->log(window_entity + "/line", rerun::Clear::RECURSIVE);
		this->stream->log(window_entity + "/points", rerun::Clear::RECURSIVE);
		return;
	}

	// This is the window, not a trajectory: every state in it is still being solved for, and re-logging the whole
	// thing each solve is what makes the older states visibly settle as they pick up more constraints.
	this->stream->log(                                          //
	    window_entity + "/line",                                //
	    rerun::LineStrips3D({rerun::LineStrip3D(strip_points)}) //
	        .with_radii({kTrajectoryLineRadiusM})               //
	        .with_colors({deviceColor(device.id, 0.5f)})        //
	);                                                          //
	this->stream->log(                                          //
	    window_entity + "/points",                              //
	    rerun::Points3D(positions)                              //
	        .with_radii({kTrajectoryPointRadiusM})              //
	        .with_colors({deviceColor(device.id, 1.0f)})        //
	);                                                          //
}

void
RerunContext::logDeviceMetrics(const SolveSnapshot &snapshot)
{
	const SnapshotDevice &device = snapshot.device;
	const SolvedDeviceKeyframe &newest = snapshot.keyframes[0].device;

	if (!(device.keyframe_mask & 1)) {
		return;
	}

	const std::string metrics = getDeviceMetricsEntityName(device.id);

	this->stream->log(metrics + "/states_in_solve",
	                  rerun::Scalars(static_cast<double>(std::popcount(device.keyframe_mask))));

	this->stream->log(metrics + "/speed_m_s", rerun::Scalars(map_vec3_f64(newest.velocity).norm()));

	// The biases are what the IMU factors are really solving for, and they are the first thing to go somewhere
	// unphysical when the window is too weak to constrain them.
	this->stream->log(metrics + "/accel_bias_m_s2", rerun::Scalars(map_vec3_f64(newest.accel_bias).norm()));
	this->stream->log(metrics + "/gyro_bias_rad_s", rerun::Scalars(map_vec3_f64(newest.gyro_bias).norm()));

	// Per-axis, so these can be read straight against what the rig calibration reported.
	this->stream->log(metrics + "/accel_bias_body_x", rerun::Scalars(newest.accel_bias.x));
	this->stream->log(metrics + "/accel_bias_body_y", rerun::Scalars(newest.accel_bias.y));
	this->stream->log(metrics + "/accel_bias_body_z", rerun::Scalars(newest.accel_bias.z));

	/*
	 * The same bias rotated into the world.
	 *
	 * Accelerometer bias is a body-frame quantity, so a correct one is constant in the body frame and swings in
	 * the world as the device turns. Anything the bias is absorbing that is really world-frame, which is likely a
	 * gravity direction the solve cannot express. If you're seeing bias swing with world frame, there's probably
	 * some missing transform or something.
	 */
	const xrt_vec3 body_bias{
	    static_cast<float>(newest.accel_bias.x), //
	    static_cast<float>(newest.accel_bias.y), //
	    static_cast<float>(newest.accel_bias.z),
	};

	xrt_vec3 world_bias;
	math_quat_rotate_vec3(&newest.Tcv_world_device.orientation, &body_bias, &world_bias);

	this->stream->log(metrics + "/accel_bias_world_x", rerun::Scalars(world_bias.x));
	this->stream->log(metrics + "/accel_bias_world_y", rerun::Scalars(world_bias.y));
	this->stream->log(metrics + "/accel_bias_world_z", rerun::Scalars(world_bias.z));
	this->stream->log(metrics + "/accel_scale_x", rerun::Scalars(newest.accel_scale.x));
	this->stream->log(metrics + "/accel_scale_y", rerun::Scalars(newest.accel_scale.y));
	this->stream->log(metrics + "/accel_scale_z", rerun::Scalars(newest.accel_scale.z));

	// This *probably* shouldn't be moving much unless we're missing some large calibration step.
	this->stream->log(metrics + "/imu_model_offset_deg",
	                  rerun::Scalars(orientationErrorDegrees(XRT_QUAT_IDENTITY, device.Qcv_imu_model)));

	// How far the solve has walked from the rig calibration it was seeded with.
	this->stream->log(
	    metrics + "/imu_model_calibration_offset_deg",
	    rerun::Scalars(orientationErrorDegrees(device.calibrated_Qcv_imu_model, device.Qcv_imu_model)));

	/*
	 * Per-axis, about the IMU's own axes. The angle alone says there is an offset, this says which way it
	 * points: an error in how the driver tilts the LED model into the IMU frame shows up almost entirely on one
	 * axis.
	 */
	const xrt_vec3 imu_model_rotation_deg = imuModelRotationVectorDegrees(device.Qcv_imu_model);

	this->stream->log(metrics + "/imu_model_rotation_x_deg", rerun::Scalars(imu_model_rotation_deg.x));
	this->stream->log(metrics + "/imu_model_rotation_y_deg", rerun::Scalars(imu_model_rotation_deg.y));
	this->stream->log(metrics + "/imu_model_rotation_z_deg", rerun::Scalars(imu_model_rotation_deg.z));
}

void
RerunContext::logObservations(const SolveSnapshot &snapshot)
{
	/*
	 * Only the newest keyframe: the older ones in the window were drawn on the solves they were newest for, and
	 * stacking every keyframe's blobs would bury the current one.
	 */
	const SnapshotKeyframe &keyframe = snapshot.keyframes[0];

	if (!(snapshot.device.keyframe_mask & 1)) {
		return;
	}

	/*
	 * The camera factors project the LEDs through the solved IMU -> model rotation, so everything below is measured
	 * against the model pose that produces, not the bare IMU pose the keyframe holds. The PnP seed is a model pose
	 * as well, so this is also what makes the seed comparison like for like.
	 */
	const xrt_pose Tcv_world_model =
	    solvedWorldModelPose(keyframe.device.Tcv_world_device, snapshot.device.Qcv_imu_model);

	for (uint32_t i = 0; i < keyframe.num_observations; i++) {
		const SnapshotObservation &observation = keyframe.observations[i];
		const SnapshotDevice &device = snapshot.device;

		const std::string observation_entity =
		    getCameraObservationEntityName(observation.mosaic_idx, observation.camera_idx);
		const std::string matched_leds_entity =
		    getMatchedLedsEntityName(observation.mosaic_idx, observation.camera_idx);

		// Where the solve says this camera is looking at the LED model, as a pose under the camera entity.
		const xrt_pose Tcv_cam_model = predictedCameraDevicePose(observation.Tcv_world_cam, Tcv_world_model);

		this->stream->log(observation_entity, toRerunTransform(Tcv_cam_model));
		this->stream->log(observation_entity + "/axes", rerun::TransformAxes3D(kAxisLength));

		/*
		 * The LEDs this camera matched, placed in the world by the solved pose. Where these sit relative to the
		 * device's model is the visual form of the reprojection error logged below.
		 */
		std::vector<rerun::Position3D> led_positions;
		led_positions.reserve(observation.num_points);

		for (uint32_t i = 0; i < observation.num_points; i++) {
			const xrt_vec3 &T_model_led = observation.points3d[i];
			xrt_vec3 T_world_led;
			math_quat_rotate_vec3(&Tcv_world_model.orientation, &T_model_led, &T_world_led);
			T_world_led = T_world_led + Tcv_world_model.position;

			led_positions.emplace_back(T_world_led.x, T_world_led.y, T_world_led.z);
		}

		this->stream->log(                                                                        //
		    matched_leds_entity,                                                                  //
		    rerun::Points3D(led_positions)                                                        //
		        .with_radii({kLedPointRadiusM})                                                   //
		        .with_colors({cameraColor(observation.mosaic_idx, observation.camera_idx, 1.0f)}) //
		);                                                                                        //

		const std::string camera_metrics =
		    getCameraMetricsEntityName(observation.mosaic_idx, observation.camera_idx, device.id);

		const t_camera_model_params &dist =
		    kOptimizeUndistortedPoints ? observation.params.calib_pinhole : observation.params.calib_true;

		float reprojection_error = reprojectionErrorPixels( //
		    dist,                                           //
		    observation.Tcv_world_cam,                      //
		    Tcv_world_model,                                //
		    observation.num_points,                         //
		    observation.points2d,                           //
		    observation.points3d);                          //

		this->stream->log(camera_metrics + "/reprojection_error_px", rerun::Scalars(reprojection_error));
		this->stream->log(camera_metrics + "/correspondence_count",
		                  rerun::Scalars(static_cast<double>(observation.num_points)));

		/*
		 * Measure how far the solve was from it's seed, if this is large ,there was either a bad seed or a bad
		 * solve.
		 */
		const float position_error_m =
		    m_vec3_len(Tcv_cam_model.position - observation.Tcv_cam_device_seed.position);
		const float orientation_error_deg =
		    orientationErrorDegrees(observation.Tcv_cam_device_seed.orientation, Tcv_cam_model.orientation);

		this->stream->log(camera_metrics + "/seed_position_error_m", rerun::Scalars(position_error_m));
		this->stream->log(camera_metrics + "/seed_orientation_error_deg",
		                  rerun::Scalars(orientation_error_deg));
	}
}

/*
 *
 * Public methods
 *
 */

void
RerunContext::logSolve(const SolveSnapshot &snapshot)
{
	// A solve with an empty window has nothing to say, and every helper below indexes keyframe 0.
	if (snapshot.num_keyframes == 0) {
		return;
	}

	this->logStaticScene();

	/*
	 * The sequence is what advances on every solve; the timestamp is the exposure the newest keyframe came from.
	 * Keeping both means the same recording can be read either as "one step per solve" or against wall time,
	 * which matters because solves are not evenly spaced.
	 */
	this->stream->set_time_timestamp_nanos_since_epoch(kTimeTimelineName, snapshot.keyframes[0].timestamp_ns);
	this->stream->set_time_sequence(kSolveTimelineName, static_cast<int64_t>(snapshot.solve_index));

	this->logSolveSummary(snapshot);
	this->logSolverMetrics(snapshot);
	this->logCameras(snapshot);
	this->logObservations(snapshot);

	this->logDevice(snapshot);
	this->logDeviceWindow(snapshot);
	this->logDeviceMetrics(snapshot);

	this->stream->reset_time();
}

}; // namespace xrt::tracking::constellation::optimizer::sensor_fusion
