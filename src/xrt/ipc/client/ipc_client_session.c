// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Client side wrapper of @ref xrt_session.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_defines.h"
#include "xrt/xrt_session.h"

#include "ipc_client_generated.h"


/*!
 * IPC client implementation of @ref xrt_session.
 *
 * @implements xrt_session.
 * @ingroup ipc_client
 */
struct ipc_client_session
{
	struct xrt_session base;

	struct ipc_connection *ipc_c;
};


/*
 *
 * Helpers
 *
 */

static inline struct ipc_client_session *
ipc_session(struct xrt_session *xs)
{
	return (struct ipc_client_session *)xs;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
ipc_client_session_poll_events(struct xrt_session *xs, union xrt_session_event *out_xse)
{
	struct ipc_client_session *ics = ipc_session(xs);
	xrt_result_t xret;

	xret = ipc_call_session_poll_events(ics->ipc_c, out_xse);
	IPC_CHK_ALWAYS_RET(ics->ipc_c, xret, "ipc_call_session_poll_events");
}

static void
ipc_client_session_destroy(struct xrt_session *xs)
{
	struct ipc_client_session *ics = ipc_session(xs);
	xrt_result_t xret;

	/*
	 * We own the session in both cases of headless or created with a
	 * native compositor, so we need to destroy it.
	 */
	xret = ipc_call_session_destroy(ics->ipc_c);

	/*
	 * We are probably in a really bad state if we fail, at
	 * least print out the error and continue as best we can.
	 */
	IPC_CHK_ONLY_PRINT(ics->ipc_c, xret, "ipc_call_session_destroy");

	free(ics);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_session *
ipc_client_session_create(struct ipc_connection *ipc_c)
{
	struct ipc_client_session *ics = U_TYPED_CALLOC(struct ipc_client_session);
	ics->base.poll_events = ipc_client_session_poll_events;
	ics->base.destroy = ipc_client_session_destroy;
	ics->ipc_c = ipc_c;

	return &ics->base;
}
