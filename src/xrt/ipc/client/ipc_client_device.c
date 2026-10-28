// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client device.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_device.h"

#include "os/os_time.h"

#include "math/m_api.h"

#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_device.h"

#include "client/ipc_client.h"
#include "client/ipc_client_connection.h"
#include "client/ipc_client_xdev.h"
#include "ipc_client_generated.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*
 *
 * Structs and defines.
 *
 */

/*!
 * An IPC client proxy for an controller or other non-MHD @ref xrt_device and
 * @ref ipc_client_xdev. Using a typedef reduce impact of refactor change.
 *
 * @implements ipc_client_xdev
 * @ingroup ipc_client
 */
typedef struct ipc_client_xdev ipc_client_device_t;


/*
 *
 * Functions
 *
 */

static inline ipc_client_device_t *
ipc_client_device(struct xrt_device *xdev)
{
	return (ipc_client_device_t *)xdev;
}

static void
ipc_client_device_destroy(struct xrt_device *xdev)
{
	ipc_client_device_t *icd = ipc_client_device(xdev);

	// Remove the variable tracking.
	u_var_remove_root(icd);

	// Free and de-init the shared things.
	ipc_client_xdev_fini(icd);

	// Free this device with the helper.
	u_device_free(&icd->base);
}

/*!
 * @public @memberof ipc_client_device
 */
struct xrt_device *
ipc_client_device_create(struct ipc_connection *ipc_c,
                         struct ipc_client_tracking_origin_manager *ictom,
                         uint32_t device_id)
{
	// Allocate and setup the basics.
	enum u_device_alloc_flags flags = (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD);
	ipc_client_device_t *icd = U_DEVICE_ALLOCATE(ipc_client_device_t, flags, 0, 0);

	// Fills in almost everything a regular device needs.
	xrt_result_t xret = ipc_client_xdev_init(icd, ipc_c, ictom, device_id, ipc_client_device_destroy);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ipc_c, "Failed to initialize IPC client device: %d", xret);
		u_device_free(&icd->base);
		return NULL;
	}

	// Setup variable tracker.
	u_var_add_root(icd, icd->base.str, true);
	u_var_add_ro_u32(icd, &icd->device_id, "device_id");

	return &icd->base;
}
