// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Shared functions for IPC client @ref xrt_device.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup ipc_client
 */

#pragma once

#include "util/u_device.h"


#ifdef __cplusplus
extern "C" {
#endif


struct ipc_connection;
struct ipc_client_tracking_origin_manager;

/*!
 * An IPC client proxy for an @ref xrt_device.
 *
 * @implements xrt_device
 * @ingroup ipc_client
 */
struct ipc_client_xdev
{
	struct xrt_device base;

	struct ipc_connection *ipc_c;

	/*!
	 * Per-client-connection slot index for looking up this device on the IPC
	 * server. Passed in IPC message calls such as @c device_get_tracked_pose.
	 * This is not the same as @ref xrt_device::id, which is synchronized from
	 * the service via @ref ipc_device_info.
	 */
	uint32_t device_id;
};

/*!
 * Convenience helper to go from a xdev to @ref ipc_client_xdev.
 *
 * @ingroup ipc_client
 */
static inline struct ipc_client_xdev *
ipc_client_xdev(struct xrt_device *xdev)
{
	return (struct ipc_client_xdev *)xdev;
}

/*!
 * Initializes a ipc_client_xdev so that it's basically fully usable as a
 * @ref xrt_device object. Does not fill in the destroy function or the any
 * if the HMD components / functions.
 *
 * @ingroup ipc_client
 * @public @memberof ipc_client_xdev
 */
xrt_result_t
ipc_client_xdev_init(struct ipc_client_xdev *icx,
                     struct ipc_connection *ipc_c,
                     struct ipc_client_tracking_origin_manager *itom,
                     uint32_t device_id,
                     u_device_destroy_function_t destroy_fn);

/*!
 * Frees any memory that was allocated as part of init and resets some pointers.
 *
 * @ingroup ipc_client
 * @public @memberof ipc_client_xdev
 */
void
ipc_client_xdev_fini(struct ipc_client_xdev *icx);


#ifdef __cplusplus
}
#endif
