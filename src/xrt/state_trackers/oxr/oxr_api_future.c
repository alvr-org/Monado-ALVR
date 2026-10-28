// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  future related API entrypoint functions.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_api
 */

#include "util/u_trace_marker.h"

#include "oxr_objects.h"
#include "oxr_logger.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"
#include "oxr_handle_base.h"
#include "oxr_defines.h"


XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrPollFutureEXT(XrInstance instance, const XrFuturePollInfoEXT *pollInfo, XrFuturePollResultEXT *pollResult)
{
	struct oxr_logger log;
	struct oxr_instance *inst = NULL;
	struct oxr_future_ext *oxr_future = NULL;
	OXR_VERIFY_INSTANCE_AND_INIT_LOG(&log, instance, inst, "xrPollFutureEXT");
	OXR_VERIFY_EXTENSION(&log, inst, EXT_future);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, pollInfo, XR_TYPE_FUTURE_POLL_INFO_EXT);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, pollResult, XR_TYPE_FUTURE_POLL_RESULT_EXT);
	OXR_VERIFY_FUTURE_AND_INIT_LOG(&log, pollInfo->future, oxr_future, "xrPollFutureEXT");
	return oxr_future_ext_poll(&log, oxr_future, pollResult);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCancelFutureEXT(XrInstance instance, const XrFutureCancelInfoEXT *cancelInfo)
{
	struct oxr_logger log;
	struct oxr_instance *inst = NULL;
	struct oxr_future_ext *oxr_future = NULL;
	OXR_VERIFY_INSTANCE_AND_INIT_LOG(&log, instance, inst, "xrCancelFutureEXT");
	OXR_VERIFY_EXTENSION(&log, inst, EXT_future);
	OXR_VERIFY_FUTURE_AND_INIT_LOG(&log, cancelInfo->future, oxr_future, "xrCancelFutureEXT");
	return oxr_future_ext_cancel(&log, oxr_future);
}
