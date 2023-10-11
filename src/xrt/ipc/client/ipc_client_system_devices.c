// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client system devices.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

#include "ipc_client.h"
#include "ipc_client_generated.h"

#include "util/u_system_helpers.h"


struct ipc_client_system_devices
{
	//! @public Base
	struct u_system_devices base;

	//! Connection to service.
	struct ipc_connection *ipc_c;
};


/*
 *
 * Helpers
 *
 */

static inline struct ipc_client_system_devices *
ipc_system_devices(struct xrt_system_devices *xsysd)
{
	return (struct ipc_client_system_devices *)xsysd;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
ipc_client_system_devices_get_roles(struct xrt_system_devices *xsysd, struct xrt_system_roles *out_roles)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);

	return ipc_call_system_devices_get_roles(usysd->ipc_c, out_roles);
}

static void
ipc_client_system_devices_destroy(struct xrt_system_devices *xsysd)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);

	u_system_devices_close(&usysd->base.base);

	free(usysd);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_system_devices *
ipc_client_system_devices_create(struct ipc_connection *ipc_c)
{
	struct ipc_client_system_devices *icsd = U_TYPED_CALLOC(struct ipc_client_system_devices);
	icsd->base.base.get_roles = ipc_client_system_devices_get_roles;
	icsd->base.base.destroy = ipc_client_system_devices_destroy;
	icsd->ipc_c = ipc_c;

	return &icsd->base.base;
}
