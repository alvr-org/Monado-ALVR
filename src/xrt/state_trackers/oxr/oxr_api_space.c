// Copyright 2019-2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Space, space, space, SPAAAAAAAAAAAAAAAAAAAAAAAAAACE!
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_api
 */

#include "openxr/openxr.h"
#include "oxr_chain.h"
#include "util/u_misc.h"
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
		if (OXR_API_VERSION_AT_LEAST(sys->inst, 1, 1)) {
			return XR_SUCCESS;
		}

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
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_NOT_NULL(&log, bounds);

	ret = is_reference_space_type_valid(&log, sess->sys, "referenceSpaceType", referenceSpaceType);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	ret = is_reference_space_type_supported(&log, sess->sys, "referenceSpaceType", referenceSpaceType);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	return oxr_space_get_reference_bounds_rect(&log, sess, referenceSpaceType, bounds);
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
		// The CTS currently requires us to return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED.
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

static XrResult
verify_space(struct oxr_logger *log, XrSpace space, struct oxr_space **out_space)
{
	struct oxr_space *spc;
	OXR_VERIFY_SPACE_NOT_NULL(log, space, spc);
	*out_space = spc;
	return XR_SUCCESS;
}

static XrResult
locate_spaces(XrSession session, const XrSpacesLocateInfo *locateInfo, XrSpaceLocations *spaceLocations, char *fn)
{
	struct oxr_space *spc;
	struct oxr_space *baseSpc;
	struct oxr_logger log;
	OXR_VERIFY_SPACE_AND_INIT_LOG(&log, locateInfo->baseSpace, spc, fn);
	OXR_VERIFY_SESSION_NOT_LOST(&log, spc->sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, locateInfo, XR_TYPE_SPACES_LOCATE_INFO_KHR);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, spaceLocations, XR_TYPE_SPACE_LOCATIONS_KHR);
	OXR_VERIFY_SPACE_NOT_NULL(&log, locateInfo->baseSpace, baseSpc);

	OXR_VERIFY_ARG_NOT_ZERO(&log, locateInfo->spaceCount);
	OXR_VERIFY_ARG_NOT_ZERO(&log, spaceLocations->locationCount);

	if (locateInfo->spaceCount != spaceLocations->locationCount) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE,
		                 "(locateInfo->spaceCount == %d) must equal (spaceLocations->locationCount == %d)",
		                 locateInfo->spaceCount, spaceLocations->locationCount);
	}


	if (locateInfo->time <= (XrTime)0) {
		return oxr_error(&log, XR_ERROR_TIME_INVALID, "(time == %" PRIi64 ") is not a valid time.",
		                 locateInfo->time);
	}

	XrSpaceVelocitiesKHR *velocities =
	    OXR_GET_OUTPUT_FROM_CHAIN((void *)spaceLocations->next, XR_TYPE_SPACE_VELOCITIES_KHR, XrSpaceVelocitiesKHR);
	if (velocities) {
		if (velocities->velocityCount != locateInfo->spaceCount) {
			return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE,
			                 "(next->velocityCount == %d) must equal (locateInfo->spaceCount == %d)",
			                 velocities->velocityCount, locateInfo->spaceCount);
		}
	}


	uint32_t space_count = locateInfo->spaceCount;
	struct oxr_space **spaces = U_TYPED_ARRAY_CALLOC(struct oxr_space *, space_count);

	XrResult res;
	for (uint32_t i = 0; i < space_count; i++) {
		res = verify_space(&log, locateInfo->spaces[i], &spaces[i]);
		if (res != XR_SUCCESS) {
			break;
		}
	}

	if (res == XR_SUCCESS) {
		res = oxr_spaces_locate(&log, spaces, space_count, baseSpc, locateInfo->time, spaceLocations);
	}

	free(spaces);

	return res;
}

#ifdef OXR_HAVE_KHR_locate_spaces
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrLocateSpacesKHR(XrSession session, const XrSpacesLocateInfoKHR *locateInfo, XrSpaceLocationsKHR *spaceLocations)
{
	OXR_TRACE_MARKER();

	return locate_spaces(session, locateInfo, spaceLocations, "xrLocateSpacesKHR");
}
#endif

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrLocateSpaces(XrSession session, const XrSpacesLocateInfo *locateInfo, XrSpaceLocations *spaceLocations)
{
	OXR_TRACE_MARKER();

	return locate_spaces(session, locateInfo, spaceLocations, "xrLocateSpaces");
}
