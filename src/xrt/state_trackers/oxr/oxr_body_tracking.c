// Copyright 2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  body tracking related API entrypoint functions.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math/m_api.h"
#include "math/m_mathinclude.h"
#include "math/m_space.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle.h"
#include "oxr_conversions.h"

static enum xrt_body_joint_set_type_fb
oxr_to_xrt_body_joint_set_type_fb(XrBodyJointSetFB joint_set_type)
{
	if (joint_set_type == XR_BODY_JOINT_SET_DEFAULT_FB) {
		return XRT_BODY_JOINT_SET_DEFAULT_FB;
	}
	return XRT_BODY_JOINT_SET_UNKNOWN;
}

static XrResult
oxr_body_tracker_fb_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_body_tracker_fb *body_tracker_fb = (struct oxr_body_tracker_fb *)hb;
	free(body_tracker_fb);
	return XR_SUCCESS;
}

XrResult
oxr_create_body_tracker_fb(struct oxr_logger *log,
                           struct oxr_session *sess,
                           const XrBodyTrackerCreateInfoFB *createInfo,
                           struct oxr_body_tracker_fb **out_body_tracker_fb)
{

	if (!oxr_system_get_body_tracking_fb_support(log, sess->sys->inst)) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "System does not support FB body tracking");
	}

	const enum xrt_body_joint_set_type_fb joint_set_type =
	    oxr_to_xrt_body_joint_set_type_fb(createInfo->bodyJointSet);

	if (joint_set_type == XRT_BODY_JOINT_SET_UNKNOWN) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED,
		                 "\"bodyJointSet\" set to an unknown body joint set type");
	}

	struct xrt_device *xdev = GET_XDEV_BY_ROLE(sess->sys, body);
	if (xdev == NULL || !xdev->body_tracking_supported) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "No device found for body tracking role");
	}

	struct oxr_body_tracker_fb *body_tracker_fb = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, body_tracker_fb, OXR_XR_DEBUG_BTRACKER, oxr_body_tracker_fb_destroy_cb,
	                              &sess->handle);

	body_tracker_fb->sess = sess;
	body_tracker_fb->xdev = xdev;
	body_tracker_fb->joint_set_type = joint_set_type;

	*out_body_tracker_fb = body_tracker_fb;
	return XR_SUCCESS;
}

XrResult
oxr_get_body_skeleton_fb(struct oxr_logger *log,
                         struct oxr_body_tracker_fb *body_tracker_fb,
                         XrBodySkeletonFB *skeleton)
{

	if (body_tracker_fb->xdev == NULL || !body_tracker_fb->xdev->body_tracking_supported) {
		return oxr_error(log, XR_ERROR_FUNCTION_UNSUPPORTED,
		                 "Device not found or does not support body tracking.");
	}

	if (skeleton->jointCount < XRT_BODY_JOINT_COUNT_FB) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "joint count is too small");
	}

	struct xrt_body_skeleton body_skeleton_result = {0};
	struct xrt_body_skeleton_joint_fb *src_skeleton_joints = body_skeleton_result.body_skeleton_fb.joints;

	if (xrt_device_get_body_skeleton(body_tracker_fb->xdev, XRT_INPUT_FB_BODY_TRACKING, &body_skeleton_result) !=
	    XRT_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to get body skeleton");
	}

	for (size_t joint_index = 0; joint_index < XRT_BODY_JOINT_COUNT_FB; ++joint_index) {
		const struct xrt_body_skeleton_joint_fb *src_skeleton_joint = &src_skeleton_joints[joint_index];
		XrBodySkeletonJointFB *dst_skeleton_joint = &skeleton->joints[joint_index];
		OXR_XRT_POSE_TO_XRPOSEF(src_skeleton_joint->pose, dst_skeleton_joint->pose);
		dst_skeleton_joint->joint = src_skeleton_joint->joint;
		dst_skeleton_joint->parentJoint = src_skeleton_joint->parent_joint;
	}
	return XR_SUCCESS;
}

XrResult
oxr_locate_body_joints_fb(struct oxr_logger *log,
                          struct oxr_body_tracker_fb *body_tracker_fb,
                          struct oxr_space *base_spc,
                          const XrBodyJointsLocateInfoFB *locateInfo,
                          XrBodyJointLocationsFB *locations)
{
	if (body_tracker_fb->xdev == NULL || !body_tracker_fb->xdev->body_tracking_supported) {
		return oxr_error(log, XR_ERROR_FUNCTION_UNSUPPORTED,
		                 "Device not found or does not support body tracking.");
	}

	if (locations->jointCount < XRT_BODY_JOINT_COUNT_FB) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "joint count is too small");
	}

	if (locateInfo->time <= (XrTime)0) {
		return oxr_error(log, XR_ERROR_TIME_INVALID, "(time == %" PRIi64 ") is not a valid time.",
		                 locateInfo->time);
	}

	const struct oxr_instance *inst = body_tracker_fb->sess->sys->inst;
	const uint64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(inst->timekeeping, locateInfo->time);

	struct xrt_body_joint_set body_joint_set_result = {0};
	const struct xrt_body_joint_location_fb *src_body_joints =
	    body_joint_set_result.body_joint_set_fb.joint_locations;

	if (xrt_device_get_body_joints(body_tracker_fb->xdev, XRT_INPUT_FB_BODY_TRACKING, at_timestamp_ns,
	                               &body_joint_set_result) != XRT_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to get FB body joint set");
	}

	// Get the body pose in the base space.
	struct xrt_space_relation T_base_body;
	const XrResult ret = oxr_get_base_body_pose(log, &body_joint_set_result, base_spc, body_tracker_fb->xdev,
	                                            locateInfo->time, &T_base_body);
	if (ret != XR_SUCCESS) {
		locations->isActive = XR_FALSE;
		return ret;
	}

	const struct xrt_base_body_joint_set_meta *body_joint_set_fb = &body_joint_set_result.base_body_joint_set_meta;

	locations->isActive = body_joint_set_fb->is_active;
	if (!body_joint_set_fb->is_active || T_base_body.relation_flags == 0) {
		locations->isActive = XR_FALSE;
		for (size_t joint_index = 0; joint_index < XRT_BODY_JOINT_COUNT_FB; ++joint_index) {
			locations->jointLocations[joint_index].locationFlags = XRT_SPACE_RELATION_BITMASK_NONE;
		}
		return XR_SUCCESS;
	}

	locations->time = time_state_monotonic_to_ts_ns(inst->timekeeping, body_joint_set_fb->sample_time_ns);

	locations->confidence = body_joint_set_fb->confidence;
	locations->skeletonChangedCount = body_joint_set_fb->skeleton_changed_count;

	for (size_t joint_index = 0; joint_index < XRT_BODY_JOINT_COUNT_FB; ++joint_index) {
		const struct xrt_body_joint_location_fb *src_joint = &src_body_joints[joint_index];
		XrBodyJointLocationFB *dst_joint = &locations->jointLocations[joint_index];

		dst_joint->locationFlags = xrt_to_xr_space_location_flags(src_joint->relation.relation_flags);

		struct xrt_space_relation result;
		struct xrt_relation_chain chain = {0};
		m_relation_chain_push_relation(&chain, &src_joint->relation);
		m_relation_chain_push_relation(&chain, &T_base_body);
		m_relation_chain_resolve(&chain, &result);
		OXR_XRT_POSE_TO_XRPOSEF(result.pose, dst_joint->pose);
	}
	return XR_SUCCESS;
}
