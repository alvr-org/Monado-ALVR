// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Base implementation of @ref xrt_hand_tracker.
 * @ingroup base
 */

#include "b_hand_tracker.h"
#include "b_hand_tracker.hpp"

#include "xrt/xrt_device.h"
#include "xrt/xrt_space.h"

#include "math/m_space.h"

#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_misc.h"
#include "util/u_pretty_print.h"

#include <cassert>
#include <memory>


DEBUG_GET_ONCE_LOG_OPTION(hand_tracker_log, "B_HAND_TRACKER_LOG", U_LOGGING_INFO)

#define BHT_TRACE(...) U_LOG_IFL_T(debug_get_log_option_hand_tracker_log(), __VA_ARGS__)
#define BHT_DEBUG(...) U_LOG_IFL_D(debug_get_log_option_hand_tracker_log(), __VA_ARGS__)
#define BHT_INFO(...) U_LOG_IFL_I(debug_get_log_option_hand_tracker_log(), __VA_ARGS__)
#define BHT_WARN(...) U_LOG_IFL_W(debug_get_log_option_hand_tracker_log(), __VA_ARGS__)
#define BHT_ERROR(...) U_LOG_IFL_E(debug_get_log_option_hand_tracker_log(), __VA_ARGS__)

namespace {

static xrt_input_name
get_unobstructed_input_name(xrt_hand hand)
{
	return hand == XRT_HAND_LEFT ? XRT_INPUT_HT_UNOBSTRUCTED_LEFT : XRT_INPUT_HT_UNOBSTRUCTED_RIGHT;
}

static xrt_input_name
get_conforming_input_name(xrt_hand hand)
{
	return hand == XRT_HAND_LEFT ? XRT_INPUT_HT_CONFORMING_LEFT : XRT_INPUT_HT_CONFORMING_RIGHT;
}

static bool
is_hand_tracker_input_name(xrt_input_name input_name, xrt_hand hand)
{
	return input_name == get_unobstructed_input_name(hand) || input_name == get_conforming_input_name(hand);
}

static const char *
mode_to_string(HandTrackerMode mode)
{
	switch (mode) {
	case HandTrackerMode::System: return "system";
	case HandTrackerMode::DeviceLocked: return "device_locked";
	case HandTrackerMode::Callback: return "callback";
	default: return "unknown";
	}
}

static const char *
hand_to_string(xrt_hand hand)
{
	switch (hand) {
	case XRT_HAND_LEFT: return "left";
	case XRT_HAND_RIGHT: return "right";
	default: return "unknown";
	}
}

static const char *
xdev_name(xrt_device *xdev)
{
	return xdev != nullptr ? xdev->str : "<none>";
}

static xrt_device *
get_role_xdev(xrt_system_devices *xsysd, xrt_input_name input_name)
{
	switch (input_name) {
	case XRT_INPUT_HT_UNOBSTRUCTED_LEFT: return xsysd->static_roles.hand_tracking.unobstructed.left;
	case XRT_INPUT_HT_UNOBSTRUCTED_RIGHT: return xsysd->static_roles.hand_tracking.unobstructed.right;
	case XRT_INPUT_HT_CONFORMING_LEFT: return xsysd->static_roles.hand_tracking.conforming.left;
	case XRT_INPUT_HT_CONFORMING_RIGHT: return xsysd->static_roles.hand_tracking.conforming.right;
	default: return nullptr;
	}
}

static bool
xdev_has_input(struct xrt_device *xdev, enum xrt_input_name input_name)
{
	if (xdev == nullptr || !xdev->supported.hand_tracking) {
		return false;
	}

	for (uint32_t i = 0; i < xdev->input_count; i++) {
		if (xdev->inputs[i].name == input_name) {
			return true;
		}
	}

	return false;
}

static void
resolve_joint_relation(const xrt_space_relation &joint_relation,
                       const xrt_space_relation &base_hand_relation,
                       xrt_space_relation &out_relation)
{
	xrt_relation_chain chain = {};
	m_relation_chain_push_relation(&chain, &joint_relation);
	m_relation_chain_push_relation(&chain, &base_hand_relation);
	m_relation_chain_resolve(&chain, &out_relation);
}

static xrt_result_t
create_hand_tracker(xrt_system_devices *xsysd,
                    const xrt_hand_tracker_create_info *info,
                    xrt_hand_tracker **out_xht,
                    b_hand_tracker_get_devices_func_t getDevices,
                    void *data)
{
	assert(info != nullptr);
	assert(out_xht != nullptr);

	*out_xht = nullptr;

	auto hand_tracker = std::make_unique<HandTracker>(xsysd, *info, getDevices, data);
	xrt_result_t xret = hand_tracker->init();
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	*out_xht = hand_tracker.release()->getXHT();

	return XRT_SUCCESS;
}

} // namespace

HandTracker::HandTracker(xrt_system_devices *xsysd,
                         const xrt_hand_tracker_create_info &info,
                         b_hand_tracker_get_devices_func_t getDevices,
                         void *callbackData)
    : mSystemDevices(xsysd), mInfo(info), mGetDevices(getDevices), mCallbackData(callbackData)
{
	if (info.locked_xdev != nullptr) {
		mMode = HandTrackerMode::DeviceLocked;
	} else if (getDevices != nullptr) {
		mMode = HandTrackerMode::Callback;
	} else {
		mMode = HandTrackerMode::System;
	}
}

xrt_result_t
HandTracker::init()
{
	assert(mSystemDevices != nullptr);
	assert(mInfo.hand == XRT_HAND_LEFT || mInfo.hand == XRT_HAND_RIGHT);
	assert(mInfo.requested_source_count <= ARRAY_SIZE(mInfo.requested_sources));

	if (mInfo.requested_source_count == 0) {
		mInfo.requested_sources[mInfo.requested_source_count++] = get_unobstructed_input_name(mInfo.hand);
		mInfo.requested_sources[mInfo.requested_source_count++] = get_conforming_input_name(mInfo.hand);
	}

	for (uint32_t i = 0; i < mInfo.requested_source_count; i++) {
		xrt_input_name input_name = mInfo.requested_sources[i];
		if (!is_hand_tracker_input_name(input_name, mInfo.hand)) {
			return XRT_ERROR_INVALID_ARGUMENT;
		}

		xrt_result_t xret = XRT_SUCCESS;
		if (mMode == HandTrackerMode::Callback) {
			xret = addCallbackInputs(input_name);
		} else {
			Source source{};
			xret = resolveSource(input_name, source);
			if (xret == XRT_SUCCESS) {
				xret = addInput(source);
				addOutput(source.xdev);
			}
		}

		if (xret != XRT_SUCCESS) {
			return xret;
		}
	}

	if (mInputCount == 0) {
		return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	u_pp(dg,
	     "Created hand tracker:"
	     "\n\tmode: %s"
	     "\n\thand: %s"
	     "\n\tlocked xdev: %s"
	     "\n\tinput count: %u"
	     "\n\toutput count: %u",
	     mode_to_string(mMode),        //
	     hand_to_string(mInfo.hand),   //
	     xdev_name(mInfo.locked_xdev), //
	     mInputCount,                  //
	     mOutputCount);                //

	for (uint32_t i = 0; i < mInputCount; i++) {
		const Source &source = mInputs[i];
		u_pp(dg, "\n\tinput[%u]: ", i);
		u_pp_xrt_input_name(dg, source.input_name);
		u_pp(dg, " xdev='%s'", xdev_name(source.xdev));
	}

	BHT_INFO("%s", sink.buffer);

	return XRT_SUCCESS;
}

xrt_result_t
HandTracker::locate(xrt_space_overseer *xso,
                    xrt_space *base_space,
                    const xrt_pose *base_offset,
                    int64_t at_timestamp_ns,
                    xrt_hand_tracker_location *out_location)
{
	assert(xso != nullptr);
	assert(base_space != nullptr);
	assert(out_location != nullptr);

	*out_location = {};

	xrt_hand_joint_set value = {};
	const Source *active_source = nullptr;
	for (uint32_t i = 0; i < mInputCount; i++) {
		const Source &source = mInputs[i];
		if (source.xdev == nullptr) {
			continue;
		}

		int64_t ignored = 0;
		value = {};
		xrt_result_t xret = xrt_device_get_hand_tracking( //
		    source.xdev,                                  //
		    source.input_name,                            //
		    at_timestamp_ns,                              //
		    &value,                                       //
		    &ignored);                                    //
		if (xret != XRT_SUCCESS) {
			return xret;
		}

		if (value.is_active) {
			active_source = &source;
			break;
		}
	}

	if (active_source == nullptr) {
		return XRT_SUCCESS;
	}

	const struct xrt_pose identity = XRT_POSE_IDENTITY;
	const struct xrt_pose *offset = base_offset != nullptr ? base_offset : &identity;

	struct xrt_space_relation T_base_xdev = XRT_SPACE_RELATION_ZERO;
	xrt_result_t xret = xrt_space_overseer_locate_device( //
	    xso,                                              //
	    base_space,                                       //
	    offset,                                           //
	    at_timestamp_ns,                                  //
	    active_source->xdev,                              //
	    &T_base_xdev);                                    //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	if (T_base_xdev.relation_flags == 0) {
		return XRT_SUCCESS;
	}

	struct xrt_space_relation T_base_hand = XRT_SPACE_RELATION_ZERO;
	{
		struct xrt_relation_chain chain = {};
		m_relation_chain_push_relation(&chain, &value.hand_pose);
		m_relation_chain_push_relation(&chain, &T_base_xdev);
		m_relation_chain_resolve(&chain, &T_base_hand);
	}

	if (T_base_hand.relation_flags == 0) {
		return XRT_SUCCESS;
	}

	out_location->hand_joint_set = value;
	out_location->hand_joint_set.hand_pose = T_base_hand;
	out_location->hand_joint_set.is_active = true;
	out_location->source = active_source->input_name;
	out_location->is_active = true;

	for (uint32_t i = 0; i < XRT_HAND_JOINT_COUNT; i++) {
		struct xrt_space_relation joint_relation = value.values.hand_joint_set_default[i].relation;
		struct xrt_space_relation result = XRT_SPACE_RELATION_ZERO;
		resolve_joint_relation(joint_relation, T_base_hand, result);
		out_location->hand_joint_set.values.hand_joint_set_default[i].relation = result;
	}

	return XRT_SUCCESS;
}

xrt_result_t
HandTracker::setOutput(xrt_output_name name, const xrt_output_value *value)
{
	assert(value != nullptr);

	for (xrt_device *xdev : mOutputs) {
		if (xdev == nullptr) {
			continue;
		}

		xrt_result_t xret = xrt_device_set_output(xdev, name, value);
		if (xret != XRT_SUCCESS) {
			return xret;
		}
	}

	return XRT_SUCCESS;
}

void
HandTracker::destroyHandTracker(struct xrt_hand_tracker *xht)
{
	delete HandTracker::fromXHT(xht);
}


/*
 *
 * Private functions.
 *
 */

xrt_result_t
HandTracker::resolveSource(xrt_input_name input_name, Source &out_source)
{
	assert(is_hand_tracker_input_name(input_name, mInfo.hand));

	xrt_device *xdev{nullptr};
	switch (mMode) {
	case HandTrackerMode::DeviceLocked: xdev = mInfo.locked_xdev; break;
	case HandTrackerMode::System: xdev = get_role_xdev(mSystemDevices, input_name); break;
	case HandTrackerMode::Callback: assert(false); return XRT_ERROR_INVALID_ARGUMENT;
	}

	// Does null check for xdev.
	if (!xdev_has_input(xdev, input_name)) {
		out_source = {};
		return XRT_SUCCESS;
	}

	out_source = {
	    .xdev = xdev,
	    .input_name = input_name,
	};

	return XRT_SUCCESS;
}

xrt_result_t
HandTracker::addCallbackInputs(xrt_input_name input_name)
{
	assert(is_hand_tracker_input_name(input_name, mInfo.hand));
	assert(mGetDevices != nullptr);

	std::array<xrt_device *, XRT_SYSTEM_MAX_DEVICES> xdevs{};
	uint32_t xdev_count = 0;
	xrt_result_t xret = mGetDevices( //
	    input_name,                  //
	    mSystemDevices,              //
	    mCallbackData,               //
	    (uint32_t)xdevs.size(),      //
	    xdevs.data(),                //
	    &xdev_count);                //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	if (xdev_count > xdevs.size()) {
		BHT_ERROR("xdev_count > xdevs.size() should not happen!");
		assert(false);
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	for (uint32_t i = 0; i < xdev_count; i++) {
		xrt_device *xdev = xdevs[i];
		if (!xdev_has_input(xdev, input_name)) {
			continue;
		}

		xret = addInput({
		    .xdev = xdev,
		    .input_name = input_name,
		});
		if (xret != XRT_SUCCESS) {
			return xret;
		}
	}

	return XRT_SUCCESS;
}

xrt_result_t
HandTracker::addInput(const Source &source)
{
	if (source.xdev == nullptr) {
		return XRT_SUCCESS;
	}

	// Remove duplicates
	for (uint32_t i = 0; i < mInputCount; i++) {
		if (mInputs[i].xdev == source.xdev && mInputs[i].input_name == source.input_name) {
			return XRT_SUCCESS;
		}
	}

	if (mInputCount >= mInputs.size()) {
		BHT_WARN("Dropping xdev '%s' from hand tracking input array", source.xdev->str);
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	mInputs[mInputCount++] = source;

	return XRT_SUCCESS;
}

void
HandTracker::addOutput(xrt_device *xdev)
{
	if (xdev == nullptr) {
		return;
	}

	// De-duplicate
	for (uint32_t i = 0; i < mOutputCount; i++) {
		if (mOutputs[i] == xdev) {
			return;
		}
	}

	if (mOutputCount >= mOutputs.size()) {
		BHT_WARN("Dropping xdev '%s' from hand tracking output array", xdev->str);
		return;
	}

	mOutputs[mOutputCount++] = xdev;
}


/*
 *
 * 'Exported' functions.
 *
 */

extern "C" xrt_result_t
b_hand_tracker_create(xrt_system_devices *xsysd, const xrt_hand_tracker_create_info *info, xrt_hand_tracker **out_xht)
{
	return create_hand_tracker(xsysd, info, out_xht, nullptr, nullptr);
}

extern "C" xrt_result_t
b_hand_tracker_create_with_cb(xrt_system_devices *xsysd,
                              const xrt_hand_tracker_create_info *info,
                              b_hand_tracker_get_devices_func_t get_devices,
                              void *data,
                              xrt_hand_tracker **out_xht)
{
	return create_hand_tracker(xsysd, info, out_xht, get_devices, data);
}
