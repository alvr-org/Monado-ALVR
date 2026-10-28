// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of OpenVR event handling and related functionality.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#include "os/os_time.h"

#include "openvr_events.hpp"

#include <cstring>


namespace xrt::state_trackers::openvr {

/*
 *
 * Helper functions
 *
 */

static uint32_t
getEventSize(vr::EVREventType event_type)
{
	uint32_t event_size_header = sizeof(vr::VREvent_t) - sizeof(vr::VREvent_Data_t);

	uint32_t data_size = 0;

	// We have to disable this warning since some enumerations are missing from the OpenVR header.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"

	// Rough data sizes documented in openvr.h, we assume the largest if it's not documented.
	switch (event_type) {
	case vr::VREvent_ButtonPress:
	case vr::VREvent_ButtonUnpress:
	case vr::VREvent_ButtonTouch:
	case vr::VREvent_ButtonUntouch: data_size = sizeof(vr::VREvent_Controller_t); break;

	case vr::VREvent_FocusEnter:
	case vr::VREvent_FocusLeave:
	case vr::VREvent_OverlayFocusChanged:
	case (vr::EVREventType)504: // vr::VREvent_DashboardThumbSelected
	case (vr::EVREventType)505: // vr::VREvent_DashboardRequested
	case vr::VREvent_OverlayCreated:
	case vr::VREvent_OverlayDestroyed:
	case vr::VREvent_OverlayNameChanged: data_size = sizeof(vr::VREvent_Overlay_t); break;

	case vr::VREvent_ScrollDiscrete:
	case vr::VREvent_ScrollSmooth: data_size = sizeof(vr::VREvent_Scroll_t); break;

	case vr::VREvent_MouseMove:
	case vr::VREvent_MouseButtonDown:
	case vr::VREvent_MouseButtonUp:
	case vr::VREvent_TouchPadMove:
	case vr::VREvent_LockMousePosition:
	case vr::VREvent_UnlockMousePosition: data_size = sizeof(vr::VREvent_Mouse_t); break;

	case vr::VREvent_InputFocusCaptured:
	case vr::VREvent_InputFocusReleased:
	case (vr::EVREventType)402: // vr::VREvent_SceneFocusLost
	case (vr::EVREventType)403: // vr::VREvent_SceneFocusGained
	case vr::VREvent_SceneApplicationChanged:
	case (vr::EVREventType)405: // vr::VREvent_SceneFocusChanged
	case vr::VREvent_InputFocusChanged:
	case vr::VREvent_SceneApplicationUsingWrongGraphicsAdapter:
	case vr::VREvent_ActionBindingReloaded:
	case vr::VREvent_SceneAppPipeDisconnected:
	case vr::VREvent_Quit:
	case vr::VREvent_ProcessQuit:
	case (vr::EVREventType)702: // vr::VREvent_QuitAborted_UserPrompt
	case vr::VREvent_QuitAcknowledged:
	case vr::VREvent_Monitor_ShowHeadsetView:
	case vr::VREvent_Monitor_HideHeadsetView: data_size = sizeof(vr::VREvent_Process_t); break;

	case vr::VREvent_ShowUI: data_size = sizeof(vr::VREvent_ShowUI_t); break;

	case vr::VREvent_ShowDevTools: data_size = sizeof(vr::VREvent_ShowDevTools_t); break;

	case vr::VREvent_KeyboardClosed:
	case vr::VREvent_KeyboardCharInput:
	case vr::VREvent_KeyboardDone:
	case vr::VREvent_KeyboardOpened_Global:
	case vr::VREvent_KeyboardClosed_Global: data_size = sizeof(vr::VREvent_Keyboard_t); break;

	case vr::VREvent_Compositor_HDCPError: data_size = sizeof(vr::VREvent_HDCPError_t); break;

	case vr::VREvent_Input_HapticVibration: data_size = sizeof(vr::VREvent_HapticVibration_t); break;

	case vr::VREvent_Input_BindingLoadFailed:
	case vr::VREvent_Input_BindingLoadSuccessful: data_size = sizeof(vr::VREvent_InputBindingLoad_t); break;

	case vr::VREvent_Input_ActionManifestLoadFailed:
		data_size = sizeof(vr::VREvent_InputActionManifestLoad_t);
		break;

	case vr::VREvent_Input_ProgressUpdate: data_size = sizeof(vr::VREvent_ProgressUpdate_t); break;

	case vr::VREvent_SpatialAnchors_PoseUpdated:
	case vr::VREvent_SpatialAnchors_DescriptorUpdated:
	case vr::VREvent_SpatialAnchors_RequestPoseUpdate:
	case vr::VREvent_SpatialAnchors_RequestDescriptorUpdate: data_size = sizeof(vr::VREvent_SpatialAnchor_t); break;

	default: return sizeof(vr::VREvent_t); // We don't know what this is, assume it's the biggest possible
	}

#pragma GCC diagnostic pop

	return event_size_header + data_size;
}

/*
 *
 * Events implementations
 *
 */

Events::Events(xrt_session *xs) : xs(xs) {}

void
Events::trackedDeviceActivated(vr::TrackedDeviceIndex_t index)
{
	vr::VREvent_t event = {
	    .eventType = vr::VREvent_TrackedDeviceActivated,
	    .trackedDeviceIndex = index,
	    .eventAgeSeconds = 0.f,
	    .data = {},
	};

	this->event_queue.push({.timestamp = os_monotonic_get_ns(), .event = event});
}

void
Events::trackedDeviceUserInteractionStarted(vr::TrackedDeviceIndex_t index)
{
	vr::VREvent_t event = {
	    .eventType = vr::VREvent_TrackedDeviceUserInteractionStarted,
	    .trackedDeviceIndex = index,
	    .eventAgeSeconds = 0.f,
	    .data = {},
	};

	this->event_queue.push({.timestamp = os_monotonic_get_ns(), .event = event});
}

xrt_result_t
Events::handleSessionEvents(openvr_logger &logger)
{
	xrt_session_event xse = {};
	while (true) {
		xrt_result_t xret = xrt_session_poll_events(this->xs, &xse);
		if (xret != XRT_SUCCESS) {
			OPENVR_LOG_ERROR_XRET(logger, "Failed to poll session events", xret);
			return xret;
		}

		switch (xse.type) {
		case XRT_SESSION_EVENT_STATE_CHANGE: {
			xrt_session_event_state_change &state_change = xse.state;

			if (state_change.visible != this->visible) {
				// @todo dox the processes in question to the application
				vr::VREvent_t event = {
				    .eventType = vr::VREvent_SceneApplicationChanged,
				    .trackedDeviceIndex = vr::k_unTrackedDeviceIndexInvalid,
				    .eventAgeSeconds = 0.f,
				    .data = {.process =
				                 {
				                     .pid = 0,
				                     .oldPid = 0,
				                     .bForced = false,
				                     .bConnectionLost = false,
				                 }},
				};

				this->event_queue.push({.timestamp = state_change.timestamp_ns, .event = event});
			}

			if (state_change.focused != this->focused) {
				// @todo dox the processes in question to the application
				vr::VREvent_t event = {
				    .eventType = vr::VREvent_InputFocusChanged,
				    .trackedDeviceIndex = vr::k_unTrackedDeviceIndexInvalid,
				    .eventAgeSeconds = 0.f,
				    .data = {.process =
				                 {
				                     .pid = 0,
				                     .oldPid = 0,
				                     .bForced = false,
				                     .bConnectionLost = false,
				                 }},
				};

				this->event_queue.push({.timestamp = state_change.timestamp_ns, .event = event});

				// Push the deprecated forms of these events as well.
				event.eventType = state_change.focused ? vr::VREvent_InputFocusCaptured
				                                       : vr::VREvent_InputFocusReleased;
				this->event_queue.push({.timestamp = state_change.timestamp_ns, .event = event});
			}

			this->visible = state_change.visible;
			this->focused = state_change.focused;

			break;
		}
		default: continue;                               // Unknown event, keep going
		case XRT_SESSION_EVENT_NONE: return XRT_SUCCESS; // No events left
		}
	}

	assert(!"unreachable");
}

bool
Events::pollEvent(openvr_logger &logger, vr::VREvent_t &out_event, uint32_t event_size)
{
	if (this->event_queue.empty()) {
		return false;
	}

	auto &event = this->event_queue.front();
	this->event_queue.pop();

	auto &vr_event = event.event;

	uint32_t required_size = getEventSize(static_cast<vr::EVREventType>(vr_event.eventType));

	// Event unsupported by client?
	if (event_size < required_size) {
		OPENVR_LOG_ERROR(logger,
		                 "Provided event buffer size %u is too small for event type %d, required size is %u",
		                 static_cast<unsigned int>(event_size), vr_event.eventType, required_size);
		return false;
	}

	vr_event.eventAgeSeconds = (os_monotonic_get_ns() - event.timestamp) / (float)U_TIME_1S_IN_NS;

	// Copy specifically the parts that we need to, so we don't overflow the output pointer.
	std::memcpy(&out_event, &vr_event, required_size);

	OPENVR_LOG_TRACE(logger, "Polled event type %d (size %u)", vr_event.eventType, required_size);

	return true;
}

}; // namespace xrt::state_trackers::openvr
