// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Session entrypoints for the OpenXR state tracker.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_api
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math/m_space.h"
#include "xrt/xrt_compiler.h"

#include "util/u_debug.h"
#include "util/u_trace_marker.h"

#include "oxr_frame_sync.h"
#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_two_call.h"

#include "oxr_api_funcs.h"
#include "oxr_api_verify.h"
#include "oxr_handle_base.h"
#include "oxr_chain.h"
#include "oxr_roles.h"


XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateSession(XrInstance instance, const XrSessionCreateInfo *createInfo, XrSession *out_session)
{
	OXR_TRACE_MARKER();

	XrResult ret;
	struct oxr_instance *inst;
	struct oxr_session *sess;
	struct oxr_session **link;
	struct oxr_logger log;
	OXR_VERIFY_INSTANCE_AND_INIT_LOG(&log, instance, inst, "xrCreateSession");

	ret = oxr_verify_XrSessionCreateInfo(&log, inst, createInfo);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	ret = oxr_session_create(&log, &inst->system, createInfo, &sess);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	*out_session = oxr_session_to_openxr(sess);

	/* Add to session list */
	os_mutex_lock(&inst->sessions_mutex);
	link = &inst->sessions_locked;
	while (*link) {
		link = &(*link)->next;
	}
	*link = sess;
	os_mutex_unlock(&inst->sessions_mutex);

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroySession(XrSession session)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_session **link;
	struct oxr_instance *inst;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrDestroySession");

	/*
	 * Remove from session list, holding the lock also waits for any
	 * in-flight xrPollEvent to stop using this session.
	 */
	inst = sess->sys->inst;
	os_mutex_lock(&inst->sessions_mutex);
	link = &inst->sessions_locked;
	while (*link != sess) {
		link = &(*link)->next;
	}
	*link = sess->next;
	os_mutex_unlock(&inst->sessions_mutex);

	return oxr_handle_parent_destroy(&log, &sess->handle);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrBeginSession(XrSession session, const XrSessionBeginInfo *beginInfo)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrBeginSession");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, beginInfo, XR_TYPE_SESSION_BEGIN_INFO);

	// in a headless session there is no compositor and primaryViewConfigurationType must be ignored
	if (sess->compositor != NULL) {
		OXR_VERIFY_VIEW_CONFIG_TYPE(&log, sess->sys->inst, beginInfo->primaryViewConfigurationType);
		OXR_VERIFY_VIEW_CONFIG_TYPE_SUPPORTED(&log, sess->sys, beginInfo->primaryViewConfigurationType);
	}

	// Going to effectively double check this, but this gives us an early out.
	if (oxr_frame_sync_is_session_running(&sess->frame_sync)) {
		return oxr_error(&log, XR_ERROR_SESSION_RUNNING, "Session is already running");
	}

	return oxr_session_begin(&log, sess, beginInfo);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrEndSession(XrSession session)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrEndSession");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_SESSION_RUNNING(&log, sess);

	return oxr_session_end(&log, sess);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrWaitFrame(XrSession session, const XrFrameWaitInfo *frameWaitInfo, XrFrameState *frameState)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrWaitFrame");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_SESSION_RUNNING(&log, sess);
	OXR_VERIFY_ARG_TYPE_CAN_BE_NULL(&log, frameWaitInfo, XR_TYPE_FRAME_WAIT_INFO);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, frameState, XR_TYPE_FRAME_STATE);
	OXR_VERIFY_ARG_NOT_NULL(&log, frameState);

	return oxr_session_frame_wait(&log, sess, frameState);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrBeginFrame(XrSession session, const XrFrameBeginInfo *frameBeginInfo)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrBeginFrame");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_SESSION_RUNNING(&log, sess);
	// NULL explicitly allowed here because it's a basically empty struct.
	OXR_VERIFY_ARG_TYPE_CAN_BE_NULL(&log, frameBeginInfo, XR_TYPE_FRAME_BEGIN_INFO);

	XrResult res = oxr_session_frame_begin(&log, sess);

#ifdef XRT_FEATURE_RENDERDOC
	if (sess->sys->inst->rdoc_api) {
#ifndef XR_USE_PLATFORM_ANDROID
		sess->sys->inst->rdoc_api->StartFrameCapture(NULL, NULL);
#endif
	}
#endif

	return res;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrEndFrame(XrSession session, const XrFrameEndInfo *frameEndInfo)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrEndFrame");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_SESSION_RUNNING(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, frameEndInfo, XR_TYPE_FRAME_END_INFO);

	// Get from compositor.
	struct xrt_system_compositor_info *info = sess->sys->xsysc ? &sess->sys->xsysc->info : NULL;

	// headless extension does not modify the layer count
	uint32_t max_layers = XRT_MAX_LAYERS;
	if (info) {
		max_layers = info->max_layers;
	}

	if (frameEndInfo->layerCount > max_layers) {
		return oxr_error(&log, XR_ERROR_LAYER_LIMIT_EXCEEDED, "(layerCount == %u) exceeds limit %u",
		                 frameEndInfo->layerCount, max_layers);
	}


#ifdef XRT_FEATURE_RENDERDOC
	if (sess->sys->inst->rdoc_api) {
#ifndef XR_USE_PLATFORM_ANDROID
		sess->sys->inst->rdoc_api->EndFrameCapture(NULL, NULL);
#endif
	}
#endif

	XrResult res = oxr_session_frame_end(&log, sess, frameEndInfo);

	return res;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrRequestExitSession(XrSession session)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrRequestExitSession");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_SESSION_RUNNING(&log, sess);

	return oxr_session_request_exit(&log, sess);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrLocateViews(XrSession session,
                  const XrViewLocateInfo *viewLocateInfo,
                  XrViewState *viewState,
                  uint32_t viewCapacityInput,
                  uint32_t *viewCountOutput,
                  XrView *views)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_space *spc;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrLocateViews");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, viewLocateInfo, XR_TYPE_VIEW_LOCATE_INFO);
	OXR_VERIFY_SPACE_NOT_NULL(&log, viewLocateInfo->space, spc);
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, viewState, XR_TYPE_VIEW_STATE);
	OXR_VERIFY_VIEW_CONFIG_TYPE(&log, sess->sys->inst, viewLocateInfo->viewConfigurationType);
	OXR_VERIFY_VIEW_CONFIG_TYPE_SUPPORTED(&log, sess->sys, viewLocateInfo->viewConfigurationType);

	if (viewLocateInfo->viewConfigurationType != sess->current_view_config_type) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE,
		                 "(viewLocateInfo->viewConfigurationType == 0x%" PRIx32
		                 ") is not view configuration passed into xrBeginSession(0x%" PRIx32 ")",
		                 viewLocateInfo->viewConfigurationType, sess->current_view_config_type);
	}

	if (viewCapacityInput == 0) {
		OXR_VERIFY_ARG_NOT_NULL(&log, viewCountOutput);
	} else {
		OXR_VERIFY_ARG_NOT_NULL(&log, views);
	}

	for (uint32_t i = 0; i < viewCapacityInput; i++) {
		OXR_VERIFY_ARG_ARRAY_ELEMENT_TYPE(&log, views, i, XR_TYPE_VIEW);
	}

	if (viewLocateInfo->displayTime <= (XrTime)0) {
		return oxr_error(&log, XR_ERROR_TIME_INVALID, "(time == %" PRIi64 ") is not a valid time.",
		                 viewLocateInfo->displayTime);
	}

	return oxr_session_locate_views( //
	    &log,                        //
	    sess,                        //
	    viewLocateInfo,              //
	    viewState,                   //
	    viewCapacityInput,           //
	    viewCountOutput,             //
	    views);                      //
}


/*
 *
 * XR_KHR_visibility_mask
 *
 */

#ifdef OXR_HAVE_KHR_visibility_mask
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetVisibilityMaskKHR(XrSession session,
                           XrViewConfigurationType viewConfigurationType,
                           uint32_t viewIndex,
                           XrVisibilityMaskTypeKHR visibilityMaskType,
                           XrVisibilityMaskKHR *visibilityMask)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess = NULL;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrGetVisibilityMaskKHR");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);

	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, KHR_visibility_mask);

	visibilityMask->vertexCountOutput = 0;
	visibilityMask->indexCountOutput = 0;

	OXR_VERIFY_VIEW_CONFIG_TYPE(&log, sess->sys->inst, viewConfigurationType);
	OXR_VERIFY_VIEW_CONFIG_TYPE_SUPPORTED(&log, sess->sys, viewConfigurationType);

	OXR_VERIFY_VIEW_INDEX(&log, viewIndex);

	if (visibilityMaskType != XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR &&
	    visibilityMaskType != XR_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH_KHR &&
	    visibilityMaskType != XR_VISIBILITY_MASK_TYPE_LINE_LOOP_KHR) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE, "(visibilityMaskType == %d) is invalid",
		                 visibilityMaskType);
	}

	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, visibilityMask, XR_TYPE_VISIBILITY_MASK_KHR);

	if (visibilityMask->vertexCapacityInput != 0) {
		OXR_VERIFY_ARG_NOT_NULL(&log, visibilityMask->vertices);
	}

	if (visibilityMask->indexCapacityInput != 0) {
		OXR_VERIFY_ARG_NOT_NULL(&log, visibilityMask->indices);
	}

	return oxr_session_get_visibility_mask(&log, sess, visibilityMaskType, viewIndex, visibilityMask);
}
#endif // OXR_HAVE_KHR_visibility_mask


/*
 *
 * XR_EXT_performance_settings
 *
 */

#ifdef OXR_HAVE_EXT_performance_settings
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrPerfSettingsSetPerformanceLevelEXT(XrSession session,
                                         XrPerfSettingsDomainEXT domain,
                                         XrPerfSettingsLevelEXT level)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrPerfSettingsSetPerformanceLevelEXT");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);
	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, EXT_performance_settings);
	// check parameters
	if (domain != XR_PERF_SETTINGS_DOMAIN_CPU_EXT && domain != XR_PERF_SETTINGS_DOMAIN_GPU_EXT) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE, "Invalid domain %d, must be 1(CPU) or 2(GPU)",
		                 domain);
	}

	if (level != XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT && level != XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT &&
	    level != XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT && level != XR_PERF_SETTINGS_LEVEL_BOOST_EXT) {
		return oxr_error(
		    &log, XR_ERROR_VALIDATION_FAILURE,
		    "Invalid level %d, must be 0(POWER SAVE), 25(SUSTAINED LOW), 50(SUSTAINED_HIGH) or 75(BOOST)",
		    level);
	}

	return oxr_session_set_perf_level(&log, sess, domain, level);
}
#endif // OXR_HAVE_EXT_performance_settings


/*
 *
 * XR_EXT_thermal_query
 *
 */

#ifdef XR_EXT_thermal_query

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrThermalGetTemperatureTrendEXT(XrSession session,
                                    XrPerfSettingsDomainEXT domain,
                                    XrPerfSettingsNotificationLevelEXT *notificationLevel,
                                    float *tempHeadroom,
                                    float *tempSlope)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrThermalGetTemperatureTrendEXT");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);

	return oxr_error(&log, XR_ERROR_HANDLE_INVALID, "Not implemented");
}

#endif


/*
 *
 * XR_FB_display_refresh_rate
 *
 */

#ifdef OXR_HAVE_FB_display_refresh_rate

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrEnumerateDisplayRefreshRatesFB(XrSession session,
                                     uint32_t displayRefreshRateCapacityInput,
                                     uint32_t *displayRefreshRateCountOutput,
                                     float *displayRefreshRates)
{
	struct oxr_session *sess = NULL;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrEnumerateDisplayRefreshRatesFB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);

	// headless
	if (!sess->sys->xsysc) {
		*displayRefreshRateCountOutput = 0;
		return XR_SUCCESS;
	}

	OXR_TWO_CALL_HELPER(&log, displayRefreshRateCapacityInput, displayRefreshRateCountOutput, displayRefreshRates,
	                    sess->sys->xsysc->info.refresh_rate_count, sess->sys->xsysc->info.refresh_rates_hz,
	                    XR_SUCCESS);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetDisplayRefreshRateFB(XrSession session, float *displayRefreshRate)
{
	struct oxr_session *sess = NULL;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrGetDisplayRefreshRateFB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);

	// headless
	if (!sess->sys->xsysc) {
		*displayRefreshRate = 0.0f;
		return XR_SUCCESS;
	}

	if (sess->sys->xsysc->info.refresh_rate_count < 1) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return oxr_session_get_display_refresh_rate(&log, sess, displayRefreshRate);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrRequestDisplayRefreshRateFB(XrSession session, float displayRefreshRate)
{
	struct oxr_session *sess = NULL;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrRequestDisplayRefreshRateFB");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);

	if (displayRefreshRate == 0.0f) {
		return XR_SUCCESS;
	}

	/*
	 * For the requested display refresh rate, truncating to two decimal
	 * places and checking if it's in the supported refresh rates.
	 */
	bool found = false;
	for (int i = 0; i < (int)sess->sys->xsysc->info.refresh_rate_count; ++i) {
		if ((int)(displayRefreshRate * 100.0f) == (int)(sess->sys->xsysc->info.refresh_rates_hz[i] * 100.0f)) {
			found = true;
			break;
		}
	}
	if (!found) {
		return XR_ERROR_DISPLAY_REFRESH_RATE_UNSUPPORTED_FB;
	}

	return oxr_session_request_display_refresh_rate(&log, sess, displayRefreshRate);
}

#endif

/*
 *
 * XR_KHR_android_thread_settings
 *
 */

#ifdef OXR_HAVE_KHR_android_thread_settings

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrSetAndroidApplicationThreadKHR(XrSession session, XrAndroidThreadTypeKHR threadType, uint32_t threadId)
{
	OXR_TRACE_MARKER();

	struct oxr_session *sess = NULL;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrSetAndroidApplicationThreadKHR");
	OXR_VERIFY_SESSION_NOT_LOST(&log, sess);

	if (threadType != XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR &&
	    threadType != XR_ANDROID_THREAD_TYPE_APPLICATION_WORKER_KHR &&
	    threadType != XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR &&
	    threadType != XR_ANDROID_THREAD_TYPE_RENDERER_WORKER_KHR) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE, "(threadType == %d) is invalid", threadType);
	}

	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, KHR_android_thread_settings);

	return oxr_session_android_thread_settings(&log, sess, threadType, threadId);
}

#endif

#ifdef OXR_HAVE_EXT_plane_detection

static XrResult
oxr_plane_detector_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_plane_detector_ext *pd = (struct oxr_plane_detector_ext *)hb;

	free(pd->xr_locations);

	if (pd->detection_id > 0) {
		enum xrt_result xret = xrt_device_destroy_plane_detection_ext(pd->xdev, pd->detection_id);
		if (xret != XRT_SUCCESS) {
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
			                 "Internal error in xrDestroyPlaneDetectorEXT: %d", xret);
		}
	}

	xrt_plane_detections_ext_clear(&pd->detections);

	free(pd);

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreatePlaneDetectorEXT(XrSession session,
                             const XrPlaneDetectorCreateInfoEXT *createInfo,
                             XrPlaneDetectorEXT *planeDetector)
{
	struct oxr_session *sess;
	struct oxr_logger log;
	OXR_VERIFY_SESSION_AND_INIT_LOG(&log, session, sess, "xrCreatePlaneDetectorEXT");
	OXR_VERIFY_ARG_TYPE_AND_NOT_NULL(&log, createInfo, XR_TYPE_PLANE_DETECTOR_CREATE_INFO_EXT);
	OXR_VERIFY_EXTENSION(&log, sess->sys->inst, EXT_plane_detection);

	//! @todo support planes on other devices
	struct xrt_device *xdev = GET_STATIC_XDEV_BY_ROLE(sess->sys, head);
	if (!xdev->supported.planes) {
		return XR_ERROR_FEATURE_UNSUPPORTED;
	}

	if (createInfo->flags != 0 && createInfo->flags != XR_PLANE_DETECTOR_ENABLE_CONTOUR_BIT_EXT) {
		//! @todo: Disabled to allow Monado forks with internal extensions to have more values.
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE, "Invalid plane detector creation flags: %lx",
		                 createInfo->flags);
	}

	struct oxr_plane_detector_ext *out_pd = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(&log, out_pd, OXR_XR_DEBUG_PLANEDET, oxr_plane_detector_destroy_cb,
	                              &sess->handle);

	out_pd->sess = sess;
	if ((createInfo->flags & XR_PLANE_DETECTOR_ENABLE_CONTOUR_BIT_EXT) != 0) {
		out_pd->flags |= XRT_PLANE_DETECTOR_FLAGS_CONTOUR_EXT;
	}

	out_pd->xdev = xdev;

	// no plane detection started on creation
	out_pd->state = XR_PLANE_DETECTION_STATE_NONE_EXT;

	out_pd->detection_id = 0;

	out_pd->xr_locations = NULL;

	*planeDetector = oxr_plane_detector_to_openxr(out_pd);

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyPlaneDetectorEXT(XrPlaneDetectorEXT planeDetector)
{
	struct oxr_logger log;
	struct oxr_plane_detector_ext *pd;

	OXR_VERIFY_PLANE_DETECTOR_AND_INIT_LOG(&log, planeDetector, pd, "xrDestroyPlaneDetectorEXT");

	return oxr_handle_destroy(&log, &pd->handle);
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrBeginPlaneDetectionEXT(XrPlaneDetectorEXT planeDetector, const XrPlaneDetectorBeginInfoEXT *beginInfo)
{
	struct oxr_logger log;
	struct oxr_plane_detector_ext *pd;
	struct oxr_space *spc;

	OXR_VERIFY_PLANE_DETECTOR_AND_INIT_LOG(&log, planeDetector, pd, "xrBeginPlaneDetectionEXT");
	OXR_VERIFY_SPACE_NOT_NULL(&log, beginInfo->baseSpace, spc);
	OXR_VERIFY_ARG_NOT_ZERO(&log, beginInfo->maxPlanes);
	OXR_VERIFY_POSE(&log, beginInfo->boundingBoxPose);
	if (beginInfo->time < 1) {
		return oxr_error(&log, XR_ERROR_TIME_INVALID, "Time %" PRId64 " invalid", beginInfo->time);
	}

	if (!pd->xdev->supported.planes) {
		return XR_ERROR_FEATURE_UNSUPPORTED;
	}

	if (beginInfo->orientationCount > 0) {
		OXR_VERIFY_ARG_NOT_NULL(&log, beginInfo->orientations);
	}


	// BoundingBox, pose is relative to baseSpace spc
	struct xrt_pose T_base_bb = {
	    .orientation =
	        {
	            .x = beginInfo->boundingBoxPose.orientation.x,
	            .y = beginInfo->boundingBoxPose.orientation.y,
	            .z = beginInfo->boundingBoxPose.orientation.z,
	            .w = beginInfo->boundingBoxPose.orientation.w,
	        },
	    .position =
	        {
	            .x = beginInfo->boundingBoxPose.position.x,
	            .y = beginInfo->boundingBoxPose.position.y,
	            .z = beginInfo->boundingBoxPose.position.z,
	        },
	};

	// Get plane tracker xdev relation in bounding box baseSpc too. The inverse of this relation is the transform
	// from baseSpace spc to xdev space and can transform bounding box pose into xdev space.
	struct xrt_space_relation T_base_xdev; // What we get, xdev in base space.
	XrResult ret = oxr_space_locate_device(&log, pd->xdev, spc, beginInfo->time, &T_base_xdev);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	if (T_base_xdev.relation_flags == 0) {
		return XR_ERROR_SPACE_NOT_LOCATABLE_EXT;
	}

	struct xrt_space_relation T_xdev_bb; // This is what we want, BoundingBox in xdev space.
	struct xrt_relation_chain xrc = {0};
	m_relation_chain_push_pose_if_not_identity(&xrc, &T_base_bb); // T_base_bb
	m_relation_chain_push_inverted_relation(&xrc, &T_base_xdev);  // T_xdev_base
	m_relation_chain_resolve(&xrc, &T_xdev_bb);

	if (T_xdev_bb.relation_flags == 0) {
		return XR_ERROR_SPACE_NOT_LOCATABLE_EXT;
	}

	struct xrt_plane_detector_begin_info_ext query;
	// the plane detector id is the raw handle (in our implementation: pointer to xrt_plane_detector)
	query.detector_flags = pd->flags;

	//! @todo be more graceful
	if (beginInfo->orientationCount > XRT_MAX_PLANE_ORIENTATIONS_EXT) {
		return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE, "Too many plane orientations");
	}

	if (beginInfo->semanticTypeCount > XRT_MAX_PLANE_SEMANTIC_TYPE_EXT) {
		return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE, "Too many plane semantic types");
	}

	query.orientation_count = beginInfo->orientationCount;
	for (uint32_t i = 0; i < beginInfo->orientationCount; i++) {
		// 1:1 mapped
		query.orientations[i] = (enum xrt_plane_detector_orientation_ext)beginInfo->orientations[i];
	}

	query.semantic_type_count = beginInfo->semanticTypeCount;
	for (uint32_t i = 0; i < beginInfo->semanticTypeCount; i++) {
		// 1:1 mapped
		query.semantic_types[i] = (enum xrt_plane_detector_semantic_type_ext)beginInfo->semanticTypes[i];
	}

	query.max_planes = beginInfo->maxPlanes;
	query.min_area = beginInfo->minArea;

	// extents are invariant under pose transforms
	query.bounding_box_extent.x = beginInfo->boundingBoxExtent.width;
	query.bounding_box_extent.y = beginInfo->boundingBoxExtent.height;
	query.bounding_box_extent.z = beginInfo->boundingBoxExtent.depth;

	query.bounding_box_pose = T_xdev_bb.pose;


	enum xrt_result xret;

	// The xrt backend tracks plane detections to be able to clean up when the client dies.
	// Because it tracks them as standalone objects, it can not know that this plane detection is replacing a
	// previous one. Therefore we need to explicitly destroy the previous detection.
	if (pd->detection_id > 0) {
		xret = xrt_device_destroy_plane_detection_ext(pd->xdev, pd->detection_id);
		if (xret != XRT_SUCCESS) {
			return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE,
			                 "Internal error in xrBeginPlaneDetectionEXT: Failed to destroy previous plane "
			                 "detection: %d",
			                 xret);
		}
	}

	xret = xrt_device_begin_plane_detection_ext(pd->xdev, &query, pd->detection_id, &pd->detection_id);
	if (xret != XRT_SUCCESS) {
		return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE, "Internal error in xrBeginPlaneDetectionEXT: %d",
		                 xret);
	}

	xrt_plane_detections_ext_clear(&pd->detections);

	// This makes sure a call to xrGetPlaneDetectionsEXT won't see a previous state, in particular a previous DONE.
	pd->state = XR_PLANE_DETECTION_STATE_PENDING_EXT;

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetPlaneDetectionStateEXT(XrPlaneDetectorEXT planeDetector, XrPlaneDetectionStateEXT *state)
{
	struct oxr_logger log;
	struct oxr_plane_detector_ext *pd;
	enum xrt_result xret;

	OXR_VERIFY_PLANE_DETECTOR_AND_INIT_LOG(&log, planeDetector, pd, "xrGetPlaneDetectionStateEXT");

	enum xrt_plane_detector_state_ext xstate = 0;

	xret = xrt_device_get_plane_detection_state_ext(pd->xdev, pd->detection_id, &xstate);
	if (xret != XRT_SUCCESS) {
		return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE, "Internal error in xrGetPlaneDetectionStateEXT: %d",
		                 xret);
	}

	*state = (XrPlaneDetectionStateEXT)xstate; // 1:1 mapped

	pd->state = *state;

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetPlaneDetectionsEXT(XrPlaneDetectorEXT planeDetector,
                            const XrPlaneDetectorGetInfoEXT *info,
                            XrPlaneDetectorLocationsEXT *locations)
{
	struct oxr_logger log;
	struct oxr_plane_detector_ext *pd;
	struct oxr_space *spc;
	enum xrt_result xret;

	OXR_VERIFY_PLANE_DETECTOR_AND_INIT_LOG(&log, planeDetector, pd, "xrGetPlaneDetectionsEXT");
	OXR_VERIFY_SPACE_NOT_NULL(&log, info->baseSpace, spc);
	if (info->time < 1) {
		return oxr_error(&log, XR_ERROR_TIME_INVALID, "Time %" PRId64 " invalid", info->time);
	}

	if (!pd->xdev->supported.planes) {
		return XR_ERROR_FEATURE_UNSUPPORTED;
	}

	if (pd->state != XR_PLANE_DETECTION_STATE_DONE_EXT) {
		locations->planeLocationCountOutput = 0;
		return XR_ERROR_CALL_ORDER_INVALID;
	}

	xret = xrt_device_get_plane_detections_ext(pd->xdev, pd->detection_id, &pd->detections);
	if (xret != XRT_SUCCESS) {
		return oxr_error(&log, XR_ERROR_RUNTIME_FAILURE, "Internal error in xrGetPlaneDetectionsEXT: %d", xret);
	}

	struct xrt_space_relation T_base_xdev; // What we get, xdev in base space.
	XrResult ret = oxr_space_locate_device(&log, pd->xdev, spc, info->time, &T_base_xdev);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	if (T_base_xdev.relation_flags == 0) {
		return XR_ERROR_SPACE_NOT_LOCATABLE_EXT;
	}

	// create dynamic array for two call idiom
	U_ARRAY_REALLOC_OR_FREE(pd->xr_locations, XrPlaneDetectorLocationEXT, pd->detections.location_count);
	if (pd->detections.location_count == 0) {
		pd->xr_locations = NULL;
	}

	// populate pd->xr_locations from pd->detections.locations, also transform plane poses into baseSpace.
	for (uint32_t i = 0; i < pd->detections.location_count; i++) {
		pd->xr_locations[i].planeId = pd->detections.locations[i].planeId;
		pd->xr_locations[i].extents.width = pd->detections.locations[i].extents.x;
		pd->xr_locations[i].extents.height = pd->detections.locations[i].extents.y;
		// 1:1 mapped
		pd->xr_locations[i].orientation =
		    (XrPlaneDetectorOrientationEXT)pd->detections.locations[i].orientation;
		pd->xr_locations[i].semanticType =
		    (XrPlaneDetectorSemanticTypeEXT)pd->detections.locations[i].semantic_type;
		pd->xr_locations[i].polygonBufferCount = pd->detections.locations[i].polygon_buffer_count;


		// The plane poses are returned in the xdev's space.
		struct xrt_space_relation T_xdev_plane = pd->detections.locations[i].relation;

		// Get the plane pose in the base space.
		struct xrt_space_relation T_base_plane;
		struct xrt_relation_chain xrc = {0};
		m_relation_chain_push_relation(&xrc, &T_xdev_plane);
		m_relation_chain_push_relation(&xrc, &T_base_xdev);
		m_relation_chain_resolve(&xrc, &T_base_plane);

		OXR_XRT_POSE_TO_XRPOSEF(T_base_plane.pose, pd->xr_locations[i].pose);

		pd->xr_locations[i].locationFlags = 0;
		if ((T_base_plane.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0) {
			pd->xr_locations[i].locationFlags |= XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
		}
		if ((T_base_plane.relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) != 0) {
			pd->xr_locations[i].locationFlags |= XR_SPACE_LOCATION_POSITION_VALID_BIT;
		}
		if ((T_base_plane.relation_flags & XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT) != 0) {
			pd->xr_locations[i].locationFlags |= XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
		}
		if ((T_base_plane.relation_flags & XRT_SPACE_RELATION_POSITION_TRACKED_BIT) != 0) {
			pd->xr_locations[i].locationFlags |= XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
		}
	}


	OXR_TWO_CALL_HELPER(&log, locations->planeLocationCapacityInput, &locations->planeLocationCountOutput,
	                    locations->planeLocations, pd->detections.location_count, pd->xr_locations, XR_SUCCESS);

	return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetPlanePolygonBufferEXT(XrPlaneDetectorEXT planeDetector,
                               uint64_t planeId,
                               uint32_t polygonBufferIndex,
                               XrPlaneDetectorPolygonBufferEXT *polygonBuffer)
{
	struct oxr_logger log;
	struct oxr_plane_detector_ext *pd;
	OXR_VERIFY_PLANE_DETECTOR_AND_INIT_LOG(&log, planeDetector, pd, "xrGetPlanePolygonBufferEXT");

	//! @todo can't reasonably retrieve a polygon without having retrieved plane data first.
	if (pd->state != XR_PLANE_DETECTION_STATE_DONE_EXT) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE,
		                 "xrGetPlanePolygonBufferEXT called but plane detector state is %d", pd->state);
	}

	// find the index of the plane in both pd->locations and pd->polygons_ptr arrays.
	uint32_t plane_index = UINT32_MAX;
	for (uint32_t i = 0; i < pd->detections.location_count; i++) {
		if (pd->detections.locations[i].planeId == planeId) {
			plane_index = i;
			break;
		}
	}

	if (plane_index == UINT32_MAX) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE, "Invalid plane id %" PRId64, planeId);
	}

	if (polygonBufferIndex + 1 > pd->detections.locations[plane_index].polygon_buffer_count) {
		return oxr_error(&log, XR_ERROR_VALIDATION_FAILURE, "Invalid polygon buffer index %u (> %u)",
		                 polygonBufferIndex, pd->detections.locations[plane_index].polygon_buffer_count - 1);
	}

	uint32_t polygons_start_index = pd->detections.polygon_info_start_index[plane_index];
	uint32_t polygon_index = polygons_start_index + polygonBufferIndex;

	uint32_t vertices_start_index = pd->detections.polygon_infos[polygon_index].vertices_start_index;
	// xrt_vec2 should mapped 1:1 to XrVector2f
	XrVector2f *polygon_vertices = (XrVector2f *)&pd->detections.vertices[vertices_start_index];

	uint32_t vertex_count = pd->detections.polygon_infos[polygon_index].vertex_count;

	OXR_TWO_CALL_HELPER(&log, polygonBuffer->vertexCapacityInput, &polygonBuffer->vertexCountOutput,
	                    polygonBuffer->vertices, vertex_count, polygon_vertices, XR_SUCCESS);
}

#endif // OXR_HAVE_EXT_plane_detection
