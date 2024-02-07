// Copyright 2018-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds system related entrypoints.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "xrt/xrt_device.h"
#include "util/u_debug.h"
#include "util/u_verify.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_two_call.h"
#include "oxr_chain.h"


DEBUG_GET_ONCE_NUM_OPTION(scale_percentage, "OXR_VIEWPORT_SCALE_PERCENTAGE", 100)

static enum xrt_form_factor
convert_form_factor(XrFormFactor form_factor)
{
	switch (form_factor) {
	case XR_FORM_FACTOR_HANDHELD_DISPLAY: return XRT_FORM_FACTOR_HANDHELD;
	case XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY: return XRT_FORM_FACTOR_HMD;
	default: return XRT_FORM_FACTOR_HMD;
	}
}

static bool
oxr_system_matches(struct oxr_logger *log, struct oxr_system *sys, XrFormFactor form_factor)
{
	return form_factor == sys->form_factor;
}

XrResult
oxr_system_select(struct oxr_logger *log,
                  struct oxr_system **systems,
                  uint32_t system_count,
                  XrFormFactor form_factor,
                  struct oxr_system **out_selected)
{
	if (system_count == 0) {
		return oxr_error(log, XR_ERROR_FORM_FACTOR_UNSUPPORTED,
		                 "(getInfo->formFactor) no system available (given: %i)", form_factor);
	}

	struct oxr_system *selected = NULL;
	for (uint32_t i = 0; i < system_count; i++) {
		if (oxr_system_matches(log, systems[i], form_factor)) {
			selected = systems[i];
			break;
		}
	}

	if (selected == NULL) {
		return oxr_error(log, XR_ERROR_FORM_FACTOR_UNSUPPORTED,
		                 "(getInfo->formFactor) no matching system "
		                 "(given: %i, first: %i)",
		                 form_factor, systems[0]->form_factor);
	}

	struct xrt_device *xdev = GET_XDEV_BY_ROLE(selected, head);
	if (xdev->form_factor_check_supported &&
	    !xrt_device_is_form_factor_available(xdev, convert_form_factor(form_factor))) {
		return oxr_error(log, XR_ERROR_FORM_FACTOR_UNAVAILABLE, "request form factor %i is unavailable now",
		                 form_factor);
	}

	*out_selected = selected;

	return XR_SUCCESS;
}

XrResult
oxr_system_verify_id(struct oxr_logger *log, const struct oxr_instance *inst, XrSystemId systemId)
{
	if (systemId != XRT_SYSTEM_ID) {
		return oxr_error(log, XR_ERROR_SYSTEM_INVALID, "Invalid system %" PRIu64, systemId);
	}
	return XR_SUCCESS;
}

XrResult
oxr_system_get_by_id(struct oxr_logger *log, struct oxr_instance *inst, XrSystemId systemId, struct oxr_system **system)
{
	XrResult result = oxr_system_verify_id(log, inst, systemId);
	if (result != XR_SUCCESS) {
		return result;
	}

	/* right now only have one system. */
	*system = &inst->system;

	return XR_SUCCESS;
}



XrResult
oxr_system_fill_in(
    struct oxr_logger *log, struct oxr_instance *inst, XrSystemId systemId, uint32_t view_count, struct oxr_system *sys)
{
	//! @todo handle other subaction paths?

	sys->inst = inst;
	sys->systemId = systemId;
	sys->form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	if (view_count == 1) {
		sys->view_config_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
	} else if (view_count == 2) {
		sys->view_config_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	} else {
		assert(false && "view_count must be 1 or 2");
	}
	U_LOG_D("sys->view_config_type = %d", sys->view_config_type);
	sys->dynamic_roles_cache = (struct xrt_system_roles)XRT_SYSTEM_ROLES_INIT;

#ifdef XR_USE_GRAPHICS_API_VULKAN
	sys->vulkan_enable2_instance = VK_NULL_HANDLE;
	sys->suggested_vulkan_physical_device = VK_NULL_HANDLE;
#endif
#if defined(XR_USE_GRAPHICS_API_D3D11) || defined(XR_USE_GRAPHICS_API_D3D12)
	U_ZERO(&(sys->suggested_d3d_luid));
	sys->suggested_d3d_luid_valid = false;
#endif

	// Headless.
	if (sys->xsysc == NULL) {
		sys->blend_modes[0] = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		sys->blend_mode_count = 1;
		return XR_SUCCESS;
	}

	double scale = debug_get_num_option_scale_percentage() / 100.0;
	if (scale > 2.0) {
		scale = 2.0;
		oxr_log(log, "Clamped scale to 200%%\n");
	}

	struct xrt_system_compositor_info *info = &sys->xsysc->info;

#define imin(a, b) (a < b ? a : b)
	for (uint32_t i = 0; i < view_count; ++i) {
		uint32_t w = (uint32_t)(info->views[i].recommended.width_pixels * scale);
		uint32_t h = (uint32_t)(info->views[i].recommended.height_pixels * scale);
		uint32_t w_2 = info->views[i].max.width_pixels;
		uint32_t h_2 = info->views[i].max.height_pixels;

		w = imin(w, w_2);
		h = imin(h, h_2);

		sys->views[i].recommendedImageRectWidth = w;
		sys->views[i].maxImageRectWidth = w_2;
		sys->views[i].recommendedImageRectHeight = h;
		sys->views[i].maxImageRectHeight = h_2;
		sys->views[i].recommendedSwapchainSampleCount = info->views[i].recommended.sample_count;
		sys->views[i].maxSwapchainSampleCount = info->views[i].max.sample_count;
	}

#undef imin


	/*
	 * Blend mode support.
	 */

	assert(info->supported_blend_mode_count <= ARRAY_SIZE(sys->blend_modes));
	assert(info->supported_blend_mode_count != 0);

	for (uint8_t i = 0; i < info->supported_blend_mode_count; i++) {
		assert(u_verify_blend_mode_valid(info->supported_blend_modes[i]));
		sys->blend_modes[i] = (XrEnvironmentBlendMode)info->supported_blend_modes[i];
	}
	sys->blend_mode_count = (uint32_t)info->supported_blend_mode_count;


	/*
	 * Reference space support.
	 */

	static_assert(5 <= ARRAY_SIZE(sys->reference_spaces), "Not enough space in array");

	if (sys->xso->semantic.view != NULL) {
		sys->reference_spaces[sys->reference_space_count++] = XR_REFERENCE_SPACE_TYPE_VIEW;
	}

	if (sys->xso->semantic.local != NULL) {
		sys->reference_spaces[sys->reference_space_count++] = XR_REFERENCE_SPACE_TYPE_LOCAL;
	}

#ifdef OXR_HAVE_EXT_local_floor
	if (sys->inst->extensions.EXT_local_floor) {
		if (sys->xso->semantic.local_floor != NULL) {
			sys->reference_spaces[sys->reference_space_count++] = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
		} else {
			oxr_warn(log,
			         "XR_EXT_local_floor enabled but system doesn't support local_floor,"
			         " breaking spec by not exposing the reference space.");
		}
	}
#endif

	if (sys->xso->semantic.stage != NULL) {
		sys->reference_spaces[sys->reference_space_count++] = XR_REFERENCE_SPACE_TYPE_STAGE;
	}

#ifdef OXR_HAVE_MSFT_unbounded_reference_space
	if (sys->inst->extensions.MSFT_unbounded_reference_space && sys->xso->semantic.unbounded != NULL) {
		sys->reference_spaces[sys->reference_space_count++] = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
	}
#endif


	/*
	 * Done.
	 */

	return XR_SUCCESS;
}

bool
oxr_system_get_hand_tracking_support(struct oxr_logger *log, struct oxr_instance *inst)
{
	struct oxr_system *sys = &inst->system;
	struct xrt_device *ht_left = GET_XDEV_BY_ROLE(sys, hand_tracking_left);
	struct xrt_device *ht_right = GET_XDEV_BY_ROLE(sys, hand_tracking_right);

	bool left_supported = ht_left && ht_left->hand_tracking_supported;
	bool right_supported = ht_right && ht_right->hand_tracking_supported;

	return left_supported || right_supported;
}

bool
oxr_system_get_eye_gaze_support(struct oxr_logger *log, struct oxr_instance *inst)
{
	struct oxr_system *sys = &inst->system;
	struct xrt_device *eyes = GET_XDEV_BY_ROLE(sys, eyes);

	return eyes && eyes->eye_gaze_supported;
}

bool
oxr_system_get_force_feedback_support(struct oxr_logger *log, struct oxr_instance *inst)
{
	struct oxr_system *sys = &inst->system;
	struct xrt_device *ffb_left = GET_XDEV_BY_ROLE(sys, hand_tracking_left);
	struct xrt_device *ffb_right = GET_XDEV_BY_ROLE(sys, hand_tracking_right);

	bool left_supported = ffb_left && ffb_left->force_feedback_supported;
	bool right_supported = ffb_right && ffb_right->force_feedback_supported;

	return left_supported || right_supported;
}

void
oxr_system_get_face_tracking_htc_support(struct oxr_logger *log,
                                         struct oxr_instance *inst,
                                         bool *supports_eye,
                                         bool *supports_lip)
{
	struct oxr_system *sys = &inst->system;
	struct xrt_device *face_xdev = GET_XDEV_BY_ROLE(sys, face);

	if (supports_eye)
		*supports_eye = false;
	if (supports_lip)
		*supports_lip = false;

	if (face_xdev == NULL || !face_xdev->face_tracking_supported || face_xdev->inputs == NULL) {
		return;
	}

	for (size_t input_idx = 0; input_idx < face_xdev->input_count; ++input_idx) {
		const struct xrt_input *input = &face_xdev->inputs[input_idx];
		if (supports_eye != NULL && input->name == XRT_INPUT_HTC_EYE_FACE_TRACKING) {
			*supports_eye = true;
		}
		if (supports_lip != NULL && input->name == XRT_INPUT_HTC_LIP_FACE_TRACKING) {
			*supports_lip = true;
		}
	}
}

XrResult
oxr_system_get_properties(struct oxr_logger *log, struct oxr_system *sys, XrSystemProperties *properties)
{
	properties->systemId = sys->systemId;
	properties->vendorId = sys->xsys->properties.vendor_id;
	memcpy(properties->systemName, sys->xsys->properties.name, sizeof(properties->systemName));

	struct xrt_device *xdev = GET_XDEV_BY_ROLE(sys, head);

	// Get from compositor.
	struct xrt_system_compositor_info *info = sys->xsysc ? &sys->xsysc->info : NULL;

	if (info) {
		properties->graphicsProperties.maxLayerCount = info->max_layers;
	} else {
		// probably using the headless extension, but the extension does not modify the 16 layer minimum.
		properties->graphicsProperties.maxLayerCount = 16;
	}
	properties->graphicsProperties.maxSwapchainImageWidth = 1024 * 16;
	properties->graphicsProperties.maxSwapchainImageHeight = 1024 * 16;
	properties->trackingProperties.orientationTracking = xdev->orientation_tracking_supported;
	properties->trackingProperties.positionTracking = xdev->position_tracking_supported;

	XrSystemHandTrackingPropertiesEXT *hand_tracking_props = NULL;
	// We should only be looking for extension structs if the extension has been enabled.
	if (sys->inst->extensions.EXT_hand_tracking) {
		hand_tracking_props = OXR_GET_OUTPUT_FROM_CHAIN(properties, XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT,
		                                                XrSystemHandTrackingPropertiesEXT);
	}

	if (hand_tracking_props) {
		hand_tracking_props->supportsHandTracking = oxr_system_get_hand_tracking_support(log, sys->inst);
	}

#ifdef OXR_HAVE_EXT_eye_gaze_interaction
	XrSystemEyeGazeInteractionPropertiesEXT *eye_gaze_props = NULL;
	if (sys->inst->extensions.EXT_eye_gaze_interaction) {
		eye_gaze_props =
		    OXR_GET_OUTPUT_FROM_CHAIN(properties, XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT,
		                              XrSystemEyeGazeInteractionPropertiesEXT);
	}

	if (eye_gaze_props) {
		eye_gaze_props->supportsEyeGazeInteraction = oxr_system_get_eye_gaze_support(log, sys->inst);
	}
#endif

#ifdef OXR_HAVE_MNDX_force_feedback_curl
	XrSystemForceFeedbackCurlPropertiesMNDX *force_feedback_props = NULL;
	if (sys->inst->extensions.MNDX_force_feedback_curl) {
		force_feedback_props =
		    OXR_GET_OUTPUT_FROM_CHAIN(properties, XR_TYPE_SYSTEM_FORCE_FEEDBACK_CURL_PROPERTIES_MNDX,
		                              XrSystemForceFeedbackCurlPropertiesMNDX);
	}

	if (force_feedback_props) {
		force_feedback_props->supportsForceFeedbackCurl = oxr_system_get_force_feedback_support(log, sys->inst);
	}
#endif

#ifdef OXR_HAVE_FB_passthrough
	XrSystemPassthroughPropertiesFB *passthrough_props = NULL;
	XrSystemPassthroughProperties2FB *passthrough_props2 = NULL;
	if (sys->inst->extensions.FB_passthrough) {
		passthrough_props = OXR_GET_OUTPUT_FROM_CHAIN(properties, XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES_FB,
		                                              XrSystemPassthroughPropertiesFB);
		if (passthrough_props) {
			passthrough_props->supportsPassthrough = true;
		}

		passthrough_props2 = OXR_GET_OUTPUT_FROM_CHAIN(properties, XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES2_FB,
		                                               XrSystemPassthroughProperties2FB);
		if (passthrough_props2) {
			passthrough_props2->capabilities = XR_PASSTHROUGH_CAPABILITY_BIT_FB;
		}
	}
#endif

#ifdef OXR_HAVE_HTC_facial_tracking
	XrSystemFacialTrackingPropertiesHTC *htc_facial_tracking_props = NULL;
	if (sys->inst->extensions.HTC_facial_tracking) {
		htc_facial_tracking_props = OXR_GET_OUTPUT_FROM_CHAIN(
		    properties, XR_TYPE_SYSTEM_FACIAL_TRACKING_PROPERTIES_HTC, XrSystemFacialTrackingPropertiesHTC);
	}

	if (htc_facial_tracking_props) {
		bool supports_eye = false;
		bool supports_lip = false;
		oxr_system_get_face_tracking_htc_support(log, sys->inst, &supports_eye, &supports_lip);
		htc_facial_tracking_props->supportEyeFacialTracking = supports_eye;
		htc_facial_tracking_props->supportLipFacialTracking = supports_lip;
	}
#endif // OXR_HAVE_HTC_facial_tracking
	return XR_SUCCESS;
}

XrResult
oxr_system_enumerate_view_confs(struct oxr_logger *log,
                                struct oxr_system *sys,
                                uint32_t viewConfigurationTypeCapacityInput,
                                uint32_t *viewConfigurationTypeCountOutput,
                                XrViewConfigurationType *viewConfigurationTypes)
{
	OXR_TWO_CALL_HELPER(log, viewConfigurationTypeCapacityInput, viewConfigurationTypeCountOutput,
	                    viewConfigurationTypes, 1, &sys->view_config_type, XR_SUCCESS);
}

XrResult
oxr_system_enumerate_blend_modes(struct oxr_logger *log,
                                 struct oxr_system *sys,
                                 XrViewConfigurationType viewConfigurationType,
                                 uint32_t environmentBlendModeCapacityInput,
                                 uint32_t *environmentBlendModeCountOutput,
                                 XrEnvironmentBlendMode *environmentBlendModes)
{
	//! @todo Take into account viewConfigurationType
	OXR_TWO_CALL_HELPER(log, environmentBlendModeCapacityInput, environmentBlendModeCountOutput,
	                    environmentBlendModes, sys->blend_mode_count, sys->blend_modes, XR_SUCCESS);
}

XrResult
oxr_system_get_view_conf_properties(struct oxr_logger *log,
                                    struct oxr_system *sys,
                                    XrViewConfigurationType viewConfigurationType,
                                    XrViewConfigurationProperties *configurationProperties)
{
	if (viewConfigurationType != sys->view_config_type) {
		return oxr_error(log, XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED, "Invalid view configuration type");
	}

	configurationProperties->viewConfigurationType = sys->view_config_type;
	configurationProperties->fovMutable = XR_FALSE;

	return XR_SUCCESS;
}

static void
view_configuration_view_fill_in(XrViewConfigurationView *target_view, XrViewConfigurationView *source_view)
{
	// clang-format off
	target_view->recommendedImageRectWidth       = source_view->recommendedImageRectWidth;
	target_view->maxImageRectWidth               = source_view->maxImageRectWidth;
	target_view->recommendedImageRectHeight      = source_view->recommendedImageRectHeight;
	target_view->maxImageRectHeight              = source_view->maxImageRectHeight;
	target_view->recommendedSwapchainSampleCount = source_view->recommendedSwapchainSampleCount;
	target_view->maxSwapchainSampleCount         = source_view->maxSwapchainSampleCount;
	// clang-format on
}

XrResult
oxr_system_enumerate_view_conf_views(struct oxr_logger *log,
                                     struct oxr_system *sys,
                                     XrViewConfigurationType viewConfigurationType,
                                     uint32_t viewCapacityInput,
                                     uint32_t *viewCountOutput,
                                     XrViewConfigurationView *views)
{
	if (viewConfigurationType != sys->view_config_type) {
		return oxr_error(log, XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED, "Invalid view configuration type");
	}
	if (sys->view_config_type == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO) {
		OXR_TWO_CALL_FILL_IN_HELPER(log, viewCapacityInput, viewCountOutput, views, 1,
		                            view_configuration_view_fill_in, sys->views, XR_SUCCESS);
	} else {
		OXR_TWO_CALL_FILL_IN_HELPER(log, viewCapacityInput, viewCountOutput, views, 2,
		                            view_configuration_view_fill_in, sys->views, XR_SUCCESS);
	}
}
