// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining xrt body tracker.
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_results.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_device;
struct xrt_body_tracker;
struct xrt_space;
struct xrt_space_overseer;

/*!
 * Creation parameters for @ref xrt_body_tracker.
 *
 * @ingroup xrt_iface
 */
struct xrt_body_tracker_create_info
{
	/*!
	 * Which body-tracking data to produce. One of the body-tracking input
	 * names, for example @ref XRT_INPUT_FB_BODY_TRACKING,
	 * @ref XRT_INPUT_META_FULL_BODY_TRACKING, @ref XRT_INPUT_BD_BODY_TRACKING,
	 * or @ref XRT_INPUT_BD_BODY_TRACKING_WITHOUT_ARM.
	 */
	enum xrt_input_name body_tracking_type;

	/*!
	 * Device to pull body-tracking data from, or NULL to let the system
	 * choose the backing device (typically the body-tracking role).
	 */
	struct xrt_device *locked_xdev;
};

/*!
 * Capability flags for a body tracker, fixed when it is created.
 *
 * Determined by @ref xrt_body_tracker_create_info::body_tracking_type: each
 * OpenXR body tracker variant implies which of these capabilities the tracker
 * must provide. The flags are set once at creation and do not change. The
 * tracker is responsible for meeting them for every successful data path; if
 * it cannot, it must not return body-tracking data (for example @ref locate
 * reports inactive / empty results rather than partial or inconsistent
 * capability).
 *
 * @ingroup xrt_iface
 */
struct xrt_body_tracker_supported
{
	bool body_tracking;
	bool body_tracking_calibration;
	bool body_tracking_fidelity;
};

/*!
 * Result of a locate call. joint_set is the RAW device joint set (joint
 * relations still expressed relative to the body pose). base_body is the body
 * pose resolved into the requested base space (T_base_body). The caller chains
 * each joint relation with base_body, exactly as oxr does today.
 *
 * @ingroup xrt_iface
 */
struct xrt_body_tracker_location
{
	struct xrt_body_joint_set joint_set;
	struct xrt_space_relation base_body;
	bool is_active;
};

/*!
 * A body tracker, owns the policy for selecting which device and body-tracking
 * source back a single OpenXR body-tracker handle (XrBodyTrackerFB /
 * XrBodyTrackerBD).
 *
 * Created from an @ref xrt_system_devices (see
 * @ref xrt_system_devices::create_body_tracker). The OpenXR state tracker holds
 * one of these per body-tracker handle instead of reaching into an
 * @ref xrt_device directly, mirroring @ref xrt_hand_tracker. This moves
 * device/source selection and the base-space transform (in @ref locate) to the
 * service side of the IPC layer, and lets external runtimes provide different
 * body-tracking behavior without changing the OpenXR code. Implementations may
 * change which @ref xrt_device backs a tracker over its lifetime (for example
 * when the body-tracking role changes); the interface allows that, though not
 * every implementation must support runtime switching. The FB, META full-body
 * and BD (PICO) variants are selected via
 * @ref xrt_body_tracker_create_info::body_tracking_type.
 *
 * @ingroup xrt_iface
 */
struct xrt_body_tracker
{
	//! Capability flags for this tracker; see @ref xrt_body_tracker_supported.
	struct xrt_body_tracker_supported supported;

	/*!
	 * Locate the tracked body joints.
	 *
	 * @param xbt             Pointer to self.
	 * @param xso             Space overseer used to resolve the base space.
	 * @param base_space      Space to locate the body in.
	 * @param base_offset     Offset applied on top of @p base_space.
	 * @param at_timestamp_ns Time to locate at.
	 * @param out_location    Resulting raw joint set plus the body pose
	 *                        resolved into the base space, see
	 *                        @ref xrt_body_tracker_location.
	 */
	xrt_result_t (*locate)(struct xrt_body_tracker *xbt,
	                       struct xrt_space_overseer *xso,
	                       struct xrt_space *base_space,
	                       const struct xrt_pose *base_offset,
	                       int64_t at_timestamp_ns,
	                       struct xrt_body_tracker_location *out_location);

	/*!
	 * Get the body skeleton (the joint hierarchy and rest pose) for this
	 * tracker's body-tracking source.
	 *
	 * @param xbt          Pointer to self.
	 * @param out_skeleton Resulting skeleton.
	 */
	xrt_result_t (*get_skeleton)(struct xrt_body_tracker *xbt, struct xrt_body_skeleton *out_skeleton);

	/*!
	 * Reset the body-tracking calibration, for
	 * XR_META_body_tracking_calibration.
	 *
	 * @param xbt Pointer to self.
	 */
	xrt_result_t (*reset_calibration_meta)(struct xrt_body_tracker *xbt);

	/*!
	 * Suggest a body-tracking calibration override, for
	 * XR_META_body_tracking_calibration.
	 *
	 * @param xbt         Pointer to self.
	 * @param body_height Suggested body height, in meters.
	 */
	xrt_result_t (*set_calibration_override_meta)(struct xrt_body_tracker *xbt, float body_height);

	/*!
	 * Request a body-tracking fidelity level, for
	 * XR_META_body_tracking_fidelity.
	 *
	 * @param xbt      Pointer to self.
	 * @param fidelity Requested fidelity level.
	 */
	xrt_result_t (*set_fidelity_meta)(struct xrt_body_tracker *xbt, enum xrt_body_tracking_fidelity_meta fidelity);

	/*!
	 * Destroy this body tracker.
	 *
	 * Code consuming this interface should use @ref xrt_body_tracker_destroy.
	 *
	 * @param xbt Pointer to self.
	 */
	void (*destroy)(struct xrt_body_tracker *xbt);
};

/*!
 * @copydoc xrt_body_tracker::locate
 *
 * @public @memberof xrt_body_tracker
 */
static inline xrt_result_t
xrt_body_tracker_locate(struct xrt_body_tracker *xbt,
                        struct xrt_space_overseer *xso,
                        struct xrt_space *base_space,
                        const struct xrt_pose *base_offset,
                        int64_t at_timestamp_ns,
                        struct xrt_body_tracker_location *out_location)
{
	return xbt->locate(xbt, xso, base_space, base_offset, at_timestamp_ns, out_location);
}

/*!
 * @copydoc xrt_body_tracker::get_skeleton
 *
 * @public @memberof xrt_body_tracker
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_body_tracker_get_skeleton(struct xrt_body_tracker *xbt, struct xrt_body_skeleton *out_skeleton)
{
	return xbt->get_skeleton(xbt, out_skeleton);
}

/*!
 * @copydoc xrt_body_tracker::reset_calibration_meta
 *
 * @public @memberof xrt_body_tracker
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_body_tracker_reset_calibration_meta(struct xrt_body_tracker *xbt)
{
	return xbt->reset_calibration_meta(xbt);
}

/*!
 * @copydoc xrt_body_tracker::set_calibration_override_meta
 *
 * @public @memberof xrt_body_tracker
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_body_tracker_set_calibration_override_meta(struct xrt_body_tracker *xbt, float body_height)
{
	return xbt->set_calibration_override_meta(xbt, body_height);
}

/*!
 * @copydoc xrt_body_tracker::set_fidelity_meta
 *
 * @public @memberof xrt_body_tracker
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_body_tracker_set_fidelity_meta(struct xrt_body_tracker *xbt, enum xrt_body_tracking_fidelity_meta fidelity)
{
	return xbt->set_fidelity_meta(xbt, fidelity);
}

/*!
 * Destroy an xrt_body_tracker - helper function.
 *
 * @param[in,out] xbt_ptr A pointer to the xrt_body_tracker struct pointer.
 *
 * Will destroy the tracker if `*xbt_ptr` is not NULL. Will then set
 * `*xbt_ptr` to NULL.
 *
 * @public @memberof xrt_body_tracker
 */
XRT_NONNULL_ALL static inline void
xrt_body_tracker_destroy(struct xrt_body_tracker **xbt_ptr)
{
	struct xrt_body_tracker *xbt = *xbt_ptr;
	if (xbt == NULL) {
		return;
	}

	*xbt_ptr = NULL;
	xbt->destroy(xbt);
}

#ifdef __cplusplus
}
#endif
