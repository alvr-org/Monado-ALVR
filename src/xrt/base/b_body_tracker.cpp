// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Base implementation of @ref xrt_body_tracker.
 * @ingroup base
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_space.h"

#include "math/m_space.h"

#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_misc.h"

#include "b_body_tracker.h"
#include "b_body_tracker.hpp"

#include <cassert>
#include <memory>


DEBUG_GET_ONCE_LOG_OPTION(body_tracker_log, "B_BODY_TRACKER_LOG", U_LOGGING_INFO)

#define BBT_TRACE(...) U_LOG_IFL_T(debug_get_log_option_body_tracker_log(), __VA_ARGS__)
#define BBT_DEBUG(...) U_LOG_IFL_D(debug_get_log_option_body_tracker_log(), __VA_ARGS__)
#define BBT_INFO(...) U_LOG_IFL_I(debug_get_log_option_body_tracker_log(), __VA_ARGS__)
#define BBT_WARN(...) U_LOG_IFL_W(debug_get_log_option_body_tracker_log(), __VA_ARGS__)
#define BBT_ERROR(...) U_LOG_IFL_E(debug_get_log_option_body_tracker_log(), __VA_ARGS__)

namespace {

static bool
is_bd_body_tracking_type(enum xrt_input_name input_name)
{
	return input_name == XRT_INPUT_BD_BODY_TRACKING || input_name == XRT_INPUT_BD_BODY_TRACKING_WITHOUT_ARM;
}

static bool
body_tracker_is_active(const struct xrt_body_joint_set *joint_set, enum xrt_input_name input_name)
{
	if (is_bd_body_tracking_type(input_name)) {
		return joint_set->body_joint_set_bd.is_active;
	}

	return joint_set->base_body_joint_set_meta.is_active;
}

static xrt_device *
resolve_xdev(xrt_system_devices *xsysd, const xrt_body_tracker_create_info *info)
{
	if (info->locked_xdev != nullptr) {
		return info->locked_xdev;
	}

	return xsysd->static_roles.body;
}

static xrt_result_t
create_body_tracker(xrt_system_devices *xsysd, const xrt_body_tracker_create_info *info, xrt_body_tracker **out_xbt)
{
	assert(info != nullptr);
	assert(out_xbt != nullptr);

	*out_xbt = nullptr;

	xrt_device *xdev = resolve_xdev(xsysd, info);
	if (xdev == nullptr || !xdev->supported.body_tracking) {
		return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	auto body_tracker = std::make_unique<BodyTracker>(*info, xdev);

	xrt_body_tracker *xbt = body_tracker.release()->getXBT();
	xbt->supported.body_tracking = xdev->supported.body_tracking;
	xbt->supported.body_tracking_calibration = xdev->supported.body_tracking_calibration;
	xbt->supported.body_tracking_fidelity = xdev->supported.body_tracking_fidelity;

	*out_xbt = xbt;

	BBT_INFO("Created body tracker for xdev='%s'", xdev->str);

	return XRT_SUCCESS;
}

} // namespace

BodyTracker::BodyTracker(const xrt_body_tracker_create_info &info, xrt_device *xdev) : mInfo(info), mXdev(xdev) {}

xrt_result_t
BodyTracker::locate(xrt_space_overseer *xso,
                    xrt_space *base_space,
                    const xrt_pose *base_offset,
                    int64_t at_timestamp_ns,
                    xrt_body_tracker_location *out_location)
{
	assert(xso != nullptr);
	assert(base_space != nullptr);
	assert(out_location != nullptr);

	*out_location = {};

	xrt_result_t xret = xrt_device_get_body_joints( //
	    mXdev,                                      //
	    mInfo.body_tracking_type,                   //
	    at_timestamp_ns,                            //
	    &out_location->joint_set);                  //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	const struct xrt_pose identity = XRT_POSE_IDENTITY;
	const struct xrt_pose *offset = base_offset != nullptr ? base_offset : &identity;

	const struct xrt_space_relation T_xdev_body = out_location->joint_set.body_pose;

	struct xrt_space_relation T_base_xdev = XRT_SPACE_RELATION_ZERO;
	xret = xrt_space_overseer_locate_device( //
	    xso,                                 //
	    base_space,                          //
	    offset,                              //
	    at_timestamp_ns,                     //
	    mXdev,                               //
	    &T_base_xdev);                       //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	if (T_base_xdev.relation_flags == 0) {
		out_location->base_body = XRT_SPACE_RELATION_ZERO;
		out_location->is_active = false;
		return XRT_SUCCESS;
	}

	struct xrt_relation_chain chain = {};
	m_relation_chain_push_relation(&chain, &T_xdev_body);
	m_relation_chain_push_relation(&chain, &T_base_xdev);
	m_relation_chain_resolve(&chain, &out_location->base_body);

	out_location->is_active = body_tracker_is_active(&out_location->joint_set, mInfo.body_tracking_type);

	return XRT_SUCCESS;
}

xrt_result_t
BodyTracker::getSkeleton(xrt_body_skeleton *out_skeleton)
{
	return xrt_device_get_body_skeleton(mXdev, mInfo.body_tracking_type, out_skeleton);
}

xrt_result_t
BodyTracker::resetCalibrationMeta()
{
	return xrt_device_reset_body_tracking_calibration_meta(mXdev);
}

xrt_result_t
BodyTracker::setCalibrationOverrideMeta(float body_height)
{
	return xrt_device_set_body_tracking_calibration_override_meta(mXdev, body_height);
}

xrt_result_t
BodyTracker::setFidelityMeta(enum xrt_body_tracking_fidelity_meta fidelity)
{
	return xrt_device_set_body_tracking_fidelity_meta(mXdev, fidelity);
}

void
BodyTracker::destroyBodyTracker(struct xrt_body_tracker *xbt)
{
	delete BodyTracker::fromXBT(xbt);
}


/*
 *
 * 'Exported' functions.
 *
 */

extern "C" xrt_result_t
b_body_tracker_create(xrt_system_devices *xsysd, const xrt_body_tracker_create_info *info, xrt_body_tracker **out_xbt)
try {
	return create_body_tracker(xsysd, info, out_xbt);
}
G_CATCH_GUARDS
