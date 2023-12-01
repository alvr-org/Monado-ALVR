// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Client side wrapper of @ref xrt_system.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_defines.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_session.h"

#include "ipc_client_generated.h"


/*!
 * IPC client implementation of @ref xrt_system.
 *
 * @implements xrt_system.
 * @ingroup ipc_client
 */
struct ipc_client_system
{
	struct xrt_system base;

	struct ipc_connection *ipc_c;

	struct xrt_system_compositor *xsysc;
};


/*
 *
 * Helpers
 *
 */

static inline struct ipc_client_system *
ipc_system(struct xrt_system *xsys)
{
	return (struct ipc_client_system *)xsys;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
ipc_client_system_create_session(struct xrt_system *xsys,
                                 const struct xrt_session_info *xsi,
                                 struct xrt_session **out_xs,
                                 struct xrt_compositor_native **out_xcn)
{
	struct ipc_client_system *icsys = ipc_system(xsys);
	xrt_result_t xret = XRT_SUCCESS;

	if (out_xcn != NULL && icsys->xsysc == NULL) {
		U_LOG_E("No system compositor in system, can't create native compositor.");
		return XRT_ERROR_COMPOSITOR_NOT_SUPPORTED;
	}

	struct xrt_session *xs = ipc_client_session_create(icsys->ipc_c);

	// Skip making a native compositor if not asked for.
	if (out_xcn == NULL) {
		goto out_session;
	}

	xret = xrt_syscomp_create_native_compositor( //
	    icsys->xsysc,                            //
	    xsi,                                     //
	    out_xcn);                                //
	if (xret != XRT_SUCCESS) {
		goto err;
	}

out_session:
	*out_xs = xs;

	return XRT_SUCCESS;

err:
	return xret;
}

static void
ipc_client_system_destroy(struct xrt_system *xsys)
{
	struct ipc_client_system *icsys = ipc_system(xsys);

	free(icsys);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_system *
ipc_client_system_create(struct ipc_connection *ipc_c, struct xrt_system_compositor *xsysc)
{
	struct ipc_client_system *icsys = U_TYPED_CALLOC(struct ipc_client_system);
	icsys->base.create_session = ipc_client_system_create_session;
	icsys->base.destroy = ipc_client_system_destroy;
	icsys->ipc_c = ipc_c;
	icsys->xsysc = xsysc;

	return &icsys->base;
}
