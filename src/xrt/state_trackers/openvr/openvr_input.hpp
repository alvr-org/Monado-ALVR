// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of OpenVR input.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#pragma once

#include "os/os_threading.h"

#include "openvr_devices.hpp"

#include <unordered_map>


namespace xrt::state_trackers::openvr {

namespace os = xrt::auxiliary::os;

struct Input
{
private: // Fields
	// Handle 0 is reserved (k_ulInvalidActionSetHandle / k_ulInvalidActionHandle /
	// k_ulInvalidInputValueHandle are all 0), so allocation starts at 1. Shared for simplicity.
	xrt_atomic_s64_t next_handle{1};

	mutable os::Mutex lock;

	std::unordered_map<std::string, vr::VRActionSetHandle_t> action_set_handles;
	std::unordered_map<std::string, vr::VRActionHandle_t> action_handles;
	std::unordered_map<std::string, vr::VRInputValueHandle_t> input_source_handles;

	vr::ETrackedControllerRole dominant_hand{vr::ETrackedControllerRole::TrackedControllerRole_RightHand};

public: // Methods
	Input();

	// State
	vr::EVRInputError
	setDominantHand(vr::ETrackedControllerRole dominant_hand);

	vr::EVRInputError
	getDominantHand(vr::ETrackedControllerRole &dominant_hand);

	// Handles
	vr::EVRInputError
	getActionSetHandle(const std::string &name, vr::VRActionSetHandle_t &handle);

	vr::EVRInputError
	getActionHandle(const std::string &name, vr::VRActionHandle_t &handle);

	vr::EVRInputError
	getInputSourceHandle(const std::string &name, vr::VRInputValueHandle_t &handle);
};

}; // namespace xrt::state_trackers::openvr
