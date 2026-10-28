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

#include "kine_common.hpp"
#include <span>

namespace xrt::tracking::hand::mercury {

namespace detail {} // namespace detail

constexpr uint32_t kNumHands = 2;
constexpr uint32_t kNumViews = 2;

/*!
 * Hides the details of how hand size refinement/optimization is managed.
 *
 */
class HandSizeRefinement
{
public:
	HandSizeRefinement() = default;

	/// Should the hand size be optimized (normally)
	bool
	isOptimizing() const noexcept
	{
		return data.optimizing;
	}

	/// Y component of hand size refinement schedule.
	float
	getHandSizeErrorMul() const noexcept
	{
		return data.hand_size_refinement_schedule_y;
	}

	/// Return the current target hand size
	float
	getTargetHandSize() const noexcept
	{
		return data.target_hand_size;
	}

	/// Per-hand data that goes in a per-view structure
	struct ViewHandData
	{
		bool foundROIthisFrame = false;
	};

	/// Per-view input data structure
	struct ViewData
	{
		ViewHandData viewHand[kNumHands];
	};

	/// Per-hand input data structure
	struct HandData
	{
		bool this_frame_hand_detected = false;
		bool hand_seen_before = false;
	};

	/*!
	 * Perform the updates for a single frame before running the optimizer.
	 *
	 * @param[in] views Per-view data
	 * @param[in] hands Per-hand data over all views
	 * @param[out] out_target_hand_size Populated with the current target hand size, used as optimizer input among
	 * other things.
	 *
	 * @return true if the optimizer should optimize hand size
	 */
	bool
	framePreOptimizer(std::span<const ViewData, kNumViews> views, std::span<const HandData, kNumHands> hands);

	/*!
	 * Call after your optimizer run for a single hand.
	 *
	 * Accumulates per-hand data and also advances the schedule.
	 *
	 * @param hand_size Hand size output from the optimizer
	 * @param reprojection_error Reprojection error from the optimizer
	 * @param input Some keypoint estimator outputs
	 */
	void
	frameHandPostOptimizer(float hand_size, float reprojection_error, const one_frame_input &input);

	/*!
	 * Finish processing for a frame, once no more hands are left.
	 *
	 * Updates target hand size if applicable.
	 */
	void
	framePost();

	/// Reset state after a new user is found
	void
	newUserEvent();

	/// Pointers that the debug gui wants
	void
	get_refinement_schedule_ptrs(float *&hand_size_refinement_schedule_x, float *&hand_size_refinement_schedule_y)
	{
		hand_size_refinement_schedule_x = &this->data.hand_size_refinement_schedule_x;
		hand_size_refinement_schedule_y = &this->data.hand_size_refinement_schedule_y;
	}

private:
	struct HandSizeRefinementData
	{
		float hand_size_refinement_schedule_x = 0;
		float hand_size_refinement_schedule_y = 0;
		bool optimizing = true;
		float target_hand_size = STANDARD_HAND_SIZE;
	};

	/// State data reset on a new user event
	HandSizeRefinementData data{};

	struct FrameData
	{
		float m_accum_hand_size = 0;
		uint8_t num_hands = 0;
		bool any_hands_are_only_visible_in_one_view = false;
	};

	/// State data only used for a single frame
	FrameData frameData{};
};


} // namespace xrt::tracking::hand::mercury
