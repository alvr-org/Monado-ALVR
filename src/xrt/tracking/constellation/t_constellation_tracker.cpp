// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the main logic for the constellation tracker.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "xrt/xrt_config_build.h"
#include "xrt/xrt_defines.h"

#include "util/u_time.h"

#include "math/m_predict.h"

#include "constellation/correspondence_search.h"

#include "t_constellation_tracker_internal.hpp"
#include "t_constellation_tracker_dataset.hpp"

#ifdef XRT_FEATURE_RERUN
#include "constellation_tracker_rerun.hpp"
#endif

#include <atomic>
#include <algorithm>
#include <limits>
#include <string>
#include <cstring>


namespace xrt::tracking::constellation {

DEBUG_GET_ONCE_LOG_OPTION(constellation_tracker_log, "CONSTELLATION_TRACKER_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_OPTION(constellation_tracker_data_recorder_output, "CONSTELLATION_TRACKER_DATA_RECORDER_OUTPUT", "")

// Unconditionally present to allow warning that the feature is not enabled.
DEBUG_GET_ONCE_BOOL_OPTION(constellation_tracker_enable_rerun, "CONSTELLATION_TRACKER_RERUN_ENABLE", false)
#ifdef XRT_FEATURE_RERUN
DEBUG_GET_ONCE_BOOL_OPTION(constellation_tracker_rerun_spawn, "CONSTELLATION_TRACKER_RERUN_SPAWN", true)
#endif

/*
 *
 * Helper functions
 *
 */

//! Gets the gravity vector of a pose, in the pose's local frame.
static void
get_pose_gravity_vector(xrt_pose &T_world_pose, xrt_vec3 &gravity)
{
	// Extract the gravity vector from the pose's orientation
	gravity = XRT_VEC3_UNIT_Y;

	xrt_quat T_pose_world_orientation;
	math_quat_invert(&T_world_pose.orientation, &T_pose_world_orientation);

	math_quat_rotate_vec3(&T_pose_world_orientation, &gravity, &gravity);
}

static uint32_t
num_blobs_for_device(CameraSample &sample, t_constellation_device_id_t device_id)
{
	uint32_t out_num_blobs = 0;
	for (uint32_t i = 0; i < sample.blob_count; i++) {
		t_blob &b = sample.blobs[i];
		if (b.matched_device_id == device_id) {
			out_num_blobs++;
		}
	}
	return out_num_blobs;
}

/*!
 * Sort key used to search for the devices closest to the camera first, since a near device covers more of the image and
 * is the one most likely to explain the blobs. Devices we have no prediction for sort last.
 *
 * @note The prediction is stored on the sample, so this is usable from the slow thread as well, where the fast thread's
 *       predictions are no longer being recomputed.
 */
static float
device_state_camera_distance_sqrd(const DeviceState *device_state)
{
	if (device_state == nullptr || !device_state->Tcv_cam_device_predicted.has_value()) {
		return std::numeric_limits<float>::infinity();
	}

	return m_vec3_len_sqrd(device_state->Tcv_cam_device_predicted->pose.position);
}

/*!
 * Collects the tracker's devices into @p out_devices, ordered nearest-camera-first for @p sample.
 *
 * The caller must hold @ref ConstellationTracker::device_lock for as long as it uses the result, since these are
 * borrowed pointers into @ref ConstellationTracker::devices.
 */
static uint32_t
collect_devices_nearest_first(ConstellationTracker *tracker,
                              CameraSample &sample,
                              std::array<Device *, XRT_CONSTELLATION_MAX_DEVICES> &out_devices)
{
	uint32_t device_count = 0;
	for (std::unique_ptr<Device> &device : tracker->devices) {
		assert(device_count < out_devices.max_size());
		out_devices[device_count++] = device.get();
	}

	std::stable_sort(out_devices.data(), out_devices.data() + device_count, [&sample](Device *x, Device *y) {
		return device_state_camera_distance_sqrd(sample.getDeviceState(x->id).value_or(nullptr)) <
		       device_state_camera_distance_sqrd(sample.getDeviceState(y->id).value_or(nullptr));
	});

	return device_count;
}

extern "C" void *
run_room_calibration_func(void *ptr)
{
	ConstellationTracker *tracker = static_cast<ConstellationTracker *>(ptr);
	auto &calib = tracker->rig_calibration;

	rig_calibration::CalibrationResult result;
	if (!calib->optimize(result)) {
		std::unique_lock<os::Mutex> lock(tracker->rig_calibration_lock);

		CT_ERROR(tracker, "Offline sensor calibration failed!");
		tracker->running_calibration = false;
		return nullptr;
	}

	for (const auto &calibration_camera : result.cameras) {
		if (calibration_camera.optimized) {
			auto &camera =
			    tracker->mosaics[calibration_camera.mosaic_idx]->cameras[calibration_camera.camera_idx];

			std::unique_lock<os::Mutex> cam_lock(camera->processing_lock);
			camera->locked_data.has_concrete_pose = true;
			// Copy the OpenCV pose in
			camera->locked_data.Txr_origin_cam = calibration_camera.Tcv_world_cam;
			// Convert to OpenXR
			math_pose_convert_from_opencv(&camera->locked_data.Txr_origin_cam,
			                              &camera->locked_data.Txr_origin_cam);
		}
	}

	{
		std::unique_lock<os::Mutex> lock(tracker->rig_calibration_lock);
		tracker->running_calibration = false;
	}

	std::shared_lock devices_lock(tracker->device_lock);
	for (auto &device : tracker->devices) {
		// Re-enable all devices now that setup is done.
		device->enabled.store(true);

		if (device->id == result.device_id) {
			device->sensor_fusion->setDeviceImuCalibration(sensor_fusion::DeviceImuCalibration{
			    .accel_bias = result.imu.accel_bias,
			    .gyro_bias = result.imu.gyro_bias,
			    .accel_scale = result.imu.accel_scale,
			    .Qcv_imu_model = result.imu.Qcv_imu_model,
			});
		}
	}

	return nullptr;
}

/*
 *
 * CameraSample implementations
 *
 */

std::optional<DeviceState *>
CameraSample::getDeviceState(t_constellation_device_id_t device_id)
{
	for (uint32_t i = 0; i < this->device_count; i++) {
		if (this->device_states[i].device_id == device_id) {
			return &this->device_states[i];
		}
	}

	return std::nullopt;
}

DeviceState &
CameraSample::putDeviceState(t_constellation_device_id_t device_id)
{
	assert(this->device_count < XRT_CONSTELLATION_MAX_DEVICES);

	DeviceState &new_device_state = this->device_states[this->device_count++];
	new_device_state = {
	    .device_id = device_id,
	    .Txr_world_device_prior = std::nullopt,
	    .found_pose = std::nullopt,
	    .needs_slow_processing = false,
	};

	return new_device_state;
}


CameraSample::CameraSample(t_blob_observation &blobservation, Camera *camera)
{
	// Copy the blob observation into this sample, since we need the data to be safe.
	this->source = blobservation.source;
	this->id = blobservation.id;
	this->sequence_id = blobservation.sequence_id;
	this->timestamp_ns = blobservation.timestamp_ns;
	memcpy(blobs, blobservation.blobs, sizeof(t_blob) * blobservation.num_blobs);
	this->blob_count = blobservation.num_blobs;

	// Get the camera pose
	this->Txr_world_cam = camera->getWorldPose(blobservation.timestamp_ns);

	this->device_states = {};
	this->device_count = 0;

	auto mosaic = camera->mosaic.lock();
	U_ASSERT_WEAK_PTR_THROW(mosaic, "Camera's mosaic was destroyed while we still had a sample referencing it");

	this->mosaic_index = mosaic->index;
	this->camera_index = camera->index;
}

void
CameraSample::markMatchingBlobs(ConstellationTracker *ct,
                                t_constellation_tracker_led_model &led_model,
                                t_constellation_device_id_t device_id,
                                pose_metrics_blob_match_info &blob_match_info)
{
	// First clear existing blob labels for this device
	for (uint32_t i = 0; i < this->blob_count; i++) {
		t_blob &b = this->blobs[i];

		// Skip blobs which already have an ID not belonging to this device
		if (b.matched_device_id != device_id) {
			continue;
		}

		if (b.matched_device_led_id != XRT_CONSTELLATION_INVALID_LED_ID) {
			// @todo is this needed?
			// b.prev_led_id = b.led_id;
		}

		b.matched_device_led_id = XRT_CONSTELLATION_INVALID_LED_ID;
		b.matched_device_id = XRT_CONSTELLATION_INVALID_DEVICE_ID;
	}

	// Iterate the visible LEDs and mark matching blobs with this device ID and LED ID
	for (int i = 0; i < blob_match_info.num_visible_leds; i++) {
		pose_metrics_visible_led_info &led_info = blob_match_info.visible_leds[i];
		t_constellation_tracker_led *led = led_info.led;

		if (led_info.matched_blob != NULL) {
			t_blob *b = led_info.matched_blob;

			b->matched_device_led_id = led->id;
			b->matched_device_id = device_id;

			CT_DEBUG(ct, "Marking LED %d/%d at %f,%f angle %f now %d (was %d)", device_id, led->id,
			         b->center_distorted.x, b->center_distorted.y, RAD_TO_DEG(acosf(led_info.facing_dot)),
			         b->matched_device_led_id, /* b->prev_led_id */ -1);
		} else {
			CT_DEBUG(ct, "No blob for device %d LED %d @ %f,%f size %f px angle %f", device_id, led->id,
			         led_info.pos_px_undistorted.x, led_info.pos_px_undistorted.y,
			         2 * led_info.led_radius_px_undistorted, RAD_TO_DEG(acosf(led_info.facing_dot)));
		}
	}
}

/*
 *
 * Camera implementations
 *
 */

Camera::Camera(ConstellationTracker *tracker,
               std::weak_ptr<CameraMosaic> mosaic,
               const t_constellation_tracker_camera &camera_params,
               enum u_logging_level *log_level_ptr,
               size_t index)
    : tracker(tracker), mosaic(mosaic), calibration(camera_params.calibration), index(index)
{
	struct t_camera_model_params calib_true;
	t_camera_model_params_from_t_camera_calibration(&this->calibration, &calib_true);
	this->model = camera_model_init(calibration.image_size_pixels.w, calibration.image_size_pixels.h, &calib_true);

	this->locked_data = {
	    .Txr_origin_cam = camera_params.pose_in_origin,
	    .has_concrete_pose = camera_params.has_concrete_pose,
	};

	this->slow_processing_thread_data.cs = correspondence_search_new(log_level_ptr, &this->model);
	this->fast_processing_thread_data.cs = correspondence_search_new(log_level_ptr, &this->model);

	if (!this->tracker->single_threaded) {
		if (os_thread_helper_init(&this->slow_processing_thread) < 0) {
			throw std::runtime_error("Slow processing thread failed to init");
		}
		if (os_thread_helper_init(&this->fast_processing_thread) < 0) {
			throw std::runtime_error("Fast processing thread failed to init");
		}

		if (os_thread_helper_start(&this->slow_processing_thread, constellation_tracker_camera_slow_thread,
		                           this) < 0) {
			throw std::runtime_error("Starting slow processing thread failed");
		}
		if (os_thread_helper_start(&this->fast_processing_thread, constellation_tracker_camera_fast_thread,
		                           this) < 0) {
			throw std::runtime_error("Starting fast processing thread failed");
		}
	}

	u_sink_debug_init(&this->slow_processing_thread_data.debug_sink);
	u_sink_debug_init(&this->fast_processing_thread_data.debug_sink);
}

Camera::~Camera()
{
	if (this->slow_processing_thread.initialized) {
		os_thread_helper_destroy(&this->slow_processing_thread);
	}

	if (this->fast_processing_thread.initialized) {
		os_thread_helper_destroy(&this->fast_processing_thread);
	}

	if (this->slow_processing_thread_data.cs) {
		correspondence_search_free(this->slow_processing_thread_data.cs);
		this->slow_processing_thread_data.cs = nullptr;
	}

	if (this->fast_processing_thread_data.cs) {
		correspondence_search_free(this->fast_processing_thread_data.cs);
		this->fast_processing_thread_data.cs = nullptr;
	}

	u_sink_debug_destroy(&this->slow_processing_thread_data.debug_sink);
	u_sink_debug_destroy(&this->fast_processing_thread_data.debug_sink);
}

std::optional<xrt_pose>
Camera::getWorldPose(timepoint_ns when_ns)
{
	std::shared_ptr<CameraMosaic> mosaic = this->mosaic.lock();
	U_ASSERT_WEAK_PTR_RET(
	    mosaic, "Camera's mosaic was destroyed while we still had a pointer to it, this should never happen",
	    std::nullopt);

	auto Txr_world_origin = mosaic->getTrackingOriginPose(when_ns);
	if (!Txr_world_origin.has_value()) {
		return std::nullopt;
	}

	std::unique_lock<os::Mutex> lock(this->processing_lock);

	if (!this->locked_data.has_concrete_pose) {
		return std::nullopt;
	}

	xrt_pose Txr_world_cam;
	math_pose_transform(&Txr_world_origin.value(), &this->locked_data.Txr_origin_cam, &Txr_world_cam);
	return Txr_world_cam;
}

void
Camera::deferSampleToSlowThread(CameraSample &sample)
{
	os_thread_helper_lock(&this->slow_processing_thread);
	{
		if (this->slow_processing_thread_data.sample.has_value()) {
			// Warn that we're dropping a frame.
			CT_WARN(this->tracker,
			        "Dropping slow sample %" PRIu64 " at ts %" PRIi64 ". Tracker is likely running slow.",
			        this->slow_processing_thread_data.sample->id,
			        this->slow_processing_thread_data.sample->timestamp_ns);
		}

		this->slow_processing_thread_data.sample = sample;
		os_thread_helper_signal_locked(&this->slow_processing_thread);
	}
	os_thread_helper_unlock(&this->slow_processing_thread);
}

bool
Camera::tryDevicePose(Device *device,
                      CameraSample &sample,
                      DeviceState &device_state,
                      const std::optional<xrt_space_relation> &Tcv_cam_device_prior,
                      const xrt_pose &Tcv_cam_device_candidate)
{
	pose_metrics score;
	if (Tcv_cam_device_prior.has_value()) {
		pose_metrics_evaluate_pose_with_prior(  //
		    &score,                             //
		    &Tcv_cam_device_candidate,          //
		    false,                              //
		    &Tcv_cam_device_prior.value().pose, //
		    &device->prior_pos_error,           //
		    &device->prior_rot_error,           //
		    sample.blobs,                       //
		    sample.blob_count,                  //
		    &device->params.led_model,          //
		    device->id,                         //
		    &this->model,                       //
		    NULL);                              //
	} else {
		pose_metrics_evaluate_pose(    //
		    &score,                    //
		    &Tcv_cam_device_candidate, //
		    sample.blobs,              //
		    sample.blob_count,         //
		    &device->params.led_model, //
		    device->id,                //
		    &this->model,              //
		    NULL);                     //
	}

	if (POSE_HAS_FLAGS(&score, POSE_MATCH_GOOD | POSE_MATCH_LED_IDS)) {
		this->pushPose(               //
		    sample,                   //
		    device_state,             //
		    device,                   //
		    score,                    //
		    Tcv_cam_device_candidate, //
		    std::nullopt);            //
		return true;
	}

	return false;
}

bool
Camera::tryDeviceBlobRecovery(Device *device,
                              CameraSample &sample,
                              DeviceState &device_state,
                              const std::optional<xrt_space_relation> &Tcv_cam_device_prior)
{
	auto tracker = this->tracker;

	const uint32_t needed_blobs = 4;
	uint32_t num_blobs = 0;
	for (uint32_t index = 0; index < sample.blob_count; index++) {
		t_blob &b = sample.blobs[index];
		if (b.matched_device_id == device->id) {
			num_blobs++;

			if (num_blobs >= needed_blobs) {
				break;
			}
		}
	}
	if (num_blobs < needed_blobs) {
		return false;
	}

	OldOptimizationData old_optimization_data;
	U_ZERO(&old_optimization_data);

	// RANSAC-PnP with the matched blobs
	// The optimization should still be able to run with an identity pose.
	xrt_pose Tcv_cam_device =
	    Tcv_cam_device_prior
	        .value_or(xrt_space_relation{
	            .relation_flags = static_cast<xrt_space_relation_flags>(0),
	            .pose =
	                {
	                    .orientation = {.x = 0, .y = 1, .z = 0, .w = 0}, // 180 around Y, facing the camera
	                    .position = {0, 0, 1},
	                },
	            .linear_velocity = {},
	            .angular_velocity = {},
	        })
	        .pose;
	xrt_pose Tcv_cam_device_optimized;
	bool success = optimizePose(           //
	    tracker->log_level,                //
	    tracker->deterministic,            //
	    this->model,                       //
	    Tcv_cam_device,                    //
	    sample.blobs,                      //
	    sample.blob_count,                 //
	    &device->params.led_model,         //
	    device->id,                        //
	    Tcv_cam_device_optimized,          //
	    old_optimization_data.covariance); //
	if (!success) {
		CT_DEBUG(tracker, "Camera %p RANSAC-PnP blob recovery for device %d from %u blobs failed", (void *)this,
		         device->id, sample.blob_count);
		return false;
	}

	CT_DEBUG(tracker, "Camera %p RANSAC-PnP blob recovery for device %d from %u blobs succeeded", (void *)this,
	         device->id, sample.blob_count);
	Tcv_cam_device = Tcv_cam_device_optimized;

	pose_metrics score;
	// Evaluate the pose, using the prior if available
	if (Tcv_cam_device_prior.has_value()) {
		pose_metrics_evaluate_pose_with_prior(  //
		    &score,                             //
		    &Tcv_cam_device,                    //
		    true,                               //
		    &Tcv_cam_device_prior.value().pose, //
		    &device->prior_pos_error,           //
		    &device->prior_rot_error,           //
		    sample.blobs,                       //
		    sample.blob_count,                  //
		    &device->params.led_model,          //
		    device->id,                         //
		    &this->model,                       //
		    NULL);
	} else {
		pose_metrics_evaluate_pose(&score, &Tcv_cam_device, sample.blobs, sample.blob_count,
		                           &device->params.led_model, device->id, &this->model, NULL);
	}

	/*
	 * @note I saw some cases where an otherwise valid solve was dropped because we didn't trust the RANSAC solve.
	 *       I don't think this will make things *worse*, but it seems to at least get some more valid solves
	 *       through, so this is fine for now, set as a constexpr bool for easy tweaking purposes.
	 *       Since a RANSAC has already made an assessment over the validity of the pose, and we already trust that
	 *       assessment, this shouldn't cause many problems. RANSAC is also only meaningful if more than 4 blobs
	 *       were matched in the RANSAC run, otherwise RANSAC did not even run.
	 */
	constexpr bool kAlwaysTrustRansac = true;

	if ((kAlwaysTrustRansac && score.matched_blobs > 4) || POSE_HAS_FLAGS(&score, POSE_MATCH_GOOD)) {
		CT_DEBUG(tracker, "Camera %p RANSAC-PnP recovered pose was good enough for device %d from %u blobs",
		         (void *)this, device->id, sample.blob_count);

		this->pushPose(              //
		    sample,                  //
		    device_state,            //
		    device,                  //
		    score,                   //
		    Tcv_cam_device,          //
		    &old_optimization_data); //

		return true;
	} else {
		CT_DEBUG(tracker, "Camera %p RANSAC-PnP recovered pose for device %d from %u blobs was not good enough",
		         (void *)this, device->id, sample.blob_count);
	}

	return false;
}

void
Camera::handleProcessingComplete(CameraSample &sample)
{
	auto tracker = this->tracker;

#ifdef XRT_FEATURE_RERUN
	if (tracker->rerun_stream) {
		tracker->rerun_stream->logSample(*tracker, sample);
	}
#endif

	// Both callers hold `tracker->device_lock`, which is what lets this resolve the sample's devices.
	tracker->rig_calibration->pushCameraSample(tracker, sample);

	if (!tracker->checkSensorCalibration()) {
		CT_ERROR(tracker, "Sensor tried to do calibration but it failed.");
	}

	for (uint32_t i = 0; i < sample.device_count; i++) {
		auto &sample_device = sample.device_states[i];
		auto device_iter = std::find_if(
		    tracker->devices.begin(), tracker->devices.end(),
		    [&sample_device](const auto &device) { return sample_device.device_id == device->id; });
		// Skip removed devices
		if (device_iter == tracker->devices.end()) {
			continue;
		}
		auto &device = *device_iter;

		// Mark the device's found pose as handled.
		if (auto &found_pose = sample_device.found_pose) {
			if (!found_pose->handled) {
				device->sensor_fusion->pushCameraSample(tracker, sample);
			}

			found_pose->handled = true;
		}
	}
}

void
Camera::processSampleSlow(CameraSample &sample)
{
	ConstellationTracker *tracker = this->tracker;

	auto &data = this->slow_processing_thread_data;

	CT_TRACE(tracker, "Starting slow processing for camera %p with %u blobs", (void *)this, sample.blob_count);

	correspondence_search_set_blobs(data.cs, sample.blobs, sample.blob_count);

	std::shared_ptr<CameraMosaic> mosaic = this->mosaic.lock();
	U_ASSERT_WEAK_PTR_RET(mosaic,
	                      "Camera mosaic was destroyed while processing a sample, this should never happen since "
	                      "the mosaic owns the camera");

	std::shared_lock lock(tracker->device_lock);

	auto Txr_world_cam = sample.Txr_world_cam;

	// Borrowed pointers, only valid while we hold the device lock taken above.
	std::array<Device *, XRT_CONSTELLATION_MAX_DEVICES> devices{};
	const uint32_t device_count = collect_devices_nearest_first(tracker, sample, devices);

	bool any_shallow_failed = false;

	for (int pass = 0; pass < 2; pass++) {
		for (uint32_t device_iter = 0; device_iter < device_count; device_iter++) {
			Device *device = devices[device_iter];

			// Device is not enabled.
			if (!device->enabled.load()) {
				continue;
			}

			// Do a shallow search first go around
			correspondence_search_flags search_flags =
			    pass == 0 ? CS_FLAG_SHALLOW_SEARCH : CS_FLAG_DEEP_SEARCH;

			{
				std::lock_guard<os::Mutex> lock(device->data_lock);

				bool enable_slow_skip =
				    device->locked_data.last_known_pose.has_value() && // device has been seen
				    Txr_world_cam.has_value();                         // we have a pose

				if (enable_slow_skip) {
					const auto &last_known_pose = *device->locked_data.last_known_pose;

					time_duration_ns time_since_solve_ns =
					    sample.timestamp_ns - last_known_pose.timestamp_ns;

					if (time_since_solve_ns < (U_TIME_1MS_IN_NS * 150LL)) {
						// If the device has been seen in the last 150ms, don't try to brute
						// force search it, hope that it will be seen by a fast solve in that
						// time period.
						continue;
					} else if ((time_since_solve_ns < (U_TIME_1MS_IN_NS * 1000LL) ||
					            this->shallow_searches_failed.load() < 10) &&
					           (search_flags & CS_FLAG_DEEP_SEARCH)) {
						// Don't do a deep search until a shallow search has failed for one full
						// second, or we've failed less than 10 shallow searches for any device.
						continue;
					}
				}
			}

			search_flags = (correspondence_search_flags)(search_flags | CS_FLAG_STOP_FOR_STRONG_MATCH);

			DeviceState *device_state_ptr = sample.getDeviceState(device->id).value_or(nullptr);
			// If there was no device state in the sample, that means this device appeared after the
			// constellation tracker started this sample, so we need to fill out the device state here.
			if (device_state_ptr == nullptr) {
				device_state_ptr = &sample.putDeviceState(device->id);

				// we need to do a slow process for this device since it wasn't present in the fast
				// processing
				device_state_ptr->needs_slow_processing = true;
			}
			DeviceState &device_state = *device_state_ptr;

			if (!device_state.needs_slow_processing) {
				// we already did a fast process for this device and it succeeded, so there is no need
				// to do a slow search
				continue;
			}

			xrt_pose Tcv_cam_device = XRT_POSE_IDENTITY;
			if (device_state.Txr_world_device_prior.has_value() && Txr_world_cam.has_value()) {
				xrt_pose Txr_cam_world;
				math_pose_invert(&Txr_world_cam.value(), &Txr_cam_world);

				xrt_pose Txr_cam_device;
				math_pose_transform(&Txr_cam_world, &device_state.Txr_world_device_prior.value(),
				                    &Txr_cam_device);

				math_pose_convert_from_opencv(&Txr_cam_device, &Tcv_cam_device);

				search_flags = (correspondence_search_flags)(search_flags | CS_FLAG_HAVE_POSE_PRIOR);
			}

			// Arbitrary threshold to prevent trusting a gravity vector if the device itself isn't confident
			// in it's own gravity.
			const float gravity_error_threshold_rad = DEG_TO_RAD(25.f);

			xrt_vec3 cv_camera_gravity_vector = {0.0, 1.0, 0.0};
			if ((search_flags & CS_FLAG_HAVE_POSE_PRIOR) != 0 &&
			    device->gravity_error_rad < gravity_error_threshold_rad) {
				// If we have a pose for the camera and we have a prior pose
				// (required by correspondence for search gravity matching)
				if (Txr_world_cam.has_value()) {
					xrt_pose Tcv_world_cam;
					math_pose_convert_from_opencv(&Txr_world_cam.value(), &Tcv_world_cam);

					// Acquire the camera's gravity vector under the processing lock
					get_pose_gravity_vector(Tcv_world_cam, cv_camera_gravity_vector);

					// Add in to check gravity
					search_flags =
					    (correspondence_search_flags)(search_flags | CS_FLAG_MATCH_GRAVITY);
				}
			}

			pose_metrics score;
			bool found_pose = correspondence_search_find_one_pose( //
			    data.cs,                                           //
			    device->search_model,                              //
			    search_flags,                                      //
			    &Tcv_cam_device,                                   //
			    &device->prior_pos_error,                          //
			    &device->prior_rot_error,                          //
			    &cv_camera_gravity_vector,                         //
			    device->gravity_error_rad,                         //
			    &score);                                           //
			if (found_pose) {
				this->pushPose(sample,         //
				               device_state,   //
				               device,         //
				               score,          //
				               Tcv_cam_device, //
				               std::nullopt);  //

				// We found a pose for this device in this sample
				device_state.needs_slow_processing = false;
			} else {
				CT_TRACE(tracker, "Camera %p slow processing for device %d failed to find a pose",
				         (void *)this, device->id);

				if (search_flags & CS_FLAG_SHALLOW_SEARCH) {
					any_shallow_failed = true;
				}
			}
		}
	}

	if (any_shallow_failed) {
		this->shallow_searches_failed.fetch_add(1);
	} else {
		this->shallow_searches_failed.store(0);
	}

	for (uint32_t i = 0; i < sample.device_count; i++) {
		auto &device = sample.device_states[i];

		device.needs_slow_processing = false;
	}

	this->debugScribbleSample(sample, false);

	this->handleProcessingComplete(sample);
}

bool
Camera::processSampleFast(CameraSample &sample)
{
	ConstellationTracker *tracker = this->tracker;

	CT_TRACE(tracker, "Starting fast processing for camera %p with %u blobs", (void *)this, sample.blob_count);

	std::shared_ptr<CameraMosaic> mosaic = this->mosaic.lock();
	U_ASSERT_WEAK_PTR_RET(mosaic,
	                      "Camera mosaic was destroyed while processing a sample, this should never happen since "
	                      "the mosaic owns the camera",
	                      false);

	auto Txr_world_cam = sample.Txr_world_cam;
	auto Tcv_world_cam = Txr_world_cam;
	auto Tcv_cam_world = Txr_world_cam;
	if (Txr_world_cam.has_value()) {
		math_pose_convert_from_opencv(&Txr_world_cam.value(), &Tcv_world_cam.value());
		math_pose_invert(&Tcv_world_cam.value(), &Tcv_cam_world.value());
	}

	std::shared_lock lock(tracker->device_lock);

	// Figure out where all the devices are first, so we can then sort it.
	for (std::unique_ptr<Device> &device : tracker->devices) {
		xrt_space_relation device_predicted_relation = XRT_SPACE_RELATION_ZERO; //< AKA "the prior"

		if (device->params.tracking_source != nullptr) {
			t_constellation_tracker_tracking_source_get_tracked_pose(
			    device->params.tracking_source, sample.timestamp_ns, &device_predicted_relation);
		}

		bool prior_pose_valid =
		    (device_predicted_relation.relation_flags &
		     (XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_VALID_BIT)) ==
		    (XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_VALID_BIT);

		std::optional<xrt_space_relation> Tcv_cam_device_predicted = std::nullopt; //< AKA "the prior"
		// Whether the prior pose is actually valid and we have a pose for the camera
		if (Tcv_cam_world.has_value() && prior_pose_valid) {
			xrt_space_relation Tcv_world_device_predicted = device_predicted_relation;
			math_space_relation_convert_from_opencv(&device_predicted_relation,
			                                        &Tcv_world_device_predicted);

			Tcv_cam_device_predicted.emplace(Tcv_world_device_predicted); // initialize it to a value
			math_pose_transform(&Tcv_cam_world.value(), &Tcv_world_device_predicted.pose,
			                    &Tcv_cam_device_predicted.value().pose);
			// @note technically we need to transform the velocities here but we aren't actually reading the
			//       values, just the magnitude rn so its fine.
		}

		auto &device_state = sample.putDeviceState(device->id);
		device_state.Txr_world_device_prior =
		    prior_pose_valid ? std::optional<xrt_pose>(device_predicted_relation.pose) : std::nullopt;
		device_state.Tcv_cam_device_predicted = Tcv_cam_device_predicted;
	}

	// Borrowed pointers, only valid while we hold the device lock taken above.
	std::array<Device *, XRT_CONSTELLATION_MAX_DEVICES> devices{};
	const uint32_t device_count = collect_devices_nearest_first(tracker, sample, devices);

	bool need_slow_search = false;
	for (uint32_t device_iter = 0; device_iter < device_count; device_iter++) {
		Device *device = devices[device_iter];

		// Device is not enabled.
		if (!device->enabled.load()) {
			continue;
		}

		// The first pass above put a state in the sample for every device we are about to walk.
		DeviceState &device_state = *sample.getDeviceState(device->id).value();

		const auto Tcv_cam_device_predicted = device_state.Tcv_cam_device_predicted; //< AKA "the prior"

		bool do_blob_recovery = true;
		if (Tcv_cam_device_predicted.has_value()) {
			// If the device is moving more than 5 meters per second, or it's rotating at more than one
			// rotation every second, then skip blob recovery, and also wipe associations, that's too fast
			// to keep sane blob associations.
			if (m_vec3_len(Tcv_cam_device_predicted->linear_velocity) > 5 ||
			    m_vec3_len(Tcv_cam_device_predicted->angular_velocity) > (M_PI * 2)) {
				do_blob_recovery = false;
			}
		}

		// if we have a valid prior pose, try to use it for fast matching
		if (Tcv_cam_device_predicted.has_value() && //
		    this->tryDevicePose(device,             //
		                        sample,             //
		                        device_state,       //
		                        std::nullopt,       //
		                        Tcv_cam_device_predicted.value().pose)) {
			CT_DEBUG(tracker, "Fast processing for device %d succeeded with predicted pose", device->id);
			continue; // try the next device, we found a pose!
		}

		// Try to use blobs after we've checked the prior itself
		bool wipe_blob_associations = false;
		if (do_blob_recovery &&                       //
		    this->tryDeviceBlobRecovery(device,       //
		                                sample,       //
		                                device_state, //
		                                Tcv_cam_device_predicted)) {
			CT_DEBUG(tracker, "Fast processing for device %d succeeded with blob recovery", device->id);
			continue; // try the next device, we found a pose!
		} else {
			wipe_blob_associations = true;
		}

		// Try to get a last known pose
		std::optional<xrt_pose> Tcv_cam_device_last_known = std::nullopt;
		{
			std::unique_lock<os::Mutex> lock(device->data_lock);

			// If we know where the camera is, we found any pose, and that pose had a valid world-space
			// pose.
			if (device->locked_data.last_known_pose.has_value()) {
				Tcv_cam_device_last_known = device->locked_data.last_known_pose->Tcv_cam_device;
			}
		}

		if (Tcv_cam_device_last_known.has_value() &&      //
		    this->tryDevicePose(device,                   //
		                        sample,                   //
		                        device_state,             //
		                        Tcv_cam_device_predicted, //
		                        Tcv_cam_device_last_known.value())) {
			CT_DEBUG(tracker, "Fast processing for device %d succeeded with last known pose", device->id);
			continue; // try the next device, we found a pose!
		}

		if (wipe_blob_associations) {
			// Blob-based recovery failed, unmark all blobs for this device.
			for (uint32_t i = 0; i < sample.blob_count; i++) {
				t_blob &b = sample.blobs[i];
				if (b.matched_device_id == device->id) {
					b.matched_device_led_id = XRT_CONSTELLATION_INVALID_LED_ID;
					b.matched_device_id = XRT_CONSTELLATION_INVALID_DEVICE_ID;
				}
			}
		}

		device_state.needs_slow_processing = true;

		need_slow_search = true;
	}

	this->debugScribbleSample(sample, true);

	// Only save samples on the fast processing thread, since the slow processing thread is *triggered* by the fast
	// processing thread.
	if (tracker->data_recorder) {
		tracker->data_recorder->recordSample(sample);
	}

	/*
	 * Handle any poses that this sample may now have, it's okay if this is called again in the slow thread, that's
	 * what @ref FoundDevicePose::handled is for.
	 */
	this->handleProcessingComplete(sample);

	return need_slow_search;
}

void
Camera::pushPose(CameraSample &camera_sample,
                 DeviceState &device_state,
                 Device *device,
                 pose_metrics &score,
                 const xrt_pose &Tcv_cam_device_initial,
                 std::optional<OldOptimizationData *> was_optimized)
{
	// We should never find two poses for the same device in a single frame
	assert(device_state.found_pose.has_value() == false);

	ConstellationTracker *tracker = this->tracker;

	uint32_t blobs_marked_before_update = num_blobs_for_device(camera_sample, device->id);

	// Match visible blobs to the pose we found
	pose_metrics_blob_match_info blob_match_info;
	pose_metrics_match_pose_to_blobs(&Tcv_cam_device_initial, camera_sample.blobs, camera_sample.blob_count,
	                                 &device->params.led_model, device->id, &this->model, &blob_match_info);

	// Mark all the new blobs using the match info
	camera_sample.markMatchingBlobs(tracker, device->params.led_model, device->id, blob_match_info);

	RawPoseCovarianceMatrix covariance;
	// If the pose was already optimized once, pull the covariance matrix from that
	if (was_optimized) {
		std::memcpy(covariance, (*was_optimized)->covariance, sizeof(RawPoseCovarianceMatrix));
	}

	xrt_pose Tcv_cam_device = Tcv_cam_device_initial;

	// Only do an optimization if we haven't already optimized, or we marked new blobs.
	// This prevents us from optimizing a pose multiple times in a single frame.
	if (!was_optimized || num_blobs_for_device(camera_sample, device->id) > blobs_marked_before_update) {
		// Try to optimize the pose again, unmarking outliers
		xrt_pose Tcv_cam_device_optimized;
		bool success = optimizePose(   //
		    tracker->log_level,        //
		    tracker->deterministic,    //
		    this->model,               //
		    Tcv_cam_device,            //
		    camera_sample.blobs,       //
		    camera_sample.blob_count,  //
		    &device->params.led_model, //
		    device->id,                //
		    Tcv_cam_device_optimized,  //
		    covariance);               //
		if (!success) {
			CT_DEBUG(tracker,
			         "Camera %d (group %d) RANSAC-PnP refinement for device %d from %u "
			         "blobs failed",
			         0, 0, device->id, camera_sample.blob_count);

			if (!was_optimized) {
				// If we didn't do a pose optimization, and this one failed, just compute the covariance
				// directly.
				computePoseCovariance(         //
				    tracker->log_level,        //
				    this->model,               //
				    Tcv_cam_device,            //
				    camera_sample.blobs,       //
				    camera_sample.blob_count,  //
				    &device->params.led_model, //
				    device->id,                //
				    covariance);               //
			}
		} else {
			CT_DEBUG(tracker,
			         "Camera %d (group %d) RANSAC-PnP refinement for device %d from %u "
			         "blobs. Produced pose %f,%f,%f,%f pos %f,%f,%f",
			         0, 0, device->id, camera_sample.blob_count, Tcv_cam_device.orientation.x,
			         Tcv_cam_device.orientation.y, Tcv_cam_device.orientation.z,
			         Tcv_cam_device.orientation.w, Tcv_cam_device.position.x, Tcv_cam_device.position.y,
			         Tcv_cam_device.position.z);

			Tcv_cam_device = Tcv_cam_device_optimized;

			// Now that we've optimized the pose, we gotta update the match info
			pose_metrics_match_pose_to_blobs(&Tcv_cam_device, camera_sample.blobs, camera_sample.blob_count,
			                                 &device->params.led_model, device->id, &this->model,
			                                 &blob_match_info);

			// Unmark any other blobs
			camera_sample.markMatchingBlobs(tracker, device->params.led_model, device->id, blob_match_info);
		}

		// If after RANSAC and optimization no blobs remain, skip, so that we aren't putting an invalid pose
		// into the fusion/latest field.
		if (num_blobs_for_device(camera_sample, device->id) == 0) {
			return;
		}

		// We need to re-evaluate the pose after optimization, since the reprojection error may have changed.
		pose_metrics_evaluate_pose(    //
		    &score,                    //
		    &Tcv_cam_device,           //
		    camera_sample.blobs,       //
		    camera_sample.blob_count,  //
		    &device->params.led_model, //
		    device->id,                //
		    &this->model,              //
		    NULL);                     //
	}

	// Move to OpenXR space
	xrt_pose Txr_cam_device;
	math_pose_convert_from_opencv(&Tcv_cam_device, &Txr_cam_device);

	CT_DEBUG(tracker, "Pose: orient %f %f %f %f pos %f %f %f", Txr_cam_device.orientation.x,
	         Txr_cam_device.orientation.y, Txr_cam_device.orientation.z, Txr_cam_device.orientation.w,
	         Txr_cam_device.position.x, Txr_cam_device.position.y, Txr_cam_device.position.z);

	std::shared_ptr<CameraMosaic> mosaic = this->mosaic.lock();
	U_ASSERT_WEAK_PTR_RET(mosaic,
	                      "Camera mosaic was destroyed while processing a sample, this should never happen since "
	                      "the mosaic owns the camera");

	float average_brightness = 0.0f;
	uint32_t used_blobs = 0;
	for (uint32_t i = 0; i < camera_sample.blob_count; i++) {
		struct t_blob &b = camera_sample.blobs[i];
		if (b.matched_device_id == device->id) {
			average_brightness += b.brightness;
			used_blobs++;
		}
	}

	if (used_blobs > 0) {
		average_brightness /= used_blobs;
	} else {
		average_brightness = 1.0f;
	}

	// Mark that we found a pose
	device_state.found_pose = {
	    .Tcv_cam_device = Tcv_cam_device,
	    .average_blob_brightness = average_brightness,
	    .covariance = {},
	};
	std::memcpy(device_state.found_pose->covariance, covariance, sizeof(RawPoseCovarianceMatrix));

	auto Txr_world_cam = camera_sample.Txr_world_cam;

	std::optional<xrt_pose> Txr_world_device = Txr_world_cam;
	if (Txr_world_cam.has_value()) {
		math_pose_transform(&Txr_world_cam.value(), &Txr_cam_device, &Txr_world_device.value());
	}

	// Compute the metrics
	t_constellation_tracker_sample_metrics metrics = {
	    .matched_blob_count = score.matched_blobs,
	    .visible_led_count = score.visible_leds,
	    .reprojection_error = score.matched_blobs > 0 ? sqrtf(score.reprojection_error / score.matched_blobs) : 0.0,
	};

	// Push the sample to the device
	t_constellation_tracker_sample sample = {
	    .timestamp_ns = camera_sample.timestamp_ns,
	    .sequence_id = camera_sample.sequence_id,
	    .has_world_pose = Txr_world_device.has_value(),
	    .world_pose = XRT_POSE_IDENTITY,
	    .mosaic_index = mosaic->index,
	    .camera_index = this->index,
	    .average_brightness = average_brightness, // @todo compute this
	    .metrics = metrics,
	};
	if (Txr_world_device.has_value()) {
		sample.world_pose = Txr_world_device.value();
	}
	t_constellation_tracker_device_push_sample(device->device, &sample);

	if (Txr_world_device.has_value()) {
		std::unique_lock<os::Mutex> lock(device->data_lock);

		// If we already found a pose in the future, then don't mark blobs, since the device has definitely
		// moved.
		if (!device->locked_data.last_known_pose.has_value() ||
		    device->locked_data.last_known_pose->timestamp_ns <= camera_sample.timestamp_ns) {
			// Call back to the blobwatch to update the blobs for this device. Done after pose optimization
			// since the RANSAC process will unlabel any outliers.
			auto tbo = camera_sample.toBlobObservation();
			t_blobwatch_mark_blob_device(camera_sample.source, &tbo, device->id);
		}

		device->locked_data.last_known_pose =
		    DeviceLastPose(Txr_world_device.value(), Tcv_cam_device, camera_sample.timestamp_ns);
	}

	CT_DEBUG(tracker, "Found pose for device %d", device->id);
}

/*
 *
 * CameraMosaic implementations
 *
 */

CameraMosaic::CameraMosaic(ConstellationTracker *tracker,
                           const t_constellation_tracker_camera_mosaic &mosaic_params,
                           size_t index)
    : index(index)
{
	this->tracking_origin = mosaic_params.tracking_origin;

	// NOTE: the stability of this vector is important since we're passing to C callbacks and APIs!
	this->cameras.reserve(mosaic_params.num_cameras);
}

std::optional<xrt_pose>
CameraMosaic::getTrackingOriginPose(timepoint_ns when_ns)
{
	if (this->tracking_origin) {
		xrt_space_relation relation;
		t_constellation_tracker_tracking_source_get_tracked_pose(this->tracking_origin, when_ns, &relation);

		// If the tracking source has a valid position, grab it
		if ((relation.relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) != 0 &&
		    (relation.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0) {
			return relation.pose;
		} else {
			return std::nullopt;
		}
	}

	return std::optional<xrt_pose>(XRT_POSE_IDENTITY);
}

/*
 *
 * DeviceLastPose implementations
 *
 */

DeviceLastPose::DeviceLastPose(xrt_pose Txr_world_device, xrt_pose Tcv_cam_device, timepoint_ns timestamp_ns)
    : Txr_world_device(Txr_world_device), Tcv_cam_device(Tcv_cam_device), timestamp_ns(timestamp_ns)
{}

/*
 *
 * Device implementations
 *
 */

Device::Device(ConstellationTracker *tracker,
               t_constellation_tracker_device_params *params,
               t_constellation_tracker_device *device,
               t_constellation_device_id_t id)
    : params(*params), device(device), id(id), tracker(tracker), data_lock(),
      locked_data({.last_known_pose = std::nullopt})
{
	this->imu_sink = {
	    .push_imu =
	        [](xrt_imu_sink *ptr, xrt_imu_sample *sample) {
		        auto self = Device::fromXrtImuSink(ptr);
		        self->pushImuSample(*sample);
	        },
	};
	params->imu_sink = &this->imu_sink;

	// Copy the LED model leds into safe memory, since we want to mutate it into OpenCV space
	this->params.led_model.leds = new t_constellation_tracker_led[this->params.led_model.led_count];
	memcpy(this->params.led_model.leds, params->led_model.leds,
	       sizeof(t_constellation_tracker_led) * this->params.led_model.led_count);

	// flip all LEDs from OpenXR -> OpenCV coordinate space, since the tracker works in OpenCV space
	for (size_t i = 0; i < this->params.led_model.led_count; i++) {
		t_constellation_tracker_led &dst = this->params.led_model.leds[i];
		const t_constellation_tracker_led &src = params->led_model.leds[i];

		dst = src;
		math_vec3_convert_from_opencv(&src.position, &dst.position);
		math_vec3_convert_from_opencv(&src.normal, &dst.normal);
	}

	this->search_model = t_constellation_search_model_new(this->id, &this->params.led_model);
}

Device::~Device()
{
	// Clear the sensor fusion so it gets destructed.
	this->sensor_fusion.reset();

	if (this->search_model) {
		t_constellation_search_model_free(this->search_model);
		this->search_model = nullptr;
	}

	if (this->params.led_model.leds) {
		delete[] this->params.led_model.leds;
		this->params.led_model.leds = nullptr;
	}
}

void
Device::pushImuSample(const xrt_imu_sample &sample)
{
	this->tracker->rig_calibration->pushImuSample(this->id, sample);

	this->sensor_fusion->pushImuSample(sample);
}

/*
 *
 * ConstellationTracker implementations
 *
 */

ConstellationTracker::ConstellationTracker(t_constellation_tracker_params *params)
    : log_level(debug_get_log_option_constellation_tracker_log()), params(*params), rig_calibration_lock()
{
	// If the deterministic flag is set, we enable single-threaded processing, and mark it as deterministic.
	if ((params->flags & T_CONSTELLATION_TRACKER_FLAGS_DETERMINISTIC) != 0) {
		this->single_threaded = true;
		this->deterministic = true;
	}

	this->mosaics.reserve(params->num_mosaics);

	// Fill in our internal data structures based on the provided params
	for (size_t mosaic_idx = 0; mosaic_idx < params->num_mosaics; mosaic_idx++) {
		const t_constellation_tracker_camera_mosaic &mosaic_params = params->mosaics[mosaic_idx];

		std::shared_ptr<CameraMosaic> mosaic = std::make_shared<CameraMosaic>(this, mosaic_params, mosaic_idx);

		// Assert pointer stability!
		assert(mosaic->cameras.capacity() >= mosaic_params.num_cameras);

		for (size_t cam_idx = 0; cam_idx < mosaic_params.num_cameras; cam_idx++) {
			const t_constellation_tracker_camera &camera_params = mosaic_params.cameras[cam_idx];

			// This can't be in the constructor since the `shared_ptr` of the `mosaic` isn't formed
			// yet, but the camera needs to own a weak_ptr to it's mosaic.
			mosaic->cameras.push_back(
			    std::make_unique<Camera>(this, mosaic, camera_params, &this->log_level, cam_idx));
		}

		this->mosaics.push_back(mosaic);
	}

	// Fill in the blob sinks for each camera
	for (size_t i = 0; i < this->mosaics.size(); i++) {
		std::shared_ptr<CameraMosaic> &mosaic = this->mosaics[i];

		for (size_t j = 0; j < mosaic->cameras.size(); j++) {
			Camera *camera = mosaic->cameras[j].get();

			params->mosaics[i].cameras[j].blob_sink = &camera->base;
		}
	}

	std::string data_recorder_output = debug_get_option_constellation_tracker_data_recorder_output();
	if (!data_recorder_output.empty()) {
		this->data_recorder = std::make_unique<DataRecorder>(this, data_recorder_output);
		CT_INFO(this, "Constellation tracker data recorder enabled, outputting to %s",
		        data_recorder_output.c_str());
	}

	if (debug_get_bool_option_constellation_tracker_enable_rerun()) {
#ifdef XRT_FEATURE_RERUN
		this->rerun_stream = std::make_unique<RerunContext>();
		CT_INFO(this, "Constellation tracker Rerun stream enabled, outputting to constellation_tracker.rerun");

		if (debug_get_bool_option_constellation_tracker_rerun_spawn()) {
			this->rerun_stream->spawnViewer();
		} else {
			this->rerun_stream->stream->connect_grpc().exit_on_failure();
		}
#else
		CT_ERROR(this, "Rerun stream requested but XRT_FEATURE_RERUN is not enabled");
#endif
	}

	this->rig_calibration = std::make_unique<rig_calibration::RigCalibration>();

	for (auto &mosaic : this->mosaics) {
		for (auto &camera : mosaic->cameras) {
			// The solve works in OpenCV space, and its world is the tracking origin: that is the frame the
			// solved poses are written back into below in checkSensorCalibration.
			xrt_pose Tcv_origin_cam;
			math_pose_convert_from_opencv(&camera->locked_data.Txr_origin_cam, &Tcv_origin_cam);

			// Add the camera to the sensor calibration job
			this->rig_calibration->addCamera({
			    .mosaic_idx = mosaic->index,
			    .camera_idx = camera->index,
			    .params = camera->model,
			    .Tcv_world_cam = Tcv_origin_cam,
			    .has_concrete_pose = camera->locked_data.has_concrete_pose,
			});
		}
	}

	CT_DEBUG(this, "Created constellation tracker with %zu mosaics", this->mosaics.size());
}

ConstellationTracker::~ConstellationTracker()
{
	if (this->rig_calibration_thread.initialized) {
		os_thread_helper_destroy(&this->rig_calibration_thread);
	}

	CT_DEBUG(this, "Destroying constellation tracker");
}

bool
ConstellationTracker::needSensorCalibration()
{
	bool need_sensor_calibration = false;

	// Check if any cameras need calibration.
	for (const auto &mosaic : this->mosaics) {
		for (const auto &camera : mosaic->cameras) {
			if (!camera->locked_data.has_concrete_pose) {
				need_sensor_calibration = true;
			}
		}
	}

	return need_sensor_calibration;
}

bool
ConstellationTracker::checkSensorCalibration()
{
	std::unique_lock<os::Mutex> lock(this->rig_calibration_lock);

	if (this->running_calibration) {
		return true;
	}

	size_t num_cameras = 0;
	for (const auto &mosaic : this->mosaics) {
		num_cameras += mosaic->cameras.size();
	}

	// No calibration needed
	if (!this->needSensorCalibration()) {
		return true;
	}

	auto &calib = this->rig_calibration;

	// Start collecting data if we haven't
	if (!calib->isCollectingData()) {
		calib->beginCollectingData();

		// Enable the first device, disable all others, for the purposes of clean data.
		bool is_first = true;
		for (auto &device : this->devices) {
			device->enabled.store(is_first);
			is_first = false;
		}
	}

	size_t num_samples;
	if (!calib->numSamplesForBestDevice(num_samples)) {
		// Nothing yet
		return true;
	}

	// @todo Figure out a good number for this, it probably is going to depend on time, "seen by multiple cameras at
	//       once", and other factors rather than pure sample count?
	if (num_samples < (num_cameras * 1500)) {
		return true;
	}

	// Stop collecting data
	calib->endCollectingData();

	if (!this->rig_calibration_thread.initialized) {
		os_thread_helper_init(&this->rig_calibration_thread);
	}

	// If one was already running, wait for it to stop.
	os_thread_helper_stop_and_wait(&this->rig_calibration_thread);

	// Mark that we're now going to be running one
	this->running_calibration = true;

	// Try to start the new thread
	os_thread_helper_start(&this->rig_calibration_thread, run_room_calibration_func, this);

	return true;
}

void
ConstellationTracker::setupVariableTracking()
{
	u_var_add_root(this, "Constellation Tracker", true);
	u_var_add_log_level(this, &this->log_level, "Log Level");

	uint32_t mosaic_idx = 0;
	for (auto &mosaic : this->mosaics) {
		uint32_t camera_idx = 0;
		for (auto &camera : mosaic->cameras) {
			auto str =
			    "Camera " + std::to_string(camera_idx) + " (Mosaic " + std::to_string(mosaic_idx) + ")";
			u_var_add_gui_header(this, nullptr, str.c_str());

			u_var_add_sink_debug(this, &camera->fast_processing_thread_data.debug_sink,
			                     "Blob Debug Sink (Fast)");
			u_var_add_sink_debug(this, &camera->slow_processing_thread_data.debug_sink,
			                     "Blob Debug Sink (Slow)");

			u_var_add_pose(this, &camera->locked_data.Txr_origin_cam, "Camera Origin Pose");
			u_var_add_bool(this, &camera->locked_data.has_concrete_pose, "Has Concrete Pose");

			camera->scribble_settings.setupDebugTracking(this);

			camera_idx++;
		}
		mosaic_idx++;
	}
}

t_constellation_device_id_t
ConstellationTracker::addDevice(t_constellation_tracker_device_params *params, t_constellation_tracker_device *device)
{
	if (params->led_model.led_count > XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE) {
		throw std::runtime_error("Device has too many LEDs, maximum is " +
		                         std::to_string(XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE));
	}

	std::unique_lock lock(this->device_lock);

	if (this->devices.size() >= XRT_CONSTELLATION_MAX_DEVICES) {
		throw std::runtime_error("Maximum number of devices already added to constellation tracker");
	}

	t_constellation_device_id_t id = this->next_device_id++;

	this->devices.push_back(std::make_unique<Device>(this, params, device, id));

	CT_DEBUG(this, "Added device with ID %d to constellation tracker", id);

	const auto &new_device = this->devices.back();

	// Enable the device if there's no calibration needed or this is the only device
	if (!this->needSensorCalibration() || this->devices.size() == 1) {
		new_device->enabled.store(true);
	}

	if (this->data_recorder) {
		this->data_recorder->recordDeviceInfo(*new_device);
	}

	new_device->sensor_fusion =
	    std::make_unique<sensor_fusion::SensorFusion>(id, new_device->search_model->led_model);

	for (auto &mosaic : this->mosaics) {
		for (auto &camera : mosaic->cameras) {
			// Add the camera to the sensor fusion job
			new_device->sensor_fusion->addCamera({
			    .mosaic_idx = mosaic->index,
			    .camera_idx = camera->index,
			    .params = camera->model,
			});
		}
	}

	return id;
}

void
ConstellationTracker::removeDevice(t_constellation_device_id_t device_id)
{
	std::unique_lock lock(this->device_lock);

	size_t index = 0;
	for (auto &device : this->devices) {
		if (device->id == device_id) {
			break;
		}
		index++;
	}

	if (index == this->devices.size()) {
		throw std::invalid_argument("The device ID is not present in the device list.");
	}

	// Remove the device
	this->devices.erase(this->devices.begin() + index);
}

void
ConstellationTracker::getTrackedPose(t_constellation_device_id_t device_id,
                                     timepoint_ns when_ns,
                                     xrt_space_relation &out_relation)
{
	out_relation = XRT_SPACE_RELATION_ZERO;

	std::shared_lock lock(this->device_lock);
	for (const auto &device : this->devices) {
		if (device->id != device_id) {
			continue;
		}

		timepoint_ns pose_ts = 0;
		xrt_space_relation relation;
		device->sensor_fusion->getTrackedPose(when_ns, pose_ts, relation);

		// No flags just means pass directly
		if (relation.relation_flags == 0) {
			out_relation = relation;
			return;
		}

		// Predict to the requested time
		m_predict_relation(&relation, time_ns_to_s(when_ns - pose_ts), &out_relation);

		return;
	}

	throw std::runtime_error("Device missing, failed to get tracked pose.");
}

}; // namespace xrt::tracking::constellation

using namespace xrt::tracking::constellation;

void *
constellation_tracker_camera_slow_thread(void *ptr)
{
	Camera *camera = (Camera *)ptr;

	os_thread_helper_lock(&camera->slow_processing_thread);
	while (os_thread_helper_is_running_locked(&camera->slow_processing_thread)) {
		os_thread_helper_wait_locked(&camera->slow_processing_thread);

		std::optional<CameraSample> maybe_sample = camera->slow_processing_thread_data.sample;
		camera->slow_processing_thread_data.sample.reset();

		os_thread_helper_unlock(&camera->slow_processing_thread);

		if (auto sample = maybe_sample) {
			camera->processSampleSlow(*sample);
		}

		os_thread_helper_lock(&camera->slow_processing_thread);
	}
	os_thread_helper_unlock(&camera->slow_processing_thread);

	return NULL;
}

void *
constellation_tracker_camera_fast_thread(void *ptr)
{
	Camera *camera = (Camera *)ptr;

	os_thread_helper_lock(&camera->fast_processing_thread);
	while (os_thread_helper_is_running_locked(&camera->fast_processing_thread)) {
		os_thread_helper_wait_locked(&camera->fast_processing_thread);

		std::optional<CameraSample> maybe_sample = camera->fast_processing_thread_data.sample;
		camera->fast_processing_thread_data.sample.reset();

		os_thread_helper_unlock(&camera->fast_processing_thread);

		if (auto sample = maybe_sample) {
			if (camera->processSampleFast(*sample)) {
				CT_TRACE(camera->tracker,
				         "Fast processing for camera %p failed, deferring to slow thread",
				         (void *)camera);
				camera->deferSampleToSlowThread(*sample);
			}
		}

		os_thread_helper_lock(&camera->fast_processing_thread);
	}
	os_thread_helper_unlock(&camera->fast_processing_thread);

	return NULL;
}

void
constellation_tracker_camera_push_blobs(t_blob_sink *tbs, t_blob_observation *tbo)
{
	Camera *camera = Camera::Get(tbs);
	ConstellationTracker *tracker = camera->tracker;

	CT_TRACE(tracker, "Received blob observation at %" PRIi64 " with %u blobs", tbo->timestamp_ns, tbo->num_blobs);

	if (tbo->num_blobs == 0) {
		CT_TRACE(tracker, "No blobs in observation, skipping processing");
		return;
	}

	if (tracker->single_threaded) {
		auto sample = CameraSample(*tbo, camera);

		// If we're in single-threaded mode, just process the sample immediately on the fast thread
		if (camera->processSampleFast(sample)) {
			CT_TRACE(tracker,
			         "Fast processing for camera %p failed in single-threaded mode, doing slow processing",
			         (void *)camera);

			camera->processSampleSlow(sample);
		}
	} else {
		// Send to the fast thread
		os_thread_helper_lock(&camera->fast_processing_thread);
		{
			if (camera->fast_processing_thread_data.sample.has_value()) {
				// Warn that we're dropping a frame
				CT_WARN(tracker,
				        "Dropping fast sample %" PRIu64 " at ts %" PRIi64
				        ". Tracker is likely running slow.",
				        camera->fast_processing_thread_data.sample->id,
				        camera->fast_processing_thread_data.sample->timestamp_ns);
			}

			camera->fast_processing_thread_data.sample = CameraSample(*tbo, camera);
			os_thread_helper_signal_locked(&camera->fast_processing_thread);
		}
		os_thread_helper_unlock(&camera->fast_processing_thread);
	}
}

void
constellation_tracker_camera_destroy(t_blob_sink *tbs)
{
	// do nothing, the constellation tracker will clean up the blob sinks when it is destroyed.
}

void
constellation_tracker_node_break_apart(xrt_frame_node *node)
{
	ConstellationTracker *tracker = ConstellationTracker::Get(node);

	tracker->running = false;

	// Stop all the threads
	for (auto &mosaic : tracker->mosaics) {
		for (auto &camera : mosaic->cameras) {
			if (camera->slow_processing_thread.initialized) {
				os_thread_helper_stop_and_wait(&camera->slow_processing_thread);
			}

			if (camera->fast_processing_thread.initialized) {
				os_thread_helper_stop_and_wait(&camera->fast_processing_thread);
			}
		}
	}
}

void
constellation_tracker_node_destroy(xrt_frame_node *node)
{
	ConstellationTracker *tracker = ConstellationTracker::Get(node);

	u_var_remove_root(tracker);

	delete tracker;
}

int
t_constellation_tracker_create(xrt_frame_context *xfctx,
                               t_constellation_tracker_params *params,
                               t_constellation_tracker **out_tracker)
{
	try {
		ConstellationTracker *tracker = new ConstellationTracker(params);

		// Add us to the frame context
		xrt_frame_context_add(xfctx, &tracker->node);

		*out_tracker = (t_constellation_tracker *)tracker;

		tracker->setupVariableTracking();
	} catch (const std::exception &e) {
		U_LOG_E("Failed to create constellation tracker: %s", e.what());
		return -1;
	}

	return 0;
}

int
t_constellation_tracker_add_device(t_constellation_tracker *raw_tracker,
                                   t_constellation_tracker_device_params *params,
                                   t_constellation_tracker_device *device,
                                   t_constellation_device_id_t *out_device_id)
{
	ConstellationTracker *tracker = ConstellationTracker::Get(raw_tracker);

	try {
		t_constellation_device_id_t device_id = tracker->addDevice(params, device);
		*out_device_id = device_id;
	} catch (const std::exception &e) {
		CT_ERROR(tracker, "Failed to add device to constellation tracker: %s", e.what());
		return -1;
	}

	return 0;
}

int
t_constellation_tracker_remove_device(t_constellation_tracker *raw_tracker, t_constellation_device_id_t device)
{
	ConstellationTracker *tracker = ConstellationTracker::Get(raw_tracker);

	try {
		tracker->removeDevice(device);
	} catch (const std::exception &e) {
		CT_ERROR(tracker, "Failed to remove device from constellation tracker: %s", e.what());
		return -1;
	}

	return 0;
}

xrt_tracking_origin *
t_constellation_tracker_get_tracking_origin(t_constellation_tracker *raw_tracker)
{
	ConstellationTracker *tracker = ConstellationTracker::Get(raw_tracker);

	return &tracker->tracking_origin;
}

int
t_constellation_tracker_get_tracked_pose(struct t_constellation_tracker *raw_tracker,
                                         t_constellation_device_id_t device,
                                         timepoint_ns when_ns,
                                         struct xrt_space_relation *out_relation)
{
	ConstellationTracker *tracker = ConstellationTracker::Get(raw_tracker);

	try {
		tracker->getTrackedPose(device, when_ns, *out_relation);
	} catch (const std::exception &e) {
		CT_ERROR(tracker, "Failed to get the tracked pose for a device from constellation tracker: %s",
		         e.what());
		return -1;
	}

	return 0;
}
