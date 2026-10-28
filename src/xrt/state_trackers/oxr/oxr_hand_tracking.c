// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Hand tracking functions.
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_space.h"

#include "math/m_api.h"
#include "math/m_mathinclude.h"

#include "util/u_debug.h"
#include "util/u_trace_marker.h"

#include "oxr_hand_tracking.h"
#include "oxr_conversions.h"
#include "oxr_logger.h"
#include "oxr_chain.h"
#include "oxr_xret.h"


/*
 *
 * Helpers
 *
 */

DEBUG_GET_ONCE_BOOL_OPTION(hand_tracking_prioritize_conforming, "OXR_HAND_TRACKING_PRIORITIZE_CONFORMING", false)

static void
xrt_to_xr_pose(struct xrt_pose *xrt_pose, XrPosef *xr_pose)
{
	xr_pose->orientation.x = xrt_pose->orientation.x;
	xr_pose->orientation.y = xrt_pose->orientation.y;
	xr_pose->orientation.z = xrt_pose->orientation.z;
	xr_pose->orientation.w = xrt_pose->orientation.w;

	xr_pose->position.x = xrt_pose->position.x;
	xr_pose->position.y = xrt_pose->position.y;
	xr_pose->position.z = xrt_pose->position.z;
}

static enum xrt_output_name
xr_hand_to_force_feedback_output(XrHandEXT hand)
{
	switch (hand) {
	case XR_HAND_LEFT_EXT: return XRT_OUTPUT_NAME_FORCE_FEEDBACK_LEFT;
	case XR_HAND_RIGHT_EXT: return XRT_OUTPUT_NAME_FORCE_FEEDBACK_RIGHT;
	default: assert(false); return 0;
	}
}

static void
sort_hand_tracking_sources(struct xrt_hand_tracker_create_info *info)
{
	if (info->requested_source_count < 2) {
		return;
	}

	if (info->requested_sources[1] < info->requested_sources[0]) {
		enum xrt_input_name tmp = info->requested_sources[0];
		info->requested_sources[0] = info->requested_sources[1];
		info->requested_sources[1] = tmp;
	}
}

static void
swap_hand_tracking_sources(struct xrt_hand_tracker_create_info *info)
{
	if (info->requested_source_count < 2) {
		return;
	}

	enum xrt_input_name tmp = info->requested_sources[0];
	info->requested_sources[0] = info->requested_sources[1];
	info->requested_sources[1] = tmp;
}


/*
 *
 * XR_EXT_hand_tracking
 *
 */

#ifdef OXR_HAVE_EXT_hand_tracking

static XrResult
oxr_hand_tracker_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_hand_tracker *hand_tracker = (struct oxr_hand_tracker *)hb;

	xrt_hand_tracker_destroy(&hand_tracker->xht);

	free(hand_tracker);

	return XR_SUCCESS;
}

XrResult
oxr_hand_tracker_create(struct oxr_logger *log,
                        struct oxr_session *sess,
                        const XrHandTrackerCreateInfoEXT *createInfo,
                        struct xrt_device *override_xdev,
                        struct oxr_hand_tracker **out_hand_tracker)
{
	struct oxr_hand_tracker *hand_tracker = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, hand_tracker, OXR_XR_DEBUG_HTRACKER, oxr_hand_tracker_destroy_cb,
	                              &sess->handle);

	hand_tracker->sess = sess;
	hand_tracker->hand = createInfo->hand;
	hand_tracker->hand_joint_set = createInfo->handJointSet;

	struct xrt_hand_tracker_create_info info = {
	    .hand = createInfo->hand == XR_HAND_LEFT_EXT ? XRT_HAND_LEFT : XRT_HAND_RIGHT,
	    .locked_xdev = override_xdev,
	};

	// Fill out default priority.
	info.requested_sources[info.requested_source_count++] =
	    xr_hand_tracking_data_source_to_xrt(XR_HAND_TRACKING_DATA_SOURCE_UNOBSTRUCTED_EXT, createInfo->hand);
	info.requested_sources[info.requested_source_count++] =
	    xr_hand_tracking_data_source_to_xrt(XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT, createInfo->hand);

#ifdef OXR_HAVE_EXT_hand_tracking_data_source
	const XrHandTrackingDataSourceInfoEXT *data_source_info = NULL;
	if (sess->sys->inst->extensions.EXT_hand_tracking_data_source) {
		data_source_info = OXR_GET_INPUT_FROM_CHAIN(createInfo, XR_TYPE_HAND_TRACKING_DATA_SOURCE_INFO_EXT,
		                                            XrHandTrackingDataSourceInfoEXT);
	}

	if (data_source_info != NULL) {
		const uint32_t source_count =
		    MIN(data_source_info->requestedDataSourceCount, ARRAY_SIZE(info.requested_sources));

		// Will overwrite default priority.
		info.requested_source_count = 0;
		for (uint32_t i = 0; i < source_count; ++i) {
			info.requested_sources[info.requested_source_count++] = xr_hand_tracking_data_source_to_xrt(
			    data_source_info->requestedDataSources[i], createInfo->hand);
		}

		// For validation in locate joints.
		hand_tracker->has_requested_sources = info.requested_source_count > 0;
	}
#endif

	/*
	 * Ignore app and default priority, this might get changed in the future
	 * but for now we keep what we have been doing. The extension doesn't
	 * specify that the runtime should obey the order from the app.
	 */
	sort_hand_tracking_sources(&info);
	if (debug_get_bool_option_hand_tracking_prioritize_conforming()) {
		swap_hand_tracking_sources(&info);
	}

	xrt_result_t xret = xrt_system_devices_create_hand_tracker(sess->sys->xsysd, &info, &hand_tracker->xht);
	if (xret == XRT_ERROR_FEATURE_NOT_SUPPORTED) {
		oxr_handle_destroy(log, &hand_tracker->handle);
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED,
		                 "Did not find any hand-tracking inputs that matched the given hand.");
	}
	if (xret != XRT_SUCCESS) {
		oxr_handle_destroy(log, &hand_tracker->handle);
		OXR_CHECK_XRET(log, sess, xret, xrt_system_devices_create_hand_tracker);
	}

	*out_hand_tracker = hand_tracker;

	return XR_SUCCESS;
}

XrResult
oxr_hand_tracker_joints(struct oxr_logger *log,
                        struct oxr_hand_tracker *hand_tracker,
                        const XrHandJointsLocateInfoEXT *locateInfo,
                        XrHandJointLocationsEXT *locations)
{
	XrResult ret = XR_SUCCESS;

	struct oxr_space *baseSpc = XRT_CAST_OXR_HANDLE_TO_PTR(struct oxr_space *, locateInfo->baseSpace);

	struct oxr_session *sess = hand_tracker->sess;
	struct oxr_instance *inst = sess->sys->inst;

	XrHandJointVelocitiesEXT *vel =
	    OXR_GET_OUTPUT_FROM_CHAIN(locations, XR_TYPE_HAND_JOINT_VELOCITIES_EXT, XrHandJointVelocitiesEXT);

#ifdef OXR_HAVE_EXT_hand_tracking_data_source
	XrHandTrackingDataSourceStateEXT *data_source_state = NULL;
	if (hand_tracker->sess->sys->inst->extensions.EXT_hand_tracking_data_source) {
		data_source_state = OXR_GET_OUTPUT_FROM_CHAIN(locations, XR_TYPE_HAND_TRACKING_DATA_SOURCE_STATE_EXT,
		                                              XrHandTrackingDataSourceStateEXT);
	}
#endif

	const XrTime at_time = locateInfo->time;

	//! Convert at_time to monotonic and give to device.
	const int64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(inst->timekeeping, at_time);

	struct xrt_space *xbase = NULL;
	ret = oxr_space_get_xrt_space(log, baseSpc, &xbase);
	if (ret != XR_SUCCESS || xbase == NULL) {
		goto hand_tracking_inactive;
	}

	struct xrt_hand_tracker_location location = XRT_STRUCT_INIT;
	xrt_result_t xret = xrt_hand_tracker_locate(hand_tracker->xht, sess->sys->xso, xbase, &baseSpc->pose,
	                                            at_timestamp_ns, &location);
	xrt_space_reference(&xbase, NULL);
	OXR_CHECK_XRET_GOTO(log, sess, xret, xrt_hand_tracker_locate, ret, hand_tracking_inactive);

	if (!location.is_active) {
		goto hand_tracking_inactive;
	}

#ifdef OXR_HAVE_EXT_hand_tracking_data_source
	if (data_source_state != NULL) {
		data_source_state->isActive = XR_TRUE;
		data_source_state->dataSource = xrt_hand_tracking_data_source_to_xr(location.source);
	}
#endif

	// We know we are active.
	locations->isActive = true;

	for (uint32_t i = 0; i < locations->jointCount; i++) {
		struct xrt_space_relation result = location.hand_joint_set.values.hand_joint_set_default[i].relation;

		locations->jointLocations[i].locationFlags = xrt_to_xr_space_location_flags(result.relation_flags);
		locations->jointLocations[i].radius = location.hand_joint_set.values.hand_joint_set_default[i].radius;

		xrt_to_xr_pose(&result.pose, &locations->jointLocations[i].pose);

		if (vel) {
			XrHandJointVelocityEXT *v = &vel->jointVelocities[i];

			v->velocityFlags = 0;
			if ((result.relation_flags & XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT)) {
				v->velocityFlags |= XR_SPACE_VELOCITY_LINEAR_VALID_BIT;
			}
			if ((result.relation_flags & XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT)) {
				v->velocityFlags |= XR_SPACE_VELOCITY_ANGULAR_VALID_BIT;
			}

			v->linearVelocity.x = result.linear_velocity.x;
			v->linearVelocity.y = result.linear_velocity.y;
			v->linearVelocity.z = result.linear_velocity.z;

			v->angularVelocity.x = result.angular_velocity.x;
			v->angularVelocity.y = result.angular_velocity.y;
			v->angularVelocity.z = result.angular_velocity.z;
		}
	}

	return ret;

hand_tracking_inactive:
	locations->isActive = XR_FALSE;

#ifdef OXR_HAVE_EXT_hand_tracking_data_source
	if (data_source_state != NULL) {
		data_source_state->isActive = XR_FALSE;
	}
#endif

	// Loop over all joints and zero flags.
	for (uint32_t i = 0; i < locations->jointCount; i++) {
		locations->jointLocations[i].locationFlags = XRT_SPACE_RELATION_BITMASK_NONE;
		if (vel) {
			vel->jointVelocities[i].velocityFlags = XRT_SPACE_RELATION_BITMASK_NONE;
		}
	}

	return ret;
}

#endif // OXR_HAVE_EXT_hand_tracking


/*
 *
 * XR_MNDX_force_feedback_curl
 *
 */

#ifdef XR_MNDX_force_feedback_curl

XrResult
oxr_hand_tracker_apply_force_feedback(struct oxr_logger *log,
                                      struct oxr_hand_tracker *hand_tracker,
                                      const XrForceFeedbackCurlApplyLocationsMNDX *locations)
{
	struct xrt_output_value result = {0};
	result.type = XRT_OUTPUT_VALUE_TYPE_FORCE_FEEDBACK;
	result.force_feedback.force_feedback_location_count = locations->locationCount;
	for (uint32_t i = 0; i < locations->locationCount; i++) {
		result.force_feedback.force_feedback[i].location =
		    (enum xrt_force_feedback_location)locations->locations[i].location;
		result.force_feedback.force_feedback[i].value = locations->locations[i].value;
	}

	xrt_result_t xret = xrt_hand_tracker_set_output(hand_tracker->xht,
	                                                xr_hand_to_force_feedback_output(hand_tracker->hand), &result);
	if (xret != XRT_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "xrt_hand_tracker_set_output failed");
	}

	return XR_SUCCESS;
}

#endif // XR_MNDX_force_feedback_curl
