// Copyright 2022-2026, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Mercury hand size optimization heuristics
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @ingroup tracking
 */
#include "hg_hand_size_opt.hpp"
#include <cmath>

namespace xrt::tracking::hand::mercury {

bool
HandSizeRefinement::framePreOptimizer(std::span<const ViewData, kNumViews> views,
                                      std::span<const HandData, kNumHands> hands)
{
	// Reset the per-frame data
	frameData = {};

	{
		bool any_hands_are_only_visible_in_one_view = false;
		for (uint32_t hand_idx = 0; hand_idx < kNumHands; hand_idx++) {
			any_hands_are_only_visible_in_one_view =              //
			    any_hands_are_only_visible_in_one_view ||         //
			    (views[0].viewHand[hand_idx].foundROIthisFrame != //
			     views[1].viewHand[hand_idx].foundROIthisFrame);
		}
		frameData.any_hands_are_only_visible_in_one_view = any_hands_are_only_visible_in_one_view;
	}

	constexpr float mul_max = 1.0;
	constexpr float frame_max = 100;

	if ((data.hand_size_refinement_schedule_x > frame_max)) {
		data.hand_size_refinement_schedule_y = mul_max;
		data.optimizing = false;
	} else {
		data.hand_size_refinement_schedule_y =
		    powf((data.hand_size_refinement_schedule_x / frame_max), 2) * mul_max;
		data.optimizing = true;
	}

	// if either hand was not visible before the last new-user event but is visible now, reset the schedule
	// a bit.
	if ((hands[0].this_frame_hand_detected && !hands[0].hand_seen_before) ||
	    (hands[1].this_frame_hand_detected && !hands[1].hand_seen_before)) {
		data.hand_size_refinement_schedule_x = std::min(data.hand_size_refinement_schedule_x, frame_max / 2);
	}

	if (frameData.any_hands_are_only_visible_in_one_view) {
		// TODO we return false, but we potentially leave optimizing = true ?
		return false;
	}

	return data.optimizing;
}

static float
hand_confidence_value(float reprojection_error, const one_frame_input &input)
{
	float out_confidence = 0.0f;
	for (int view_idx = 0; view_idx < 2; view_idx++) {
		for (int i = 0; i < 21; i++) {
			// whatever
			out_confidence += input.views[view_idx].keypoints_in_scaled_stereographic[i].confidence_xy;
		}
	}
	out_confidence /= 42.0f; // number of hand joints
	float reproj_err_mul = 1.0f / ((reprojection_error * 10) + 1.0f);
	return out_confidence * reproj_err_mul;
}

void
HandSizeRefinement::frameHandPostOptimizer(float hand_size, float reprojection_error, const one_frame_input &input)
{
	if (!frameData.any_hands_are_only_visible_in_one_view) {
		data.hand_size_refinement_schedule_x += hand_confidence_value(reprojection_error, input);
	}
	frameData.m_accum_hand_size += hand_size;
	frameData.num_hands++;
}

void
HandSizeRefinement::framePost()
{
	if (frameData.num_hands > 0) {
		data.target_hand_size = frameData.m_accum_hand_size / (float)frameData.num_hands;
	}
}

void
HandSizeRefinement::newUserEvent()
{
	data = {};
}

} // namespace xrt::tracking::hand::mercury
