// Copyright 2025, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Android face tracking related API entrypoint functions.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle_base.h"
#include "oxr_xret.h"
#include "oxr_two_call.h"
#include "oxr_roles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static XrResult
oxr_face_tracker_android_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_face_tracker_android *face_tracker_android = (struct oxr_face_tracker_android *)hb;

	if (face_tracker_android->feature_incremented) {
		xrt_system_devices_feature_dec(face_tracker_android->sess->sys->xsysd,
		                               XRT_DEVICE_FEATURE_FACE_TRACKING);
	}

	free(face_tracker_android);
	return XR_SUCCESS;
}

XrResult
oxr_face_tracker_android_create(struct oxr_logger *log,
                                struct oxr_session *sess,
                                const XrFaceTrackerCreateInfoANDROID *createInfo,
                                XrFaceTrackerANDROID *faceTracker)
{
	bool supported = false;
	oxr_system_get_face_tracking_android_support(log, sess->sys->inst, &supported);
	if (!supported) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "System does not support Android face tracking");
	}

	struct xrt_device *xdev = GET_STATIC_XDEV_BY_ROLE(sess->sys, face);
	if (xdev == NULL) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "No device found for face tracking role");
	}

	if (!xdev->supported.face_tracking) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED, "Device does not support Android face tracking");
	}

	struct oxr_face_tracker_android *face_tracker_android = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, face_tracker_android, OXR_XR_DEBUG_FTRACKER,
	                              oxr_face_tracker_android_destroy_cb, &sess->handle);

	xrt_result_t xret = xrt_system_devices_feature_inc(sess->sys->xsysd, XRT_DEVICE_FEATURE_FACE_TRACKING);
	if (xret != XRT_SUCCESS) {
		oxr_handle_destroy(log, &face_tracker_android->handle);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to start face tracking feature");
	}

	face_tracker_android->feature_incremented = true;
	face_tracker_android->sess = sess;
	face_tracker_android->xdev = xdev;

	*faceTracker = oxr_face_tracker_android_to_openxr(face_tracker_android);
	return oxr_session_success_result(sess);
}

XrResult
oxr_get_face_state_android(struct oxr_logger *log,
                           struct oxr_face_tracker_android *facial_tracker_android,
                           const XrFaceStateGetInfoANDROID *getInfo,
                           XrFaceStateANDROID *faceStateOutput)
{
	/*!
	 * OXR_TWO_CALL_CHECK_* macro usage here is not technically necessary because validation
	 * is handled in the API layer before this function is called, but we still reuse them here
	 * for declarative purposes of handling two-call patterns of setting capacities on the first call.
	 */

	/*!
	 * Use the goto macro variant because a two-call check needs to happen with region confidences
	 * as well; we can't early exit for only one of them.
	 */
	XrResult xres = XR_SUCCESS;
	OXR_TWO_CALL_CHECK_GOTO(log, faceStateOutput->parametersCapacityInput,
	                        (&faceStateOutput->parametersCountOutput), XRT_FACE_PARAMETER_COUNT_ANDROID, xres,
	                        region_confidences_check);
region_confidences_check:
	if (xres != XR_SUCCESS) {
		return xres;
	}

	OXR_TWO_CALL_CHECK_ONLY(log, faceStateOutput->regionConfidencesCapacityInput,
	                        (&faceStateOutput->regionConfidencesCountOutput),
	                        XRT_FACE_REGION_CONFIDENCE_COUNT_ANDROID, xres);

	const struct oxr_instance *inst = facial_tracker_android->sess->sys->inst;
	const int64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(inst->timekeeping, getInfo->time);

	struct xrt_facial_expression_set facial_expression_set_result = {0};
	const xrt_result_t xret =
	    xrt_device_get_face_tracking(facial_tracker_android->xdev, XRT_INPUT_ANDROID_FACE_TRACKING, at_timestamp_ns,
	                                 &facial_expression_set_result);
	OXR_CHECK_XRET(log, facial_tracker_android->sess, xret, "oxr_get_face_state_android");

	const struct xrt_facial_expression_set_android *face_expression_set_android =
	    &facial_expression_set_result.face_expression_set_android;

	faceStateOutput->isValid = face_expression_set_android->is_valid;
	faceStateOutput->faceTrackingState = (XrFaceTrackingStateANDROID)face_expression_set_android->state;
	if (faceStateOutput->isValid == XR_FALSE) {
		return XR_SUCCESS;
	}

	faceStateOutput->sampleTime =
	    time_state_monotonic_to_ts_ns(inst->timekeeping, face_expression_set_android->sample_time_ns);

	if (faceStateOutput->parametersCapacityInput) {
		memcpy(faceStateOutput->parameters, face_expression_set_android->parameters,
		       sizeof(float) * faceStateOutput->parametersCapacityInput);
	}

	if (faceStateOutput->regionConfidencesCapacityInput) {
		memcpy(faceStateOutput->regionConfidences, face_expression_set_android->region_confidences,
		       sizeof(float) * faceStateOutput->regionConfidencesCapacityInput);
	}

	return oxr_session_success_result(facial_tracker_android->sess);
}

XrResult
oxr_get_face_calibration_state_android(struct oxr_logger *log,
                                       struct oxr_face_tracker_android *facial_tracker_android,
                                       XrBool32 *faceIsCalibratedOutput)
{
	if (!facial_tracker_android->xdev->supported.face_tracking_calibration_state) {
		return oxr_error(log, XR_ERROR_FEATURE_UNSUPPORTED,
		                 "Device does not support getting the face tracking calibration state");
	}

	bool face_is_calibrated = false;
	const xrt_result_t xret =
	    xrt_device_get_face_calibration_state_android(facial_tracker_android->xdev, &face_is_calibrated);
	OXR_CHECK_XRET(log, facial_tracker_android->sess, xret, "oxr_get_face_calibration_state_android");

	*faceIsCalibratedOutput = face_is_calibrated;

	return oxr_session_success_result(facial_tracker_android->sess);
}
