// Copyright 2019-2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  @ref xrt_frame_sink converters and other helpers.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_frame.h"
#include "xrt/xrt_tracking.h"

#include "os/os_threading.h"

#include "tracking/t_constellation.h"


#ifdef __cplusplus
extern "C" {
#endif

#define U_SINK_MAX_SPLIT_DOWNSTREAMS 5

/*!
 * @see u_sink_quirk_create
 */
struct u_sink_quirk_params
{
	//! Marks frames passing through as side-by-side stereo.
	bool stereo_sbs;
	/*!
	 * Marks the frames passing through as side-by-side stereo, and fixes up the camera's data offset to be readable
	 * as a standard side-by-side frame.
	 */
	bool ps4_cam;
	//! Sets the stereo format to the correct one for the leap motion and fixes image width.
	bool leap_motion;
	//! Reinterprets a raw bayer image as a monochrome L8 image.
	bool bayer_as_l8;
};

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_create_format_converter(struct xrt_frame_context *xfctx,
                               enum xrt_format f,
                               struct xrt_frame_sink *downstream,
                               struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_create_to_r8g8b8_or_l8(struct xrt_frame_context *xfctx,
                              struct xrt_frame_sink *downstream,
                              struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_create_to_r8g8b8_bayer_or_l8(struct xrt_frame_context *xfctx,
                                    struct xrt_frame_sink *downstream,
                                    struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_create_to_rgb_yuv_yuyv_uyvy_or_l8(struct xrt_frame_context *xfctx,
                                         struct xrt_frame_sink *downstream,
                                         struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_create_to_yuv_yuyv_uyvy_or_l8(struct xrt_frame_context *xfctx,
                                     struct xrt_frame_sink *downstream,
                                     struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_create_to_yuv_or_yuyv(struct xrt_frame_context *xfctx,
                             struct xrt_frame_sink *downstream,
                             struct xrt_frame_sink **out_xfs);

void
u_sink_create_half_scale(struct xrt_frame_context *xfctx,
                         struct xrt_frame_sink *downstream,
                         struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_create_to_r8g8b8_r8g8b8a8_r8g8b8x8_or_l8(struct xrt_frame_context *xfctx,
                                                struct xrt_frame_sink *downstream,
                                                struct xrt_frame_sink **out_xfs);
/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_deinterleaver_create(struct xrt_frame_context *xfctx,
                            struct xrt_frame_sink *downstream,
                            struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
bool
u_sink_queue_create(struct xrt_frame_context *xfctx,
                    uint64_t max_size,
                    struct xrt_frame_sink *downstream,
                    struct xrt_frame_sink **out_xfs);


/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
bool
u_sink_simple_queue_create(struct xrt_frame_context *xfctx,
                           struct xrt_frame_sink *downstream,
                           struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 */
void
u_sink_quirk_create(struct xrt_frame_context *xfctx,
                    struct xrt_frame_sink *downstream,
                    struct u_sink_quirk_params *params,
                    struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 * Takes a frame and pushes it to a maximum of U_SINK_MAX_SPLIT_DOWNSTREAMS sinks
 */
void
u_sink_split_multi_create(struct xrt_frame_context *xfctx,
                          struct xrt_frame_sink **downstreams,
                          size_t downstream_count,
                          struct xrt_frame_sink **out_xfs);

/*!
 * @public @memberof xrt_frame_sink
 * @see xrt_frame_context
 * Takes a frame and pushes it to two sinks
 */
void
u_sink_split_create(struct xrt_frame_context *xfctx,
                    struct xrt_frame_sink *left,
                    struct xrt_frame_sink *right,
                    struct xrt_frame_sink **out_xfs);

/*!
 * Splits Stereo SBS frames into two independent frames
 */
void
u_sink_stereo_sbs_split_create(struct xrt_frame_context *xfctx,
                               struct xrt_frame_sink *downstream_left,
                               struct xrt_frame_sink *downstream_right,
                               struct xrt_frame_sink **out_xfs);

/*!
 * Combines stereo frames.
 * Opposite of u_sink_stereo_sbs_split_create
 */
bool
u_sink_combiner_create(struct xrt_frame_context *xfctx,
                       struct xrt_frame_sink *downstream,
                       struct xrt_frame_sink **out_left_xfs,
                       struct xrt_frame_sink **out_right_xfs);

/*!
 * Enforces left-right push order on frames and forces them to be within a reasonable amount of time from each other
 */
bool
u_sink_force_genlock_create(struct xrt_frame_context *xfctx,
                            struct xrt_frame_sink *downstream_left,
                            struct xrt_frame_sink *downstream_right,
                            struct xrt_frame_sink **out_left_xfs,
                            struct xrt_frame_sink **out_right_xfs);


/*
 *
 * Debugging sink,
 *
 */

/*!
 * Allows more safely to debug sink inputs and outputs.
 */
struct u_sink_debug
{
	//! Is initialised/destroyed when added or root is removed.
	struct os_mutex mutex;

	// Protected by mutex, mutex must be held when frame is being pushed.
	struct xrt_frame_sink *sink;
};

static inline void
u_sink_debug_init(struct u_sink_debug *usd)
{
	os_mutex_init(&usd->mutex);
}

static inline bool
u_sink_debug_is_active(struct u_sink_debug *usd)
{
	os_mutex_lock(&usd->mutex);
	bool active = usd->sink != NULL;
	os_mutex_unlock(&usd->mutex);

	return active;
}

static inline void
u_sink_debug_push_frame(struct u_sink_debug *usd, struct xrt_frame *xf)
{
	os_mutex_lock(&usd->mutex);
	if (usd->sink != NULL) {
		xrt_sink_push_frame(usd->sink, xf);
	}
	os_mutex_unlock(&usd->mutex);
}

static inline void
u_sink_debug_set_sink(struct u_sink_debug *usd, struct xrt_frame_sink *xfs)
{
	os_mutex_lock(&usd->mutex);
	usd->sink = xfs;
	os_mutex_unlock(&usd->mutex);
}

static inline void
u_sink_debug_destroy(struct u_sink_debug *usd)
{
	os_mutex_destroy(&usd->mutex);
}


/*!
 * @public @memberof xrt_imu_sink
 * @see xrt_frame_context
 * Takes an IMU sample and pushes it to two sinks
 */
void
u_imu_sink_split_create(struct xrt_frame_context *xfctx,
                        struct xrt_imu_sink *downstream_one,
                        struct xrt_imu_sink *downstream_two,
                        struct xrt_imu_sink **out_imu_sink);


/*!
 * @public @memberof xrt_imu_sink
 * @see xrt_frame_context
 * Takes an IMU sample and only pushes it if its timestamp has monotonically increased.
 * Useful for handling hardware inconsistencies.
 */
void
u_imu_sink_force_monotonic_create(struct xrt_frame_context *xfctx,
                                  struct xrt_imu_sink *downstream,
                                  struct xrt_imu_sink **out_imu_sink);


/*!
 * @public @memberof xrt_blob_sink
 * @see xrt_frame_context
 * Visualizes blobs by rendering them to a frame and pushing that frame to a downstream sink.
 * Useful for debugging blob detection.
 *
 * @param[in] xfctx The frame context to add this sink to.
 * @param[in] downstream_blob_sink The blob sink to forward the blobs to.
 * @param[in] downstream_frame_sink The frame sink to push the visualized frames to.
 * @param[in] frame_width The width of the frames to render the blobs to.
 * @param[in] frame_height The height of the frames to render the blobs to.
 * @param[out] out_xbs The created blob sink.
 */
void
u_sink_blob_visualizer_create(struct xrt_frame_context *xfctx,
                              struct t_blob_sink *downstream_blob_sink,
                              struct u_sink_debug *downstream_frame_sink,
                              uint32_t frame_width,
                              uint32_t frame_height,
                              struct t_blob_sink **out_xbs);


#ifdef __cplusplus
}
#endif
