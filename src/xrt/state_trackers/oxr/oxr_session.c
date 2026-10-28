// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds session related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include "oxr_frame_sync.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_session.h"
#include "xrt/xrt_config_build.h" // IWYU pragma: keep
#include "xrt/xrt_config_have.h"  // IWYU pragma: keep

#ifdef XR_USE_PLATFORM_XLIB
#include "xrt/xrt_gfx_xlib.h" // IWYU pragma: keep

#endif // XR_USE_PLATFORM_XLIB

#ifdef XRT_HAVE_VULKAN
#include "xrt/xrt_gfx_vk.h" // IWYU pragma: keep

#endif // XRT_HAVE_VULKAN

#include "os/os_time.h"

#include "util/u_debug.h"
#include "util/u_misc.h"
#include "util/u_time.h"
#include "util/u_visibility_mask.h"
#include "util/u_verify.h"

#include "math/m_api.h"
#include "math/m_mathinclude.h"
#include "math/m_space.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_two_call.h"
#include "oxr_handle_base.h"
#include "oxr_chain.h"
#include "oxr_api_verify.h"
#include "oxr_pretty_print.h"
#include "oxr_conversions.h"
#include "oxr_xret.h"
#include "oxr_roles.h"

#include "actions/oxr_input.h"
#include "actions/oxr_session_action_context.h"
#include "actions/oxr_session_attached_actions.h"
#include "actions/oxr_binding.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


DEBUG_GET_ONCE_NUM_OPTION(ipd, "OXR_DEBUG_IPD_MM", 63)
DEBUG_GET_ONCE_NUM_OPTION(wait_frame_sleep, "OXR_DEBUG_WAIT_FRAME_EXTRA_SLEEP_MS", 0)
DEBUG_GET_ONCE_BOOL_OPTION(frame_timing_spew, "OXR_FRAME_TIMING_SPEW", false)


/*
 *
 * Helpers.
 *
 */

static bool
should_skip_format_vk_1_and_2(const struct oxr_instance *inst, uint64_t format)
{
	bool skip_depth_stencil = inst->quirks.disable_vulkan_format_depth_stencil;
	/*!
	 * WARNING: Stencil-only formats have not been thoroughly tested.
	 *
	 * As of writing, when enabled using the compute-pipeline path with
	 * CTS 1.1.53 on Linux with an NVIDIA GPU (driver v580.105.8.0):
	 *
	 * The CTS swapchain creation tests attempt to create a swapchain
	 * with VK_FORMAT_S8_UINT format. The test fails because native
	 * swapchain creation returns VK_ERROR_DEVICE_LOST from
	 * `do_post_create_vulkan_setup()` when waiting on a fence after
	 * submitting commands that perform image layout barrier transitions
	 * on a compute-only queue.
	 *
	 * This is most likely a driver bug with stencil image barriers on
	 * compute queues. Since no OpenXR core or extensions currently
	 * utilize stencil-only swapchains, they are not required to be
	 * supported.
	 */
	bool skip_stencil = true;
	bool skip_depth = inst->quirks.disable_vulkan_format_depth;

	// Access to Vulkan headers are not guaranteed.
	switch (format) {
	case 124: /* VK_FORMAT_D16_UNORM          */ return skip_depth;
	case 125: /* VK_FORMAT_X8_D24_UNORM_PACK32*/ return skip_depth;
	case 126: /* VK_FORMAT_D32_SFLOAT         */ return skip_depth;
	case 127: /* VK_FORMAT_S8_UINT            */ return skip_stencil;
	case 129: /* VK_FORMAT_D24_UNORM_S8_UINT  */ return skip_depth_stencil;
	case 130: /* VK_FORMAT_D32_SFLOAT_S8_UINT */ return skip_depth_stencil;
	default: return false;
	}
}

static bool
should_skip_format(const struct oxr_instance *inst, const struct oxr_session *sess, uint64_t format)
{
	if (sess->gfx_ext == OXR_SESSION_GRAPHICS_EXT_VULKAN) {
		/*
		 * Hello future computer whisperer, if we split the graphics
		 * extension enum into Vulkan1 and Vulkan2, make sure we call this
		 * function for both, kthx.
		 */
		return should_skip_format_vk_1_and_2(inst, format);
	} else {
		return false;
	}
}

static bool
should_render(XrSessionState state)
{
	switch (state) {
	case XR_SESSION_STATE_VISIBLE:
	case XR_SESSION_STATE_FOCUSED:
	case XR_SESSION_STATE_STOPPING: return true;
	default: return false;
	}
}

XRT_MAYBE_UNUSED static const char *
to_string(XrSessionState state)
{
	switch (state) {
	case XR_SESSION_STATE_UNKNOWN: return "XR_SESSION_STATE_UNKNOWN";
	case XR_SESSION_STATE_IDLE: return "XR_SESSION_STATE_IDLE";
	case XR_SESSION_STATE_READY: return "XR_SESSION_STATE_READY";
	case XR_SESSION_STATE_SYNCHRONIZED: return "XR_SESSION_STATE_SYNCHRONIZED";
	case XR_SESSION_STATE_VISIBLE: return "XR_SESSION_STATE_VISIBLE";
	case XR_SESSION_STATE_FOCUSED: return "XR_SESSION_STATE_FOCUSED";
	case XR_SESSION_STATE_STOPPING: return "XR_SESSION_STATE_STOPPING";
	case XR_SESSION_STATE_LOSS_PENDING: return "XR_SESSION_STATE_LOSS_PENDING";
	case XR_SESSION_STATE_EXITING: return "XR_SESSION_STATE_EXITING";
	case XR_SESSION_STATE_MAX_ENUM: return "XR_SESSION_STATE_MAX_ENUM";
	default: return "";
	}
}

static XrResult
emit_reference_space_change_pending(struct oxr_logger *log,
                                    struct oxr_session *sess,
                                    struct xrt_session_event_reference_space_change_pending *ref_change,
                                    XrReferenceSpaceType type)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrTime changeTime = time_state_monotonic_to_ts_ns(inst->timekeeping, ref_change->timestamp_ns);
	const XrPosef *poseInPreviousSpace = (XrPosef *)&ref_change->pose_in_previous_space;
	bool poseValid = ref_change->pose_valid;

	//! @todo properly handle return (not done yet because requires larger rewrite),
	oxr_event_push_XrEventDataReferenceSpaceChangePending( //
	    log,                                               // log
	    sess,                                              // sess
	    type,                                              // referenceSpaceType
	    changeTime,                                        // changeTime
	    poseValid,                                         // poseValid
	    poseInPreviousSpace);                              // poseInPreviousSpace

	return XR_SUCCESS;
}

static XrResult
handle_reference_space_change_pending(struct oxr_logger *log,
                                      struct oxr_session *sess,
                                      struct xrt_session_event_reference_space_change_pending *ref_change)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrReferenceSpaceType type = XR_REFERENCE_SPACE_TYPE_MAX_ENUM;

	if (inst->quirks.map_stage_to_local_floor) {
		/* When stage is mapped to local_floor:
		 * ignore stage changes
		 * for local_floor changes, send a duplicate event for stage
		 * */
		switch (ref_change->ref_type) {
		case XRT_SPACE_REFERENCE_TYPE_STAGE: return XR_SUCCESS;
		case XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR:
			emit_reference_space_change_pending(log, sess, ref_change, XR_REFERENCE_SPACE_TYPE_STAGE);
			break;
		default: break;
		}
	}

	switch (ref_change->ref_type) {
	case XRT_SPACE_REFERENCE_TYPE_VIEW: type = XR_REFERENCE_SPACE_TYPE_VIEW; break;
	case XRT_SPACE_REFERENCE_TYPE_LOCAL: type = XR_REFERENCE_SPACE_TYPE_LOCAL; break;
	case XRT_SPACE_REFERENCE_TYPE_STAGE: type = XR_REFERENCE_SPACE_TYPE_STAGE; break;
	case XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR:
#ifdef OXR_HAVE_EXT_local_floor
		if (inst->extensions.EXT_local_floor) {
			type = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
			break;
		} else {
			// Silently ignored, extension not enabled.
			return XR_SUCCESS;
		}
#else
		// Silently ignored, not compiled with this extension supported.
		return XR_SUCCESS;
#endif
	case XRT_SPACE_REFERENCE_TYPE_UNBOUNDED:
#ifdef OXR_HAVE_MSFT_unbounded_reference_space
		if (inst->extensions.MSFT_unbounded_reference_space) {
			type = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
			break;
		} else {
			// Silently ignored, extension not enabled.
			return XR_SUCCESS;
		}
#else
		// Silently ignored, not compiled with this extension supported.
		return XR_SUCCESS;
#endif
	}

	if (type == XR_REFERENCE_SPACE_TYPE_MAX_ENUM) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "invalid reference space type");
	}

	emit_reference_space_change_pending(log, sess, ref_change, type);

	return XR_SUCCESS;
}


/*
 *
 * Locate views helpers.
 *
 */

struct locate_views_data
{
	struct xrt_fov fovs[XRT_MAX_COMPOSITOR_VIEW_CONFIGS_VIEW_COUNT];
	struct xrt_pose poses[XRT_MAX_COMPOSITOR_VIEW_CONFIGS_VIEW_COUNT];
};

static XrResult
get_device_fovs_and_poses(struct oxr_logger *log,
                          struct oxr_session *sess,
                          struct xrt_device *xdev,
                          int64_t xdisplay_time_ns,
                          uint32_t view_count,
                          struct xrt_space_relation *out_T_xdev_head,
                          struct locate_views_data *out_data)
{
	xrt_result_t xret = XRT_SUCCESS;

	// To be passed down to the devices, some can override this.
	const struct xrt_vec3 default_eye_relation = {
	    sess->ipd_meters,
	    0.0f,
	    0.0f,
	};

	// Regular getting.
	if (view_count == 2 || view_count == 1 || (view_count == 4 && xdev->supported.get_views_quad)) {
		static_assert(ARRAY_SIZE(out_data->fovs) >= 4, "Too small array!");
		static_assert(ARRAY_SIZE(out_data->poses) >= 4, "Too small array!");

		// Determine view type based on view count
		enum xrt_view_type view_type = XRT_VIEW_TYPE_MONO;

		switch (view_count) {
		case 1: view_type = XRT_VIEW_TYPE_MONO; break;
		case 2: view_type = XRT_VIEW_TYPE_STEREO; break;
		case 4: view_type = XRT_VIEW_TYPE_QUAD; break;
		}

		xret = xrt_device_get_view_poses( //
		    xdev,                         //
		    &default_eye_relation,        //
		    xdisplay_time_ns,             //
		    view_type,                    //
		    view_count,                   //
		    out_T_xdev_head,              //
		    out_data->fovs,               //
		    out_data->poses);             //
		OXR_CHECK_XRET(log, sess, xret, xrt_device_get_view_poses);

		return XR_SUCCESS; // Done now.
	}

	// Device supports stereo views but we are simulating quad views.
	if (view_count != 4) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Got not 1, 2 or 4 views!");
	}

	static_assert(ARRAY_SIZE(out_data->fovs) >= 4, "Too small array!");
	static_assert(ARRAY_SIZE(out_data->poses) >= 4, "Too small array!");

	xret = xrt_device_get_view_poses( //
	    xdev,                         //
	    &default_eye_relation,        //
	    xdisplay_time_ns,             //
	    XRT_VIEW_TYPE_STEREO,         // view_type (hardcoded to stereo for this quad view case)
	    2,                            // view_count
	    out_T_xdev_head,              //
	    out_data->fovs,               //
	    out_data->poses);             //
	OXR_CHECK_XRET(log, sess, xret, xrt_device_get_view_poses);

	// Generate inset views (views 2-3) from context views (views 0-1) for quad view configuration.
	// Each inset view uses the same pose as its corresponding context view but with half the FOV.
	for (uint32_t i = 2; i < view_count; i++) {
		const uint32_t src_index = i - 2;
		struct xrt_fov src_fov = out_data->fovs[src_index];
		struct xrt_pose src_pose = out_data->poses[src_index];

		out_data->fovs[i] = (struct xrt_fov){
		    .angle_left = src_fov.angle_left / 2,
		    .angle_right = src_fov.angle_right / 2,
		    .angle_up = src_fov.angle_up / 2,
		    .angle_down = src_fov.angle_down / 2,
		};

		out_data->poses[i] = src_pose;
	}

	return XR_SUCCESS;
}

#ifdef OXR_HAVE_KHR_visibility_mask
static bool
is_emulated_quad_inset_view(struct xrt_device *xdev, uint32_t view_index)
{
	return view_index >= 2 && !xdev->supported.get_views_quad;
}

static enum xrt_visibility_mask_type
convert_mask_type(XrVisibilityMaskTypeKHR type)
{
	switch (type) {
	case XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR: return XRT_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH;
	case XR_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH_KHR: return XRT_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH;
	case XR_VISIBILITY_MASK_TYPE_LINE_LOOP_KHR: return XRT_VISIBILITY_MASK_TYPE_LINE_LOOP;
	default: return (enum xrt_visibility_mask_type)0;
	}
}
#endif // OXR_HAVE_KHR_visibility_mask


/*
 *
 * 'Exported' functions.
 *
 */

void
oxr_session_change_state(struct oxr_logger *log, struct oxr_session *sess, XrSessionState state, XrTime time)
{
	if (sess->state == state) {
		oxr_warn(log,
		         "Session state changed to the same state (%s), not sending XrEventDataSessionStateChanged",
		         to_string(state));
		return;
	}

	oxr_event_push_XrEventDataSessionStateChanged(log, sess, state, time);
	sess->state = state;
}

XrResult
oxr_session_enumerate_formats(struct oxr_logger *log,
                              struct oxr_session *sess,
                              uint32_t formatCapacityInput,
                              uint32_t *formatCountOutput,
                              int64_t *formats)
{
	struct oxr_instance *inst = sess->sys->inst;
	struct xrt_compositor *xc = sess->compositor;
	if (formatCountOutput == NULL) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "(formatCountOutput == NULL) cannot be null");
	}
	if (xc == NULL) {
		if (formatCountOutput != NULL) {
			*formatCountOutput = 0;
		}
		return oxr_session_success_result(sess);
	}

	uint32_t filtered_count = 0;
	int64_t filtered_formats[XRT_MAX_SWAPCHAIN_FORMATS];
	for (uint32_t i = 0; i < xc->info.format_count; i++) {
		int64_t format = xc->info.formats[i];

		if (should_skip_format(inst, sess, format)) {
			continue;
		}

		filtered_formats[filtered_count++] = format;
	}

	OXR_TWO_CALL_HELPER(log, formatCapacityInput, formatCountOutput, formats, filtered_count, filtered_formats,
	                    oxr_session_success_result(sess));
}

#ifdef OXR_HAVE_EXT_enumerate_swapchain_formats2
XrResult
oxr_session_enumerate_formats2(struct oxr_logger *log,
                               struct oxr_session *sess,
                               XrSwapchainFormatsEnumerateInfoEXT *enumerateInfo,
                               uint32_t formatCapacityInput,
                               uint32_t *formatCountOutput,
                               int64_t *formats)
{
	if (enumerateInfo == NULL) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "(enumerateInfo == NULL) cannot be null");
	}
	if (enumerateInfo->type != XR_TYPE_SWAPCHAIN_FORMATS_ENUMERATE_INFO_EXT) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "(enumerateInfo->type) invalid");
	}
	if (enumerateInfo->next != NULL) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "(enumerateInfo->next) must be NULL");
	}

	return oxr_session_enumerate_formats(log, sess, formatCapacityInput, formatCountOutput, formats);
}
#endif

XrResult
oxr_session_begin(struct oxr_logger *log, struct oxr_session *sess, const XrSessionBeginInfo *beginInfo)
{
	/*
	 * If the session is not running when the application calls xrBeginSession, but the session is not yet in the
	 * XR_SESSION_STATE_READY state, the runtime must return error XR_ERROR_SESSION_NOT_READY.
	 */
	if (sess->state != XR_SESSION_STATE_READY) {
		return oxr_error(log, XR_ERROR_SESSION_NOT_READY, "Session is not ready to begin");
	}

	const struct oxr_extension_status *extensions = &sess->sys->inst->extensions;
	struct xrt_compositor *xc = sess->compositor;
	if (xc != NULL) {
		const struct xrt_begin_session_info begin_session_info = {
		    .view_type = (enum xrt_view_type)beginInfo->primaryViewConfigurationType,
#ifdef OXR_HAVE_EXT_hand_tracking
		    .ext_hand_tracking_enabled = extensions->EXT_hand_tracking,
#endif
#ifdef OXR_HAVE_EXT_hand_tracking_data_source
		    .ext_hand_tracking_data_source_enabled = extensions->EXT_hand_tracking_data_source,
#endif
#ifdef OXR_HAVE_EXT_eye_gaze_interaction
		    .ext_eye_gaze_interaction_enabled = extensions->EXT_eye_gaze_interaction,
#endif
#ifdef OXR_HAVE_EXT_future
		    .ext_future_enabled = extensions->EXT_future,
#endif
#ifdef OXR_HAVE_EXT_hand_interaction
		    .ext_hand_interaction_enabled = extensions->EXT_hand_interaction,
#endif
#ifdef OXR_HAVE_HTC_facial_tracking
		    .htc_facial_tracking_enabled = extensions->HTC_facial_tracking,
#endif
#ifdef OXR_HAVE_FB_body_tracking
		    .fb_body_tracking_enabled = extensions->FB_body_tracking,
#endif
#ifdef OXR_HAVE_FB_face_tracking2
		    .fb_face_tracking2_enabled = extensions->FB_face_tracking2,
#endif
#ifdef OXR_HAVE_META_body_tracking_full_body
		    .meta_body_tracking_full_body_enabled = extensions->META_body_tracking_full_body,
#endif
#ifdef OXR_HAVE_META_body_tracking_calibration
		    .meta_body_tracking_calibration_enabled = extensions->META_body_tracking_calibration,
#endif
#ifdef OXR_HAVE_META_body_tracking_fidelity
		    .meta_body_tracking_fidelity_enabled = extensions->META_body_tracking_fidelity,
#endif
#ifdef OXR_HAVE_ANDROID_face_tracking
		    .android_face_tracking_enabled = extensions->ANDROID_face_tracking,
#endif
		};

		xrt_result_t xret = xrt_comp_begin_session(xc, &begin_session_info);
		OXR_CHECK_XRET(log, sess, xret, xrt_comp_begin_session);
	} else {
		// Headless, pretend we got event from the compositor.
		sess->compositor_visible = true;
		sess->compositor_focused = true;

		int64_t now = os_monotonic_get_ns();
		XrTime now_xr = time_state_monotonic_to_ts_ns(sess->sys->inst->timekeeping, now);
		if (now_xr <= 0) {
			// shouldn't happen but be sure to log if it does
			U_LOG_W("Time keeping oddity: XR_SESSION_STATE_SYNCHRONIZED state reached at XrTime %" PRIi64,
			        now_xr);
		}

		// Transition into focused.
		oxr_session_change_state(log, sess, XR_SESSION_STATE_SYNCHRONIZED, now_xr);
		oxr_session_change_state(log, sess, XR_SESSION_STATE_VISIBLE, now_xr);
		oxr_session_change_state(log, sess, XR_SESSION_STATE_FOCUSED, now_xr);
	}
	XrResult ret = oxr_frame_sync_begin_session(&sess->frame_sync);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret,
		                 "Frame sync object refused to let us begin session, probably already running");
	}

#ifdef OXR_HAVE_EXT_user_presence
	struct xrt_device *xdev = GET_STATIC_XDEV_BY_ROLE(sess->sys, head);
	if (extensions->EXT_user_presence && xdev->supported.presence && xdev->inputs) {
		for (size_t i = 0; i < xdev->input_count; ++i) {
			struct xrt_input *input = &xdev->inputs[i];
			if (input->name != XRT_INPUT_GENERIC_HEAD_DETECT) {
				continue;
			}

			// On init we only have the name, so trigger a sync of the inputs.
			xrt_result_t xret = xrt_device_update_inputs(xdev);
			OXR_CHECK_XRET(log, sess, xret, xrt_device_update_inputs);

			oxr_event_push_XrEventDataUserPresenceChangedEXT(log, sess, input->value.boolean);
			break;
		}
	}
#endif

	// Set the current view configuration type, used in xrEndFrame.
	sess->current_view_config_type = beginInfo->primaryViewConfigurationType;

	return oxr_session_success_result(sess);
}

XrResult
oxr_session_end(struct oxr_logger *log, struct oxr_session *sess)
{
	// there is a bug in Unreal 4 where calling this function will result in a crash, so skip it.
	if (sess->sys->inst->quirks.skip_end_session) {
		return XR_SUCCESS;
	}

	/*
	 * If the session is not running when the application calls xrEndSession, the runtime must return
	 * error XR_ERROR_SESSION_NOT_RUNNING
	 */
	if (!oxr_frame_sync_is_session_running(&sess->frame_sync)) {
		return oxr_error(log, XR_ERROR_SESSION_NOT_RUNNING, "Session is not running");
	}

	/*
	 * If the session is still running when the application calls xrEndSession, but the session is not yet in
	 * the XR_SESSION_STATE_STOPPING state, the runtime must return error XR_ERROR_SESSION_NOT_STOPPING.
	 */
	if (sess->state != XR_SESSION_STATE_STOPPING) {
		return oxr_error(log, XR_ERROR_SESSION_NOT_STOPPING, "Session is not stopping");
	}

	struct xrt_compositor *xc = sess->compositor;
	if (xc != NULL) {
		if (sess->frame_id.waited > 0) {
			xrt_comp_discard_frame(xc, sess->frame_id.waited);
			sess->frame_id.waited = -1;
		}
		if (sess->frame_id.begun > 0) {
			xrt_comp_discard_frame(xc, sess->frame_id.begun);
			sess->frame_id.begun = -1;
		}
		sess->frame_started = false;

		xrt_result_t xret = xrt_comp_end_session(xc);
		OXR_CHECK_XRET(log, sess, xret, xrt_comp_end_session);
	} else {
		// Headless, pretend we got event from the compositor.
		sess->compositor_visible = false;
		sess->compositor_focused = false;
	}

	int64_t now = os_monotonic_get_ns();
	XrTime now_xr = time_state_monotonic_to_ts_ns(sess->sys->inst->timekeeping, now);
	if (now_xr <= 0) {
		// shouldn't happen but be sure to log if it does
		U_LOG_W("Time keeping oddity: ending session at XrTime %" PRIi64, now_xr);
	}

	oxr_session_change_state(log, sess, XR_SESSION_STATE_IDLE, now_xr);
	if (sess->exiting) {
		oxr_session_change_state(log, sess, XR_SESSION_STATE_EXITING, now_xr);
	} else {
#ifndef XRT_OS_ANDROID
		// @todo In multi-clients scenario with a session being reused, changing session
		//       state to XR_SESSION_STATE_READY would cause application to call xrBeginSession
		//       immediately and this is not desired. On Android platform, runtime would
		//       change session state to XR_SESSION_STATE_READY once application goes to
		//       foreground again. But on other platform it's not handled yet.
		oxr_session_change_state(log, sess, XR_SESSION_STATE_READY, 0);
#endif // !XRT_OS_ANDROID
	}
	XrResult ret = oxr_frame_sync_end_session(&sess->frame_sync);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Frame sync object refused to let us end session, probably not running");
	}
	sess->has_ended_once = false;

	// Unset the current view configuration type here.
	sess->current_view_config_type = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;

	return oxr_session_success_result(sess);
}

XrResult
oxr_session_request_exit(struct oxr_logger *log, struct oxr_session *sess)
{
	int64_t now = os_monotonic_get_ns();
	XrTime now_xr = time_state_monotonic_to_ts_ns(sess->sys->inst->timekeeping, now);
	if (now_xr <= 0) {
		// shouldn't happen but be sure to log if it does
		U_LOG_W("Time keeping oddity: Requesting exit at XrTime %" PRIi64, now_xr);
	}

	if (sess->state == XR_SESSION_STATE_FOCUSED) {
		oxr_session_change_state(log, sess, XR_SESSION_STATE_VISIBLE, now_xr);
	}
	if (sess->state == XR_SESSION_STATE_VISIBLE) {
		oxr_session_change_state(log, sess, XR_SESSION_STATE_SYNCHRONIZED, now_xr);
	}
	if (!sess->has_ended_once && sess->state != XR_SESSION_STATE_SYNCHRONIZED) {
		oxr_session_change_state(log, sess, XR_SESSION_STATE_SYNCHRONIZED, now_xr);
		// Fake the synchronization.
		sess->has_ended_once = true;
	}

	//! @todo start fading out the app.
	oxr_session_change_state(log, sess, XR_SESSION_STATE_STOPPING, now_xr);
	sess->exiting = true;
	return oxr_session_success_result(sess);
}

#ifdef OXR_HAVE_FB_passthrough
static inline XrPassthroughStateChangedFlagsFB
xrt_to_passthrough_state_flags(enum xrt_passthrough_state state)
{
	XrPassthroughStateChangedFlagsFB res = 0;
	if (state & XRT_PASSTHROUGH_STATE_CHANGED_REINIT_REQUIRED_BIT) {
		res |= XR_PASSTHROUGH_STATE_CHANGED_REINIT_REQUIRED_BIT_FB;
	}
	if (state & XRT_PASSTHROUGH_STATE_CHANGED_NON_RECOVERABLE_ERROR_BIT) {
		res |= XR_PASSTHROUGH_STATE_CHANGED_NON_RECOVERABLE_ERROR_BIT_FB;
	}
	if (state & XRT_PASSTHROUGH_STATE_CHANGED_RECOVERABLE_ERROR_BIT) {
		res |= XR_PASSTHROUGH_STATE_CHANGED_RECOVERABLE_ERROR_BIT_FB;
	}
	if (state & XRT_PASSTHROUGH_STATE_CHANGED_RESTORED_ERROR_BIT) {
		res |= XR_PASSTHROUGH_STATE_CHANGED_RESTORED_ERROR_BIT_FB;
	}
	return res;
}
#endif

XrResult
oxr_session_poll(struct oxr_logger *log, struct oxr_session *sess)
{
	struct xrt_session *xs = sess->xs;
	xrt_result_t xret;

	if (xs == NULL) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "xrt_session is null");
	}

	int64_t now = os_monotonic_get_ns();
	XrTime now_xr = time_state_monotonic_to_ts_ns(sess->sys->inst->timekeeping, now);
	if (now_xr <= 0) {
		// shouldn't happen but be sure to log if it does
		U_LOG_W("Time keeping oddity: Polling session events at XrTime %" PRIi64, now_xr);
	}

#ifdef XRT_OS_ANDROID
	// Most recent Android activity lifecycle event was OnPause: move toward stopping
	if (sess->sys->inst->activity_state == XRT_ANDROID_LIVECYCLE_EVENT_ON_PAUSE) {
		if (sess->state == XR_SESSION_STATE_FOCUSED) {
			U_LOG_I("Activity paused: changing session state FOCUSED->VISIBLE");
			oxr_session_change_state(log, sess, XR_SESSION_STATE_VISIBLE, now_xr);
		}

		if (sess->state == XR_SESSION_STATE_VISIBLE) {
			U_LOG_I("Activity paused: changing session state VISIBLE->SYNCHRONIZED");
			oxr_session_change_state(log, sess, XR_SESSION_STATE_SYNCHRONIZED, now_xr);
		}

		if (sess->state == XR_SESSION_STATE_SYNCHRONIZED) {
			U_LOG_I("Activity paused: changing session state SYNCHRONIZED->STOPPING");
			oxr_session_change_state(log, sess, XR_SESSION_STATE_STOPPING, now_xr);
		}
		// TODO return here to avoid polling other events?
		// see https://gitlab.freedesktop.org/monado/monado/-/issues/419
	}

	// Most recent Android activity lifecycle event was OnResume: move toward ready
	if (sess->sys->inst->activity_state == XRT_ANDROID_LIVECYCLE_EVENT_ON_RESUME) {
		if (sess->state == XR_SESSION_STATE_IDLE) {
			U_LOG_I("Activity resumed: changing session state IDLE->READY");
			oxr_session_change_state(log, sess, XR_SESSION_STATE_READY, now_xr);
		}
	}
#endif // XRT_OS_ANDROID

	bool read_more_events = true;
	while (read_more_events) {
		union xrt_session_event xse = {0};
		xret = xrt_session_poll_events(xs, &xse);
		OXR_CHECK_XRET(log, sess, xret, "xrt_session_poll_events");

		// dispatch based on event type
		switch (xse.type) {
		case XRT_SESSION_EVENT_NONE:
			// No more events.
			read_more_events = false;
			break;
		case XRT_SESSION_EVENT_STATE_CHANGE:
			sess->compositor_visible = xse.state.visible;
			sess->compositor_focused = xse.state.focused;

			// Do not use xse.state.timestamp_ns, server side focused / visible state does not correspond
			// 1:1 to the cycle we tell the app. In particular the compositor may have become focused /
			// visible much earlier than what we tell the app when it became so.

			break;
		case XRT_SESSION_EVENT_OVERLAY_CHANGE:
#ifdef OXR_HAVE_EXTX_overlay
			oxr_event_push_XrEventDataMainSessionVisibilityChangedEXTX(log, sess, xse.overlay.visible);
#endif
			break;
		case XRT_SESSION_EVENT_LOSS_PENDING:
			oxr_session_change_state(
			    log, sess, XR_SESSION_STATE_LOSS_PENDING,
			    time_state_monotonic_to_ts_ns(sess->sys->inst->timekeeping, xse.loss_pending.loss_time_ns));
			break;
		case XRT_SESSION_EVENT_LOST: sess->has_lost = true; break;
		case XRT_SESSION_EVENT_REQUEST_EXIT: oxr_session_request_exit(log, sess); break;
		case XRT_SESSION_EVENT_DISPLAY_REFRESH_RATE_CHANGE:
#ifdef OXR_HAVE_FB_display_refresh_rate
			oxr_event_push_XrEventDataDisplayRefreshRateChangedFB( //
			    log,                                               //
			    sess,                                              //
			    xse.display.from_display_refresh_rate_hz,          //
			    xse.display.to_display_refresh_rate_hz);           //
#endif
			break;
		case XRT_SESSION_EVENT_REFERENCE_SPACE_CHANGE_PENDING:
			handle_reference_space_change_pending(log, sess, &xse.ref_change);
			break;
		case XRT_SESSION_EVENT_PERFORMANCE_CHANGE:
#ifdef OXR_HAVE_EXT_performance_settings
			oxr_event_push_XrEventDataPerfSettingsEXT(log, sess, xse.performance.domain,
			                                          xse.performance.sub_domain,
			                                          xse.performance.from_level, xse.performance.to_level);
#endif // OXR_HAVE_EXT_performance_settings
			break;
		case XRT_SESSION_EVENT_PASSTHRU_STATE_CHANGE:
#ifdef OXR_HAVE_FB_passthrough
			oxr_event_push_XrEventDataPassthroughStateChangedFB(
			    log, sess, xrt_to_passthrough_state_flags(xse.passthru.state));
#endif // OXR_HAVE_FB_passthrough
			break;
		case XRT_SESSION_EVENT_VISIBILITY_MASK_CHANGE:
#ifdef OXR_HAVE_KHR_visibility_mask
			// Assume mask changed for all view configuration types.
			for (uint32_t i = 0; i < sess->sys->view_config_count; i++) {
				struct oxr_view_config_properties *props = &sess->sys->view_configs[i];
				oxr_event_push_XrEventDataVisibilityMaskChangedKHR( //
				    log,                                            //
				    sess,                                           //
				    props->view_config_type,                        //
				    xse.mask_change.view_index);                    //
			}
			break;
#endif // OXR_HAVE_KHR_visibility_mask
		default: U_LOG_W("unhandled event type! %d", xse.type); break;
		}
	}

#ifdef OXR_HAVE_EXT_user_presence
	struct xrt_device *xdev = GET_STATIC_XDEV_BY_ROLE(sess->sys, head);
	if (sess->sys->inst->extensions.EXT_user_presence && xdev->supported.presence && xdev->inputs) {
		for (size_t i = 0; i < xdev->input_count; ++i) {
			struct xrt_input *input = &xdev->inputs[i];
			if (input->name != XRT_INPUT_GENERIC_HEAD_DETECT) {
				continue;
			}

			// Make sure the presence input is updated.
			xrt_result_t xret = xrt_device_update_inputs(xdev);
			OXR_CHECK_XRET(log, sess, xret, xrt_device_update_inputs);

			bool presence = input->value.boolean;
			if (sess->presence != presence) {
				oxr_event_push_XrEventDataUserPresenceChangedEXT(log, sess, presence);
				sess->presence = presence;
			}
			break;
		}
	}
#endif // OXR_HAVE_EXT_user_presence

	if (sess->state == XR_SESSION_STATE_SYNCHRONIZED && sess->compositor_visible) {
		oxr_session_change_state(log, sess, XR_SESSION_STATE_VISIBLE, now_xr);
	}

	if (sess->state == XR_SESSION_STATE_VISIBLE && sess->compositor_focused) {
		oxr_session_change_state(log, sess, XR_SESSION_STATE_FOCUSED, now_xr);
	}

	if (sess->state == XR_SESSION_STATE_FOCUSED && !sess->compositor_focused) {
		oxr_session_change_state(log, sess, XR_SESSION_STATE_VISIBLE, now_xr);
	}

	if (sess->state == XR_SESSION_STATE_VISIBLE && !sess->compositor_visible) {
		oxr_session_change_state(log, sess, XR_SESSION_STATE_SYNCHRONIZED, now_xr);
	}

	return XR_SUCCESS;
}

static inline XrViewStateFlags
xrt_to_view_state_flags(enum xrt_space_relation_flags flags)
{
	XrViewStateFlags res = 0;
	if (flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) {
		res |= XR_VIEW_STATE_ORIENTATION_VALID_BIT;
	}
	if (flags & XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT) {
		res |= XR_VIEW_STATE_ORIENTATION_TRACKED_BIT;
	}
	if (flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) {
		res |= XR_VIEW_STATE_POSITION_VALID_BIT;
	}
	if (flags & XRT_SPACE_RELATION_POSITION_TRACKED_BIT) {
		res |= XR_VIEW_STATE_POSITION_TRACKED_BIT;
	}
	return res;
}

XrResult
oxr_session_locate_views(struct oxr_logger *log,
                         struct oxr_session *sess,
                         const XrViewLocateInfo *viewLocateInfo,
                         XrViewState *viewState,
                         uint32_t viewCapacityInput,
                         uint32_t *viewCountOutput,
                         XrView *views)
{
	struct oxr_sink_logger slog = {0};
	bool print = sess->sys->inst->debug_views;
	struct xrt_device *xdev = GET_STATIC_XDEV_BY_ROLE(sess->sys, head);
	struct oxr_space *baseSpc = XRT_CAST_OXR_HANDLE_TO_PTR(struct oxr_space *, viewLocateInfo->space);
	XrResult ret;

	uint32_t view_count = 0;
	switch (sess->current_view_config_type) {
	case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO: view_count = 1; break;
	case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO: view_count = 2; break;
	case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO_WITH_FOVEATED_INSET: view_count = 4; break;
	default:
		assert(false && "view type validation unimplemented");
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "view type %d not supported",
		                 sess->current_view_config_type);
	}

	// Start two call handling.
	if (viewCountOutput != NULL) {
		*viewCountOutput = view_count;
	}
	if (viewCapacityInput == 0) {
		return oxr_session_success_result(sess);
	}
	if (viewCapacityInput < view_count) {
		return oxr_error(log, XR_ERROR_SIZE_INSUFFICIENT, "(viewCapacityInput == %u) need %u",
		                 viewCapacityInput, view_count);
	}
	// End two call handling.

	if (print) {
		oxr_slog(&slog, "\n\tviewLocateInfo->displayTime: %" PRIu64, viewLocateInfo->displayTime);
		oxr_pp_space_indented(&slog, baseSpc, "viewLocateInfo->baseSpace");
	}

	/*
	 * Get head relation, fovs and view poses.
	 */

	const uint64_t xdisplay_time =
	    time_state_ts_to_monotonic_ns(sess->sys->inst->timekeeping, viewLocateInfo->displayTime);

	// The head pose as in the xdev's space, aka XRT_INPUT_GENERIC_HEAD_POSE.
	struct xrt_space_relation T_xdev_head = XRT_SPACE_RELATION_ZERO;

	// Data including fovs and poses.
	struct locate_views_data data = {0};
	ret = get_device_fovs_and_poses( //
	    log,                         //
	    sess,                        //
	    xdev,                        //
	    xdisplay_time,               //
	    view_count,                  //
	    &T_xdev_head,                //
	    &data);                      //
	if (ret != XR_SUCCESS) {
		if (print) {
			oxr_slog(&slog, "\n\tReturning invalid poses");
			oxr_log_slog(log, &slog);
		} else {
			oxr_slog_cancel(&slog);
		}
		return ret;
	}

	// The xdev pose in the base space.
	struct xrt_space_relation T_base_xdev = XRT_SPACE_RELATION_ZERO;
	ret = oxr_space_locate_device(   //
	    log,                         //
	    xdev,                        //
	    baseSpc,                     //
	    viewLocateInfo->displayTime, //
	    &T_base_xdev);               //
	if (ret != XR_SUCCESS || T_base_xdev.relation_flags == 0) {
		if (print) {
			oxr_slog(&slog, "\n\tReturning invalid poses");
			oxr_log_slog(log, &slog);
		} else {
			oxr_slog_cancel(&slog);
		}
		return ret;
	}

	struct xrt_space_relation T_base_head;
	struct xrt_relation_chain xrc = {0};
	m_relation_chain_push_relation(&xrc, &T_xdev_head);
	m_relation_chain_push_relation(&xrc, &T_base_xdev);
	m_relation_chain_resolve(&xrc, &T_base_head);

	if (print) {
		for (uint32_t i = 0; i < view_count; i++) {
			char tmp[32];
			snprintf(tmp, 32, "xdev.view[%i]", i);
			oxr_pp_fov_indented_as_object(&slog, &data.fovs[i], tmp);
			oxr_pp_pose_indented_as_object(&slog, &data.poses[i], tmp);
		}
		oxr_pp_relation_indented(&slog, &T_xdev_head, "T_xdev_head");
		oxr_pp_relation_indented(&slog, &T_base_xdev, "T_base_xdev");
	}

	for (uint32_t i = 0; i < view_count; i++) {
		/*
		 * Pose
		 */

		struct xrt_pose view_pose = data.poses[i];

		bool parallelize_views =
		    sess->sys->inst->quirks.parallel_views && !math_quat_is_identity(&view_pose.orientation, 0.0001f);

		if (parallelize_views) {
			view_pose.orientation = (struct xrt_quat)XRT_QUAT_IDENTITY;
		}

		// Do the magical space relation dance here.
		struct xrt_space_relation result = {0};
		struct xrt_relation_chain xrc = {0};
		m_relation_chain_push_pose_if_not_identity(&xrc, &view_pose);
		m_relation_chain_push_relation(&xrc, &T_base_head);
		m_relation_chain_resolve(&xrc, &result);
		OXR_XRT_POSE_TO_XRPOSEF(result.pose, views[i].pose);


		/*
		 * Fov
		 */

		struct xrt_fov fov = data.fovs[i];

		if (parallelize_views) {
			math_compute_parallelized_fov(  //
			    &data.fovs[i],              // in
			    &data.poses[i].orientation, // in
			    &fov);                      // out
		}

		OXR_XRT_FOV_TO_XRFOVF(fov, views[i].fov);


		/*
		 * Printing.
		 */

		if (print) {
			char tmp[16];
			snprintf(tmp, 16, "view[%i]", i);
			oxr_pp_relation_indented(&slog, &result, tmp);
		}

		/*
		 * Checking, debug and flag handling.
		 */

		struct xrt_pose *pose = (struct xrt_pose *)&views[i].pose;
		if ((result.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0 &&
		    !math_quat_ensure_normalized(&pose->orientation)) {
			struct xrt_quat *q = &pose->orientation;
			struct xrt_quat norm = *q;
			math_quat_normalize(&norm);
			oxr_slog_cancel(&slog);
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
			                 "Quaternion %a %a %a %a (normalized %a %a %a %a) "
			                 "in xrLocateViews was invalid",
			                 q->x, q->y, q->z, q->w, norm.x, norm.y, norm.z, norm.w);
		}

		if (i == 0) {
			viewState->viewStateFlags = xrt_to_view_state_flags(result.relation_flags);
		} else {
			viewState->viewStateFlags &= xrt_to_view_state_flags(result.relation_flags);
		}
	}

	if (print) {
		oxr_log_slog(log, &slog);
	} else {
		oxr_slog_cancel(&slog);
	}

	return oxr_session_success_result(sess);
}

static double
ns_to_ms(int64_t ns)
{
	double ms = ((double)ns) * 1. / 1000. * 1. / 1000.;
	return ms;
}

static double
ts_ms(struct oxr_session *sess)
{
	timepoint_ns now = time_state_get_now(sess->sys->inst->timekeeping);
	int64_t monotonic = time_state_ts_to_monotonic_ns(sess->sys->inst->timekeeping, now);
	return ns_to_ms(monotonic);
}

static XrResult
do_wait_frame_and_checks(struct oxr_logger *log,
                         struct oxr_session *sess,
                         int64_t *out_frame_id,
                         int64_t *out_predicted_display_time,
                         int64_t *out_predicted_display_period,
                         XrTime *out_converted_time)
{
	assert(sess->compositor != NULL);

	int64_t frame_id = -1;
	int64_t predicted_display_time = 0;
	int64_t predicted_display_period = 0;

	xrt_result_t xret = xrt_comp_wait_frame( //
	    sess->compositor,                    // compositor
	    &frame_id,                           // out_frame_id
	    &predicted_display_time,             // out_predicted_display_time
	    &predicted_display_period);          // out_predicted_display_period
	OXR_CHECK_XRET(log, sess, xret, xrt_comp_wait_frame);

	if (frame_id < 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Got a negative frame id '%" PRIi64 "'", frame_id);
	}

	if ((int64_t)predicted_display_time <= 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Got a negative display time '%" PRIi64 "'",
		                 (int64_t)predicted_display_time);
	}

	XrTime converted_time = time_state_monotonic_to_ts_ns(sess->sys->inst->timekeeping, predicted_display_time);
	if (converted_time <= 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Got '%" PRIi64 "' from time_state_monotonic_to_ts_ns",
		                 converted_time);
	}

	*out_frame_id = frame_id;
	*out_predicted_display_time = predicted_display_time;
	*out_predicted_display_period = predicted_display_period;
	*out_converted_time = converted_time;

	return XR_SUCCESS;
}

XrResult
oxr_session_frame_wait(struct oxr_logger *log, struct oxr_session *sess, XrFrameState *frameState)
{
	//! @todo this should be carefully synchronized, because there may be
	//! more than one session per instance.
	XRT_MAYBE_UNUSED timepoint_ns now = time_state_get_now_and_update(sess->sys->inst->timekeeping);

	struct xrt_compositor *xc = sess->compositor;
	if (xc == NULL) {
		frameState->shouldRender = XR_FALSE;
		return oxr_session_success_result(sess);
	}

	if (sess->frame_timing_spew) {
		oxr_log(log, "Called at %8.3fms", ts_ms(sess));
	}

	/*
	 * A subsequent xrWaitFrame call must: block until the previous frame
	 * has been begun. It's extremely forbidden to call xrWaitFrame from
	 * multiple threads. We do this before so we call predicted after any
	 * waiting for xrBeginFrame has happened, for better timing information.
	 */
	XrResult ret = oxr_frame_sync_wait_frame(&sess->frame_sync);
	if (XR_SUCCESS != ret) {
		// session not running
		return ret;
	}

	if (sess->frame_timing_spew) {
		oxr_log(log, "Finished waiting for previous frame begin at %8.3fms", ts_ms(sess));
	}

	int64_t frame_id = -1;
	int64_t predicted_display_time = 0;
	int64_t predicted_display_period = 0;
	XrTime converted_time = 0;

	ret = do_wait_frame_and_checks( //
	    log,                        // log
	    sess,                       // sess
	    &frame_id,                  // out_frame_id
	    &predicted_display_time,    // out_predicted_display_time
	    &predicted_display_period,  // out_predicted_display_period
	    &converted_time);           // out_converted_time
	if (ret != XR_SUCCESS) {
		// On error we need to release the semaphore ourselves as xrBeginFrame won't do it.
		// Should not get an error.
		XrResult release_ret = oxr_frame_sync_release(&sess->frame_sync);
		assert(release_ret == XR_SUCCESS);
		(void)release_ret;
		// Error already logged.
		return ret;
	}
	assert(predicted_display_time != 0);
	assert(predicted_display_period != 0);
	assert(converted_time != 0);

	/*
	 * We set the frame_id along with the number of active waited frames to
	 * avoid races with xrBeginFrame. The function xrBeginFrame will only
	 * allow xrWaitFrame to continue from the semaphore above once it has
	 * cleared the `sess->frame_id.waited`.
	 */
	os_mutex_lock(&sess->active_wait_frames_lock);
	sess->active_wait_frames++;
	sess->frame_id.waited = frame_id;
	os_mutex_unlock(&sess->active_wait_frames_lock);

	frameState->shouldRender = should_render(sess->state);
	frameState->predictedDisplayPeriod = predicted_display_period;
	frameState->predictedDisplayTime = converted_time;

	if (sess->frame_timing_spew) {
		oxr_log(log,
		        "Waiting finished at %8.3fms. Predicted display time "
		        "%8.3fms, "
		        "period %8.3fms",
		        ts_ms(sess), ns_to_ms(predicted_display_time), ns_to_ms(predicted_display_period));
	}

	if (sess->frame_timing_wait_sleep_ms > 0) {
		int64_t sleep_ns = U_TIME_1MS_IN_NS * sess->frame_timing_wait_sleep_ms;
		os_precise_sleeper_nanosleep(&sess->sleeper, sleep_ns);
	}

	return oxr_session_success_result(sess);
}

XrResult
oxr_session_frame_begin(struct oxr_logger *log, struct oxr_session *sess)
{
	struct xrt_compositor *xc = sess->compositor;

	os_mutex_lock(&sess->active_wait_frames_lock);
	int active_wait_frames = sess->active_wait_frames;
	os_mutex_unlock(&sess->active_wait_frames_lock);

	XrResult ret;
	if (active_wait_frames == 0) {
		return oxr_error(log, XR_ERROR_CALL_ORDER_INVALID, "xrBeginFrame without xrWaitFrame");
	}

	if (sess->frame_started) {
		// max 2 xrWaitFrame can be in flight so a second xrBeginFrame
		// is only valid if we have a second xrWaitFrame in flight
		if (active_wait_frames != 2) {
			return oxr_error(log, XR_ERROR_CALL_ORDER_INVALID, "xrBeginFrame without xrWaitFrame");
		}


		ret = XR_FRAME_DISCARDED;
		if (xc != NULL) {
			xrt_result_t xret = xrt_comp_discard_frame(xc, sess->frame_id.begun);
			OXR_CHECK_XRET(log, sess, xret, xrt_comp_discard_frame);
			sess->frame_id.begun = -1;

			os_mutex_lock(&sess->active_wait_frames_lock);
			sess->active_wait_frames--;
			os_mutex_unlock(&sess->active_wait_frames_lock);
		}
	} else {
		ret = oxr_session_success_result(sess);
		sess->frame_started = true;
	}
	if (xc != NULL) {
		xrt_result_t xret = xrt_comp_begin_frame(xc, sess->frame_id.waited);
		OXR_CHECK_XRET(log, sess, xret, xrt_comp_begin_frame);
		sess->frame_id.begun = sess->frame_id.waited;
		sess->frame_id.waited = -1;
	}

	// beginFrame is about to succeed, we can release an xrWaitFrame, if available.
	XrResult osh_ret = oxr_frame_sync_release(&sess->frame_sync);
	if (XR_SUCCESS != osh_ret) {
		// session not running
		return osh_ret;
	}

	return ret;
}

static XrResult
oxr_session_destroy(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_session *sess = (struct oxr_session *)hb;

	XrResult ret = oxr_event_remove_session_events(log, sess);

	oxr_session_action_context_fini(&sess->action_context);
	oxr_session_attached_actions_fini(&sess->attached_actions);

	xrt_comp_destroy(&sess->compositor);
	xrt_comp_native_destroy(&sess->xcn);
	xrt_session_destroy(&sess->xs);

	os_precise_sleeper_deinit(&sess->sleeper);
	oxr_frame_sync_fini(&sess->frame_sync);
	os_mutex_destroy(&sess->active_wait_frames_lock);

	free(sess);

	return ret;
}

static XrResult
oxr_session_allocate_and_init(struct oxr_logger *log,
                              struct oxr_system *sys,
                              enum oxr_session_graphics_ext gfx_ext,
                              struct oxr_session **out_session)
{
	struct oxr_session *sess = NULL;
	XrResult ret;
	OXR_ALLOCATE_HANDLE_PARENT_OR_RETURN(log, sess, OXR_XR_DEBUG_SESSION, oxr_session_destroy, &sys->inst->handle);

	// Needs to happen early due to mutex.
	ret = oxr_session_attached_actions_init(&sess->attached_actions);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to initialize attached actions");
	}

	// Needs to happen early due to mutex.
	ret = oxr_session_action_context_init(&sess->action_context, sys->xsysd, sys->xso);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to initialize action context");
	}

	// What graphics API type was this created with.
	sess->gfx_ext = gfx_ext;

	// What system is this session based on.
	sess->sys = sys;

	// Init the begin/wait frame handler.
	oxr_frame_sync_init(&sess->frame_sync);

	// Init the wait frame precise sleeper.
	os_precise_sleeper_init(&sess->sleeper);

	sess->active_wait_frames = 0;
	os_mutex_init(&sess->active_wait_frames_lock);

	// Debug and user options.
	sess->ipd_meters = debug_get_num_option_ipd() / 1000.0f;
	sess->frame_timing_spew = debug_get_bool_option_frame_timing_spew();
	sess->frame_timing_wait_sleep_ms = debug_get_num_option_wait_frame_sleep();

	// This is set to something valid in begin session, used in xrEndFrame.
	sess->current_view_config_type = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;

	// Done with basic init, set out variable.
	*out_session = sess;

	return XR_SUCCESS;
}

#define OXR_CHECK_XSYSC(LOG, SYS)                                                                                      \
	do {                                                                                                           \
		if (sys->xsysc == NULL) {                                                                              \
			return oxr_error(log, XR_ERROR_VALIDATION_FAILURE,                                             \
			                 " Can not use graphics bindings when have asked to not create graphics");     \
		}                                                                                                      \
	} while (false)

#define OXR_CHECK_XR_SUCCESS(LOG, FUNC, MSG)                                                                           \
	do {                                                                                                           \
		XrResult _xr_result = FUNC;                                                                            \
		if (_xr_result != XR_SUCCESS) {                                                                        \
			return oxr_error(LOG, _xr_result, MSG);                                                        \
		}                                                                                                      \
	} while (false)



static XrResult
oxr_create_xrt_session_and_native_compositor(struct oxr_logger *log,
                                             const struct xrt_session_info *xsi,
                                             struct oxr_session *sess)
{
	if (sess->sys->xsysc == NULL) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "The system compositor wasn't created, can't create native compositor!");
	}
	xrt_result_t xret = xrt_system_create_session(sess->sys->xsys, xsi, &sess->xs, &sess->xcn);
	if (xret == XRT_ERROR_MULTI_SESSION_NOT_IMPLEMENTED) {
		return oxr_error(log, XR_ERROR_LIMIT_REACHED, "Per instance multi-session not supported.");
	}
	if (xret != XRT_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Failed to create xrt_session and xrt_compositor_native! '%i'", xret);
	}
	if (sess->sys->xsysc->xmcc != NULL) {
		xrt_syscomp_set_state(sess->sys->xsysc, &sess->xcn->base, true, true, os_monotonic_get_ns());
		xrt_syscomp_set_z_order(sess->sys->xsysc, &sess->xcn->base, 0);
	}
	return XR_SUCCESS;
}



/*
 * Does allocation, population and basic init, so we can use early-returns to
 * simplify code flow and avoid weird if/else.
 */
static XrResult
oxr_session_create_impl(struct oxr_logger *log,
                        struct oxr_system *sys,
                        const XrSessionCreateInfo *createInfo,
                        const struct xrt_session_info *xsi,
                        struct oxr_session **out_session)
{
	XrResult ret = XR_SUCCESS;

#if defined(XR_USE_PLATFORM_XLIB) && defined(XR_USE_GRAPHICS_API_OPENGL)
	XrGraphicsBindingOpenGLXlibKHR const *opengl_xlib = OXR_GET_INPUT_FROM_CHAIN(
	    createInfo, XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR, XrGraphicsBindingOpenGLXlibKHR);
	if (opengl_xlib != NULL) {
		OXR_CHECK_XSYSC(log, sys);

		if (!sys->gotten_requirements) {
			return oxr_error(log, XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING,
			                 "Has not called "
			                 "xrGetOpenGL[ES]GraphicsRequirementsKHR");
		}

		ret = oxr_session_allocate_and_init(log, sys, OXR_SESSION_GRAPHICS_EXT_XLIB_GL, out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to allocate session");

		ret = oxr_create_xrt_session_and_native_compositor(log, xsi, *out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to create session/compositor");

		return oxr_session_populate_gl_xlib(log, sys, opengl_xlib, *out_session);
	}
#endif


#if defined(XR_USE_PLATFORM_ANDROID) && defined(XR_USE_GRAPHICS_API_OPENGL_ES)
	XrGraphicsBindingOpenGLESAndroidKHR const *opengles_android = OXR_GET_INPUT_FROM_CHAIN(
	    createInfo, XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR, XrGraphicsBindingOpenGLESAndroidKHR);
	if (opengles_android != NULL) {
		OXR_CHECK_XSYSC(log, sys);

		if (!sys->gotten_requirements) {
			return oxr_error(log, XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING,
			                 "Has not called "
			                 "xrGetOpenGLESGraphicsRequirementsKHR");
		}

		ret = oxr_session_allocate_and_init(log, sys, OXR_SESSION_GRAPHICS_EXT_ANDROID_GLES, out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to allocate session");

		ret = oxr_create_xrt_session_and_native_compositor(log, xsi, *out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to create session/compositor");

		return oxr_session_populate_gles_android(log, sys, opengles_android, *out_session);
	}
#endif

#if defined(XR_USE_PLATFORM_WIN32) && defined(XR_USE_GRAPHICS_API_OPENGL)
	XrGraphicsBindingOpenGLWin32KHR const *opengl_win32 = OXR_GET_INPUT_FROM_CHAIN(
	    createInfo, XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, XrGraphicsBindingOpenGLWin32KHR);
	if (opengl_win32 != NULL) {
		OXR_CHECK_XSYSC(log, sys);

		if (!sys->gotten_requirements) {
			return oxr_error(log, XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING,
			                 "Has not called xrGetOpenGLGraphicsRequirementsKHR");
		}

		ret = oxr_session_allocate_and_init(log, sys, OXR_SESSION_GRAPHICS_EXT_WIN32_GL, out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to allocate session");

		ret = oxr_create_xrt_session_and_native_compositor(log, xsi, *out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to create session/compositor");

		return oxr_session_populate_gl_win32(log, sys, opengl_win32, *out_session);
	}
#endif

#ifdef XR_USE_GRAPHICS_API_VULKAN
	XrGraphicsBindingVulkanKHR const *vulkan =
	    OXR_GET_INPUT_FROM_CHAIN(createInfo, XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR, XrGraphicsBindingVulkanKHR);
	if (vulkan != NULL) {
		OXR_CHECK_XSYSC(log, sys);

		OXR_VERIFY_ARG_NOT_ZERO(log, vulkan->instance);
		OXR_VERIFY_ARG_NOT_ZERO(log, vulkan->physicalDevice);
		if (vulkan->device == VK_NULL_HANDLE) {
			return oxr_error(log, XR_ERROR_GRAPHICS_DEVICE_INVALID, "VkDevice must not be VK_NULL_HANDLE");
		}

		if (!sys->gotten_requirements) {
			return oxr_error(log, XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING,
			                 "Has not called "
			                 "xrGetVulkanGraphicsRequirementsKHR");
		}

		if (sys->suggested_vulkan_physical_device == VK_NULL_HANDLE) {
			char *fn = sys->inst->extensions.KHR_vulkan_enable ? "xrGetVulkanGraphicsDeviceKHR"
			                                                   : "xrGetVulkanGraphicsDevice2KHR";
			return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "Has not called %s", fn);
		}

		if (sys->suggested_vulkan_physical_device != vulkan->physicalDevice) {
			char *fn = sys->inst->extensions.KHR_vulkan_enable ? "xrGetVulkanGraphicsDeviceKHR"
			                                                   : "xrGetVulkanGraphicsDevice2KHR";
			return oxr_error(
			    log, XR_ERROR_VALIDATION_FAILURE,
			    "XrGraphicsBindingVulkanKHR::physicalDevice %p must match device %p specified by %s",
			    (void *)vulkan->physicalDevice, (void *)sys->suggested_vulkan_physical_device, fn);
		}

		ret = oxr_session_allocate_and_init(log, sys, OXR_SESSION_GRAPHICS_EXT_VULKAN, out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to allocate session");

		ret = oxr_create_xrt_session_and_native_compositor(log, xsi, *out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to create session/compositor");

		return oxr_session_populate_vk(log, sys, vulkan, *out_session);
	}
#endif

#ifdef XR_USE_PLATFORM_EGL
	XrGraphicsBindingEGLMNDX const *egl =
	    OXR_GET_INPUT_FROM_CHAIN(createInfo, XR_TYPE_GRAPHICS_BINDING_EGL_MNDX, XrGraphicsBindingEGLMNDX);
	if (egl != NULL) {
		OXR_CHECK_XSYSC(log, sys);

		if (!sys->gotten_requirements) {
			return oxr_error(log, XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING,
			                 "Has not called "
			                 "xrGetOpenGL[ES]GraphicsRequirementsKHR");
		}

		ret = oxr_session_allocate_and_init(log, sys, OXR_SESSION_GRAPHICS_EXT_EGL, out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to allocate session");

		ret = oxr_create_xrt_session_and_native_compositor(log, xsi, *out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to create session/compositor");

		return oxr_session_populate_egl(log, sys, egl, *out_session);
	}
#endif

#ifdef XR_USE_GRAPHICS_API_D3D11
	XrGraphicsBindingD3D11KHR const *d3d11 =
	    OXR_GET_INPUT_FROM_CHAIN(createInfo, XR_TYPE_GRAPHICS_BINDING_D3D11_KHR, XrGraphicsBindingD3D11KHR);
	if (d3d11 != NULL) {
		// we know the fields of this struct are OK by now since they were checked with XrSessionCreateInfo

		OXR_CHECK_XSYSC(log, sys);

		if (!sys->gotten_requirements) {
			return oxr_error(log, XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING,
			                 "Has not called xrGetD3D11GraphicsRequirementsKHR");
		}
		ret = oxr_d3d11_check_device(log, sys, d3d11->device);

		if (!XR_SUCCEEDED(ret)) {
			return ret;
		}

		ret = oxr_session_allocate_and_init(log, sys, OXR_SESSION_GRAPHICS_EXT_D3D11, out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to allocate session");

		ret = oxr_create_xrt_session_and_native_compositor(log, xsi, *out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to create session/compositor");

		return oxr_session_populate_d3d11(log, sys, d3d11, *out_session);
	}
#endif

#ifdef XR_USE_GRAPHICS_API_D3D12
	XrGraphicsBindingD3D12KHR const *d3d12 =
	    OXR_GET_INPUT_FROM_CHAIN(createInfo, XR_TYPE_GRAPHICS_BINDING_D3D12_KHR, XrGraphicsBindingD3D12KHR);
	if (d3d12 != NULL) {
		// we know the fields of this struct are OK by now since they were checked with XrSessionCreateInfo

		OXR_CHECK_XSYSC(log, sys);

		if (!sys->gotten_requirements) {
			return oxr_error(log, XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING,
			                 "Has not called xrGetD3D12GraphicsRequirementsKHR");
		}
		ret = oxr_d3d12_check_device(log, sys, d3d12->device);

		if (!XR_SUCCEEDED(ret)) {
			return ret;
		}

		ret = oxr_session_allocate_and_init(log, sys, OXR_SESSION_GRAPHICS_EXT_D3D12, out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to allocate session");

		ret = oxr_create_xrt_session_and_native_compositor(log, xsi, *out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to create session/compositor");

		return oxr_session_populate_d3d12(log, sys, d3d12, *out_session);
	}
#endif
	/*
	 * Add any new graphics binding structs here - before the headless
	 * check. (order for non-headless checks not specified in standard.)
	 * Any new addition will also need to be added to
	 * oxr_verify_XrSessionCreateInfo and have its own associated verify
	 * function added.
	 */

#ifdef OXR_HAVE_MND_headless
	if (sys->inst->extensions.MND_headless) {

		ret = oxr_session_allocate_and_init(log, sys, OXR_SESSION_GRAPHICS_EXT_HEADLESS, out_session);
		OXR_CHECK_XR_SUCCESS(log, ret, "Failed to allocate session");

		(*out_session)->compositor = NULL;
		(*out_session)->create_swapchain = NULL;

		xrt_result_t xret = xrt_system_create_session(sys->xsys, xsi, &(*out_session)->xs, NULL);
		if (xret == XRT_ERROR_MULTI_SESSION_NOT_IMPLEMENTED) {
			return oxr_error(log, XR_ERROR_LIMIT_REACHED, "Per instance multi-session not supported.");
		}
		if (xret != XRT_SUCCESS) {
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to create xrt_session! '%i'", xret);
		}

		return XR_SUCCESS;
	}
#endif
	return oxr_error(log, XR_ERROR_VALIDATION_FAILURE,
	                 "(createInfo->next->type) doesn't contain a valid "
	                 "graphics binding structs");
}

XrResult
oxr_session_create(struct oxr_logger *log,
                   struct oxr_system *sys,
                   const XrSessionCreateInfo *createInfo,
                   struct oxr_session **out_session)
{
	struct oxr_session *sess = NULL;

	struct xrt_session_info xsi = {0};
	const XrSessionCreateInfoOverlayEXTX *overlay_info = OXR_GET_INPUT_FROM_CHAIN(
	    createInfo, XR_TYPE_SESSION_CREATE_INFO_OVERLAY_EXTX, XrSessionCreateInfoOverlayEXTX);
	if (overlay_info) {
		xsi.is_overlay = true;
		xsi.flags = overlay_info->createFlags;
		xsi.z_order = overlay_info->sessionLayersPlacement;
	}

	/* Try allocating and populating. */
	XrResult ret = oxr_session_create_impl(log, sys, createInfo, &xsi, &sess);
	if (ret != XR_SUCCESS) {
		if (sess != NULL) {
			/* clean up allocation first */
			XrResult cleanup_result = oxr_handle_parent_destroy(log, &sess->handle);
			assert(cleanup_result == XR_SUCCESS);
			(void)cleanup_result;
		}
		return ret;
	}

	int64_t now = os_monotonic_get_ns();
	XrTime now_xr = time_state_monotonic_to_ts_ns(sess->sys->inst->timekeeping, now);
	if (now_xr <= 0) {
		// shouldn't happen but be sure to log if it does
		U_LOG_W("Time keeping oddity: XR_SESSION_STATE_IDLE reached at XrTime %" PRIi64, now_xr);
	}

	// Everything is in order, start the state changes.
	oxr_session_change_state(log, sess, XR_SESSION_STATE_IDLE, now_xr);
	oxr_session_change_state(log, sess, XR_SESSION_STATE_READY, now_xr);

	*out_session = sess;

	return ret;
}

/*
 * Gets the body pose in the base space.
 */
XrResult
oxr_get_base_body_pose(struct oxr_logger *log,
                       const struct xrt_body_joint_set *body_joint_set,
                       struct oxr_space *base_spc,
                       struct xrt_device *body_xdev,
                       XrTime at_time,
                       struct xrt_space_relation *out_base_body)
{
	const struct xrt_space_relation space_relation_zero = XRT_SPACE_RELATION_ZERO;
	*out_base_body = space_relation_zero;

	// The body pose is returned in the xdev's space.
	const struct xrt_space_relation *T_xdev_body = &body_joint_set->body_pose;

	// Get the xdev's pose in the base space.
	struct xrt_space_relation T_base_xdev = XRT_SPACE_RELATION_ZERO;

	XrResult ret = oxr_space_locate_device(log, body_xdev, base_spc, at_time, &T_base_xdev);
	if (ret != XR_SUCCESS) {
		return ret;
	}
	if (T_base_xdev.relation_flags == 0) {
		return XR_SUCCESS;
	}

	struct xrt_relation_chain xrc = {0};
	m_relation_chain_push_relation(&xrc, T_xdev_body);
	m_relation_chain_push_relation(&xrc, &T_base_xdev);
	m_relation_chain_resolve(&xrc, out_base_body);

	return XR_SUCCESS;
}

#ifdef OXR_HAVE_KHR_android_thread_settings
static enum xrt_thread_hint
xr_thread_type_to_thread_hint(XrAndroidThreadTypeKHR type)
{
	switch (type) {
	case XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR: return XRT_THREAD_HINT_APPLICATION_MAIN;
	case XR_ANDROID_THREAD_TYPE_APPLICATION_WORKER_KHR: return XRT_THREAD_HINT_APPLICATION_WORKER;
	case XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR: return XRT_THREAD_HINT_RENDERER_MAIN;
	case XR_ANDROID_THREAD_TYPE_RENDERER_WORKER_KHR: return XRT_THREAD_HINT_RENDERER_WORKER;
	default: assert(false); return 0;
	}
}

XrResult
oxr_session_android_thread_settings(struct oxr_logger *log,
                                    struct oxr_session *sess,
                                    XrAndroidThreadTypeKHR threadType,
                                    uint32_t threadId)
{
	struct xrt_compositor *xc = &sess->xcn->base;

	if (xc == NULL) {
		return oxr_error(log, XR_ERROR_FUNCTION_UNSUPPORTED,
		                 "Extension XR_KHR_android_thread_settings not be implemented");
	}

	// Convert.
	enum xrt_thread_hint xhint = xr_thread_type_to_thread_hint(threadType);

	// Do the call!
	xrt_result_t xret = xrt_comp_set_thread_hint(xc, xhint, threadId);
	OXR_CHECK_XRET(log, sess, xret, oxr_session_android_thread_settings);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_KHR_android_thread_settings

#ifdef OXR_HAVE_KHR_visibility_mask

XrResult
oxr_session_get_visibility_mask(struct oxr_logger *log,
                                struct oxr_session *sess,
                                XrVisibilityMaskTypeKHR visibilityMaskType,
                                uint32_t viewIndex,
                                XrVisibilityMaskKHR *visibilityMask)
{
	struct oxr_system *sys = sess->sys;
	struct xrt_device *xdev = GET_STATIC_XDEV_BY_ROLE(sess->sys, head);
	enum xrt_visibility_mask_type type = convert_mask_type(visibilityMaskType);

	assert(viewIndex < ARRAY_SIZE(sys->visibility_mask));

	struct xrt_visibility_mask *mask = sys->visibility_mask[viewIndex];

	// Do we need to free the mask.
	if (mask != NULL && mask->type != type) {
		free(mask);
		mask = NULL;
		sys->visibility_mask[viewIndex] = NULL;
	}

	// If we didn't have any cached mask get it.
	if (mask == NULL) {
		if (is_emulated_quad_inset_view(xdev, viewIndex)) {
			u_visibility_mask_get_noop(type, &mask);
		} else {
			xrt_result_t xret = xrt_device_get_visibility_mask(xdev, type, viewIndex, &mask);
			if (xret == XRT_ERROR_NOT_IMPLEMENTED && xdev->hmd != NULL) {
				const struct xrt_fov fov = xdev->hmd->distortion.fov[viewIndex];
				u_visibility_mask_get_default(type, &fov, &mask);
			} else {
				OXR_CHECK_XRET(log, sess, xret, get_visibility_mask);
			}
		}
		sys->visibility_mask[viewIndex] = mask;
	}

	visibilityMask->vertexCountOutput = mask->vertex_count;
	visibilityMask->indexCountOutput = mask->index_count;

	if (visibilityMask->vertexCapacityInput == 0 || visibilityMask->indexCapacityInput == 0) {
		return XR_SUCCESS;
	}

	if (visibilityMask->vertexCapacityInput < mask->vertex_count) {
		return oxr_error(log, XR_ERROR_SIZE_INSUFFICIENT, "vertexCapacityInput is %u, need %u",
		                 visibilityMask->vertexCapacityInput, mask->vertex_count);
	} else if (visibilityMask->indexCapacityInput < mask->index_count) {
		return oxr_error(log, XR_ERROR_SIZE_INSUFFICIENT, "indexCapacityInput is %u, need %u",
		                 visibilityMask->indexCapacityInput, mask->index_count);
	}

	memcpy(visibilityMask->vertices, xrt_visibility_mask_get_vertices(mask),
	       sizeof(struct xrt_vec2) * mask->vertex_count);
	memcpy(visibilityMask->indices, xrt_visibility_mask_get_indices(mask), sizeof(uint32_t) * mask->index_count);

	return XR_SUCCESS;
}

#endif // OXR_HAVE_KHR_visibility_mask

#ifdef OXR_HAVE_FB_display_refresh_rate
XrResult
oxr_session_get_display_refresh_rate(struct oxr_logger *log, struct oxr_session *sess, float *displayRefreshRate)
{
	struct xrt_compositor *xc = &sess->xcn->base;

	if (xc == NULL) {
		return oxr_session_success_result(sess);
	}

	xrt_result_t xret = xrt_comp_get_display_refresh_rate(xc, displayRefreshRate);
	OXR_CHECK_XRET(log, sess, xret, xrt_comp_get_display_refresh_rate);

	return XR_SUCCESS;
}

XrResult
oxr_session_request_display_refresh_rate(struct oxr_logger *log, struct oxr_session *sess, float displayRefreshRate)
{
	struct xrt_compositor *xc = &sess->xcn->base;

	if (xc == NULL) {
		return oxr_session_success_result(sess);
	}

	xrt_result_t xret = xrt_comp_request_display_refresh_rate(xc, displayRefreshRate);
	OXR_CHECK_XRET(log, sess, xret, xrt_comp_request_display_refresh_rate);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_FB_display_refresh_rate

#ifdef OXR_HAVE_EXT_performance_settings
XrResult
oxr_session_set_perf_level(struct oxr_logger *log,
                           struct oxr_session *sess,
                           XrPerfSettingsDomainEXT domain,
                           XrPerfSettingsLevelEXT level)
{
	struct xrt_compositor *xc = &sess->xcn->base;

	if (xc->set_performance_level == NULL) {
		return XR_ERROR_FUNCTION_UNSUPPORTED;
	}
	enum xrt_perf_domain oxr_domain = xr_perf_domain_to_xrt(domain);
	enum xrt_perf_set_level oxr_level = xr_perf_level_to_xrt(level);
	xrt_comp_set_performance_level(xc, oxr_domain, oxr_level);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_EXT_performance_settings
