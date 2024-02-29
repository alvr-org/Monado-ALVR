// Copyright 2023-2024, Qualcomm Innovation Center, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  passthrough related API entrypoint functions.
 * @author Knox Liu <quic_dengkail@qti.qualcomm.com>
 * @ingroup oxr_api
 */

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle.h"

#include "util/u_debug.h"
#include "util/u_trace_marker.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"
#include "oxr_chain.h"
#include "oxr_subaction.h"

#include <stdio.h>
#include <inttypes.h>

XrResult
oxr_xrCreateGeometryInstanceFB(XrSession session,
                               const XrGeometryInstanceCreateInfoFB *createInfo,
                               XrGeometryInstanceFB *outGeometryInstance)
{
	OXR_TRACE_MARKER();
	struct oxr_logger log;
	oxr_log_init(&log, "oxr_xrCreateGeometryInstanceFB");
	return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE, " not implemented");
}

XrResult
oxr_xrCreatePassthroughFB(XrSession session,
                          const XrPassthroughCreateInfoFB *createInfo,
                          XrPassthroughFB *outPassthrough)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "oxr_xrCreatePassthroughFB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_PASSTHROUGH_CREATE_INFO_FB);
	OXR_VERIFY_PASSTHROUGH_FLAGS(&log, createInfo->flags);

	struct oxr_passthrough *passthrough;
	XrResult ret = oxr_passthrough_create(&log, sess, createInfo, &passthrough);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	*outPassthrough = oxr_passthrough_to_openxr(passthrough);

	return oxr_session_success_result(sess);
}

XrResult
oxr_xrCreatePassthroughLayerFB(XrSession session,
                               const XrPassthroughLayerCreateInfoFB *createInfo,
                               XrPassthroughLayerFB *outLayer)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "oxr_xrCreatePassthroughLayerFB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB);
	OXR_VERIFY_ARG_NOT_NULL(&log, createInfo->passthrough);
	OXR_VERIFY_PASSTHROUGH_FLAGS(&log, createInfo->flags);
	OXR_VERIFY_PASSTHROUGH_LAYER_PURPOSE(&log, createInfo->purpose);

	struct oxr_passthrough_layer *passthroughLayer;
	XrResult ret = oxr_passthrough_layer_create(&log, sess, createInfo, &passthroughLayer);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	*outLayer = oxr_passthrough_layer_to_openxr(passthroughLayer);

	return oxr_session_success_result(sess);
}

XrResult
oxr_xrDestroyGeometryInstanceFB(XrGeometryInstanceFB instance)
{
	OXR_TRACE_MARKER();
	struct oxr_logger log;
	oxr_log_init(&log, "oxr_xrDestroyGeometryInstanceFB");
	return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE, " not implemented");
}

XrResult
oxr_xrDestroyPassthroughFB(XrPassthroughFB passthrough)
{
	OXR_TRACE_MARKER();

	struct oxr_passthrough *pt;
	struct oxr_logger log;
	OXR_VERIFY_PASSTHROUGH_AND_INIT_LOG(&log, passthrough, pt, "oxr_xrDestroyPassthroughFB");

	xrt_comp_destroy_passthrough(pt->sess->compositor);

	return oxr_handle_destroy(&log, &pt->handle);
}

XrResult
oxr_xrDestroyPassthroughLayerFB(XrPassthroughLayerFB layer)
{
	OXR_TRACE_MARKER();

	struct oxr_passthrough_layer *pl;
	struct oxr_logger log;
	OXR_VERIFY_PASSTHROUGH_LAYER_AND_INIT_LOG(&log, layer, pl, "oxr_xrDestroyPassthroughLayerFB");

	return oxr_handle_destroy(&log, &pl->handle);
}

XrResult
oxr_xrGeometryInstanceSetTransformFB(XrGeometryInstanceFB instance, const XrGeometryInstanceTransformFB *transformation)
{
	OXR_TRACE_MARKER();
	struct oxr_logger log;
	oxr_log_init(&log, "oxr_xrGeometryInstanceSetTransformFB");
	return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE, " not implemented");
}

XrResult
oxr_xrPassthroughLayerPauseFB(XrPassthroughLayerFB layer)
{
	OXR_TRACE_MARKER();
	struct oxr_passthrough_layer *pl;
	struct oxr_logger log;
	OXR_VERIFY_PASSTHROUGH_LAYER_AND_INIT_LOG(&log, layer, pl, "oxr_xrPassthroughLayerPauseFB");
	pl->paused = true;
	return XR_SUCCESS;
}

XrResult
oxr_xrPassthroughLayerResumeFB(XrPassthroughLayerFB layer)
{
	OXR_TRACE_MARKER();
	struct oxr_passthrough_layer *pl;
	struct oxr_logger log;
	OXR_VERIFY_PASSTHROUGH_LAYER_AND_INIT_LOG(&log, layer, pl, "oxr_xrPassthroughLayerResumeFB");
	pl->paused = false;
	return XR_SUCCESS;
}

XrResult
oxr_xrPassthroughLayerSetStyleFB(XrPassthroughLayerFB layer, const XrPassthroughStyleFB *style)
{
	OXR_TRACE_MARKER();
	struct oxr_passthrough_layer *pl;
	struct oxr_logger log;
	OXR_VERIFY_PASSTHROUGH_LAYER_AND_INIT_LOG(&log, layer, pl, "oxr_xrPassthroughLayerResumeFB");
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, style, XR_TYPE_PASSTHROUGH_STYLE_FB);
	OXR_VERIFY_PASSTHROUGH_LAYER_STYLE(&log, style);

	pl->style = *style;
	XrPassthroughStyleFB *next = (XrPassthroughStyleFB *)style->next;
	while (next) {
		switch (next->type) {
		case XR_TYPE_PASSTHROUGH_BRIGHTNESS_CONTRAST_SATURATION_FB:
			pl->brightnessContrastSaturation = *(XrPassthroughBrightnessContrastSaturationFB *)next;
			break;
		case XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_MONO_FB:
			pl->monoToMono = *(XrPassthroughColorMapMonoToMonoFB *)next;
			break;
		case XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_RGBA_FB:
			pl->monoToRgba = *(XrPassthroughColorMapMonoToRgbaFB *)next;
			break;
		}

		next = (XrPassthroughStyleFB *)next->next;
	}

	return XR_SUCCESS;
}

XrResult
oxr_xrPassthroughPauseFB(XrPassthroughFB passthrough)
{
	OXR_TRACE_MARKER();
	struct oxr_passthrough *pt;
	struct oxr_logger log;
	OXR_VERIFY_PASSTHROUGH_AND_INIT_LOG(&log, passthrough, pt, "oxr_xrPassthroughPauseFB");
	pt->paused = true;
	return XR_SUCCESS;
}

XrResult
oxr_xrPassthroughStartFB(XrPassthroughFB passthrough)
{
	OXR_TRACE_MARKER();
	struct oxr_passthrough *pt;
	struct oxr_logger log;
	OXR_VERIFY_PASSTHROUGH_AND_INIT_LOG(&log, passthrough, pt, "oxr_xrPassthroughStartFB");
	pt->paused = false;
	return XR_SUCCESS;
}
