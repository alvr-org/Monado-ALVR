// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the rerun recorder logic for the rig calibration.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "rig_calibration_rerun.hpp"

#include "math/m_api.h"
#include "math/m_vec3.h"

#include "tracking/t_camera_models.hpp"

#include "constellation/optimizer/internal_math.hpp"

#include "util/u_logging.h"

#include <cmath>
#include <format>
#include <string>
#include <vector>


using namespace xrt::auxiliary::tracking;
using namespace xrt::tracking::constellation;
using namespace xrt::tracking::constellation::optimizer;
using namespace xrt::tracking::constellation::optimizer::rig_calibration;

// Anonymous namespace for internal functions.
namespace {

/*
 *
 * Helper functions
 *
 */

//! This camera's view of the device at this keyframe, or null when it did not see it.
const DeviceCameraSample *
findObservation(const KeyframeCalibration &keyframe, const CameraCalibration &camera)
{
	for (const DeviceCameraSample &camera_sample : keyframe.camera_samples) {
		if (camera_sample.mosaic_idx == camera.mosaic_idx && camera_sample.camera_idx == camera.camera_idx) {
			return &camera_sample;
		}
	}

	return nullptr;
}

/*
 *
 * Common entity names
 *
 */

std::string
getCameraObservationEntityName(size_t mosaic_idx, size_t camera_idx)
{
	return std::format("{}/observed_device", getWorldCameraEntityName(mosaic_idx, camera_idx));
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

//! Sibling of the device entity, so that it is not moved around by the device's per-keyframe transform.
std::string
getTrajectoryEntityName()
{
	return std::format("{}/trajectory", getWorldEntityName());
}

std::string
getSummaryEntityName()
{
	return "summary";
}

std::string
getDeviceMetricsEntityName()
{
	return "metrics/device";
}

std::string
getCameraMetricsEntityName(size_t mosaic_idx, size_t camera_idx)
{
	return std::format("metrics/cameras/{}/{}", mosaic_idx, camera_idx);
}

std::string
getKeyframeMetricsEntityName()
{
	return "metrics/keyframe";
}

std::string
getTimelineName()
{
	return "keyframes";
}

}; // namespace

namespace xrt::tracking::constellation::optimizer::rig_calibration {

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
	this->stream->log_static(getDeviceEntityName() + "/axes", rerun::TransformAxes3D(kAxisLength));
}

void
RerunContext::logSolveSummary(const CalibrationResult &result)
{
	const ImuCalibration &imu = result.imu;

	std::string text = std::format(      //
	    "# Offline sensor calibration\n" //
	    "\n"                             //
	    "* Device: {}\n"                 //
	    "* Keyframes: {}\n"              //
	    "* Iterations: {}\n"             //
	    "* Initial cost: {:g}\n"         //
	    "* Final cost: {:g}\n"           //
	    "\n"                             //
	    "## Fit\n"                       //
	    "\n"                             //
	    "Both factors are whitened, so these are in standard deviations and 1.0 is a model which\n"
	    "explains its data.\n"                                                              //
	    "\n"                                                                                //
	    "| factor | residuals | chi^2 | chi^2/residual | rms sigma |\n"                     //
	    "| --- | --- | --- | --- | --- |\n"                                                 //
	    "| IMU | {} | {:g} | {:g} | {:g} |\n"                                               //
	    "| Camera | {} | {:g} | {:g} | {:g} |\n"                                            //
	    "\n"                                                                                //
	    "## IMU\n"                                                                          //
	    "\n"                                                                                //
	    "* Q_imu_model: {:.5f} {:.5f} {:.5f} {:.5f} m/s^2\n"                                //
	    "* Accel bias: {:.5f} {:.5f} {:.5f} m/s^2\n"                                        //
	    "* Gyro bias: {:.5f} {:.5f} {:.5f} rad/s\n"                                         //
	    "* Accel scale: {:.5f} {:.5f} {:.5f}\n"                                             //
	    "* Gravity: {:.5f} m/s^2\n"                                                         //
	    "\n"                                                                                //
	    "## Cameras\n"                                                                      //
	    "\n",                                                                               //
	    result.device_id,                                                                   //
	    result.keyframes.size(),                                                            //
	    result.iterations,                                                                  //
	    result.initial_cost,                                                                //
	    result.final_cost,                                                                  //
	    result.imu_fitness.num_residuals,                                                   //
	    result.imu_fitness.chi_squared,                                                     //
	    result.imu_fitness.meanChiSquared(),                                                //
	    result.imu_fitness.rmsSigma(),                                                      //
	    result.camera_fitness.num_residuals,                                                //
	    result.camera_fitness.chi_squared,                                                  //
	    result.camera_fitness.meanChiSquared(),                                             //
	    result.camera_fitness.rmsSigma(),                                                   //
	    imu.Qcv_imu_model.x, imu.Qcv_imu_model.y, imu.Qcv_imu_model.z, imu.Qcv_imu_model.w, //
	    imu.accel_bias.x, imu.accel_bias.y, imu.accel_bias.z,                               //
	    imu.gyro_bias.x, imu.gyro_bias.y, imu.gyro_bias.z,                                  //
	    imu.accel_scale.x, imu.accel_scale.y, imu.accel_scale.z,                            //
	    result.gravity_mag);                                                                //

	for (const CameraCalibration &camera : result.cameras) {
		const char *kind = camera.is_2dof ? "2dof" : (camera.optimized ? "solved" : "fixed");

		text += std::format(                                                                            //
		    "* `{}/{}` ({}): position {:.5f} {:.5f} {:.5f}, orientation {:.5f} {:.5f} {:.5f} {:.5f}\n", //
		    camera.mosaic_idx,                                                                          //
		    camera.camera_idx,                                                                          //
		    kind,                                                                                       //
		    camera.Tcv_world_cam.position.x,                                                            //
		    camera.Tcv_world_cam.position.y,                                                            //
		    camera.Tcv_world_cam.position.z,                                                            //
		    camera.Tcv_world_cam.orientation.x,                                                         //
		    camera.Tcv_world_cam.orientation.y,                                                         //
		    camera.Tcv_world_cam.orientation.z,                                                         //
		    camera.Tcv_world_cam.orientation.w);                                                        //
	}

	this->stream->log_static(getSummaryEntityName(),
	                         rerun::TextDocument(text).with_media_type(rerun::MediaType::markdown()));
}

void
RerunContext::logCameras(const CalibrationResult &result)
{
	for (const CameraCalibration &camera : result.cameras) {
		std::string camera_entity = getWorldCameraEntityName(camera.mosaic_idx, camera.camera_idx);

		this->stream->log_static(camera_entity, toRerunTransform(camera.Tcv_world_cam));
		this->stream->log_static(camera_entity + "/axes", rerun::TransformAxes3D(kAxisLength));
		this->stream->log_static(                                                          //
		    camera_entity + "/label",                                                      //
		    rerun::Points3D({rerun::Position3D(0.0f, 0.0f, 0.0f)})                         //
		        .with_radii({kTrajectoryPointRadiusM})                                     //
		        .with_colors({cameraColor(camera.mosaic_idx, camera.camera_idx, 1.0f)})    //
		        .with_labels({std::format("{}/{}", camera.mosaic_idx, camera.camera_idx)}) //
		);                                                                                 //
	}
}

void
RerunContext::logDeviceTrajectory(const CalibrationResult &result)
{
	std::vector<rerun::Position3D> positions;
	std::vector<rerun::Vec3D> strip_points;
	positions.reserve(result.keyframes.size());
	strip_points.reserve(result.keyframes.size());

	for (const KeyframeCalibration &keyframe : result.keyframes) {
		const xrt_vec3 &position = keyframe.Tcv_world_device.position;
		positions.emplace_back(position.x, position.y, position.z);
		strip_points.emplace_back(position.x, position.y, position.z);
	}

	std::string trajectory_entity = getTrajectoryEntityName();

	this->stream->log_static(                                   //
	    trajectory_entity + "/line",                            //
	    rerun::LineStrips3D({rerun::LineStrip3D(strip_points)}) //
	        .with_radii({kTrajectoryLineRadiusM})               //
	        .with_colors({deviceColor(result.device_id, 0.5f)}) //
	);                                                          //
	this->stream->log_static(                                   //
	    trajectory_entity + "/points",                          //
	    rerun::Points3D(positions)                              //
	        .with_radii({kTrajectoryPointRadiusM})              //
	        .with_colors({deviceColor(result.device_id, 1.0f)}) //
	);                                                          //
}

void
RerunContext::logKeyframe(const CalibrationResult &result, const KeyframeCalibration &keyframe)
{
	this->stream->log(getDeviceEntityName(), toRerunTransform(keyframe.Tcv_world_device));

	/*
	 * The velocity is in world space, but the device entity carries the device's transform, so draw the arrow
	 * from the trajectory entity instead.
	 */
	this->stream->log(                                                               //
	    getTrajectoryEntityName() + "/velocity",                                     //
	    rerun::Arrows3D::from_vectors(                                               //
	        {rerun::Vector3D(keyframe.velocity_m_s.x, keyframe.velocity_m_s.y,       //
	                         keyframe.velocity_m_s.z)})                              //
	        .with_origins({rerun::Position3D(keyframe.Tcv_world_device.position.x,   //
	                                         keyframe.Tcv_world_device.position.y,   //
	                                         keyframe.Tcv_world_device.position.z)}) //
	        .with_colors({deviceColor(result.device_id, 1.0f)})                      //
	);                                                                               //
}

void
RerunContext::logKeyframeObservations(const CalibrationResult &result, const KeyframeCalibration &keyframe)
{
	// The keyframe holds the IMU's pose, the LEDs hang off the model the camera factors rotate it into.
	const xrt_pose Tcv_world_model = solvedWorldModelPose(keyframe.Tcv_world_device, result.imu.Qcv_imu_model);

	for (const CameraCalibration &camera : result.cameras) {
		std::string observation_entity = getCameraObservationEntityName(camera.mosaic_idx, camera.camera_idx);

		const DeviceCameraSample *observation = findObservation(keyframe, camera);

		std::string matched_leds_entity = getMatchedLedsEntityName(camera.mosaic_idx, camera.camera_idx);

		if (observation == nullptr) {
			/*
			 * This camera did not see the device at this keyframe, so drop the previous keyframe's
			 * observation instead of leaving it on screen. Nothing below these entities is logged
			 * statically, since static data would shadow the clear.
			 */
			this->stream->log(observation_entity, rerun::Clear::RECURSIVE);
			this->stream->log(matched_leds_entity, rerun::Clear::RECURSIVE);
			continue;
		}

		/*
		 * The pose the tracker recovered from these correspondences, which is what the solve was seeded
		 * from rather than what it was fit to. Lives under the camera, so it inherits its transform.
		 */
		const xrt_pose &Tcv_cam_device = observation->Tcv_cam_device_seed;
		this->stream->log(observation_entity, toRerunTransform(Tcv_cam_device));
		this->stream->log(observation_entity + "/axes", rerun::TransformAxes3D(kAxisLength));

		/*
		 * Which LEDs this camera actually matched, placed by the solved device pose. This is what the
		 * camera factors were fit to, so a keyframe held up by a handful of LEDs is visible as such.
		 */
		std::vector<rerun::Position3D> led_positions;
		led_positions.reserve(observation->num_points);
		for (uint32_t i = 0; i < observation->num_points; i++) {
			const xrt_vec3 &T_model_led = observation->points3d[i];

			xrt_vec3 T_world_led;
			math_quat_rotate_vec3(&Tcv_world_model.orientation, &T_model_led, &T_world_led);
			T_world_led = T_world_led + Tcv_world_model.position;

			led_positions.emplace_back(T_world_led.x, T_world_led.y, T_world_led.z);
		}

		this->stream->log(                                                              //
		    matched_leds_entity,                                                        //
		    rerun::Points3D(led_positions)                                              //
		        .with_radii({kLedPointRadiusM})                                         //
		        .with_colors({cameraColor(camera.mosaic_idx, camera.camera_idx, 1.0f)}) //
		);                                                                              //
	}
}

void
RerunContext::logKeyframeMetrics(const CalibrationResult &result, const KeyframeCalibration &keyframe)
{
	const xrt_vec3 &velocity = keyframe.velocity_m_s;
	float speed = std::sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y) + (velocity.z * velocity.z));

	this->stream->log(getDeviceMetricsEntityName() + "/speed", rerun::Scalars(speed));
	this->stream->log(getKeyframeMetricsEntityName() + "/observation_count",
	                  rerun::Scalars(static_cast<float>(keyframe.camera_samples.size())));

	/*
	 * The camera factors were fit through the solved IMU -> model rotation, and the tracker's seed is a model pose
	 * as well, so both the residual and the seed comparison below are measured against the model, not the bare IMU
	 * pose.
	 */
	const xrt_pose Tcv_world_model = solvedWorldModelPose(keyframe.Tcv_world_device, result.imu.Qcv_imu_model);

	// How far the solved poses land from what each camera actually measured.
	for (const DeviceCameraSample &camera_sample : keyframe.camera_samples) {
		const CameraCalibration *camera = nullptr;
		for (const CameraCalibration &candidate : result.cameras) {
			if (candidate.mosaic_idx == camera_sample.mosaic_idx &&
			    candidate.camera_idx == camera_sample.camera_idx) {
				camera = &candidate;
				break;
			}
		}

		if (camera == nullptr) {
			U_LOG_W("Camera %u/%u observed the device but was not part of the calibration.",
			        camera_sample.mosaic_idx, camera_sample.camera_idx);
			continue;
		}

		xrt_pose predicted_Tcv_cam_model = predictedCameraDevicePose(camera->Tcv_world_cam, Tcv_world_model);

		std::string camera_metrics =
		    getCameraMetricsEntityName(camera_sample.mosaic_idx, camera_sample.camera_idx);

		const t_camera_model_params &dist =
		    kOptimizeUndistortedPoints ? camera->params.calib_pinhole : camera->params.calib_true;

		float reprojection_error = reprojectionErrorPixels( //
		    dist,                                           //
		    camera->Tcv_world_cam,                          //
		    Tcv_world_model,                                //
		    camera_sample.num_points,                       //
		    camera_sample.points2d,                         //
		    camera_sample.points3d);                        //

		// The residual proper: the LEDs this camera matched, reprojected through the solution.
		this->stream->log(camera_metrics + "/reprojection_error_px", rerun::Scalars(reprojection_error));
		this->stream->log(camera_metrics + "/correspondence_count",
		                  rerun::Scalars(static_cast<float>(camera_sample.num_points)));

		/*
		 * How far the solution drifted from the per-frame pose the tracker recovered on its own. The solve
		 * was never fit to this, so a large disagreement is a real signal rather than a residual.
		 */
		float position_error_m =
		    m_vec3_len(predicted_Tcv_cam_model.position - camera_sample.Tcv_cam_device_seed.position);
		float orientation_error_deg = orientationErrorDegrees(camera_sample.Tcv_cam_device_seed.orientation,
		                                                      predicted_Tcv_cam_model.orientation);

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
RerunContext::logResult(const CalibrationResult &result)
{
	this->logStaticScene();
	this->logSolveSummary(result);
	this->logCameras(result);
	this->logDeviceTrajectory(result);

	std::string timeline_name = getTimelineName();

	for (const KeyframeCalibration &keyframe : result.keyframes) {
		this->stream->set_time_timestamp_nanos_since_epoch(timeline_name, keyframe.timestamp_ns);

		this->logKeyframe(result, keyframe);
		this->logKeyframeObservations(result, keyframe);
		this->logKeyframeMetrics(result, keyframe);
	}

	this->stream->reset_time();
}

}; // namespace xrt::tracking::constellation::optimizer::rig_calibration
