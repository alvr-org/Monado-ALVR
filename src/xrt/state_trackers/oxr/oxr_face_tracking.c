// Copyright 2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  face tracking related API entrypoint functions.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle.h"

static enum xrt_facial_tracking_type_htc
oxr_to_xrt_facial_tracking_type_htc(enum XrFacialTrackingTypeHTC ft_type)
{
	return (enum xrt_facial_tracking_type_htc)ft_type;
}

static enum xrt_input_name
oxr_facial_tracking_type_htc_to_input_name(enum xrt_facial_tracking_type_htc ft_type)
{
	switch (ft_type) {
	case XRT_FACIAL_TRACKING_TYPE_LIP_DEFAULT_HTC: return XRT_INPUT_HTC_LIP_FACE_TRACKING;
	case XRT_FACIAL_TRACKING_TYPE_EYE_DEFAULT_HTC:
	default: return XRT_INPUT_HTC_EYE_FACE_TRACKING;
	}
}

static XrResult
oxr_facial_tracker_htc_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_facial_tracker_htc *face_tracker_htc = (struct oxr_facial_tracker_htc *)hb;
	free(face_tracker_htc);
	return XR_SUCCESS;
}

XrResult
oxr_facial_tracker_htc_create(struct oxr_logger *log,
                              struct oxr_session *sess,
                              const XrFacialTrackerCreateInfoHTC *createInfo,
                              struct oxr_facial_tracker_htc **out_face_tracker_htc)
{
	bool supports_eye = false;
	bool supports_lip = false;
	oxr_system_get_face_tracking_htc_support(log, sess->sys->inst, &supports_eye, &supports_lip);

	const enum xrt_facial_tracking_type_htc facial_tracking_type =
	    oxr_to_xrt_facial_tracking_type_htc(createInfo->facialTrackingType);

	if (facial_tracking_type == XRT_FACIAL_TRACKING_TYPE_EYE_DEFAULT_HTC && !supports_eye) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "System does not support HTC eye facial tracking");
	}
	if (facial_tracking_type == XRT_FACIAL_TRACKING_TYPE_LIP_DEFAULT_HTC && !supports_lip) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "System does not support HTC lip facial tracking");
	}

	struct xrt_device *xdev = GET_XDEV_BY_ROLE(sess->sys, face);
	if (xdev == NULL) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "No device found for face tracking role");
	}

	if (!xdev->face_tracking_supported) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "Device does not support HTC facial tracking");
	}

	struct oxr_facial_tracker_htc *face_tracker_htc = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, face_tracker_htc, OXR_XR_DEBUG_FTRACKER, oxr_facial_tracker_htc_destroy_cb,
	                              &sess->handle);

	face_tracker_htc->sess = sess;
	face_tracker_htc->xdev = xdev;
	face_tracker_htc->facial_tracking_type = facial_tracking_type;

	*out_face_tracker_htc = face_tracker_htc;

	return XR_SUCCESS;
}

XrResult
oxr_get_facial_expressions_htc(struct oxr_logger *log,
                               struct oxr_facial_tracker_htc *facial_tracker_htc,
                               XrFacialExpressionsHTC *facialExpressions)
{
	const bool is_eye_tracking =
	    facial_tracker_htc->facial_tracking_type == XRT_FACIAL_TRACKING_TYPE_EYE_DEFAULT_HTC;
	const size_t expression_count =
	    is_eye_tracking ? XRT_FACIAL_EXPRESSION_EYE_COUNT_HTC : XRT_FACIAL_EXPRESSION_LIP_COUNT_HTC;

	struct xrt_facial_expression_set facial_expression_set_result = {0};
	float *expression_weights = is_eye_tracking
	                                ? facial_expression_set_result.eye_expression_set_htc.expression_weights
	                                : facial_expression_set_result.lip_expression_set_htc.expression_weights;
	memset(expression_weights, 0, sizeof(float) * expression_count);

	const enum xrt_input_name ft_input_name =
	    oxr_facial_tracking_type_htc_to_input_name(facial_tracker_htc->facial_tracking_type);

	int64_t at_timestamp_ns = os_monotonic_get_ns();

	xrt_device_get_face_tracking(facial_tracker_htc->xdev, ft_input_name, at_timestamp_ns,
	                             &facial_expression_set_result);

	facialExpressions->isActive = facial_expression_set_result.base_expression_set_htc.is_active;
	if (facialExpressions->isActive == XR_FALSE)
		return XR_SUCCESS;

	const struct oxr_instance *inst = facial_tracker_htc->sess->sys->inst;
	facialExpressions->sampleTime = time_state_monotonic_to_ts_ns(
	    inst->timekeeping, facial_expression_set_result.base_expression_set_htc.sample_time_ns);

	memcpy(facialExpressions->expressionWeightings, expression_weights, sizeof(float) * expression_count);

	return XR_SUCCESS;
}
