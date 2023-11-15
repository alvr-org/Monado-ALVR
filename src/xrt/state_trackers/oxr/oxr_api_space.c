// Copyright 2019-2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Space, space, space, SPAAAAAAAAAAAAAAAAAAAAAAAAAACE!
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_api
 */

#include "xrt/xrt_compiler.h"

#include "util/u_debug.h"
#include "util/u_trace_marker.h"

#include "math/m_api.h" // IWYU pragma: keep

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_conversions.h"
#include "oxr_two_call.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


/*
 *
 * Helpers.
 *
 */

static XrResult
is_reference_space_type_valid(struct oxr_logger *log,
                              struct oxr_system *sys,
                              const char *field_name,
                              XrReferenceSpaceType referenceSpaceType)
{
	switch (referenceSpaceType) {
	case XR_REFERENCE_SPACE_TYPE_VIEW:
	case XR_REFERENCE_SPACE_TYPE_LOCAL:
	case XR_REFERENCE_SPACE_TYPE_STAGE: return XR_SUCCESS;
	case XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT:
#ifdef OXR_HAVE_EXT_local_floor
		if (sys->inst->extensions.EXT_local_floor) {
			return XR_SUCCESS;
		}
#endif
		return oxr_error(
		    log, XR_ERROR_VALIDATION_FAILURE,
		    "(%s == XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT) is only valid if XR_EXT_local_floor is enabled",
		    field_name);
	case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT:
#ifdef OXR_HAVE_MSFT_unbounded_reference_space
		if (sys->inst->extensions.MSFT_unbounded_reference_space) {
			return XR_SUCCESS;
		}
#endif
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE,
		                 "(%s == XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT) is only valid if "
		                 "XR_MSFT_unbounded_reference_space is enabled",
		                 field_name);
	default: break;
	}

	return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "(%s == 0x%08x) is not a valid XrReferenceSpaceType",
	                 field_name, referenceSpaceType);
}

static XrResult
is_reference_space_type_supported(struct oxr_logger *log,
                                  struct oxr_system *sys,
                                  const char *field_name,
                                  XrReferenceSpaceType referenceSpaceType)
{
	for (uint32_t i = 0; i < sys->reference_space_count; i++) {
		if (sys->reference_spaces[i] == referenceSpaceType) {
			return XR_SUCCESS;
		}
	}

	/*
	 * This function assumes that the referenceSpaceType has
	 * been validated with is_reference_space_type_valid first.
	 */
	return oxr_error(log, XR_ERROR_REFERENCE_SPACE_UNSUPPORTED,
	                 "(%s == %s) is not a supported XrReferenceSpaceType", field_name,
	                 xr_ref_space_to_string(referenceSpaceType));
}


/*
 *
 * API functions.
 *
 */

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateActionSpace(XrSession session, const XrActionSpaceCreateInfo *createInfo, XrSpace *space)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_action *act;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateActionSpace");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_ACTION_SPACE_CREATE_INFO);
	OXR_VERIFY_POSE(&log, createInfo->poseInActionSpace);
	OXR_VERIFY_ACTION_NOT_NULL(&log, createInfo->action, act);

	struct oxr_space *spc;
	XrResult ret = oxr_space_action_create(&log, sess, act->act_key, createInfo, &spc);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	*space = oxr_space_to_openxr(spc);

	return oxr_session_success_result(sess);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrEnumerateReferenceSpaces(XrSession session,
                               uint32_t spaceCapacityInput,
                               uint32_t *spaceCountOutput,
                               XrReferenceSpaceType *spaces)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrEnumerateReferenceSpaces");

	OXR_TWO_CALL_HELPER(                   //
	    &log,                              //
	    spaceCapacityInput,                //
	    spaceCountOutput,                  //
	    spaces,                            //
	    sess->sys->reference_space_count,  //
	    sess->sys->reference_spaces,       //
	    oxr_session_success_result(sess)); //
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetReferenceSpaceBoundsRect(XrSession session, XrReferenceSpaceType referenceSpaceType, XrExtent2Df *bounds)
{
	OXR_TRACE_MARKER();

	XrResult ret;
	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrGetReferenceSpaceBoundsRect");
	OXR_VERIFY_ARG_NOT_NULL(&log, bounds);

	ret = is_reference_space_type_valid(&log, sess->sys, "referenceSpaceType", referenceSpaceType);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	ret = is_reference_space_type_supported(&log, sess->sys, "referenceSpaceType", referenceSpaceType);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	bounds->width = 0.0;
	bounds->height = 0.0;

	// Silently return that the bounds aren't available.
	return XR_SPACE_BOUNDS_UNAVAILABLE;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo *createInfo, XrSpace *out_space)
{
	OXR_TRACE_MARKER();

	XrResult ret;
	struct oxr_session *sess;
	struct oxr_space *spc = NULL;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateReferenceSpace");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_REFERENCE_SPACE_CREATE_INFO);
	OXR_VERIFY_POSE(&log, createInfo->poseInReferenceSpace);

	ret = is_reference_space_type_valid(&log, sess->sys, "createInfo->referenceSpaceType",
	                                    createInfo->referenceSpaceType);
	if (ret != XR_SUCCESS) {
		// The CTS currently requiers us to return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED.
		if (sess->sys->inst->quirks.no_validation_error_in_create_ref_space &&
		    ret == XR_ERROR_VALIDATION_FAILURE) {
			return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED;
		} else {
			return ret;
		}
	}

	ret = is_reference_space_type_supported(&log, sess->sys, "createInfo->referenceSpaceType",
	                                        createInfo->referenceSpaceType);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	ret = oxr_space_reference_create(&log, sess, createInfo, &spc);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	*out_space = oxr_space_to_openxr(spc);

	return oxr_session_success_result(sess);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation *location)
{
	OXR_TRACE_MARKER();

	struct oxr_space *spc;
	struct oxr_space *baseSpc;
	struct oxr_logger log;
	OXR_VERIFY_SPACE_AND_INIT_LOG(&log, space, spc, "xrLocateSpace");
	OXR_VERIFY_SESSION_NOT_LOST(&log, spc->sess);
	OXR_VERIFY_SPACE_NOT_NULL(&log, baseSpace, baseSpc);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, location, XR_TYPE_SPACE_LOCATION);

	if (time <= (XrTime)0) {
		return oxr_error(&log, XR_ERROR_TIME_INVALID, "(time == %" PRIi64 ") is not a valid time.", time);
	}

	return oxr_space_locate(&log, spc, baseSpc, time, location);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroySpace(XrSpace space)
{
	OXR_TRACE_MARKER();

	struct oxr_space *spc;
	struct oxr_logger log;
	OXR_VERIFY_SPACE_AND_INIT_LOG(&log, space, spc, "xrDestroySpace");

	return oxr_handle_destroy(&log, &spc->handle);
}
