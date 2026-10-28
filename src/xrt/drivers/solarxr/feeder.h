// Copyright 2025, rcelyte
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  SolarXR pose feeder
 * @ingroup drv_solarxr
 */

#pragma once
#include "solarxr_ipc_socket.h"

#include "os/os_threading.h"

/*!
 * Object that observes a collection of @ref xrt_device objects,
 * reporting their poses and status to a SolarXR server over IPC
 * @ingroup drv_solarxr
 */
struct u_hashmap_int;
struct feeder
{
	struct os_mutex mutex;
	struct solarxr_ipc_socket socket;
	uint32_t next_id;
	struct u_hashmap_int *devices;
};

bool
feeder_init(struct feeder *feeder, enum u_logging_level log_level);

void
feeder_fini(struct feeder *feeder);

/*!
 * Register a device to observe and announce it to the server
 *
 * Holds the mutex during most of the operation.
 *
 * @param feeder self
 * @param xdev device to register
 *
 * @return true on success.
 *
 * @public @memberof feeder
 */
bool
feeder_add_device(struct feeder *feeder, struct xrt_device *xdev); // thread safe

/*!
 * Unregister a previously added device
 *
 * Holds the mutex during most of the operation.
 *
 * @param feeder self
 * @param xdev device to unregister
 *
 * @public @memberof feeder
 */
void
feeder_remove_device(struct feeder *feeder, struct xrt_device *xdev); // thread safe

/*!
 * Unregister all observed devices
 *
 * Holds the mutex during most of the operation.
 *
 * @param feeder self
 *
 * @public @memberof feeder
 */
void
feeder_clear_devices(struct feeder *feeder); // thread safe

/*!
 * Poll all observed xdevs and
 *
 * Holds the mutex during most of the operation.
 *
 * @param feeder self
 *
 * @public @memberof feeder
 */
void
feeder_send_feedback(struct feeder *feeder); // thread safe
