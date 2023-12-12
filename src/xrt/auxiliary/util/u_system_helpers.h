// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for system objects like @ref xrt_system_devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_results.h"
#include "xrt/xrt_frame.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_instance.h"
#include "xrt/xrt_tracking.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_session_event_sink;


/*!
 * Helper struct to manage devices by implementing the @ref xrt_system_devices.
 *
 * The default destroy function that is set by @ref u_system_devices_allocate
 * will first destroy all of the @ref xrt_device and then destroy all nodes
 * in the @ref xrt_frame_context.
 *
 * @ingroup aux_util
 */
struct u_system_devices
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
 * @ingroup aux_util
 */
static inline struct u_system_devices *
u_system_devices(struct xrt_system_devices *xsysd)
{
	return (struct u_system_devices *)xsysd;
}

/*!
 * Allocates a empty @ref u_system_devices to be filled in by the caller, only
 * the destroy function is filled in.
 *
 * @ingroup aux_util
 */
struct u_system_devices *
u_system_devices_allocate(void);

/*!
 * Destroys all devices and clears out the frame context, doesn't free the
 * struct itself, useful for code embedding the system devices struct into
 * other objects where it's not the first member or C++ classes.
 *
 * @ingroup aux_util
 */
void
u_system_devices_close(struct xrt_system_devices *xsysd);

/*!
 * Destroy an u_system_devices_allocate and owned devices - helper function.
 *
 * @param[in,out] usysd_ptr A pointer to the u_system_devices_allocate struct pointer.
 *
 * Will destroy the system devices if *usysd_ptr is not NULL. Will then set *usysd_ptr to NULL.
 *
 * @public @memberof u_system_devices_allocate
 */
static inline void
u_system_devices_destroy(struct u_system_devices **usysd_ptr)
{
	struct u_system_devices *usysd = *usysd_ptr;
	if (usysd == NULL) {
		return;
	}

	*usysd_ptr = NULL;
	usysd->base.destroy(&usysd->base);
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
 * @ingroup aux_util
 */
struct u_system_devices_static
{
	struct u_system_devices base;

	//! Is automatically returned.
	struct xrt_system_roles cached;
};

/*!
 * Small inline helper to cast from @ref xrt_system_devices.
 *
 * @ingroup aux_util
 */
static inline struct u_system_devices_static *
u_system_devices_static(struct xrt_system_devices *xsysd)
{
	return (struct u_system_devices_static *)xsysd;
}

/*!
 * Allocates a empty @ref u_system_devices to be filled in by the caller, only
 * the destroy function is filled in.
 *
 * @ingroup aux_util
 */
struct u_system_devices_static *
u_system_devices_static_allocate(void);

/*!
 * Finalizes the static struct with the given input devices, the system devices
 * will always return these devices for the left and right role. This function
 * must be called before @ref xrt_system_devices_get_roles is called.
 *
 * @ingroup aux_util
 */
void
u_system_devices_static_finalize(struct u_system_devices_static *usysds,
                                 struct xrt_device *left,
                                 struct xrt_device *right);


/*
 *
 * Generic system devices helper.
 *
 */

/*!
 * Takes a @ref xrt_instance, gets the prober from it and then uses the prober
 * to allocate a filled in @ref u_system_devices.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_system_devices_create_from_prober(struct xrt_instance *xinst,
                                    struct xrt_session_event_sink *broadcast,
                                    struct xrt_system_devices **out_xsysd,
                                    struct xrt_space_overseer **out_xso);

/*!
 * Helper function.
 *
 * Looks through @ref xrt_system_devices's devices and returns the first device
 * that supports hand tracking and the supplied input name.
 *
 * Used by target_builder_lighthouse to find Knuckles controllers in the list of
 * devices returned, the legacy builder to find hand tracking devices, etc.
 *
 * @ingroup aux_util
 */
struct xrt_device *
u_system_devices_get_ht_device(struct xrt_system_devices *xsysd, enum xrt_input_name name);

/*!
 * Helper to get the first left hand-tracking device,
 * uses @ref u_system_devices_get_ht_device.
 *
 * @ingroup aux_util
 */
static inline struct xrt_device *
u_system_devices_get_ht_device_left(struct xrt_system_devices *xsysd)
{
	return u_system_devices_get_ht_device(xsysd, XRT_INPUT_GENERIC_HAND_TRACKING_LEFT);
}

/*!
 * Helper to get the first right hand-tracking device,
 * uses @ref u_system_devices_get_ht_device.
 *
 * @ingroup aux_util
 */
static inline struct xrt_device *
u_system_devices_get_ht_device_right(struct xrt_system_devices *xsysd)
{
	return u_system_devices_get_ht_device(xsysd, XRT_INPUT_GENERIC_HAND_TRACKING_RIGHT);
}


#ifdef __cplusplus
}
#endif
