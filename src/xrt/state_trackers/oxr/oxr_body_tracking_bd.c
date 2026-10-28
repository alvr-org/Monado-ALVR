// Copyright 2024, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  BD (PICO) body tracking implementation.
 * @ingroup oxr_main
 */

#include "xrt/xrt_body_tracker.h"
#include "xrt/xrt_space.h"

#include "math/m_api.h"
#include "math/m_mathinclude.h"
#include "math/m_space.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle_base.h"
#include "oxr_conversions.h"
#include "oxr_chain.h"
#include "oxr_roles.h"
#include "oxr_xret.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef OXR_HAVE_BD_body_tracking

static enum xrt_body_joint_set_type_bd
oxr_to_xrt_body_joint_set_type_bd(XrBodyJointSetBD joint_set_type)
{
	if (joint_set_type == XR_BODY_JOINT_SET_BODY_WITHOUT_ARM_BD) {
		return XRT_BODY_JOINT_SET_BODY_WITHOUT_ARM_BD;
	}
	if (joint_set_type == XR_BODY_JOINT_SET_FULL_BODY_JOINTS_BD) {
		return XRT_BODY_JOINT_SET_FULL_BODY_BD;
	}
	return XRT_BODY_JOINT_SET_UNKNOWN_BD;
}

static enum xrt_input_name
xrt_body_joint_set_type_bd_to_input_name(enum xrt_body_joint_set_type_bd joint_set_type)
{
	switch (joint_set_type) {
	case XRT_BODY_JOINT_SET_FULL_BODY_BD: return XRT_INPUT_BD_BODY_TRACKING;
	case XRT_BODY_JOINT_SET_BODY_WITHOUT_ARM_BD: return XRT_INPUT_BD_BODY_TRACKING_WITHOUT_ARM;
	default: break;
	}
	return (enum xrt_input_name) - 1;
}

static XrResult
oxr_body_tracker_bd_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_body_tracker_bd *body_tracker_bd = (struct oxr_body_tracker_bd *)hb;
	xrt_body_tracker_destroy(&body_tracker_bd->xbt);
	free(body_tracker_bd);
	return XR_SUCCESS;
}

XrResult
oxr_create_body_tracker_bd(struct oxr_logger *log,
                           struct oxr_session *sess,
                           const XrBodyTrackerCreateInfoBD *createInfo,
                           struct oxr_body_tracker_bd **out_body_tracker_bd)
{
	if (!oxr_system_get_body_tracking_bd_support(log, sess->sys->inst)) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "System does not support BD body tracking");
	}

	const enum xrt_body_joint_set_type_bd joint_set_type = oxr_to_xrt_body_joint_set_type_bd(createInfo->jointSet);

	if (joint_set_type == XRT_BODY_JOINT_SET_UNKNOWN_BD) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED,
		                 "\"jointSet\" set to an unknown body joint set type");
	}

	const enum xrt_input_name input_name = xrt_body_joint_set_type_bd_to_input_name(joint_set_type);
	if (input_name == (enum xrt_input_name) - 1) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED,
		                 "\"jointSet\" set to an unknown body joint set type");
	}

	struct oxr_body_tracker_bd *body_tracker_bd = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, body_tracker_bd, OXR_XR_DEBUG_BTRACKER_BD, oxr_body_tracker_bd_destroy_cb,
	                              &sess->handle);

	body_tracker_bd->sess = sess;
	body_tracker_bd->joint_set_type = joint_set_type;

	struct xrt_body_tracker_create_info info = {
	    .body_tracking_type = input_name,
	    .locked_xdev = NULL,
	};
	xrt_result_t xret = xrt_system_devices_create_body_tracker(sess->sys->xsysd, &info, &body_tracker_bd->xbt);
	if (xret != XRT_SUCCESS) {
		oxr_handle_destroy(log, &body_tracker_bd->handle);
		// To get good logging and correct error code.
		if (xret == XRT_ERROR_FEATURE_NOT_SUPPORTED) {
			return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED,
			                 "System reported body tracking is unsupported");
		}
		// To get good logging and set the instance lost state if needed.
		OXR_CHECK_XRET(log, sess, xret, xrt_system_devices_create_body_tracker);
	}

	*out_body_tracker_bd = body_tracker_bd;
	return XR_SUCCESS;
}

XrResult
oxr_locate_body_joints_bd(struct oxr_logger *log,
                          struct oxr_body_tracker_bd *body_tracker_bd,
                          struct oxr_space *base_spc,
                          const XrBodyJointsLocateInfoBD *locateInfo,
                          XrBodyJointLocationsBD *locations)
{
	const bool is_full_body = body_tracker_bd->joint_set_type == XRT_BODY_JOINT_SET_FULL_BODY_BD;
	const uint32_t body_joint_count = is_full_body ? XRT_BODY_JOINT_COUNT_BD : XRT_BODY_JOINT_WITHOUT_ARM_COUNT_BD;

	if (locations->jointLocationCount < body_joint_count) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "joint count is too small");
	}

	if (locateInfo->time <= (XrTime)0) {
		return oxr_error(log, XR_ERROR_TIME_INVALID, "(time == %" PRIi64 ") is not a valid time.",
		                 locateInfo->time);
	}

	const struct oxr_instance *inst = body_tracker_bd->sess->sys->inst;
	const uint64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(inst->timekeeping, locateInfo->time);

	struct xrt_space *xbase = NULL;
	XrResult ret = oxr_space_get_xrt_space(log, base_spc, &xbase);
	if (ret != XR_SUCCESS || xbase == NULL) {
		locations->allJointPosesTracked = XR_FALSE;
		for (size_t joint_index = 0; joint_index < body_joint_count; ++joint_index) {
			locations->jointLocations[joint_index].locationFlags = 0;
		}
		return ret;
	}

	struct xrt_body_tracker_location loc = XRT_STRUCT_INIT;
	xrt_result_t xret = xrt_body_tracker_locate(body_tracker_bd->xbt, body_tracker_bd->sess->sys->xso, xbase,
	                                            &base_spc->pose, at_timestamp_ns, &loc);
	xrt_space_reference(&xbase, NULL);
	if (xret != XRT_SUCCESS) {
		locations->allJointPosesTracked = XR_FALSE;
		for (size_t joint_index = 0; joint_index < body_joint_count; ++joint_index) {
			locations->jointLocations[joint_index].locationFlags = 0;
		}
		return XR_SUCCESS;
	}

	struct xrt_body_joint_set body_joint_set_result = loc.joint_set;
	const struct xrt_space_relation T_base_body = loc.base_body;

	// Access BD-specific joint data
	const struct xrt_body_joint_set_bd *bd_joint_set = &body_joint_set_result.body_joint_set_bd;

	locations->allJointPosesTracked = bd_joint_set->all_joint_poses_tracked ? XR_TRUE : XR_FALSE;

	if (!loc.is_active || T_base_body.relation_flags == 0) {
		locations->allJointPosesTracked = XR_FALSE;
		for (size_t joint_index = 0; joint_index < body_joint_count; ++joint_index) {
			locations->jointLocations[joint_index].locationFlags = 0;
		}
		return XR_SUCCESS;
	}

	for (size_t joint_index = 0; joint_index < body_joint_count; ++joint_index) {
		const struct xrt_body_joint_location_bd *src_joint = &bd_joint_set->joint_locations[joint_index];
		XrBodyJointLocationBD *dst_joint = &locations->jointLocations[joint_index];

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

bool
oxr_system_get_body_tracking_bd_support(struct oxr_logger *log, struct oxr_instance *inst)
{
	struct xrt_device *xdev = GET_STATIC_XDEV_BY_ROLE(&inst->system, body);
	if (xdev == NULL) {
		return false;
	}
	return xdev->supported.body_tracking;
}

#endif // OXR_HAVE_BD_body_tracking
