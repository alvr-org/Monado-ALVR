// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining the tracking system integration in Monado.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t t_constellation_device_id_t;
typedef int8_t t_constellation_led_id_it;

#define XRT_CONSTELLATION_MAX_BLOBS_PER_FRAME 250
/*!
 * The maximum amount of devices the constellation tracker is able to track at once.
 *
 * @note This value is dictated by the device bitmask in the sensor fusion, update that there when this is changed to
 *       another power of two.
 */
#define XRT_CONSTELLATION_MAX_DEVICES 4

/*!
 * The maximum number of LEDs that a single device can have.
 *
 * This is a hard limit since we use some bitfields to track LEDs in the pose optimization.
 * If this ever needs to be raised, you need to go update pose_optimize.cpp to use some other data type.
 */
#define XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE 64

#define XRT_CONSTELLATION_INVALID_DEVICE_ID -1
#define XRT_CONSTELLATION_INVALID_LED_ID -1

/*!
 * A blob is a 2d position in a camera sensor's view that is being tracked. Generally used to
 * represent found LEDs in a camera's sensor.
 *
 * Blobs are given in pixel coordinates, with the origin at the top left of the image, and x
 * going right and y going down. The units are in pixels, but may be subpixel accurate. The tracking
 * system is expected to handle the undistortion of the blob positions.
 */
struct t_blob
{
	/*!
	 * The ID of a blob, which may be used to track it across frames. The meaning of the ID is
	 * up to the tracking system, but it attempts to be consistent across frames for the same
	 * blob.
	 */
	uint32_t blob_id;

	/*!
	 * The device ID this blob is associated with, if any. XRT_CONSTELLATION_INVALID_DEVICE_ID
	 * for unmatched. The tracker is expected to fill this in.
	 */
	t_constellation_device_id_t matched_device_id;

	/*!
	 * The LED ID this blob is associated with, if any. XRT_CONSTELLATION_INVALID_LED_ID for
	 * unmatched. The tracker is expected to fill this in.
	 */
	t_constellation_led_id_it matched_device_led_id;

	//! Centre of blob, in distorted pixels
	struct xrt_vec2 center_distorted;

	//! Center of blob, in undistorted pixels
	struct xrt_vec2 center_undistorted;

	//! Center of blob, in homogenized coordinates
	struct xrt_vec3 center_homogenized;

	/*!
	 * Estimated motion vector of blob, in distorted pixels per second. Only valid if the tracking system
	 * provides it.
	 */
	struct xrt_vec2 motion_vector;

	//! The bounding box of the blob in distorted pixel coordinates.
	struct xrt_rect bounding_box;

	//! The size of the blob, in distorted pixels. May be {0,0}, and may be subpixel accurate.
	struct xrt_vec2 size;

	/*!
	 * The brightness of the brightest pixel of the blob, on a scale from 0.0 (black) to 1.0 (pure white).
	 * Set to 1.0 for non-brightness-aware blobwatches.
	 */
	float brightness;
};

struct t_blob_observation
{
	struct t_blobwatch *source;

	/*!
	 * Internal ID for this observation, may be set by the blobwatch implementation if it needs
	 * to know this.
	 */
	uint64_t id;

	//! The sequence ID of this mosaic, must be stable across all frames of a mosaic. Monotonically increases.
	uint64_t sequence_id;

	int64_t timestamp_ns;
	struct t_blob *blobs;
	uint32_t num_blobs;
};

/*!
 * @interface t_blob_sink
 *
 * A generic interface to allow a tracking system to receive "snapshots" of seen @ref t_blob in a
 * frame.
 */
struct t_blob_sink
{
	/*!
	 * Push a set of blobs into the sink. The tracking system will typically call this once per
	 * frame for each camera view.
	 *
	 * @param[in] tbs         The sink to push the blobs into.
	 * @param[in] observation The blob observation to push into the sink.
	 */
	void (*push_blobs)(struct t_blob_sink *tbs, struct t_blob_observation *observation);

	/*!
	 * Destroy this blob sink.
	 */
	void (*destroy)(struct t_blob_sink *tbs);
};

/*!
 * Helper function for @ref t_blob_sink::push_blobs.
 *
 * @copydoc t_blob_sink::push_blobs
 *
 * @public @memberof t_blob_sink
 */
XRT_NONNULL_ALL static inline void
t_blob_sink_push_blobs(struct t_blob_sink *tbs, struct t_blob_observation *tbo)
{
	tbs->push_blobs(tbs, tbo);
}

/*!
 * Helper function for @ref t_blob_sink::destroy.
 *
 * Handles nulls, sets your pointer to null.
 *
 * @public @memberof t_blob_sink
 */
XRT_NONNULL_ALL static inline void
t_blob_sink_destroy(struct t_blob_sink **tbs_ptr)
{
	struct t_blob_sink *tbs = *tbs_ptr;

	if (tbs == NULL) {
		return;
	}

	tbs->destroy(tbs);
	*tbs_ptr = NULL;
}


struct t_blobwatch
{
	/*!
	 * Notify the blobwatch that the blobs in the given observation with the correct ID set are
	 * associated with the given device. The blobwatch can use this information to track which
	 * blobs are associated with which devices across frames, and to provide this information to
	 * the tracker across frames to save it from doing that work again.
	 *
	 * @param[in] tbw       The blobwatch to mark the blobs for.
	 * @param[in] tbo       The observation containing the blobs to mark. The blobwatch will look at
	 *                      the blob IDs and the matched_device_id field to determine which blobs
	 *                      internally to mark with the given device ID.
	 * @param[in] device_id The device ID to mark
	 */
	void (*mark_blob_device)(struct t_blobwatch *tbw,
	                         const struct t_blob_observation *tbo,
	                         t_constellation_device_id_t device_id);

	/*!
	 * Destroy this blobwatch.
	 */
	void (*destroy)(struct t_blobwatch *tbw);
};

/*!
 * Helper function for @ref t_blobwatch::mark_blob_device.
 *
 * @copydoc t_blobwatch::mark_blob_device
 *
 * @public @memberof t_blobwatch
 */
XRT_NONNULL_ALL static inline void
t_blobwatch_mark_blob_device(struct t_blobwatch *tbw,
                             const struct t_blob_observation *tbo,
                             t_constellation_device_id_t device_id)
{
	tbw->mark_blob_device(tbw, tbo, device_id);
}

/*!
 * Helper function for @ref t_blobwatch::destroy.
 *
 * Handles nulls, sets your pointer to null.
 *
 * @public @memberof t_blobwatch
 */
XRT_NONNULL_ALL static inline void
t_blobwatch_destroy(struct t_blobwatch **tbw_ptr)
{
	struct t_blobwatch *tbw = *tbw_ptr;

	if (tbw == NULL) {
		return;
	}

	tbw->destroy(tbw);
	*tbw_ptr = NULL;
}


/*!
 * @interface t_constellation_tracker_tracking_source
 *
 * A constellation tracker tracking source is an arbitrary source of tracking data for the
 * constellation tracker. This is used by the constellation tracker to get the current pose of a
 * device to eliminate bad guesses, or if a camera is anchored to a tracking source (a camera on a
 * headset device), this can be used by the constellation tracker to locate that camera relative to
 * the world.
 */
struct t_constellation_tracker_tracking_source
{
	void (*get_tracked_pose)(struct t_constellation_tracker_tracking_source *,
	                         int64_t when_ns,
	                         struct xrt_space_relation *out_relation);
};

/*!
 * Helper function for @ref t_constellation_tracker_tracking_source::get_tracked_pose.
 *
 * @copydoc t_constellation_tracker_tracking_source::get_tracked_pose
 *
 * @public @memberof t_constellation_tracker_tracking_source
 */
XRT_NONNULL_ALL static inline void
t_constellation_tracker_tracking_source_get_tracked_pose(
    struct t_constellation_tracker_tracking_source *tracking_source,
    int64_t when_ns,
    struct xrt_space_relation *out_relation)
{
	tracking_source->get_tracked_pose(tracking_source, when_ns, out_relation);
}


struct t_constellation_tracker_led
{
	//! The position of the LED in the model.
	struct xrt_vec3 position;
	//! The normal of the LED, determines where it is facing
	struct xrt_vec3 normal;
	//! The visible radius of the LED in meters.
	float radius_m;
	//! The angle from dead on where the LED is no longer visible, in radians.
	float visibility_angle;
	//! A unique ID for this LED, which distinguishes it from all other LEDs.
	t_constellation_led_id_it id;
};

struct t_constellation_tracker_led_model_match_parameters
{
	//! The minimum number of LEDs required to accept a brute-force solve without a prior.
	uint32_t min_leds_for_correspondence_search_without_prior;
	/*!
	 * The minimum number of LEDs required to accept a brute-force solve with a prior that said solve agrees with.
	 * If the solve has this many LEDs, but it doesn't agree with the prior, then the solve is only accepted if it
	 * has @ref min_leds_for_correspondence_search_without_prior LEDs matched.
	 */
	uint32_t min_leds_for_correspondence_search_with_prior;
};

/*!
 * @interface t_constellation_tracker_led_model
 *
 * The LED model is a series of points which define the real-world positions of all LEDs. Some LED
 * models may have self-occluding areas, such as WMR, where inner LEDs can be blocked by the ring,
 * such occlusions are modelled through the LED visibility computation function.
 */
struct t_constellation_tracker_led_model
{
	//! The LEDs in this model.
	struct t_constellation_tracker_led *leds;
	//! The number of LEDs in this model.
	size_t led_count;

	//! The match parameters to tweak how the tracker works.
	struct t_constellation_tracker_led_model_match_parameters match_parameters;

	/*!
	 * A function to compute whether a given LED is visible from a given position. This is used
	 * to allow devices to better model complex occlusion scenarios, like the inward facing LEDs
	 * on the WMR rings.
	 *
	 * @param led_model The LED model containing the LED in question.
	 * @param led       The index of the LED in question in the model.
	 * @param T_obj_cam The transform from the root of the LED model to the camera. Assume
	 *                  camera is facing the origin of the LED model.
	 *
	 * @return Whether the LED is visible from the given position.
	 */
	bool (*compute_led_visibility)(struct t_constellation_tracker_led_model *led_model,
	                               size_t led,
	                               struct xrt_vec3 T_obj_cam);
};

/*!
 * Helper function for @ref t_constellation_tracker_led_model::compute_led_visibility.
 *
 * @copydoc t_constellation_tracker_led_model::compute_led_visibility
 *
 * @public @memberof t_constellation_tracker_led_model
 */
XRT_NONNULL_ALL static inline bool
t_constellation_tracker_led_model_compute_led_visibility(struct t_constellation_tracker_led_model *led_model,
                                                         size_t led,
                                                         struct xrt_vec3 T_obj_cam)
{
	return led_model->compute_led_visibility(led_model, led, T_obj_cam);
}


struct t_constellation_tracker_sample_metrics
{
	//! The amount of blobs that were matched
	uint32_t matched_blob_count;
	//! The amount of LEDs that should be visible
	uint32_t visible_led_count;
	//! The reprojection error of the solve's LEDs to the blobs, in pixels.
	double reprojection_error;
};

struct t_constellation_tracker_sample
{
	//! The time the original blobservation was made.
	int64_t timestamp_ns;
	//! The sequence ID of the mosaic
	uint64_t sequence_id;
	/*!
	 * Whether the `world_pose` field is valid or not.
	 *
	 * In certain cases, cameras are able to observe a device without themselves knowing where they are yet.
	 */
	bool has_world_pose;
	//! The pose of the device at the time of the blobservation.
	struct xrt_pose world_pose;
	//! The mosaic index of the camera that made the blobservation in question.
	size_t mosaic_index;
	//! The index of the camera in the mosaic that made the blobservation in question.
	size_t camera_index;
	//! Average brightness of the detected blobs, from 0 (black) to 1 (pure white).
	float average_brightness;
	//! Metrics about the sample, such as reprojection error and matched LED count.
	struct t_constellation_tracker_sample_metrics metrics;
};

/*!
 * @interface t_constellation_tracker_device
 *
 * A constellation tracker device is a device that the constellation tracker will attempt to track
 * in 6dof. The constellation tracker will provide the device with samples of it's current pose as
 * it tracks it.
 */
struct t_constellation_tracker_device
{
	/*!
	 * A function that the constellation tracker will call to push a new sample of the device's
	 * pose as it tracks it.
	 *
	 * @param connection The device to push the sample to.
	 * @param sample     The sample containing the current pose of the device and the timestamp of
	 *                   the original blobservation that led to this pose being computed.
	 */
	void (*push_constellation_tracker_sample)(struct t_constellation_tracker_device *connection,
	                                          struct t_constellation_tracker_sample *sample);
};

/*!
 * Helper function for @ref t_constellation_tracker_device::push_constellation_tracker_sample.
 *
 * @copydoc t_constellation_tracker_device::push_constellation_tracker_sample
 *
 * @public @memberof t_constellation_tracker_device
 */
XRT_NONNULL_ALL static inline void
t_constellation_tracker_device_push_sample(struct t_constellation_tracker_device *device,
                                           struct t_constellation_tracker_sample *sample)
{
	device->push_constellation_tracker_sample(device, sample);
}

#ifdef __cplusplus
}
#endif
