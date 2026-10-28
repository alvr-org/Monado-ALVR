// Copyright 2024, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  BD (PICO) body tracking related API entrypoint functions.
 * @ingroup oxr_api
 */

#include "util/u_trace_marker.h"

#include "oxr_objects.h"
#include "oxr_logger.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"
#include "oxr_handle_base.h"
#include "oxr_chain.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef OXR_HAVE_BD_body_tracking

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateBodyTrackerBD(XrSession session, const XrBodyTrackerCreateInfoBD *createInfo, XrBodyTrackerBD *bodyTracker)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	XrResult ret = XR_SUCCESS;
	struct oxr_session *sess = NULL;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateBodyTrackerBD");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_BODY_TRACKER_CREATE_INFO_BD);
	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, BD_body_tracking);

	struct oxr_body_tracker_bd *body_tracker_bd = NULL;
	ret = oxr_create_body_tracker_bd(&log, sess, createInfo, &body_tracker_bd);
	if (ret == XR_SUCCESS) {
		OXR_VERIFY_ARG_NOT_NULL(&log, body_tracker_bd);
		*bodyTracker = oxr_body_tracker_bd_to_openxr(body_tracker_bd);
	}
	return ret;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyBodyTrackerBD(XrBodyTrackerBD bodyTracker)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_body_tracker_bd *body_tracker_bd = NULL;
	OXR_VERIFY_BODY_TRACKER_BD_AND_INIT_LOG(&log, bodyTracker, body_tracker_bd, "xrDestroyBodyTrackerBD");

	return oxr_handle_destroy(&log, &body_tracker_bd->handle);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrLocateBodyJointsBD(XrBodyTrackerBD bodyTracker,
                         const XrBodyJointsLocateInfoBD *locateInfo,
                         XrBodyJointLocationsBD *locations)
{
	OXR_TRACE_MARKER();

	struct oxr_logger log;
	struct oxr_space *base_spc = NULL;
	struct oxr_body_tracker_bd *body_tracker_bd = NULL;
	OXR_VERIFY_BODY_TRACKER_BD_AND_INIT_LOG(&log, bodyTracker, body_tracker_bd, "xrLocateBodyJointsBD");
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, locateInfo, XR_TYPE_BODY_JOINTS_LOCATE_INFO_BD);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, locations, XR_TYPE_BODY_JOINT_LOCATIONS_BD);
	OXR_VERIFY_SESSION_NOT_LOST(&log, body_tracker_bd->sess);
	OXR_VERIFY_ARG_NOT_NULL(&log, body_tracker_bd->xbt);
	OXR_VERIFY_ARG_NOT_NULL(&log, locations->jointLocations);
	OXR_VERIFY_SPACE_NOT_NULL(&log, locateInfo->baseSpace, base_spc);

	return oxr_locate_body_joints_bd(&log, body_tracker_bd, base_spc, locateInfo, locations);
}

#endif // OXR_HAVE_BD_body_tracking
