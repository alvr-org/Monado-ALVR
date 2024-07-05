// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Monado @ref xrt_device API functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_api
 */

#include "openxr/openxr.h"
#include "xrt/xrt_compiler.h"

#include "math/m_api.h"

#include "util/u_debug.h"
#include "util/u_trace_marker.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_two_call.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"



#ifdef OXR_HAVE_MNDX_xdev_space

/*
 *
 * Helper functions.
 *
 */

#define OXR_VERIFY_XDEV_SPACE_SUPPORT(log, sys)                                                                        \
	do {                                                                                                           \
		if (!sys->supports_xdev_space) {                                                                       \
			return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, " system doesn't support xdev space");     \
		}                                                                                                      \
	} while (0)

static bool
find_index(const struct oxr_xdev_list *xdl, uint64_t id, uint32_t *out_index)
{
	uint32_t index = 0;
	for (; index < xdl->device_count; index++) {
		if (id != xdl->ids[index]) {
			continue;
		}
		break;
	}

	if (index >= xdl->device_count) {
		return false;
	}

	*out_index = index;

	return true;
}


/*
 *
 * API functions.
 *
 */

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateXDevListMNDX(XrSession session, const XrCreateXDevListInfoMNDX *info, XrXDevListMNDX *xdevList)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateXDevListMNDX");
	OXR_VERIFY_XDEV_SPACE_SUPPORT(&log, sess->sys);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, info, XR_TYPE_CREATE_XDEV_LIST_INFO_MNDX);
	struct oxr_xdev_list *xdl = NULL;
	XrResult ret = oxr_xdev_list_create(&log, sess, info, &xdl);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	*xdevList = oxr_xdev_list_to_openxr(xdl);

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetXDevListGenerationNumberMNDX(XrXDevListMNDX xdevList, uint64_t *outGeneration)
{
	struct oxr_xdev_list *xdl;
	struct oxr_logger log;

	OXR_VERIFY_XDEVLIST_AND_INIT_LOG(&log, xdevList, xdl, "xrGetXDevListGenerationNumberMNDX");
	*outGeneration = xdl->generation_number;

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrEnumerateXDevsMNDX(XrXDevListMNDX xdevList,
                         uint32_t xdevCapacityInput,
                         uint32_t *xdevCountOutput,
                         XrXDevIdMNDX *xdevs)
{
	struct oxr_xdev_list *xdl;
	struct oxr_logger log;

	OXR_VERIFY_XDEVLIST_AND_INIT_LOG(&log, xdevList, xdl, "xrEnumerateXDevsMNDX");

	OXR_TWO_CALL_HELPER(&log, xdevCapacityInput, xdevCountOutput, xdevs, xdl->device_count, xdl->ids,
	                    oxr_session_success_result(xdl->sess));
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetXDevPropertiesMNDX(XrXDevListMNDX xdevList, const XrGetXDevInfoMNDX *info, XrXDevPropertiesMNDX *properties)
{
	struct oxr_xdev_list *xdl;
	struct oxr_logger log;
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, info, XR_TYPE_GET_XDEV_INFO_MNDX);
	OXR_VERIFY_XDEVLIST_AND_INIT_LOG(&log, xdevList, xdl, "xrGetXDevPropertiesMNDX");

	uint32_t index = 0;
	if (!find_index(xdl, info->id, &index)) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE, "(info->id == %" PRIu64 ") Not a valid id",
		                 info->id);
	}

	XrResult ret = oxr_xdev_list_get_properties(&log, xdl, index, properties);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	return oxr_session_success_result(xdl->sess);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyXDevListMNDX(XrXDevListMNDX xdevList)
{
	struct oxr_xdev_list *xdl;
	struct oxr_logger log;

	OXR_VERIFY_XDEVLIST_AND_INIT_LOG(&log, xdevList, xdl, "xrDestroyXDevListMNDX");

	return oxr_handle_destroy(&log, &xdl->handle);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateXDevSpaceMNDX(XrSession session, const XrCreateXDevSpaceInfoMNDX *createInfo, XrSpace *space)
{
	struct oxr_session *sess;
	struct oxr_xdev_list *xdl;
	struct oxr_logger log;
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_CREATE_XDEV_SPACE_INFO_MNDX);
	OXR_VERIFY_ARG_NOT_NULL(&log, space);
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreateXDevSpaceMNDX");
	OXR_VERIFY_XDEV_SPACE_SUPPORT(&log, sess->sys);
	OXR_VERIFY_XDEVLIST_NOT_NULL(&log, createInfo->xdevList, xdl);
	OXR_VERIFY_POSE(&log, createInfo->offset);

	if (sess != xdl->sess) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE,
		                 "XDevSpace XrSpace must be created on the same session as XDevList");
	}

	uint32_t index = 0;
	if (!find_index(xdl, createInfo->id, &index)) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE, "(createInfo->id == %" PRIu64 ") Not a valid id",
		                 createInfo->id);
	}

	if (xdl->names[index] == 0) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE,
		                 "(createInfo->id == %" PRIu64
		                 ") Can not create a space. Is XrXDevPropertiesMNDX::canCreateSpace true?",
		                 createInfo->id);
	}

	struct oxr_space *spc = NULL;
	XrResult ret = oxr_xdev_list_space_create(&log, xdl, createInfo, index, &spc);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	*space = oxr_space_to_openxr(spc);

	return oxr_session_success_result(sess);
}

#endif
