// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining xrt hand tracker.
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_results.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_device;
struct xrt_hand_tracker;
struct xrt_space;
struct xrt_space_overseer;

/*!
 * Creation parameters for @ref xrt_hand_tracker.
 *
 * If @ref locked_xdev is non-NULL the tracker is created in device-locked
 * mode, in which the hand-tracker will only get the hand-tracking data from the
 * provided xrt_device. Otherwise it follows the system hand-tracking role
 * selection policy.
 *
 * @ingroup xrt_iface
 */
struct xrt_hand_tracker_create_info
{
	//! Which hand this tracker represents.
	enum xrt_hand hand;

	//! Optional ordered list of requested hand-tracking input names.
	enum xrt_input_name requested_sources[2];

	//! Number of entries in @ref requested_sources, zero means all sources.
	uint32_t requested_source_count;

	//! Optional device to lock the tracker to.
	struct xrt_device *locked_xdev;
};

/*!
 * Result of locating a hand tracker.
 *
 * Joint relations are expressed in the base space supplied to
 * @ref xrt_hand_tracker::locate. @ref source is the backing input that produced
 * active data.
 *
 * @ingroup xrt_iface
 */
struct xrt_hand_tracker_location
{
	//! Located joints in the requested base space.
	struct xrt_hand_joint_set hand_joint_set;

	//! Input source that produced active data.
	enum xrt_input_name source;

	//! Set if the hand tracker found active hand data.
	bool is_active;
};

/*!
 * A hand tracker that owns device/source selection policy.
 *
 * @ingroup xrt_iface
 */
struct xrt_hand_tracker
{
	/*!
	 * Locate hand joints in the supplied base space.
	 *
	 * @param xht             Pointer to self.
	 * @param xso             Space overseer used to resolve device spaces.
	 * @param base_space      Space to locate joints in.
	 * @param base_offset     Offset from @p base_space.
	 *
	 * @note The @p base_offset argument can be removed if this offset is
	 * folded into the OpenXR XrSpace, but we do not want to change that
	 * behavior just yet.
	 *
	 * @param at_timestamp_ns Time to locate at.
	 * @param out_location    Resulting hand-tracking location.
	 */
	xrt_result_t (*locate)(struct xrt_hand_tracker *xht,
	                       struct xrt_space_overseer *xso,
	                       struct xrt_space *base_space,
	                       const struct xrt_pose *base_offset,
	                       int64_t at_timestamp_ns,
	                       struct xrt_hand_tracker_location *out_location);

	/*!
	 * Apply output to the selected backing source device(s).
	 */
	xrt_result_t (*set_output)(struct xrt_hand_tracker *xht,
	                           enum xrt_output_name name,
	                           const struct xrt_output_value *value);

	/*!
	 * Destroy this hand tracker.
	 *
	 * Code consuming this interface should use @ref xrt_hand_tracker_destroy.
	 */
	void (*destroy)(struct xrt_hand_tracker *xht);
};

/*!
 * @copydoc xrt_hand_tracker::locate
 *
 * @public @memberof xrt_hand_tracker
 */
static inline xrt_result_t
xrt_hand_tracker_locate(struct xrt_hand_tracker *xht,
                        struct xrt_space_overseer *xso,
                        struct xrt_space *base_space,
                        const struct xrt_pose *base_offset,
                        int64_t at_timestamp_ns,
                        struct xrt_hand_tracker_location *out_location)
{
	return xht->locate(xht, xso, base_space, base_offset, at_timestamp_ns, out_location);
}

/*!
 * @copydoc xrt_hand_tracker::set_output
 *
 * @public @memberof xrt_hand_tracker
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_hand_tracker_set_output(struct xrt_hand_tracker *xht,
                            enum xrt_output_name name,
                            const struct xrt_output_value *value)
{
	return xht->set_output(xht, name, value);
}

/*!
 * Destroy an xrt_hand_tracker - helper function.
 *
 * @param[in,out] xht_ptr A pointer to the xrt_hand_tracker struct pointer.
 *
 * Will destroy the tracker if `*xht_ptr` is not NULL. Will then set
 * `*xht_ptr` to NULL.
 *
 * @public @memberof xrt_hand_tracker
 */
XRT_NONNULL_ALL static inline void
xrt_hand_tracker_destroy(struct xrt_hand_tracker **xht_ptr)
{
	struct xrt_hand_tracker *xht = *xht_ptr;
	if (xht == NULL) {
		return;
	}

	*xht_ptr = NULL;
	xht->destroy(xht);
}

#ifdef __cplusplus
}
#endif
