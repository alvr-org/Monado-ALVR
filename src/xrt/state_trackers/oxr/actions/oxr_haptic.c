// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Haptic related functions.
 * @ingroup oxr_api
 */

#include "xrt/xrt_compiler.h"

#include "util/u_trace_marker.h"

#include "oxr_haptic.h"
#include "oxr_input.h"
#include "oxr_subaction.h"
#include "oxr_session_action_context.h"

#include "../oxr_objects.h"


static bool
get_action_output_pcm_sample_rate(struct oxr_action_cache *cache, float *sample_rate)
{
	for (uint32_t i = 0; i < cache->output_count; i++) {
		struct oxr_action_output *output = &cache->outputs[i];

		struct xrt_output_limits output_limits;
		xrt_result_t result = xrt_io_cache_get_output_limits( //
		    output->io_cache->xic,                            //
		    output->device_id,                                //
		    &output_limits);                                  //
		if (result != XRT_SUCCESS) {
			// default to something sane
			output_limits = (struct xrt_output_limits){0};
		}

		(*sample_rate) = output_limits.haptic_pcm_sample_rate;
		if (output_limits.haptic_pcm_sample_rate > 0) {
			return true;
		}
	}

	return false;
}

XrResult
oxr_haptic_get_attachment_pcm_sample_rate(struct oxr_action_attachment *act_attached,
                                          const struct oxr_subaction_paths subaction_paths,
                                          float *sample_rate)
{
	// find any device with a valid sample rate, and return it
#define GET_SAMPLE_RATE(X)                                                                                             \
	if (subaction_paths.X || subaction_paths.any) {                                                                \
		if (get_action_output_pcm_sample_rate(&act_attached->X, sample_rate)) {                                \
			return XR_SUCCESS;                                                                             \
		}                                                                                                      \
	}

	OXR_FOR_EACH_SUBACTION_PATH(GET_SAMPLE_RATE)
#undef GET_SAMPLE_RATE

	// no devices found
	*sample_rate = 0;

	return XR_SUCCESS;
}
