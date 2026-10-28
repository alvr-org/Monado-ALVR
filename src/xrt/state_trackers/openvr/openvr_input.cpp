// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of OpenVR input.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#include "openvr_input.hpp"

#include <mutex>


namespace {

static bool
verifyDominantHand(vr::ETrackedControllerRole role)
{
	switch (role) {
	case vr::ETrackedControllerRole::TrackedControllerRole_LeftHand:
	case vr::ETrackedControllerRole::TrackedControllerRole_RightHand: return true;
	default: return false;
	}
}

}; // namespace

namespace xrt::state_trackers::openvr {

Input::Input() : lock(), action_set_handles(), action_handles(), input_source_handles() {}

vr::EVRInputError
Input::setDominantHand(vr::ETrackedControllerRole dominant_hand)
{
	// @todo this should only work for apps which specify supporting this in their manifest. otherwise it should
	//       throw an error.

	if (!verifyDominantHand(dominant_hand)) {
		return vr::EVRInputError::VRInputError_InvalidParam;
	}

	std::unique_lock<os::Mutex> lock(this->lock);

	this->dominant_hand = dominant_hand;

	return vr::EVRInputError::VRInputError_None;
}

vr::EVRInputError
Input::getDominantHand(vr::ETrackedControllerRole &dominant_hand)
{
	std::unique_lock<os::Mutex> lock(this->lock);

	dominant_hand = this->dominant_hand;

	return vr::EVRInputError::VRInputError_None;
}

vr::EVRInputError
Input::getActionSetHandle(const std::string &name, vr::VRActionSetHandle_t &handle)
{
	std::unique_lock<os::Mutex> lock(this->lock);

	auto it = this->action_set_handles.find(name);
	if (it == this->action_set_handles.end()) {
		handle = static_cast<vr::VRActionSetHandle_t>(xrt_atomic_s64_inc_return(&this->next_handle));
		it = this->action_set_handles.emplace(name, handle).first;
	}

	return vr::EVRInputError::VRInputError_None;
}

vr::EVRInputError
Input::getActionHandle(const std::string &name, vr::VRActionHandle_t &handle)
{
	std::unique_lock<os::Mutex> lock(this->lock);

	auto it = this->action_handles.find(name);
	if (it == this->action_handles.end()) {
		handle = static_cast<vr::VRActionHandle_t>(xrt_atomic_s64_inc_return(&this->next_handle));
		it = this->action_handles.emplace(name, handle).first;
	}

	return vr::EVRInputError::VRInputError_None;
}

vr::EVRInputError
Input::getInputSourceHandle(const std::string &path, vr::VRInputValueHandle_t &handle)
{
	std::unique_lock<os::Mutex> lock(this->lock);

	auto it = this->input_source_handles.find(path);
	if (it == this->input_source_handles.end()) {
		handle = static_cast<vr::VRInputValueHandle_t>(xrt_atomic_s64_inc_return(&this->next_handle));
		it = this->input_source_handles.emplace(path, handle).first;
	}

	return vr::EVRInputError::VRInputError_None;
}

}; // namespace xrt::state_trackers::openvr
