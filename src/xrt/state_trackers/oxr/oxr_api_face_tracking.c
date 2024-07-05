// Copyright 2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  face tracking related API entrypoint functions.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_api
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/u_trace_marker.h"

#include "oxr_objects.h"
#include "oxr_logger.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"
#include "oxr_handle.h"

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateFacialTrackerHTC(XrSession session,
                             const XrFacialTrackerCreateInfoHTC *createInfo,
                             XrFacialTrackerHTC *facialTracker)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	XrResult ret = XR_SUCCESS;
	struct oxr_session *sess = NULL;
	struct oxr_facial_tracker_htc *facial_tracker_htc = NULL;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateFacialTrackerHTC");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_FACIAL_TRACKER_CREATE_INFO_HTC);
	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, HTC_facial_tracking);

	ret = oxr_facial_tracker_htc_create(&log, sess, createInfo, &facial_tracker_htc);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	OXR_VERIFY_ARG_NOT_NULL(&log, facial_tracker_htc);
	*facialTracker = oxr_facial_tracker_htc_to_openxr(facial_tracker_htc);

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyFacialTrackerHTC(XrFacialTrackerHTC facialTracker)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_facial_tracker_htc *facial_tracker_htc = NULL;
	OXR_VERIFY_FACE_TRACKER_HTC_AND_INIT_LOG(&log, facialTracker, facial_tracker_htc, "xrDestroyFacialTrackerHTC");

	return oxr_handle_destroy(&log, &facial_tracker_htc->handle);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetFacialExpressionsHTC(XrFacialTrackerHTC facialTracker, XrFacialExpressionsHTC *facialExpressions)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_facial_tracker_htc *facial_tracker_htc = NULL;
	OXR_VERIFY_FACE_TRACKER_HTC_AND_INIT_LOG(&log, facialTracker, facial_tracker_htc, "xrGetFacialExpressionsHTC");
	OXR_VERIFY_SESSION_NOT_LOST(&log, facial_tracker_htc->sess);
	OXR_VERIFY_ARG_NOT_NULL(&log, facial_tracker_htc->xdev);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, facialExpressions, XR_TYPE_FACIAL_EXPRESSIONS_HTC);
	OXR_VERIFY_ARG_NOT_NULL(&log, facialExpressions->expressionWeightings);

#define OXR_VERIFY_FACE_EXPRESSION_COUNT(fttype)                                                                       \
	if (facial_tracker_htc->facial_tracking_type == XRT_FACIAL_TRACKING_TYPE_##fttype##_DEFAULT_HTC &&             \
	    facialExpressions->expressionCount < XRT_FACIAL_EXPRESSION_##fttype##_COUNT_HTC) {                         \
		return oxr_error(                                                                                      \
		    &log, XR_ERROR_SIZE_INSUFFICIENT,                                                                  \
		    "\"expressionCount\" (%d) size is less than the minimum size (%d) required for " #fttype           \
		    " expressions.\n",                                                                                 \
		    facialExpressions->expressionCount, XRT_FACIAL_EXPRESSION_##fttype##_COUNT_HTC);                   \
	}

	OXR_VERIFY_FACE_EXPRESSION_COUNT(EYE)
	OXR_VERIFY_FACE_EXPRESSION_COUNT(LIP)
#undef OXR_VERIFY_FACE_EXPRESSION_COUNT

	return oxr_get_facial_expressions_htc(&log, facial_tracker_htc, facialExpressions);
}
