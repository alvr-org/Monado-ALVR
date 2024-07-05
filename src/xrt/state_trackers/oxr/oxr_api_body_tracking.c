// Copyright 2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  body tracking related API entrypoint functions.
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
oxr_xrCreateBodyTrackerFB(XrSession session, const XrBodyTrackerCreateInfoFB *createInfo, XrBodyTrackerFB *bodyTracker)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	XrResult ret = XR_SUCCESS;
	struct oxr_session *sess = NULL;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateBodyTrackerFB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_BODY_TRACKER_CREATE_INFO_FB);
	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, FB_body_tracking);

	struct oxr_body_tracker_fb *body_tracker_fb = NULL;
	ret = oxr_create_body_tracker_fb(&log, sess, createInfo, &body_tracker_fb);
	if (ret == XR_SUCCESS) {
		OXR_VERIFY_ARG_NOT_NULL(&log, body_tracker_fb);
		*bodyTracker = oxr_body_tracker_fb_to_openxr(body_tracker_fb);
	}
	return ret;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyBodyTrackerFB(XrBodyTrackerFB bodyTracker)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_body_tracker_fb *body_tracker_fb = NULL;
	OXR_VERIFY_BODY_TRACKER_FB_AND_INIT_LOG(&log, bodyTracker, body_tracker_fb, "xrDestroyBodyTrackerFB");

	return oxr_handle_destroy(&log, &body_tracker_fb->handle);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetBodySkeletonFB(XrBodyTrackerFB bodyTracker, XrBodySkeletonFB *skeleton)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_body_tracker_fb *body_tracker_fb = NULL;
	OXR_VERIFY_BODY_TRACKER_FB_AND_INIT_LOG(&log, bodyTracker, body_tracker_fb, "xrGetBodySkeletonFB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, body_tracker_fb->sess);
	OXR_VERIFY_ARG_NOT_NULL(&log, body_tracker_fb->xdev);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, skeleton, XR_TYPE_BODY_SKELETON_FB);

	return oxr_get_body_skeleton_fb(&log, body_tracker_fb, skeleton);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrLocateBodyJointsFB(XrBodyTrackerFB bodyTracker,
                         const XrBodyJointsLocateInfoFB *locateInfo,
                         XrBodyJointLocationsFB *locations)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_space *base_spc = NULL;
	struct oxr_body_tracker_fb *body_tracker_fb = NULL;
	OXR_VERIFY_BODY_TRACKER_FB_AND_INIT_LOG(&log, bodyTracker, body_tracker_fb, "xrLocateBodyJointsFB");
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, locateInfo, XR_TYPE_BODY_JOINTS_LOCATE_INFO_FB);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, locations, XR_TYPE_BODY_JOINT_LOCATIONS_FB);
	OXR_VERIFY_SESSION_NOT_LOST(&log, body_tracker_fb->sess);
	OXR_VERIFY_ARG_NOT_NULL(&log, body_tracker_fb->xdev);
	OXR_VERIFY_ARG_NOT_NULL(&log, locations->jointLocations);
	OXR_VERIFY_SPACE_NOT_NULL(&log, locateInfo->baseSpace, base_spc);

	return oxr_locate_body_joints_fb(&log, body_tracker_fb, base_spc, locateInfo, locations);
}
