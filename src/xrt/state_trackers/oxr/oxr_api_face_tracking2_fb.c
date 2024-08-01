// Copyright 2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  face tracking related API entrypoint functions.
 * @author galister <galister@librevr.org>
 * @ingroup oxr_api
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "openxr/openxr.h"
#include "util/u_trace_marker.h"

#include "oxr_objects.h"
#include "oxr_logger.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"
#include "oxr_handle.h"

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateFaceTracker2FB(XrSession session,
                           const XrFaceTrackerCreateInfo2FB *createInfo,
                           XrFaceTracker2FB *faceTracker)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	XrResult ret = XR_SUCCESS;
	struct oxr_session *sess = NULL;
	struct oxr_face_tracker2_fb *face_tracker2_fb = NULL;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateFaceTracker2FB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_FACE_TRACKER_CREATE_INFO2_FB);
	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, FB_face_tracking2);

	ret = oxr_face_tracker2_fb_create(&log, sess, createInfo, &face_tracker2_fb);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	OXR_VERIFY_ARG_NOT_NULL(&log, face_tracker2_fb);
	*faceTracker = oxr_face_tracker2_fb_to_openxr(face_tracker2_fb);

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyFaceTracker2FB(XrFaceTracker2FB faceTracker)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_face_tracker2_fb *face_tracker2_fb = NULL;
	OXR_VERIFY_FACE_TRACKER2_FB_AND_INIT_LOG(&log, faceTracker, face_tracker2_fb, "xrDestroyFaceTracker2FB");

	return oxr_handle_destroy(&log, &face_tracker2_fb->handle);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetFaceExpressionWeights2FB(XrFaceTracker2FB faceTracker,
                                  const XrFaceExpressionInfo2FB *expressionInfo,
                                  XrFaceExpressionWeights2FB *expressionWeights)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_face_tracker2_fb *face_tracker2_fb = NULL;
	OXR_VERIFY_FACE_TRACKER2_FB_AND_INIT_LOG(&log, faceTracker, face_tracker2_fb, "xrGetFaceExpressionWeights2FB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, face_tracker2_fb->sess);
	OXR_VERIFY_ARG_NOT_NULL(&log, face_tracker2_fb->xdev);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, expressionInfo, XR_TYPE_FACE_EXPRESSION_INFO2_FB);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, expressionWeights, XR_TYPE_FACE_EXPRESSION_WEIGHTS2_FB);
	OXR_VERIFY_ARG_NOT_NULL(&log, expressionWeights->weights);
	OXR_VERIFY_ARG_NOT_NULL(&log, expressionWeights->confidences);

	return oxr_get_face_expression_weights2_fb(&log, face_tracker2_fb, expressionInfo, expressionWeights);
}
