// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Smaller helper functions to convert between xrt and OpenXR things.
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_space.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_compositor.h"
#include "xrt/xrt_vulkan_includes.h"
#include "xrt/xrt_openxr_includes.h"

#include "oxr_defines.h"


/*
 *
 * Space things.
 *
 */

static inline XrSpaceLocationFlags
xrt_to_xr_space_location_flags(enum xrt_space_relation_flags relation_flags)
{
	// clang-format off
	bool valid_ori = (relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0;
	bool tracked_ori = (relation_flags & XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT) != 0;
	bool valid_pos = (relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) != 0;
	bool tracked_pos = (relation_flags & XRT_SPACE_RELATION_POSITION_TRACKED_BIT) != 0;

	bool linear_vel = (relation_flags & XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT) != 0;
	bool angular_vel = (relation_flags & XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT) != 0;
	// clang-format on

	XrSpaceLocationFlags location_flags = (XrSpaceLocationFlags)0;
	if (valid_ori) {
		location_flags |= XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
	}
	if (tracked_ori) {
		location_flags |= XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
	}
	if (valid_pos) {
		location_flags |= XR_SPACE_LOCATION_POSITION_VALID_BIT;
	}
	if (tracked_pos) {
		location_flags |= XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
	}
	if (linear_vel) {
		location_flags |= XR_SPACE_VELOCITY_LINEAR_VALID_BIT;
	}
	if (angular_vel) {
		location_flags |= XR_SPACE_VELOCITY_ANGULAR_VALID_BIT;
	}

	return location_flags;
}

static inline XrReferenceSpaceType
oxr_ref_space_to_xr(enum oxr_space_type space_type)
{
	switch (space_type) {
	case OXR_SPACE_TYPE_REFERENCE_VIEW: return XR_REFERENCE_SPACE_TYPE_VIEW;
	case OXR_SPACE_TYPE_REFERENCE_LOCAL: return XR_REFERENCE_SPACE_TYPE_LOCAL;
	case OXR_SPACE_TYPE_REFERENCE_LOCAL_FLOOR: return XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
	case OXR_SPACE_TYPE_REFERENCE_STAGE: return XR_REFERENCE_SPACE_TYPE_STAGE;
	case OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_MSFT: return XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
	case OXR_SPACE_TYPE_REFERENCE_COMBINED_EYE_VARJO: return XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO;
	case OXR_SPACE_TYPE_REFERENCE_LOCALIZATION_MAP_ML: return XR_REFERENCE_SPACE_TYPE_LOCALIZATION_MAP_ML;
	case OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_ANDROID: return XR_REFERENCE_SPACE_TYPE_UNBOUNDED_ANDROID;
	case OXR_SPACE_TYPE_REFERENCE_STATIONARY_EXT: return XR_REFERENCE_SPACE_TYPE_STATIONARY_EXT;

	case OXR_SPACE_TYPE_ACTION: return XR_REFERENCE_SPACE_TYPE_MAX_ENUM;
	case OXR_SPACE_TYPE_XDEV_POSE: return XR_REFERENCE_SPACE_TYPE_MAX_ENUM;
	}

	return XR_REFERENCE_SPACE_TYPE_MAX_ENUM;
}

static inline enum oxr_space_type
xr_ref_space_to_oxr(XrReferenceSpaceType space_type)
{
	switch (space_type) {
	case XR_REFERENCE_SPACE_TYPE_VIEW: return OXR_SPACE_TYPE_REFERENCE_VIEW;
	case XR_REFERENCE_SPACE_TYPE_LOCAL: return OXR_SPACE_TYPE_REFERENCE_LOCAL;
	case XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT: return OXR_SPACE_TYPE_REFERENCE_LOCAL_FLOOR;
	case XR_REFERENCE_SPACE_TYPE_STAGE: return OXR_SPACE_TYPE_REFERENCE_STAGE;
	case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT: return OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_MSFT;
	case XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO: return OXR_SPACE_TYPE_REFERENCE_COMBINED_EYE_VARJO;
	case XR_REFERENCE_SPACE_TYPE_LOCALIZATION_MAP_ML: return OXR_SPACE_TYPE_REFERENCE_LOCALIZATION_MAP_ML;
	case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_ANDROID: return OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_ANDROID;
	case XR_REFERENCE_SPACE_TYPE_STATIONARY_EXT: return OXR_SPACE_TYPE_REFERENCE_STATIONARY_EXT;

	case XR_REFERENCE_SPACE_TYPE_MAX_ENUM: return (enum oxr_space_type) - 1;
	}

	// wrap around or negative depending on enum data type, invalid value either way.
	return (enum oxr_space_type) - 1;
}

static inline const char *
xr_ref_space_to_string(XrReferenceSpaceType space_type)
{
	switch (space_type) {
	case XR_REFERENCE_SPACE_TYPE_VIEW: return "XR_REFERENCE_SPACE_TYPE_VIEW";
	case XR_REFERENCE_SPACE_TYPE_LOCAL: return "XR_REFERENCE_SPACE_TYPE_LOCAL";
	case XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT: return "XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT";
	case XR_REFERENCE_SPACE_TYPE_STAGE: return "XR_REFERENCE_SPACE_TYPE_STAGE";
	case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT: return "XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT";
	case XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO: return "XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO";
	case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_ANDROID: return "XR_REFERENCE_SPACE_TYPE_UNBOUNDED_ANDROID";
	case XR_REFERENCE_SPACE_TYPE_STATIONARY_EXT: return "XR_REFERENCE_SPACE_TYPE_STATIONARY_EXT";
	case XR_REFERENCE_SPACE_TYPE_MAX_ENUM: return "XR_REFERENCE_SPACE_TYPE_MAX_ENUM";
	default: return "UNKNOWN REFERENCE SPACE";
	}
}

static inline enum xrt_reference_space_type
oxr_ref_space_to_xrt(enum oxr_space_type space_type)
{
	switch (space_type) {
	case OXR_SPACE_TYPE_REFERENCE_VIEW: return XRT_SPACE_REFERENCE_TYPE_VIEW;
	case OXR_SPACE_TYPE_REFERENCE_LOCAL: return XRT_SPACE_REFERENCE_TYPE_LOCAL;
	case OXR_SPACE_TYPE_REFERENCE_LOCAL_FLOOR: return XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR;
	case OXR_SPACE_TYPE_REFERENCE_STAGE: return XRT_SPACE_REFERENCE_TYPE_STAGE;
	case OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_MSFT: return XRT_SPACE_REFERENCE_TYPE_UNBOUNDED;

	// Has no mapping to a Monado semantic space.
	case OXR_SPACE_TYPE_REFERENCE_COMBINED_EYE_VARJO: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case OXR_SPACE_TYPE_REFERENCE_LOCALIZATION_MAP_ML: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_ANDROID: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case OXR_SPACE_TYPE_REFERENCE_STATIONARY_EXT: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case OXR_SPACE_TYPE_ACTION: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case OXR_SPACE_TYPE_XDEV_POSE: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	}

	/*
	 * This is the default case, we do not have a explicit default case
	 * so that we get warnings for unhandled enum members. This is fine
	 * because the C specification says if there is no default case and
	 * and a non-matching value is given no case is executed.
	 */

	return XRT_SPACE_REFERENCE_TYPE_INVALID;
}

static inline enum xrt_reference_space_type
xr_ref_space_to_xrt(XrReferenceSpaceType space_type)
{
	switch (space_type) {
	case XR_REFERENCE_SPACE_TYPE_VIEW: return XRT_SPACE_REFERENCE_TYPE_VIEW;
	case XR_REFERENCE_SPACE_TYPE_LOCAL: return XRT_SPACE_REFERENCE_TYPE_LOCAL;
	case XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT: return XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR;
	case XR_REFERENCE_SPACE_TYPE_STAGE: return XRT_SPACE_REFERENCE_TYPE_STAGE;
	case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT: return XRT_SPACE_REFERENCE_TYPE_UNBOUNDED;

	case XR_REFERENCE_SPACE_TYPE_STATIONARY_EXT: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case XR_REFERENCE_SPACE_TYPE_LOCALIZATION_MAP_ML: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_ANDROID: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	case XR_REFERENCE_SPACE_TYPE_MAX_ENUM: return XRT_SPACE_REFERENCE_TYPE_INVALID;
	}

	return XRT_SPACE_REFERENCE_TYPE_INVALID;
}


/*
 *
 * Form factor things.
 *
 */

static inline enum xrt_form_factor
xr_form_factor_to_xrt(XrFormFactor form_factor)
{
	switch (form_factor) {
	case XR_FORM_FACTOR_HANDHELD_DISPLAY: return XRT_FORM_FACTOR_HANDHELD;
	case XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY: return XRT_FORM_FACTOR_HMD;
	case XR_FORM_FACTOR_MAX_ENUM: assert(false); return 0; // As good as any.
	}

	// Used as default, to get warnings.
	return XRT_FORM_FACTOR_HMD;
}

static inline enum XrFormFactor
xrt_form_factor_to_xr(enum xrt_form_factor form_factor)
{
	switch (form_factor) {
	case XRT_FORM_FACTOR_HANDHELD: return XR_FORM_FACTOR_HANDHELD_DISPLAY;
	case XRT_FORM_FACTOR_HMD: return XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	}

	// Used as default, to get warnings.
	return XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
}


/*
 *
 * IO things.
 *
 */

static inline const char *
xr_action_type_to_str(XrActionType type)
{
	// clang-format off
	switch (type) {
#define PRINT(name, value) \
	case name: return #name;
	XR_LIST_ENUM_XrActionType(PRINT)
#undef PRINT
	default: return "XR_ACTION_TYPE_UNKNOWN";
	}
	// clang-format on
}

static inline const char *
xr_action_type_to_short_str(XrActionType type)
{
	switch (type) {
	case XR_ACTION_TYPE_BOOLEAN_INPUT: return "BOOLEAN";
	case XR_ACTION_TYPE_FLOAT_INPUT: return "FLOAT";
	case XR_ACTION_TYPE_VECTOR2F_INPUT: return "VECTOR2";
	case XR_ACTION_TYPE_POSE_INPUT: return "POSE";
	case XR_ACTION_TYPE_VIBRATION_OUTPUT: return "VIBRATION";
	default: return "UNKNOWN";
	}
}

static inline const char *
xrt_input_type_to_str(enum xrt_input_type type)
{
	// clang-format off
	switch (type) {
	case XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE: return "XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE";
	case XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE: return "XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE";
	case XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE: return "XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE";
	case XRT_INPUT_TYPE_VEC3_MINUS_ONE_TO_ONE: return "XRT_INPUT_TYPE_VEC3_MINUS_ONE_TO_ONE";
	case XRT_INPUT_TYPE_BOOLEAN: return "XRT_INPUT_TYPE_BOOLEAN";
	case XRT_INPUT_TYPE_POSE: return "XRT_INPUT_TYPE_POSE";
	default: return "XRT_INPUT_UNKNOWN";
	}
	// clang-format on
}

static inline enum xrt_perf_set_level
xr_perf_level_to_xrt(XrPerfSettingsLevelEXT level)
{
	switch (level) {
	case XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT: return XRT_PERF_SET_LEVEL_POWER_SAVINGS;
	case XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT: return XRT_PERF_SET_LEVEL_SUSTAINED_LOW;
	case XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT: return XRT_PERF_SET_LEVEL_SUSTAINED_HIGH;
	case XR_PERF_SETTINGS_LEVEL_BOOST_EXT: return XRT_PERF_SET_LEVEL_BOOST;
	default: assert(false); return 0;
	}
}

static inline enum xrt_perf_domain
xr_perf_domain_to_xrt(XrPerfSettingsDomainEXT domain)
{
	switch (domain) {
	case XR_PERF_SETTINGS_DOMAIN_CPU_EXT: return XRT_PERF_DOMAIN_CPU;
	case XR_PERF_SETTINGS_DOMAIN_GPU_EXT: return XRT_PERF_DOMAIN_GPU;
	default: assert(false); return 0;
	}
}

static inline XrPerfSettingsDomainEXT
xrt_perf_domain_to_xr(enum xrt_perf_domain domain)
{
	switch (domain) {
	case XRT_PERF_DOMAIN_CPU: return XR_PERF_SETTINGS_DOMAIN_CPU_EXT;
	case XRT_PERF_DOMAIN_GPU: return XR_PERF_SETTINGS_DOMAIN_GPU_EXT;
	default: assert(false); return 0;
	}
}

static inline XrPerfSettingsSubDomainEXT
xrt_perf_sub_domain_to_xr(enum xrt_perf_sub_domain subDomain)
{
	switch (subDomain) {
	case XRT_PERF_SUB_DOMAIN_COMPOSITING: return XR_PERF_SETTINGS_SUB_DOMAIN_COMPOSITING_EXT;
	case XRT_PERF_SUB_DOMAIN_RENDERING: return XR_PERF_SETTINGS_SUB_DOMAIN_RENDERING_EXT;
	case XRT_PERF_SUB_DOMAIN_THERMAL: return XR_PERF_SETTINGS_SUB_DOMAIN_THERMAL_EXT;
	default: assert(false); return 0;
	}
}

static inline XrPerfSettingsNotificationLevelEXT
xrt_perf_notify_level_to_xr(enum xrt_perf_notify_level level)
{
	switch (level) {
	case XRT_PERF_NOTIFY_LEVEL_NORMAL: return XR_PERF_SETTINGS_NOTIF_LEVEL_NORMAL_EXT;
	case XRT_PERF_NOTIFY_LEVEL_WARNING: return XR_PERF_SETTINGS_NOTIF_LEVEL_WARNING_EXT;
	case XRT_PERF_NOTIFY_LEVEL_IMPAIRED: return XR_PERF_SETTINGS_NOTIF_LEVEL_IMPAIRED_EXT;
	default: assert(false); return 0;
	}
}

static inline enum xrt_input_name
xr_hand_tracking_data_source_to_xrt(XrHandTrackingDataSourceEXT data_source, enum XrHandEXT hand)
{
	switch (data_source) {
	case XR_HAND_TRACKING_DATA_SOURCE_UNOBSTRUCTED_EXT:
		return (hand == XR_HAND_LEFT_EXT) ? XRT_INPUT_HT_UNOBSTRUCTED_LEFT : XRT_INPUT_HT_UNOBSTRUCTED_RIGHT;
	case XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT:
		return (hand == XR_HAND_LEFT_EXT) ? XRT_INPUT_HT_CONFORMING_LEFT : XRT_INPUT_HT_CONFORMING_RIGHT;
	default: assert(false); return (enum xrt_input_name)(-1);
	}
}

static inline XrHandTrackingDataSourceEXT
xrt_hand_tracking_data_source_to_xr(enum xrt_input_name ht_input_name)
{
	switch (ht_input_name) {
	case XRT_INPUT_HT_UNOBSTRUCTED_LEFT:
	case XRT_INPUT_HT_UNOBSTRUCTED_RIGHT: return XR_HAND_TRACKING_DATA_SOURCE_UNOBSTRUCTED_EXT;
	case XRT_INPUT_HT_CONFORMING_LEFT:
	case XRT_INPUT_HT_CONFORMING_RIGHT: return XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT;
	default: assert(false); return XR_HAND_TRACKING_DATA_SOURCE_MAX_ENUM_EXT;
	}
}


/*
 *
 * Swapchain
 *
 */

static inline enum xrt_swapchain_usage_bits
xr_swapchain_usage_to_xrt(XrSwapchainUsageFlags xr_usage)
{
	enum xrt_swapchain_usage_bits usage = 0;

	if ((xr_usage & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) != 0) {
		usage |= XRT_SWAPCHAIN_USAGE_COLOR;
	}
	if ((xr_usage & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
		usage |= XRT_SWAPCHAIN_USAGE_DEPTH_STENCIL;
	}
	if ((xr_usage & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) != 0) {
		usage |= XRT_SWAPCHAIN_USAGE_UNORDERED_ACCESS;
	}
	if ((xr_usage & XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT) != 0) {
		usage |= XRT_SWAPCHAIN_USAGE_TRANSFER_SRC;
	}
	if ((xr_usage & XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT) != 0) {
		usage |= XRT_SWAPCHAIN_USAGE_TRANSFER_DST;
	}
	if ((xr_usage & XR_SWAPCHAIN_USAGE_SAMPLED_BIT) != 0) {
		usage |= XRT_SWAPCHAIN_USAGE_SAMPLED;
	}
	if ((xr_usage & XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT) != 0) {
		usage |= XRT_SWAPCHAIN_USAGE_MUTABLE_FORMAT;
	}
	// aliased to XR_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT_MND
	if ((xr_usage & XR_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT_KHR) != 0) {
		usage |= XRT_SWAPCHAIN_USAGE_INPUT_ATTACHMENT;
	}

	return usage;
}

static inline XrSwapchainUsageFlags
xrt_swapchain_usage_to_xr(enum xrt_swapchain_usage_bits bits)
{
	XrSwapchainUsageFlags xr = 0;

	if ((bits & XRT_SWAPCHAIN_USAGE_COLOR) != 0) {
		xr |= XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if ((bits & XRT_SWAPCHAIN_USAGE_DEPTH_STENCIL) != 0) {
		xr |= XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if ((bits & XRT_SWAPCHAIN_USAGE_UNORDERED_ACCESS) != 0) {
		xr |= XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT;
	}
	if ((bits & XRT_SWAPCHAIN_USAGE_TRANSFER_SRC) != 0) {
		xr |= XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
	}
	if ((bits & XRT_SWAPCHAIN_USAGE_TRANSFER_DST) != 0) {
		xr |= XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
	}
	if ((bits & XRT_SWAPCHAIN_USAGE_SAMPLED) != 0) {
		xr |= XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
	}
	if ((bits & XRT_SWAPCHAIN_USAGE_MUTABLE_FORMAT) != 0) {
		xr |= XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT;
	}
	if ((bits & XRT_SWAPCHAIN_USAGE_INPUT_ATTACHMENT) != 0) {
		xr |= XR_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT_KHR;
	}

	return xr;
}


/*
 *
 * Basic types
 *
 */

static inline XrExtent2Di
xrt_size_to_xr(const struct xrt_size *x)
{
	return (XrExtent2Di){
	    .width = x->w,
	    .height = x->h,
	};
}

static inline XrVector2f
xrt_vec2_to_xr(const struct xrt_vec2 *v)
{
	return (XrVector2f){
	    .x = v->x,
	    .y = v->y,
	};
}

static inline XrVector3f
xrt_vec3_to_xr(const struct xrt_vec3 *v)
{
	return (XrVector3f){
	    .x = v->x,
	    .y = v->y,
	    .z = v->z,
	};
}

static inline XrQuaternionf
xrt_quat_to_xr(const struct xrt_quat *q)
{
	return (XrQuaternionf){
	    .x = q->x,
	    .y = q->y,
	    .z = q->z,
	    .w = q->w,
	};
}

static inline XrPosef
xrt_pose_to_xr(const struct xrt_pose *q)
{
	return (XrPosef){
	    .orientation = xrt_quat_to_xr(&q->orientation),
	    .position = xrt_vec3_to_xr(&q->position),
	};
}


/*
 *
 * View things
 *
 */

static inline XrViewConfigurationType
xrt_view_type_to_xr(enum xrt_view_type view_type)
{
	switch (view_type) {
	case XRT_VIEW_TYPE_MAX:
	case XRT_VIEW_TYPE_INVALID: break;
	case XRT_VIEW_TYPE_MONO: return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
	case XRT_VIEW_TYPE_STEREO: return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	case XRT_VIEW_TYPE_QUAD: return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO_WITH_FOVEATED_INSET;
	}

	// Used as default, to get warnings.
	assert(false && "Invalid view type");
	return XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
}
