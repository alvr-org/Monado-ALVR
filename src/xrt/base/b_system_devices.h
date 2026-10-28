// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Default implementation helpers for @ref xrt_system_devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup base
 */

#pragma once

#include "xrt/xrt_frame.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_tracking.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_session_event_sink;

/*!
 * Helper struct to manage devices by implementing the @ref xrt_system_devices.
 *
 * The default destroy function that is set by @ref b_system_devices_allocate
 * will first destroy all of the @ref xrt_device and then destroy all nodes
 * in the @ref xrt_frame_context.
 *
 * @ingroup base
 */
struct b_system_devices
{
	struct xrt_system_devices base;

	//! Optional frame context for visual tracking.
	struct xrt_frame_context xfctx;

	//! Optional shared tracking origin.
	struct xrt_tracking_origin origin;
};

/*!
 * Small inline helper to cast from @ref xrt_system_devices.
 *
 * @ingroup base
 */
static inline struct b_system_devices *
b_system_devices(struct xrt_system_devices *xsysd)
{
	return (struct b_system_devices *)xsysd;
}

/*!
 * Allocates a empty @ref b_system_devices to be filled in by the caller, only
 * the destroy function is filled in.
 *
 * @ingroup base
 */
struct b_system_devices *
b_system_devices_allocate(void);

/*!
 * Destroys all devices and clears out the frame context, doesn't free the
 * struct itself, useful for code embedding the system devices struct into
 * other objects where it's not the first member or C++ classes.
 *
 * @ingroup base
 */
void
b_system_devices_close(struct xrt_system_devices *xsysd);

/*!
 * Destroy an b_system_devices_allocate and owned devices - helper function.
 *
 * @param[in,out] bsysd_ptr A pointer to the b_system_devices_allocate struct pointer.
 *
 * Will destroy the system devices if *bsysd_ptr is not NULL. Will then set *bsysd_ptr to NULL.
 *
 * @public @memberof b_system_devices_allocate
 */
static inline void
b_system_devices_destroy(struct b_system_devices **bsysd_ptr)
{
	struct b_system_devices *bsysd = *bsysd_ptr;
	if (bsysd == NULL) {
		return;
	}

	*bsysd_ptr = NULL;
	bsysd->base.destroy(&bsysd->base);
}


/*
 *
 * Static helper.
 *
 */

/*!
 * Helper struct to manage devices by implementing the @ref xrt_system_devices,
 * this has only static device roles.
 *
 * @ingroup base
 */
struct b_system_devices_static
{
	struct b_system_devices base;

	//! Is automatically returned.
	struct xrt_system_roles cached;

	//! Tracks usage of device features.
	struct xrt_reference feature_use[XRT_DEVICE_FEATURE_MAX_ENUM];
};

/*!
 * Small inline helper to cast from @ref xrt_system_devices.
 *
 * @ingroup base
 */
static inline struct b_system_devices_static *
b_system_devices_static(struct xrt_system_devices *xsysd)
{
	return (struct b_system_devices_static *)xsysd;
}

/*!
 * Allocates a empty @ref b_system_devices to be filled in by the caller, only
 * the destroy function is filled in.
 *
 * @ingroup base
 */
struct b_system_devices_static *
b_system_devices_static_allocate(void);

/*!
 * Finalizes the static struct with the given input devices, the system devices
 * will always return these devices for the left and right role. This function
 * must be called before @ref xrt_system_devices_get_roles is called.
 *
 * @ingroup base
 */
xrt_result_t
b_system_devices_static_finalize(struct b_system_devices_static *bsysds,
                                 struct xrt_device *left,
                                 struct xrt_device *right,
                                 struct xrt_device *gamepad);

#ifdef __cplusplus
}
#endif
