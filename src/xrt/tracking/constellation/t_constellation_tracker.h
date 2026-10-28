// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining the constellation tracker parameters and functions.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_tracking.h"

#include "tracking/t_constellation.h"

#include "tracking/t_tracking.h"


#ifdef __cplusplus
extern "C" {
#endif

#define XRT_CONSTELLATION_MAX_TRACKING_MOSAICS (1)

struct t_constellation_tracker;

/*!
 * @public @memberof t_constellation_tracker

 * A constellation tracker camera is a single camera that the constellation tracker will use to track devices. The
 * constellation tracker will provide the blob sink for this camera.
 */
struct t_constellation_tracker_camera
{
	//! The calibration for this camera
	struct t_camera_calibration calibration;

	//! The position of this camera, in the mosaic's tracking origin.
	struct xrt_pose pose_in_origin;
	/*
	 * Whether this camera has a concrete pose in the tracking origin, or if we don't know the current position and
	 * need to compute it.
	 */
	bool has_concrete_pose;

	/*!
	 * The blob sink for this camera, this is an out parameter filled in by the constellation tracker, and you are
	 * expected to pass this to your blobwatch implementation.
	 */
	struct t_blob_sink *blob_sink;
};

/*!
 * @public @memberof t_constellation_tracker
 *
 * A constellation tracker camera mosaic is a set of cameras that may or may not be physically attached, but
 * importantly, they all fire at the same time and are synchronized with each other.
 */
struct t_constellation_tracker_camera_mosaic
{
	/*
	 * The constellation tracking source for this mosaic, if any. This is used as the origin of any cameras in the
	 * mosaic.
	 */
	struct t_constellation_tracker_tracking_source *tracking_origin;

	//! The cameras in this mosaic, with their blob sinks filled in by the constellation tracker.
	struct t_constellation_tracker_camera cameras[XRT_TRACKING_MAX_CAMS];
	//! The number of cameras in this mosaic.
	size_t num_cameras;
};

/*!
 * @public @memberof t_constellation_tracker
 *
 * Parameters for adding a device to the constellation tracker.
 */
struct t_constellation_tracker_device_params
{
	//! The constellation pattern for this device.
	struct t_constellation_tracker_led_model led_model;

	/*
	 * An optional tracking source to give the constellation tracker extra information to better throw out bad
	 * guesses when finding the device.
	 */
	struct t_constellation_tracker_tracking_source *tracking_source;

	/*!
	 * The IMU sink for this device. This pointer is filled in by the constellation tracker. After adding the device
	 * to the constellation tracker, drivers should send IMU samples to this sink.
	 */
	struct xrt_imu_sink *imu_sink;
};

enum t_constellation_tracker_flags
{
	T_CONSTELLATION_TRACKER_FLAGS_NONE = 0,
	/*!
	 * Whether the constellation tracker should be deterministic.
	 * Turn this on if you require consistent results, at the cost of performance.
	 */
	T_CONSTELLATION_TRACKER_FLAGS_DETERMINISTIC = 1 << 0,
};

/*!
 * @public @memberof t_constellation_tracker
 */
struct t_constellation_tracker_params
{
	//! Flags for the constellation tracker, see t_constellation_tracker_flags
	enum t_constellation_tracker_flags flags;
	struct t_constellation_tracker_camera_mosaic mosaics[XRT_CONSTELLATION_MAX_TRACKING_MOSAICS];
	size_t num_mosaics;
};

/*!
 * @public @memberof t_constellation_tracker
 */
int
t_constellation_tracker_create(struct xrt_frame_context *xfctx,
                               struct t_constellation_tracker_params *params,
                               struct t_constellation_tracker **out_tracker);

int
t_constellation_tracker_add_device(struct t_constellation_tracker *tracker,
                                   struct t_constellation_tracker_device_params *params,
                                   struct t_constellation_tracker_device *device,
                                   t_constellation_device_id_t *out_device_id);

int
t_constellation_tracker_remove_device(struct t_constellation_tracker *tracker, t_constellation_device_id_t device);

struct xrt_tracking_origin *
t_constellation_tracker_get_tracking_origin(struct t_constellation_tracker *tracker);

int
t_constellation_tracker_get_tracked_pose(struct t_constellation_tracker *tracker,
                                         t_constellation_device_id_t device,
                                         timepoint_ns when_ns,
                                         struct xrt_space_relation *out_relation);

#ifdef __cplusplus
}
#endif
