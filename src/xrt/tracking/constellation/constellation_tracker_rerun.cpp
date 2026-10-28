// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the rerun recorder logic for the constellation tracker.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "constellation_tracker_rerun.hpp"
#include "constellation_tracker_rerun_blobwatch.h"

#include "math/m_api.h"

#include <algorithm>
#include <cstdint>
#include <format>


using namespace xrt::auxiliary;
using namespace xrt::tracking::constellation;

// Anonymous namespace for internal functions.
namespace {

/*
 *
 * Helper functions
 *
 */

rerun::components::Color
unmatchedBlobColor(float brightness)
{
	uint8_t r = 255;
	uint8_t g = 255;
	uint8_t b = 0;

	return rerun::components::Color(r, g, b, static_cast<uint8_t>(255 * brightness));
}

/*
 *
 * Common entity names
 *
 */

static constexpr std::string timeline_name = "keyframes";

std::string
getCameraImageEntityName(size_t mosaic_idx, size_t camera_idx)
{
	return std::format("{}/image", getWorldCameraEntityName(mosaic_idx, camera_idx));
}

std::string
getCameraImageBlobsEntityName(size_t mosaic_idx, size_t camera_idx)
{
	return std::format("{}/blobs", getCameraImageEntityName(mosaic_idx, camera_idx));
}

std::string
getCameraTrackedDeviceEntityName(const CameraSample &camera_sample, t_constellation_device_id_t device_id)
{
	return std::format("{}/tracked_devices/{}",
	                   getWorldCameraEntityName(camera_sample.mosaic_index, camera_sample.camera_index), device_id);
}

std::string
getCameraTrackedDevicePoseEntityName(const CameraSample &camera_sample,
                                     t_constellation_device_id_t device_id,
                                     bool prior)
{
	return std::format("{}/{}", getCameraTrackedDeviceEntityName(camera_sample, device_id),
	                   prior ? "prior_pose" : "pose");
}

}; // namespace

namespace xrt::tracking::constellation {

/*
 *
 * Private methods
 *
 */

void
RerunContext::logStaticScene(const CameraSample &camera_sample, const t_camera_calibration &calibration)
{
	std::string camera_entity = getWorldCameraEntityName(camera_sample.mosaic_index, camera_sample.camera_index);
	std::string camera_image_entity =
	    getCameraImageEntityName(camera_sample.mosaic_index, camera_sample.camera_index);

	this->stream->log_static("/", rerun::ViewCoordinates::RIGHT_HAND_Y_DOWN);
	this->stream->log_static(getWorldEntityName(), rerun::TransformAxes3D(kAxisLength));
	this->stream->log_static(camera_entity + "/axes", rerun::TransformAxes3D(kAxisLength));
	this->stream->log_static(camera_image_entity, makePinhole(calibration));
}

void
RerunContext::logLedModel(const std::string &entity_name,
                          t_constellation_device_id_t device_id,
                          const t_constellation_tracker_led_model &led_model,
                          bool prior)
{
	std::vector<rerun::Position3D> positions;
	std::vector<rerun::Radius> radii;
	std::vector<rerun::components::Color> colors;
	std::vector<rerun::components::Text> labels;
	positions.reserve(led_model.led_count);
	radii.reserve(led_model.led_count);
	colors.reserve(led_model.led_count);
	labels.reserve(led_model.led_count);

	for (size_t i = 0; i < led_model.led_count; i++) {
		const t_constellation_tracker_led &led = led_model.leds[i];
		positions.emplace_back(led.position.x, led.position.y, led.position.z);
		radii.emplace_back(led.radius_m);
		colors.emplace_back(deviceColor(device_id, prior ? 0.1f : 1.0f)); // Full brightness for LED model
		labels.emplace_back(std::to_string(led.id));
	}

	this->stream->log_static(entity_name + "/axes", rerun::TransformAxes3D(kAxisLength));
	this->stream->log_static(      //
	    entity_name + "/leds",     //
	    rerun::Points3D(positions) //
	        .with_radii(radii)     //
	        .with_colors(colors)   //
	        .with_labels(labels)   //
	);                             //
}

void
RerunContext::logBlobSet(const CameraSample &camera_sample)
{
	std::string camera_image_blobs_entity =
	    getCameraImageBlobsEntityName(camera_sample.mosaic_index, camera_sample.camera_index);

	std::vector<rerun::Position2D> positions;
	std::vector<rerun::Radius> radii;
	std::vector<rerun::components::Color> colors;
	std::vector<rerun::components::Text> labels;
	positions.reserve(camera_sample.blob_count);
	radii.reserve(camera_sample.blob_count);
	colors.reserve(camera_sample.blob_count);
	labels.reserve(camera_sample.blob_count);

	for (uint32_t i = 0; i < camera_sample.blob_count; i++) {
		const t_blob &blob = camera_sample.blobs[i];
		bool matched = (blob.matched_device_id != XRT_CONSTELLATION_INVALID_DEVICE_ID);

		// Blobs are offset by 0.5px because in OpenCV distortion integers are pixel centers, but they aren't in
		// Rerun.
		positions.emplace_back(blob.center_distorted.x + 0.5f, blob.center_distorted.y + 0.5f);
		float radius = std::max(std::max(blob.size.x, blob.size.y) * 0.5f, kBlobRadiusPixels);
		radii.emplace_back(rerun::Radius::ui_points(radius));
		colors.emplace_back(matched ? deviceColor(blob.matched_device_id, blob.brightness)
		                            : unmatchedBlobColor(blob.brightness));

		if (matched) {
			labels.emplace_back(std::format("LED {}", blob.matched_device_led_id));
		} else {
			labels.emplace_back(std::format("blob {}", blob.blob_id));
		}
	}

	this->stream->log(             //
	    camera_image_blobs_entity, //
	    rerun::Points2D(positions) //
	        .with_radii(radii)     //
	        .with_colors(colors)   //
	        .with_labels(labels)   //
	);                             //
}

void
RerunContext::logFrameCameraMetrics(const CameraSample &camera_sample)
{
	float brightness = 0.0f;
	for (uint32_t i = 0; i < camera_sample.blob_count; i++) {
		brightness += camera_sample.blobs[i].brightness;
	}
	if (camera_sample.blob_count > 0) {
		brightness /= static_cast<float>(camera_sample.blob_count);
	} else {
		brightness = 0.0f;
	}

	std::string camera_metrics =
	    getWorldCameraEntityName(camera_sample.mosaic_index, camera_sample.camera_index) + "/metrics";

	this->stream->log(camera_metrics + "/brightness", rerun::Scalars(brightness));
	this->stream->log(camera_metrics + "/blob_count", rerun::Scalars(static_cast<float>(camera_sample.blob_count)));
}

void
RerunContext::logFrameDeviceMetrics(const CameraSample &camera_sample, const DeviceState &device_state)
{
	if (!device_state.found_pose.has_value()) {
		return;
	}

	auto &found_pose = device_state.found_pose.value();

	uint32_t matched_blob_count = 0;
	for (uint32_t i = 0; i < camera_sample.blob_count; i++) {
		if (camera_sample.blobs[i].matched_device_id == device_state.device_id) {
			matched_blob_count++;
		}
	}

	std::string device_metrics =
	    std::format("{}/metrics", getCameraTrackedDeviceEntityName(camera_sample, device_state.device_id));

	this->stream->log(device_metrics + "/brightness", rerun::Scalars(found_pose.average_blob_brightness));
	this->stream->log(device_metrics + "/matched_blob_count", rerun::Scalars(matched_blob_count));
}

void
RerunContext::logFoundPose(const CameraSample &camera_sample,
                           const std::unique_ptr<Device> &device,
                           const FoundDevicePose &found_pose)
{
	std::string camera_device_pose_entity = getCameraTrackedDevicePoseEntityName(camera_sample, device->id, false);
	std::string camera_device_covariance_entity = camera_device_pose_entity + "/covariance";

	const xrt_pose &Tcv_cam_device = found_pose.Tcv_cam_device;
	this->stream->log(camera_device_pose_entity, toRerunTransform(Tcv_cam_device));
	this->logLedModel(camera_device_pose_entity, device->id, device->params.led_model, false);

	xrt_vec3 radii;
	xrt_quat Q_cam_cov;
	covariancePositionRadius(found_pose.covariance, radii, Q_cam_cov);

	xrt_pose Tcv_device_cov = XRT_POSE_IDENTITY;
	// Bring the covariance's orientation into camera space
	math_quat_invert(&Tcv_cam_device.orientation, &Tcv_device_cov.orientation);
	// Rotate by the covariance's orientation
	math_quat_rotate(&Tcv_device_cov.orientation, &Q_cam_cov, &Tcv_device_cov.orientation);

	this->stream->log(camera_device_covariance_entity, toRerunTransform(Tcv_device_cov));
	this->stream->log(
	    camera_device_covariance_entity,
	    rerun::archetypes::Ellipsoids3D::from_half_sizes(rerun::components::HalfSize3D(radii.x, radii.y, radii.z)));
}

/*
 *
 * Public methods
 *
 */

void
RerunContext::logSample(const ConstellationTracker &tracker, const CameraSample &camera_sample)
{
	std::string camera_entity = getWorldCameraEntityName(camera_sample.mosaic_index, camera_sample.camera_index);

	const auto &calibration =
	    tracker.mosaics[camera_sample.mosaic_index]->cameras[camera_sample.camera_index]->calibration;

	this->logStaticScene(camera_sample, calibration);
	this->stream->set_time_timestamp_nanos_since_epoch(timeline_name, camera_sample.timestamp_ns);

	std::optional<xrt_pose> Tcv_world_cam = std::nullopt;
	if (camera_sample.Txr_world_cam.has_value()) {
		xrt_pose Tcv_world_cam_value;
		math_pose_convert_from_opencv(&camera_sample.Txr_world_cam.value(), &Tcv_world_cam_value);
		Tcv_world_cam = Tcv_world_cam_value;

		this->stream->log(camera_entity, toRerunTransform(Tcv_world_cam_value));
	}

	for (uint32_t i = 0; i < camera_sample.device_count; i++) {
		const auto &device_state = camera_sample.device_states[i];
		t_constellation_device_id_t device_id = device_state.device_id;

		auto device_iter =
		    std::find_if(tracker.devices.begin(), tracker.devices.end(),
		                 [device_id](const std::unique_ptr<Device> &d) { return d->id == device_id; });
		if (device_iter == tracker.devices.end()) {
			U_LOG_W("Device with ID %u not found in tracker devices.", device_id);
			continue;
		}

		auto &device = *device_iter;

		// Found pose in this frame
		if (device_state.found_pose.has_value()) {
			const auto &found_pose = device_state.found_pose.value();

			/*
			 * Skip this device, it's already been logged. This isn't strictly necessary but it'll save us
			 * some Rerun bandwidth
			 */
			if (found_pose.handled) {
				continue;
			}

			this->logFoundPose(camera_sample, device, found_pose);
		}

		// Prior pose in this frame
		if (device_state.Txr_world_device_prior.has_value() && camera_sample.Txr_world_cam.has_value()) {
			std::string camera_device_prior_entity =
			    getCameraTrackedDevicePoseEntityName(camera_sample, device_id, true);

			xrt_pose Txr_cam_world;
			math_pose_invert(&camera_sample.Txr_world_cam.value(), &Txr_cam_world);

			xrt_pose Txr_cam_device_prior;
			math_pose_transform(&Txr_cam_world,                               //
			                    &device_state.Txr_world_device_prior.value(), //
			                    &Txr_cam_device_prior);                       //

			xrt_pose Tcv_cam_device_prior;
			math_pose_convert_from_opencv(&Txr_cam_device_prior, &Tcv_cam_device_prior);

			this->stream->log(camera_device_prior_entity, toRerunTransform(Tcv_cam_device_prior));
			this->logLedModel(camera_device_prior_entity, device_id, device->params.led_model, true);
		}

		this->logFrameDeviceMetrics(camera_sample, device_state);
	}

	this->logFrameCameraMetrics(camera_sample);
	this->logBlobSet(camera_sample);
}

void
RerunContext::logImageFrame(const ConstellationTracker &tracker,
                            uint32_t mosaic_index,
                            uint32_t camera_index,
                            const xrt_frame &frame)
{
	std::string camera_image_entity = getCameraImageEntityName(mosaic_index, camera_index);

	this->stream->set_time_timestamp_nanos_since_epoch(timeline_name, frame.timestamp);
	this->stream->log(camera_image_entity, makeImage(frame));
}

}; // namespace xrt::tracking::constellation

void
constellation_tracker_rerun_blobwatch_push_frame(struct t_constellation_tracker *tracker,
                                                 uint32_t mosaic_index,
                                                 uint32_t camera_index,
                                                 struct xrt_frame *frame)
{
	ConstellationTracker *ct = ConstellationTracker::Get(tracker);

	if (ct->rerun_stream) {
		ct->rerun_stream->logImageFrame(*ct, mosaic_index, camera_index, *frame);
	}
}
