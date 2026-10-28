// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds action state get related functions.
 * @ingroup oxr_main
 */

#include "oxr_set_haptic.h"
#include "oxr_subaction.h"
#include "oxr_input.h"
#include "oxr_session_action_context.h"

#include "../oxr_objects.h"
#include "../oxr_logger.h"


static void
set_action_output_vibration(struct oxr_logger *log,
                            struct oxr_session *sess,
                            struct oxr_action_cache *cache,
                            int64_t stop,
                            const XrHapticVibration *data)
{
	cache->stop_output_time = stop;

	struct xrt_output_value value = {0};
	value.vibration.frequency = data->frequency;
	value.vibration.amplitude = data->amplitude;
	value.vibration.duration_ns = data->duration;
	value.type = XRT_OUTPUT_VALUE_TYPE_VIBRATION;

	for (uint32_t i = 0; i < cache->output_count; i++) {
		struct oxr_action_output *output = &cache->outputs[i];

		xrt_result_t xret =
		    xrt_io_cache_set_output(output->io_cache->xic, output->device_id, output->name, &value);
		if (xret != XRT_SUCCESS) {
			struct oxr_sink_logger slog = {0};
			oxr_slog(&slog, "Failed to set output vibration ");
			u_pp_xrt_output_name(oxr_slog_dg(&slog), output->name);
			oxr_log_slog(log, &slog);
		}
	}
}

XRT_MAYBE_UNUSED static void
set_action_output_vibration_pcm(struct oxr_logger *log,
                                struct oxr_session *sess,
                                struct oxr_action_cache *cache,
                                const XrHapticPcmVibrationFB *data)
{
	struct xrt_output_value value = {0};
	value.pcm_vibration.append = data->append;
	value.pcm_vibration.buffer = data->buffer;
	value.pcm_vibration.buffer_size = data->bufferSize;
	value.pcm_vibration.sample_rate = data->sampleRate;
	value.pcm_vibration.samples_consumed = data->samplesConsumed;
	value.type = XRT_OUTPUT_VALUE_TYPE_PCM_VIBRATION;

	for (uint32_t i = 0; i < cache->output_count; i++) {
		struct oxr_action_output *output = &cache->outputs[i];

		xrt_result_t xret =
		    xrt_io_cache_set_output(output->io_cache->xic, output->device_id, output->name, &value);
		if (xret != XRT_SUCCESS) {
			struct oxr_sink_logger slog = {0};
			oxr_slog(&slog, "Failed to set output vibration PCM ");
			u_pp_xrt_output_name(oxr_slog_dg(&slog), output->name);
			oxr_log_slog(log, &slog);
		}
	}
}

XrResult
oxr_action_apply_haptic_feedback(struct oxr_logger *log,
                                 struct oxr_session *sess,
                                 uint32_t act_key,
                                 struct oxr_subaction_paths subaction_paths,
                                 const XrHapticBaseHeader *hapticEvent)
{
	struct oxr_action_attachment *act_attached = NULL;

	oxr_session_attached_actions_find(&sess->attached_actions, act_key, &act_attached);
	if (act_attached == NULL) {
		return oxr_error(log, XR_ERROR_ACTIONSET_NOT_ATTACHED, "Action has not been attached to this session");
	}

	if (sess->state != XR_SESSION_STATE_FOCUSED) {
		return oxr_session_success_focused_result(sess);
	}

	if (hapticEvent->type == XR_TYPE_HAPTIC_VIBRATION) {
		const XrHapticVibration *data = (const XrHapticVibration *)hapticEvent;

		// This should all be moved into the drivers.
		const int64_t min_pulse_time_ns = time_s_to_ns(0.1);
		int64_t now_ns = time_state_get_now(sess->sys->inst->timekeeping);
		int64_t stop_ns = 0;
		if (data->duration <= 0) {
			stop_ns = now_ns + min_pulse_time_ns;
		} else {
			stop_ns = now_ns + data->duration;
		}

#define SET_OUT_VIBRATION(X)                                                                                           \
	if (act_attached->X.current.active && (subaction_paths.X || subaction_paths.any)) {                            \
		set_action_output_vibration(log, sess, &act_attached->X, stop_ns, data);                               \
	}

		OXR_FOR_EACH_SUBACTION_PATH(SET_OUT_VIBRATION)
#undef SET_OUT_VIBRATION
#ifdef OXR_HAVE_FB_haptic_pcm
	} else if (hapticEvent->type == XR_TYPE_HAPTIC_PCM_VIBRATION_FB) {
		const XrHapticPcmVibrationFB *data = (const XrHapticPcmVibrationFB *)hapticEvent;

#define SET_OUT_VIBRATION(X)                                                                                           \
	if (act_attached->X.current.active && (subaction_paths.X || subaction_paths.any)) {                            \
		set_action_output_vibration_pcm(log, sess, &act_attached->X, data);                                    \
	}

		OXR_FOR_EACH_SUBACTION_PATH(SET_OUT_VIBRATION)
#undef SET_OUT_VIBRATION
#endif /* OXR_HAVE_FB_haptic_pcm */
	} else {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "Received haptic feedback of invalid type");
	}

	return oxr_session_success_focused_result(sess);
}

XrResult
oxr_action_stop_haptic_feedback(struct oxr_logger *log,
                                struct oxr_session *sess,
                                uint32_t act_key,
                                struct oxr_subaction_paths subaction_paths)
{
	struct oxr_action_attachment *act_attached = NULL;

	oxr_session_attached_actions_find(&sess->attached_actions, act_key, &act_attached);
	if (act_attached == NULL) {
		return oxr_error(log, XR_ERROR_ACTIONSET_NOT_ATTACHED, "Action has not been attached to this session");
	}

	bool is_focused = sess->state == XR_SESSION_STATE_FOCUSED;

#define STOP_VIBRATION(X)                                                                                              \
	if (is_focused && act_attached->X.current.active && (subaction_paths.X || subaction_paths.any)) {              \
		oxr_action_cache_stop_output(log, sess, &act_attached->X);                                             \
	}

	OXR_FOR_EACH_SUBACTION_PATH(STOP_VIBRATION)
#undef STOP_VIBRATION

	return oxr_session_success_focused_result(sess);
}
