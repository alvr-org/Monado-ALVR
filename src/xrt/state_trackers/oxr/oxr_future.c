// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  future related functions.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle_base.h"
#include "oxr_xret.h"


static inline XrFutureStateEXT
oxr_to_XrFutureStateEXT(const xrt_future_state_t fts)
{
	switch (fts) {
	case XRT_FUTURE_STATE_PENDING: return XR_FUTURE_STATE_PENDING_EXT;
	case XRT_FUTURE_STATE_READY: return XR_FUTURE_STATE_READY_EXT;
	default: return XR_FUTURE_STATE_MAX_ENUM_EXT;
	}
}

XrResult
oxr_future_invalidate(struct oxr_logger *log, struct oxr_future_ext *oxr_future)
{
	(void)log;
	if (oxr_future && oxr_future->xft) {
		xrt_future_reference(&oxr_future->xft, NULL);
		assert(oxr_future->xft == NULL);
	}
	return XR_SUCCESS;
}

XrResult
oxr_future_ext_poll(struct oxr_logger *log, const struct oxr_future_ext *oxr_future, XrFuturePollResultEXT *pollResult)
{
	assert(log && oxr_future && oxr_future->xft && pollResult);
	xrt_future_state_t fts;
	const xrt_result_t xres = xrt_future_get_state(oxr_future->xft, &fts);
	OXR_CHECK_XRET(log, oxr_future->sess, xres, oxr_future_ext_poll);
	pollResult->state = oxr_to_XrFutureStateEXT(fts);
	return XR_SUCCESS;
}

XrResult
oxr_future_ext_cancel(struct oxr_logger *log, struct oxr_future_ext *oxr_future)
{
	assert(log && oxr_future && oxr_future->xft);
	const xrt_result_t xres = xrt_future_cancel(oxr_future->xft);
	OXR_CHECK_XRET(log, oxr_future->sess, xres, oxr_future_ext_cancel);
	return oxr_future_invalidate(log, oxr_future);
}

XrResult
oxr_future_ext_complete(struct oxr_logger *log,
                        struct oxr_future_ext *oxr_future,
                        struct xrt_future_result *out_ft_result)
{
	struct oxr_session *sess = oxr_future->sess;
	const xrt_result_t xret = xrt_future_get_result(oxr_future->xft, out_ft_result);
	OXR_CHECK_XRET(log, sess, xret, oxr_future_ext_complete);
	if (xret == XRT_ERROR_FUTURE_RESULT_NOT_READY) {
		return oxr_error(log, XR_ERROR_FUTURE_PENDING_EXT, "Call to oxr_future_ext_complete failed");
	}
	return oxr_future_invalidate(log, oxr_future);
}

static XrResult
oxr_future_ext_destroy(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_future_ext *future_ext = (struct oxr_future_ext *)hb;
	if (future_ext && future_ext->xft) {
		oxr_future_ext_cancel(log, future_ext);
	}
	free(future_ext);
	return XR_SUCCESS;
}

XrResult
oxr_future_create(struct oxr_logger *log,
                  struct oxr_session *sess,
                  struct xrt_future *xft,
                  struct oxr_handle_parent_base *parent_handle,
                  struct oxr_future_ext **out_oxr_future_ext)
{
	struct oxr_future_ext *new_future = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, new_future, OXR_XR_DEBUG_FUTURE, oxr_future_ext_destroy, parent_handle);
	new_future->sess = sess;
	new_future->inst = sess->sys->inst;
	new_future->xft = xft;
	*out_oxr_future_ext = new_future;
	return XR_SUCCESS;
}
