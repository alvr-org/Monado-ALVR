// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client futures.
 * @author Korcan Hussein <korcan@hussein.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_defines.h"

#include "shared/ipc_message_channel.h"

#include "client/ipc_client.h"
#include "client/ipc_client_connection.h"

#include "ipc_client_generated.h"

struct ipc_client_future
{
	struct xrt_future base;
	struct ipc_connection *ipc_c;
	uint32_t id;
};

static inline struct ipc_client_future *
ipc_client_future(struct xrt_future *xft)
{
	return (struct ipc_client_future *)xft;
}

static inline const struct ipc_client_future *
const_ipc_client_future(const struct xrt_future *xft)
{
	return (const struct ipc_client_future *)xft;
}

static xrt_result_t
get_state(const struct xrt_future *xft, enum xrt_future_state *out_state)
{
	if (out_state == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}
	const struct ipc_client_future *ipc_xft = const_ipc_client_future(xft);
	const xrt_result_t xret = ipc_call_future_get_state(ipc_xft->ipc_c, ipc_xft->id, out_state);
	IPC_CHK_ALWAYS_RET(ipc_xft->ipc_c, xret, "ipc_call_future_get_state");
}

static xrt_result_t
get_result(const struct xrt_future *xft, struct xrt_future_result *out_ft_result)
{
	if (out_ft_result == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}
	const struct ipc_client_future *ipc_xft = const_ipc_client_future(xft);
	const xrt_result_t xret = ipc_call_future_get_result(ipc_xft->ipc_c, ipc_xft->id, out_ft_result);
	IPC_CHK_ALWAYS_RET(ipc_xft->ipc_c, xret, "ipc_call_future_get_result");
}

static xrt_result_t
cancel(struct xrt_future *xft)
{
	struct ipc_client_future *ipc_xft = ipc_client_future(xft);
	const xrt_result_t xret = ipc_call_future_cancel(ipc_xft->ipc_c, ipc_xft->id);
	IPC_CHK_ALWAYS_RET(ipc_xft->ipc_c, xret, "ipc_call_future_cancel");
}

static xrt_result_t
wait(struct xrt_future *xft, int64_t timeout_ns)
{
	return XRT_ERROR_NOT_IMPLEMENTED;
}

static xrt_result_t
is_cancel_requested(const struct xrt_future *xft, bool *out_request_cancel)
{
	return XRT_ERROR_NOT_IMPLEMENTED;
}

static xrt_result_t
complete(struct xrt_future *xft, const struct xrt_future_result *ft_result)
{
	return XRT_ERROR_NOT_IMPLEMENTED;
}

static void
destroy(struct xrt_future *xft)
{
	struct ipc_client_future *ipc_xft = ipc_client_future(xft);
	if (ipc_xft == NULL) {
		return;
	}
	const xrt_result_t xret = ipc_call_future_destroy(ipc_xft->ipc_c, ipc_xft->id);
	IPC_CHK_ONLY_PRINT(ipc_xft->ipc_c, xret, "ipc_call_future_destroy");

	free(ipc_xft);
}

struct xrt_future *
ipc_client_future_create(struct ipc_connection *ipc_c, uint32_t future_id)
{
	assert(ipc_c != NULL);
	struct ipc_client_future *icft = U_TYPED_CALLOC(struct ipc_client_future);
	struct xrt_future *xft = &icft->base;
	xft->destroy = destroy;
	xft->get_state = get_state;
	xft->get_result = get_result;
	xft->cancel = cancel;
	xft->wait = wait;
	xft->is_cancel_requested = is_cancel_requested;
	xft->complete = complete;
	xft->reference.count = 1;
	icft->id = future_id;
	icft->ipc_c = ipc_c;

	return xft;
}
