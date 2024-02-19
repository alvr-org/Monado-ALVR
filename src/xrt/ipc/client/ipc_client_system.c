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

#include <assert.h>

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

static inline xrt_result_t
create_headless(struct ipc_client_system *icsys, const struct xrt_session_info *xsi, struct xrt_session **out_xs)
{
	xrt_result_t xret = XRT_SUCCESS;

	// We create the session ourselves.
	xret = ipc_call_session_create( //
	    icsys->ipc_c,               // ipc_c
	    xsi,                        // xsi
	    false);                     // create_native_compositor
	IPC_CHK_AND_RET(icsys->ipc_c, xret, "ipc_call_session_create");

	struct xrt_session *xs = ipc_client_session_create(icsys->ipc_c);
	assert(xs != NULL);

	*out_xs = xs;

	return XRT_SUCCESS;
}

static inline xrt_result_t
create_with_comp(struct ipc_client_system *icsys,
                 const struct xrt_session_info *xsi,
                 struct xrt_session **out_xs,
                 struct xrt_compositor_native **out_xcn)
{
	xrt_result_t xret = XRT_SUCCESS;

	assert(icsys->xsysc != NULL);

	// The native compositor creates the session.
	xret = ipc_client_create_native_compositor( //
	    icsys->xsysc,                           //
	    xsi,                                    //
	    out_xcn);                               //
	IPC_CHK_AND_RET(icsys->ipc_c, xret, "ipc_client_create_native_compositor");

	struct xrt_session *xs = ipc_client_session_create(icsys->ipc_c);
	assert(xs != NULL);

	*out_xs = xs;

	return XRT_SUCCESS;
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

	if (out_xcn != NULL && icsys->xsysc == NULL) {
		U_LOG_E("No system compositor in system, can't create native compositor.");
		return XRT_ERROR_COMPOSITOR_NOT_SUPPORTED;
	}

	// Skip making a native compositor if not asked for.
	if (out_xcn == NULL) {
		return create_headless(icsys, xsi, out_xs);
	} else {
		return create_with_comp(icsys, xsi, out_xs, out_xcn);
	}
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
	xrt_result_t xret = ipc_call_system_get_properties(ipc_c, &icsys->base.properties);
	assert(xret == XRT_SUCCESS);
	icsys->base.create_session = ipc_client_system_create_session;
	icsys->base.destroy = ipc_client_system_destroy;
	icsys->ipc_c = ipc_c;
	icsys->xsysc = xsysc;

	return &icsys->base;
}
