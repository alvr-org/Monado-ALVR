// Copyright 2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  face tracking related API entrypoint functions.
 * @author galister <galister@librevr.org>
 * @ingroup oxr_main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle.h"
#include "xrt/xrt_defines.h"

static XrResult
oxr_face_tracker2_fb_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_face_tracker2_fb *face_tracker2_fb = (struct oxr_face_tracker2_fb *)hb;
	free(face_tracker2_fb);
	return XR_SUCCESS;
}

XrResult
oxr_face_tracker2_fb_create(struct oxr_logger *log,
                            struct oxr_session *sess,
                            const XrFaceTrackerCreateInfo2FB *createInfo,
                            struct oxr_face_tracker2_fb **out_face_tracker2_fb)
{
	if (createInfo->faceExpressionSet != XR_FACE_EXPRESSION_SET2_DEFAULT_FB) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "Unsupported expression set");
	}

	struct xrt_device *xdev = GET_XDEV_BY_ROLE(sess->sys, face);
	if (xdev == NULL) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "No device found for face tracking role");
	}

	if (!xdev->face_tracking_supported || xdev->get_face_tracking == NULL) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "Device does not support FB2 face tracking");
	}

	bool supports_audio, supports_visual;

	oxr_system_get_face_tracking2_fb_support(log, sess->sys->inst, &supports_audio, &supports_visual);

	if (!supports_audio && !supports_visual) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "System does not support FB2 face tracking");
	}

	bool want_audio = false;
	bool want_visual = false;

	// spec: the runtime must ensure no duplicates in requestedDataSources
	for (uint32_t i = 0; i < createInfo->requestedDataSourceCount; i++) {
		if (createInfo->requestedDataSources[i] == XR_FACE_TRACKING_DATA_SOURCE2_AUDIO_FB) {
			if (!supports_audio) {
				return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "Audio source not supported");
			}
			if (want_audio) {
				return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "Duplicate entry for data source");
			}
			want_audio = true;
		} else if (createInfo->requestedDataSources[i] == XR_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB) {
			if (!supports_visual) {
				return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "Visual source not supported");
			}
			if (want_visual) {
				return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "Duplicate entry for data source");
			}
			want_visual = true;
		} else {
			return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "Unsupported data source");
		}
	}

	// spec: if no data source is requested, select the highest fidelity available
	if (!want_audio && !want_visual) {
		if (supports_visual) {
			want_visual = true;
		} else {
			want_audio = true;
		}
	}

	struct oxr_face_tracker2_fb *face_tracker2_fb = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, face_tracker2_fb, OXR_XR_DEBUG_FTRACKER, oxr_face_tracker2_fb_destroy_cb,
	                              &sess->handle);

	face_tracker2_fb->sess = sess;
	face_tracker2_fb->xdev = xdev;
	face_tracker2_fb->audio_enabled = want_audio;
	face_tracker2_fb->visual_enabled = want_visual;

	*out_face_tracker2_fb = face_tracker2_fb;

	return XR_SUCCESS;
}

XrResult
oxr_get_face_expression_weights2_fb(struct oxr_logger *log,
                                    struct oxr_face_tracker2_fb *face_tracker2_fb,
                                    const XrFaceExpressionInfo2FB *expression_info,
                                    XrFaceExpressionWeights2FB *expression_weights)
{
	if (expression_weights->weightCount < XRT_FACE_EXPRESSION2_COUNT_FB) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "weightCount != XR_FACE_EXPRESSION2_COUNT_FB");
	}
	if (expression_weights->confidenceCount < XRT_FACE_CONFIDENCE2_COUNT_FB) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "confidenceCount != XR_FACE_CONFIDENCE2_COUNT_FB");
	}
	struct xrt_facial_expression_set result = {0};

	const struct oxr_instance *inst = face_tracker2_fb->sess->sys->inst;
	int64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(inst->timekeeping, expression_info->time);

	// spec: visual is allowed to use both camera and audio
	enum xrt_input_name ft_input_name =
	    face_tracker2_fb->visual_enabled ? XRT_INPUT_FB_FACE_TRACKING2_VISUAL : XRT_INPUT_FB_FACE_TRACKING2_AUDIO;

	xrt_result_t xres =
	    xrt_device_get_face_tracking(face_tracker2_fb->xdev, ft_input_name, at_timestamp_ns, &result);
	if (xres != XRT_SUCCESS) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	expression_weights->isValid = result.face_expression_set2_fb.is_valid;
	if (!expression_weights->isValid) {
		return XR_SUCCESS;
	}

	expression_weights->isEyeFollowingBlendshapesValid =
	    result.face_expression_set2_fb.is_eye_following_blendshapes_valid;

	expression_weights->time =
	    time_state_monotonic_to_ts_ns(inst->timekeeping, result.face_expression_set2_fb.sample_time_ns);

	expression_weights->dataSource = (XrFaceTrackingDataSource2FB)result.face_expression_set2_fb.data_source;

	expression_weights->weightCount = XRT_FACE_EXPRESSION2_COUNT_FB;
	expression_weights->confidenceCount = XRT_FACE_CONFIDENCE2_COUNT_FB;

	memcpy(expression_weights->weights, result.face_expression_set2_fb.weights,
	       sizeof(float) * XRT_FACE_EXPRESSION2_COUNT_FB);
	memcpy(expression_weights->confidences, result.face_expression_set2_fb.confidences,
	       sizeof(float) * XRT_FACE_CONFIDENCE2_COUNT_FB);

	return XR_SUCCESS;
}
