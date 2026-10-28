// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Android face tracking related API entrypoint functions.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_api
 */

#include "util/u_trace_marker.h"

#include "oxr_objects.h"
#include "oxr_logger.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"
#include "oxr_handle_base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateFaceTrackerANDROID(XrSession session,
                               const XrFaceTrackerCreateInfoANDROID *createInfo,
                               XrFaceTrackerANDROID *faceTracker)
{
	OXR_TRACE_MARKER();
	struct oxr_logger log;
	struct oxr_session *sess = NULL;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateFaceTrackerANDROID");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, ANDROID_face_tracking);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_FACE_TRACKER_CREATE_INFO_ANDROID);
	return oxr_face_tracker_android_create(&log, sess, createInfo, faceTracker);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyFaceTrackerANDROID(XrFaceTrackerANDROID facialTracker)
{
	OXR_TRACE_MARKER();
	struct oxr_logger log;
	struct oxr_face_tracker_android *face_tracker_android = NULL;
	OXR_VERIFY_FACE_TRACKER_ANDROID_AND_INIT_LOG(&log, facialTracker, face_tracker_android,
	                                             "xrDestroyFaceTrackerANDROID");
	return oxr_handle_destroy(&log, &face_tracker_android->handle);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetFaceStateANDROID(XrFaceTrackerANDROID faceTracker,
                          const XrFaceStateGetInfoANDROID *getInfo,
                          XrFaceStateANDROID *faceStateOutput)
{
	OXR_TRACE_MARKER();
	struct oxr_logger log;
	struct oxr_face_tracker_android *face_tracker_android = NULL;
	OXR_VERIFY_FACE_TRACKER_ANDROID_AND_INIT_LOG(&log, faceTracker, face_tracker_android, "xrGetFaceStateANDROID");
	OXR_VERIFY_SESSION_NOT_LOST(&log, face_tracker_android->sess);
	OXR_VERIFY_ARG_NOT_NULL(&log, face_tracker_android->xdev);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, getInfo, XR_TYPE_FACE_STATE_GET_INFO_ANDROID);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, faceStateOutput, XR_TYPE_FACE_STATE_ANDROID);
	OXR_VERIFY_ARG_TIME_NOT_ZERO(&log, getInfo->time);
	OXR_VERIFY_TWO_CALL_ARRAY(&log, faceStateOutput->parametersCapacityInput,
	                          (&faceStateOutput->parametersCountOutput), faceStateOutput->parameters);
	OXR_VERIFY_TWO_CALL_ARRAY(&log, faceStateOutput->regionConfidencesCapacityInput,
	                          (&faceStateOutput->regionConfidencesCountOutput), faceStateOutput->regionConfidences);
	return oxr_get_face_state_android(&log, face_tracker_android, getInfo, faceStateOutput);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetFaceCalibrationStateANDROID(XrFaceTrackerANDROID faceTracker, XrBool32 *faceIsCalibratedOutput)
{
	OXR_TRACE_MARKER();
	struct oxr_logger log;
	struct oxr_face_tracker_android *face_tracker_android = NULL;
	OXR_VERIFY_FACE_TRACKER_ANDROID_AND_INIT_LOG(&log, faceTracker, face_tracker_android,
	                                             "xrGetFaceCalibrationStateANDROID");
	OXR_VERIFY_SESSION_NOT_LOST(&log, face_tracker_android->sess);
	OXR_VERIFY_ARG_NOT_NULL(&log, face_tracker_android->xdev);
	OXR_VERIFY_ARG_NOT_NULL(&log, faceIsCalibratedOutput);
	return oxr_get_face_calibration_state_android(&log, face_tracker_android, faceIsCalibratedOutput);
}
