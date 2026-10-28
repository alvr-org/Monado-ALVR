// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of OpenVR event handling and related functionality.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#pragma once

#include "xrt/xrt_session.h"

#include "util/u_time.h"

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"

#include <queue>


namespace xrt::state_trackers::openvr {

struct Event
{
	timepoint_ns timestamp;
	vr::VREvent_t event;
};

struct Events
{
private: // Fields
	xrt_session *xs{nullptr};

	bool visible{false};
	bool focused{false};

	std::queue<Event> event_queue{};

public: // Methods
	Events(xrt_session *xs);

	void
	trackedDeviceActivated(vr::TrackedDeviceIndex_t index);

	void
	trackedDeviceUserInteractionStarted(vr::TrackedDeviceIndex_t index);

	//! Handles active XRT session events, and pushes corresponding OpenVR events to the event queue.
	xrt_result_t
	handleSessionEvents(openvr_logger &logger);

	//! Polls a single event from the event queue, if no event is available then `false` will be returned.
	bool
	pollEvent(openvr_logger &logger, vr::VREvent_t &out_event, uint32_t event_size);
};

}; // namespace xrt::state_trackers::openvr
