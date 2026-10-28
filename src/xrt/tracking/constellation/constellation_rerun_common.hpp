// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Shared internal helpers for every constellation rerun recorder.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_frame.h"

#include "math/m_api.h"
#include "math/m_vec3.h"

#include "tracking/t_tracking.h"
#include "tracking/t_constellation.h"
#include "tracking/t_camera_models.hpp"

#include <rerun.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <memory>
#include <string>
#include <vector>


namespace xrt::tracking::constellation {

using namespace xrt::auxiliary::tracking;

/*
 *
 * Shared constants
 *
 */

//! How large a LED is drawn in the 3D views, in meters.
constexpr float kLedPointRadiusM = 0.003f;
constexpr float kAxisLength = 0.03f;
constexpr float kBlobRadiusPixels = 3.0f;
constexpr float kTrajectoryPointRadiusM = 0.005f;
constexpr float kTrajectoryLineRadiusM = 0.002f;

/*
 *
 * Helper functions
 *
 */

//! The pose the solved poses predict this camera would have observed the device at.
inline xrt_pose
predictedCameraDevicePose(const xrt_pose &Tcv_world_cam, const xrt_pose &Tcv_world_device)
{
	xrt_pose Tcv_cam_world;
	math_pose_invert(&Tcv_world_cam, &Tcv_cam_world);

	xrt_pose Tcv_cam_device;
	math_pose_transform(&Tcv_cam_world, &Tcv_world_device, &Tcv_cam_device);

	return Tcv_cam_device;
}

/*!
 * The solved LED model pose. The solvers hold the IMU's pose, and their camera factors see the LEDs through
 * `Qcv_imu_model` on top of it about a shared origin, so anything drawn or measured against the LED model has to go
 * through this rather than the bare IMU pose.
 */
inline xrt_pose
solvedWorldModelPose(const xrt_pose &Tcv_world_imu, const xrt_quat &Qcv_imu_model)
{
	xrt_pose Tcv_world_model = Tcv_world_imu;
	math_quat_rotate(&Tcv_world_imu.orientation, &Qcv_imu_model, &Tcv_world_model.orientation);

	return Tcv_world_model;
}

//! The angle between two orientations, in degrees.
inline float
orientationErrorDegrees(const xrt_quat &a, const xrt_quat &b)
{
	xrt_quat a_inverse;
	math_quat_invert(&a, &a_inverse);

	xrt_quat error;
	math_quat_rotate(&a_inverse, &b, &error);

	/*
	 * Clamped because numerical drift, or a quaternion which is very slightly denormalized, would otherwise hand
	 * std::acos a value outside of its domain.
	 */
	float w = std::min(std::abs(error.w), 1.0f);

	return static_cast<float>(2.0 * std::acos(w) * (180.0 / M_PI));
}

inline float
reprojectionErrorPixels(const t_camera_model_params &dist,
                        const xrt_pose &Tcv_world_cam,
                        const xrt_pose &Tcv_world_device,
                        uint32_t num_points,
                        std::span<const xrt_vec2> points2d,
                        std::span<const xrt_vec3> points3d)
{
	const xrt_pose Tcv_cam_device = predictedCameraDevicePose(Tcv_world_cam, Tcv_world_device);

	double sum_squared_error = 0.0;
	size_t num_projected = 0;

	for (size_t i = 0; i < num_points; i++) {
		xrt_vec3 T_cam_led;
		math_quat_rotate_vec3(&Tcv_cam_device.orientation, &points3d[i], &T_cam_led);
		T_cam_led = T_cam_led + Tcv_cam_device.position;

		if (T_cam_led.z <= 0.0f) {
			// Behind the camera, so there is no pixel to compare against.
			continue;
		}

		float projected_x = 0.0f;
		float projected_y = 0.0f;
		if (!camera_models::project<float>(dist, T_cam_led.x, T_cam_led.y, T_cam_led.z, projected_x,
		                                   projected_y)) {
			continue;
		}

		const float error_x = projected_x - points2d[i].x;
		const float error_y = projected_y - points2d[i].y;

		sum_squared_error += (error_x * error_x) + (error_y * error_y);
		num_projected++;
	}

	if (num_projected == 0) {
		return 0.0f;
	}

	return static_cast<float>(std::sqrt(sum_squared_error / static_cast<double>(num_projected)));
}

/*
 *
 * Recording streams
 *
 */

/*!
 * The recording stream a rerun recorder logs through.
 *
 * Every recorder owns exactly one stream and reaches it the same way, so they inherit this rather than each repeating
 * the pointer and its construction. The recording id stays a per-recorder choice, since it is what separates one
 * recorder's data from another's in the viewer.
 */
struct RerunStream
{
public: // Fields
	std::unique_ptr<rerun::RecordingStream> stream{};

public: // Methods
	explicit RerunStream(const std::string_view &recording_id)
	    : stream(std::make_unique<rerun::RecordingStream>(recording_id))
	{}

	/*!
	 * Starts a viewer for this stream and points the stream at it.
	 *
	 * Fatal on failure, because a recorder which was explicitly asked for and cannot be reached is a debugging
	 * session that would otherwise run to completion and silently record nothing.
	 */
	void
	spawnViewer() const
	{
		this->stream->spawn().exit_on_failure();
	}
};

/*
 *
 * Conversions into rerun's types
 *
 */

inline rerun::components::Translation3D
toRerunTranslation(const xrt_vec3 &position)
{
	return rerun::components::Translation3D(position.x, position.y, position.z);
}

inline rerun::Rotation3D
toRerunRotation(const xrt_quat &orientation)
{
	return rerun::Rotation3D(
	    rerun::datatypes::Quaternion::from_xyzw(orientation.x, orientation.y, orientation.z, orientation.w));
}

inline rerun::Transform3D
toRerunTransform(const xrt_pose &pose, bool from_parent = true)
{
	auto transform = rerun::Transform3D()
	                     .with_translation(toRerunTranslation(pose.position))
	                     .with_rotation(toRerunRotation(pose.orientation));

	if (from_parent) {
		transform = std::move(transform).with_relation(rerun::components::TransformRelation::ParentFromChild);
	}

	return transform;
}

//! The camera's intrinsics as the projection rerun needs to place an image in the 3D scene.
inline rerun::Pinhole
makePinhole(const t_camera_calibration &calibration)
{
	// Construct the 3x3 intrinsic matrix in column-major order.
	std::array<float, 9> image_from_camera = {
	    static_cast<float>(calibration.intrinsics[0][0]),
	    static_cast<float>(calibration.intrinsics[1][0]),
	    static_cast<float>(calibration.intrinsics[2][0]),
	    //
	    static_cast<float>(calibration.intrinsics[0][1]),
	    static_cast<float>(calibration.intrinsics[1][1]),
	    static_cast<float>(calibration.intrinsics[2][1]),
	    //
	    static_cast<float>(calibration.intrinsics[0][2]),
	    static_cast<float>(calibration.intrinsics[1][2]),
	    static_cast<float>(calibration.intrinsics[2][2]),
	};

	return rerun::Pinhole(rerun::components::PinholeProjection(image_from_camera))
	    .with_resolution(calibration.image_size_pixels.w, calibration.image_size_pixels.h)
	    .with_image_plane_distance(0.2f);
}

inline rerun::Image
makeImage(const xrt_frame &frame)
{
	// We only support L8 format for now
	assert(frame.format == XRT_FORMAT_L8);

	std::vector<uint8_t> image_data(frame.width * frame.height);
	// Copy the frame data into the image_data vector, accounting for stride
	for (uint32_t y = 0; y < frame.height; ++y) {
		std::memcpy(&image_data[y * frame.width], &frame.data[y * frame.stride], frame.width);
	}

	// Create a rerun image from the xrt_frame data.
	return rerun::Image(
	    rerun::archetypes::Image::from_grayscale8(std::move(image_data), {frame.width, frame.height})
	        .with_opacity(0.5f));
}

/*
 *
 * Colours
 *
 */

//! A stable colour per device, so one device keeps its colour across every recorder and every view.
inline rerun::components::Color
deviceColor(t_constellation_device_id_t device_id, float brightness)
{
	// Simple deterministic hash for device ID, ensuring device ID is non-zero
	uint8_t r = (((device_id + 1) * 37) % 127) + 128;
	uint8_t g = (((device_id + 1) * 57) % 127) + 128;
	uint8_t b = (((device_id + 1) * 97) % 127) + 128;

	return rerun::components::Color(r, g, b, static_cast<uint8_t>(255 * brightness));
}

//! A stable colour per camera, chosen the same way as @ref deviceColor and for the same reason.
inline rerun::components::Color
cameraColor(size_t mosaic_idx, size_t camera_idx, float brightness)
{
	// Simple deterministic hash for the camera, ensuring the seed is non-zero
	size_t seed = (mosaic_idx * 16) + camera_idx + 1;

	uint8_t r = ((seed * 71) % 127) + 128;
	uint8_t g = ((seed * 113) % 127) + 128;
	uint8_t b = ((seed * 29) % 127) + 128;

	return rerun::components::Color(r, g, b, static_cast<uint8_t>(255 * brightness));
}

/*
 *
 * Entity names shared between recorders
 *
 */

//! The root every 3D entity hangs off, so the recorders agree on where the world is.
inline std::string_view
getWorldEntityName()
{
	return "world";
}

//! Where a camera sits in the world. Its image, observations and metrics are logged under this.
inline std::string
getWorldCameraEntityName(size_t mosaic_idx, size_t camera_idx)
{
	return std::format("{}/cameras/{}/{}", getWorldEntityName(), mosaic_idx, camera_idx);
}

}; // namespace xrt::tracking::constellation
