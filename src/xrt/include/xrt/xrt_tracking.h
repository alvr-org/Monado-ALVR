// Copyright 2019, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining the tracking system integration in Monado.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#define XRT_TRACKING_NAME_LEN 256

#define XRT_TRACKING_MAX_CAMS 8
#define XRT_TRACKING_FOR_EACH_CAM(_) _(0) _(1) _(2) _(3) _(4) _(5) _(6) _(7)

#define XRT_TRACKING_MAX_IMUS 3
#define XRT_TRACKING_FOR_EACH_IMU(_) _(0) _(1) _(2)

#define XRT_TRACKING_MAX_MASK_SINKS 2
#define XRT_TRACKING_FOR_EACH_MASK_SINK(_) _(0) _(1)

#include "xrt/xrt_defines.h"

#include "xrt/xrt_system.h"

#ifdef __cplusplus
extern "C" {
#endif


struct time_state;
struct xrt_device;
struct xrt_tracking;
struct xrt_tracking_factory;
struct xrt_tracked_psmv;
struct xrt_tracked_psvr;
struct xrt_tracked_slam;

//! @todo This is from u_time, duplicated to avoid layer violation.
typedef int64_t timepoint_ns;

/*!
 * @addtogroup xrt_iface
 * @{
 */

/*!
 * What kind of tracking system is this.
 *
 * @todo Is none, Colour, IR, Magnetic the kind of type we need to know about?
 */
enum xrt_tracking_type
{
	//! The device(s) are never tracked.
	XRT_TRACKING_TYPE_NONE,

	//! The device(s) are tracked by RGB camera(s).
	XRT_TRACKING_TYPE_RGB,

	//! The device(s) are tracked by Lighthouse.
	XRT_TRACKING_TYPE_LIGHTHOUSE,

	//! The device(s) are tracked by magnetic fields, eg. Razer Hydra.
	XRT_TRACKING_TYPE_MAGNETIC,

	//! The device(s) are tracked by external SLAM
	XRT_TRACKING_TYPE_EXTERNAL_SLAM,

	//! The device(s) are tracked through a constellation of lights as seen by cameras.
	XRT_TRACKING_TYPE_CONSTELLATION,

	//! The device(s) are tracked by other methods.
	XRT_TRACKING_TYPE_OTHER,

	//! The device(s) are (re)attachable.
	XRT_TRACKING_TYPE_ATTACHABLE,
};

/*!
 * A tracking system or device origin.
 *
 * Tracking systems will typically extend this structure.
 */
struct xrt_tracking_origin
{
	//! For debugging.
	char name[XRT_TRACKING_NAME_LEN];

	//! What can the state tracker expect from this tracking system.
	enum xrt_tracking_type type;

	/*!
	 * Read-only. Set once by the builder, or in some cases, the driver.
	 * With some systems, the offset may be changed at runtime using the space overseer.
	 */
	struct xrt_pose initial_offset;
};

/*!
 * @interface xrt_tracking_factory
 * Tracking factory.
 */
struct xrt_tracking_factory
{
	//! Internal frame context, exposed for debugging purposes.
	struct xrt_frame_context *xfctx;

	/*!
	 * Create a tracked PSMV ball.
	 */
	int (*create_tracked_psmv)(struct xrt_tracking_factory *, struct xrt_tracked_psmv **out_psmv);

	/*!
	 * Create a tracked PSVR HMD.
	 */
	int (*create_tracked_psvr)(struct xrt_tracking_factory *, struct xrt_tracked_psvr **out_psvr);



	/*!
	 * Create a SLAM tracker.
	 */
	int (*create_tracked_slam)(struct xrt_tracking_factory *, struct xrt_tracked_slam **out_slam);
};

/*!
 * IMU Sample.
 * @todo Replace with @ref xrt_imu_sample
 */
struct xrt_tracking_sample
{
	struct xrt_vec3 accel_m_s2;
	struct xrt_vec3 gyro_rad_secs;
};

/*!
 * IMU Sample.
 * @todo Make @ref xrt_tracked_psmv and @ref xrt_tracked_psvr use this
 */
struct xrt_imu_sample
{
	timepoint_ns timestamp_ns;
	struct xrt_vec3_f64 accel_m_s2;
	struct xrt_vec3_f64 gyro_rad_secs;
};

/*!
 * Pose sample.
 */
struct xrt_pose_sample
{
	timepoint_ns timestamp_ns;
	struct xrt_pose pose;
};

/*!
 * Masks (bounding boxes) of different hands from current views
 */
struct xrt_device_masks_sample
{
	struct xrt_device_masks_sample_camera
	{
		struct xrt_device_masks_sample_device
		{
			bool enabled;             //!< Whether a mask for this hand is being reported
			struct xrt_rect_f32 rect; //!< The mask itself in pixel coordinates
		} devices[XRT_SYSTEM_MAX_DEVICES];
	} views[XRT_TRACKING_MAX_CAMS];
};

/*!
 * @interface xrt_imu_sink
 *
 * An object to send IMU samples to.
 *
 * Similar to @ref xrt_frame_sink but the interface implementation must manage
 * its own resources, not through a context graph.
 *
 * @todo Make @ref xrt_tracked_psmv and @ref xrt_tracked_psvr implement this
 */
struct xrt_imu_sink
{
	/*!
	 * Push an IMU sample into the sink
	 */
	void (*push_imu)(struct xrt_imu_sink *, struct xrt_imu_sample *sample);
};

/*!
 * @interface xrt_pose_sink
 *
 * An object to send pairs of timestamps and poses to. @see xrt_imu_sink.
 */
struct xrt_pose_sink
{
	void (*push_pose)(struct xrt_pose_sink *, struct xrt_pose_sample *sample);
};

/*!
 * @interface xrt_device_masks_sink
 *
 * An object to push @ref xrt_device_masks_sample to.
 */
struct xrt_device_masks_sink
{
	void (*push_device_masks)(struct xrt_device_masks_sink *, struct xrt_device_masks_sample *device_masks);
};


/*!
 * Container of pointers to sinks that could be used for a SLAM system. Sinks
 * are considered disabled if they are null.
 */
struct xrt_slam_sinks
{
	int cam_count;
	int imu_count;
	struct xrt_frame_sink *cams[XRT_TRACKING_MAX_CAMS];
	struct xrt_imu_sink *imus[XRT_TRACKING_MAX_IMUS];
	//! Can receive ground truth poses if available
	struct xrt_pose_sink *gt;
	//! SLAM masks, one per optical tracker (so they can push masks asynchronously).
	struct xrt_device_masks_sink *masks[XRT_TRACKING_MAX_MASK_SINKS];
};

/*!
 * @interface xrt_tracked_psmv
 *
 * A single tracked PS Move controller, camera and ball are not synced.
 *
 * @todo How do we communicate ball colour change?
 */
struct xrt_tracked_psmv
{
	//! The tracking system origin for this ball.
	struct xrt_tracking_origin *origin;

	//! Device owning this ball.
	struct xrt_device *xdev;

	//! Colour of the ball.
	struct xrt_colour_rgb_f32 colour;

	/*!
	 * Push a IMU sample into the tracking system.
	 */
	void (*push_imu)(struct xrt_tracked_psmv *, timepoint_ns timestamp_ns, struct xrt_tracking_sample *sample);

	/*!
	 * Called by the owning @ref xrt_device @ref xdev to get the pose of
	 * the ball in the tracking space at the given time.
	 *
	 * @todo Should we add a out_time argument as a way to signal min and
	 * maximum, and as such only do interpelation between different captured
	 * frames.
	 */
	void (*get_tracked_pose)(struct xrt_tracked_psmv *,
	                         enum xrt_input_name name,
	                         timepoint_ns when_ns,
	                         struct xrt_space_relation *out_relation);

	/*!
	 * Destroy this tracked ball.
	 */
	void (*destroy)(struct xrt_tracked_psmv *);
};

/*!
 * @interface xrt_tracked_psvr
 *
 * A tracked PSVR headset.
 *
 * @todo How do we communicate led lighting status?
 */
struct xrt_tracked_psvr
{
	//! The tracking system origin for this ball.
	struct xrt_tracking_origin *origin;

	//! Device owning this ball.
	struct xrt_device *xdev;

	/*!
	 * Push a IMU sample into the tracking system.
	 */
	void (*push_imu)(struct xrt_tracked_psvr *, timepoint_ns timestamp_ns, struct xrt_tracking_sample *sample);

	/*!
	 * Called by the owning @ref xrt_device @ref xdev to get the pose of
	 * the psvr in the tracking space at the given time.
	 */
	void (*get_tracked_pose)(struct xrt_tracked_psvr *,
	                         timepoint_ns when_ns,
	                         struct xrt_space_relation *out_relation);

	/*!
	 * Destroy this tracked psvr.
	 */
	void (*destroy)(struct xrt_tracked_psvr *);
};

/*!
 * @interface xrt_tracked_slam
 *
 * An adapter that wraps an external SLAM tracker to provide SLAM tracking.
 * Devices that want to be tracked through SLAM should create and manage an
 * instance of this type.
 */
struct xrt_tracked_slam
{
	/*!
	 * Called by the owning @ref xrt_device to get the last estimated pose
	 * of the SLAM tracker.
	 */
	void (*get_tracked_pose)(struct xrt_tracked_slam *,
	                         timepoint_ns when_ns,
	                         struct xrt_space_relation *out_relation);
};

/*
 *
 * Helper functions.
 *
 */

//! @public @memberof xrt_imu_sink
XRT_NONNULL_ALL static inline void
xrt_sink_push_imu(struct xrt_imu_sink *sink, struct xrt_imu_sample *sample)
{
	sink->push_imu(sink, sample);
}

//! @public @memberof xrt_pose_sink
XRT_NONNULL_ALL static inline void
xrt_sink_push_pose(struct xrt_pose_sink *sink, struct xrt_pose_sample *sample)
{
	sink->push_pose(sink, sample);
}

//! @public @memberof xrt_device_masks_sink
XRT_NONNULL_ALL static inline void
xrt_sink_push_device_masks(struct xrt_device_masks_sink *sink, struct xrt_device_masks_sample *device_masks)
{
	sink->push_device_masks(sink, device_masks);
}

//! @public @memberof xrt_tracked_psmv
XRT_NONNULL_ALL static inline void
xrt_tracked_psmv_get_tracked_pose(struct xrt_tracked_psmv *psmv,
                                  enum xrt_input_name name,
                                  timepoint_ns when_ns,
                                  struct xrt_space_relation *out_relation)
{
	psmv->get_tracked_pose(psmv, name, when_ns, out_relation);
}

//! @public @memberof xrt_tracked_psmv
XRT_NONNULL_ALL static inline void
xrt_tracked_psmv_push_imu(struct xrt_tracked_psmv *psmv, timepoint_ns timestamp_ns, struct xrt_tracking_sample *sample)
{
	psmv->push_imu(psmv, timestamp_ns, sample);
}

//! @public @memberof xrt_tracked_psmv
XRT_NONNULL_ALL static inline void
xrt_tracked_psmv_destroy(struct xrt_tracked_psmv **xtmv_ptr)
{
	struct xrt_tracked_psmv *xtmv = *xtmv_ptr;
	if (xtmv == NULL) {
		return;
	}

	xtmv->destroy(xtmv);
	*xtmv_ptr = NULL;
}

//! @public @memberof xrt_tracked_psmv
XRT_NONNULL_ALL static inline void
xrt_tracked_psvr_get_tracked_pose(struct xrt_tracked_psvr *psvr,
                                  timepoint_ns when_ns,
                                  struct xrt_space_relation *out_relation)
{
	psvr->get_tracked_pose(psvr, when_ns, out_relation);
}

//! @public @memberof xrt_tracked_psmv
XRT_NONNULL_ALL static inline void
xrt_tracked_psvr_push_imu(struct xrt_tracked_psvr *psvr, timepoint_ns timestamp_ns, struct xrt_tracking_sample *sample)
{
	psvr->push_imu(psvr, timestamp_ns, sample);
}

//! @public @memberof xrt_tracked_psmv
XRT_NONNULL_ALL static inline void
xrt_tracked_psvr_destroy(struct xrt_tracked_psvr **xtvr_ptr)
{
	struct xrt_tracked_psvr *xtvr = *xtvr_ptr;
	if (xtvr == NULL) {
		return;
	}

	xtvr->destroy(xtvr);
	*xtvr_ptr = NULL;
}


//! @public @memberof xrt_tracked_slam
XRT_NONNULL_ALL static inline void
xrt_tracked_slam_get_tracked_pose(struct xrt_tracked_slam *slam,
                                  timepoint_ns when_ns,
                                  struct xrt_space_relation *out_relation)
{
	slam->get_tracked_pose(slam, when_ns, out_relation);
}

/*!
 * @}
 */


#ifdef __cplusplus
}
#endif
