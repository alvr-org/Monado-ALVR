// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief C++ base implementation of @ref xrt_hand_tracker.
 * @ingroup base
 */

#pragma once

#include "b_hand_tracker.h"

#include "glue/g_hand_tracker.hpp"

#include <array>


/*!
 * How the base hand tracker selects backing hand-tracking devices.
 *
 * System mode follows the static hand-tracking roles on @ref xrt_system_devices.
 * Device-locked mode ignores those roles and only considers the
 * @ref xrt_hand_tracker_create_info::locked_xdev supplied at creation.
 * Callback mode asks a caller-provided callback for the ordered devices to use.
 */
enum class HandTrackerMode
{
	System,
	DeviceLocked,
	Callback,
};

class HandTracker : public xrt::util::HandTrackerBase<HandTracker>
{
public: // Methods
	HandTracker(xrt_system_devices *xsysd,
	            const xrt_hand_tracker_create_info &info,
	            b_hand_tracker_get_devices_func_t getDevices,
	            void *callbackData);

	xrt_result_t
	init();

	xrt_result_t
	locate(xrt_space_overseer *xso,
	       xrt_space *base_space,
	       const xrt_pose *base_offset,
	       int64_t at_timestamp_ns,
	       xrt_hand_tracker_location *out_location);

	xrt_result_t
	setOutput(xrt_output_name name, const xrt_output_value *value);

	static void
	destroyHandTracker(struct xrt_hand_tracker *xht);


private: // Types
	struct Source
	{
		xrt_device *xdev{nullptr};
		xrt_input_name input_name{XRT_INPUT_GENERIC_HEAD_POSE};
	};


private: // Members
	struct xrt_system_devices *mSystemDevices{nullptr};
	xrt_hand_tracker_create_info mInfo{};
	HandTrackerMode mMode{HandTrackerMode::System};
	b_hand_tracker_get_devices_func_t mGetDevices{nullptr};
	void *mCallbackData{nullptr};

	std::array<Source, 6> mInputs{};
	uint32_t mInputCount{0};

	//! Callback mode is intentionally ignored here: output goes only to static/locked devices.
	std::array<struct xrt_device *, 2> mOutputs{};
	uint32_t mOutputCount{0};


private: // Methods
	xrt_result_t
	resolveSource(enum xrt_input_name input_name, Source &out_source);

	xrt_result_t
	addCallbackInputs(xrt_input_name input_name);

	xrt_result_t
	addInput(const Source &source);

	void
	addOutput(struct xrt_device *xdev);
};
