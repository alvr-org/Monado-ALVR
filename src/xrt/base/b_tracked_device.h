// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Per-device tracking state for the space overseer.
 *
 * @ingroup base
 */

#pragma once

#include "xrt/xrt_space.h"


#ifdef __cplusplus
extern "C" {
#endif


struct b_tracked_device;


typedef void (*b_tracked_device_pose_space_cb)(struct xrt_space *xs, void *priv);


/*!
 * Create a tracked device with a reference to @p space.
 *
 * @public @memberof b_tracked_device
 */
struct b_tracked_device *
b_tracked_device_create(struct xrt_space *space);

/*!
 * @public @memberof b_tracked_device
 */
void
b_tracked_device_reference(struct b_tracked_device **dst, struct b_tracked_device *src);

/*!
 * Returns the device space without taking a reference.
 *
 * @public @memberof b_tracked_device
 */
struct xrt_space *
b_tracked_device_get_space(struct b_tracked_device *btd);

/*!
 * Track a pose space owned by this device. The caller must hold the space
 * overseer graph lock.
 *
 * @public @memberof b_tracked_device
 */
void
b_tracked_device_track_pose_space(struct b_tracked_device *btd, struct xrt_space *xs);

/*!
 * Stop tracking a pose space. The caller must hold the space overseer graph
 * lock.
 *
 * @public @memberof b_tracked_device
 */
void
b_tracked_device_untrack_pose_space(struct b_tracked_device *btd, struct xrt_space *xs);

/*!
 * Iterate tracked pose spaces. The caller must hold the space overseer graph
 * lock.
 *
 * @public @memberof b_tracked_device
 */
void
b_tracked_device_for_each_pose_space(struct b_tracked_device *btd, b_tracked_device_pose_space_cb cb, void *priv);

/*!
 * Drop all tracked pose spaces without destroying them. The caller must hold the
 * space overseer graph lock.
 *
 * @public @memberof b_tracked_device
 */
void
b_tracked_device_clear_pose_spaces(struct b_tracked_device *btd);


#ifdef __cplusplus
}
#endif
