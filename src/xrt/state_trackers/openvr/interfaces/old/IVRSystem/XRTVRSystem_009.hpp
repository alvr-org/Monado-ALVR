// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRSystem interface version 009.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRSystem.hpp"
#include "XRTVRSystem_010.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRSystem_009 : public XRTVRSystem_010, public vr::IVRSystem_009
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRSystem_009(XRTVRClientCore_003 *core) : XRTVRSystem_010(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRSystem_009.
		return nullptr;
	}

	ForwardDeclareBase_IVRSystem(XRTVRSystem_010);

	Forward_IVRSystem_GetRecommendedRenderTargetSize();
	Forward_IVRSystem_GetProjectionMatrix_014();
	Forward_IVRSystem_GetProjectionRaw_026();
	Forward_IVRSystem_ComputeDistortion_012();
	Forward_IVRSystem_GetEyeToHeadTransform_026();
	Forward_IVRSystem_GetTimeSinceLastVsync();
	Forward_IVRSystem_GetD3D9AdapterIndex();
	Forward_IVRSystem_GetDXGIOutputInfo_026();
	Forward_IVRSystem_IsDisplayOnDesktop();
	Forward_IVRSystem_SetDisplayVisibility();
	Forward_IVRSystem_GetDeviceToAbsoluteTrackingPose_026();
	Forward_IVRSystem_ResetSeatedZeroPose();
	Forward_IVRSystem_GetSeatedZeroPoseToStandingAbsoluteTrackingPose();
	Forward_IVRSystem_GetRawZeroPoseToStandingAbsoluteTrackingPose();
	Forward_IVRSystem_GetSortedTrackedDeviceIndicesOfClass_026();
	Forward_IVRSystem_GetTrackedDeviceActivityLevel();
	Forward_IVRSystem_ApplyTransform_026();
	Forward_IVRSystem_GetTrackedDeviceClass_026();
	Forward_IVRSystem_IsTrackedDeviceConnected();
	Forward_IVRSystem_GetBoolTrackedDeviceProperty_026();
	Forward_IVRSystem_GetFloatTrackedDeviceProperty_026();
	Forward_IVRSystem_GetInt32TrackedDeviceProperty_026();
	Forward_IVRSystem_GetUint64TrackedDeviceProperty_026();
	Forward_IVRSystem_GetMatrix34TrackedDeviceProperty_026();
	Forward_IVRSystem_GetStringTrackedDeviceProperty_026();
	Forward_IVRSystem_GetPropErrorNameFromEnum_026();
	Forward_IVRSystem_PollNextEvent_010();
	Forward_IVRSystem_PollNextEventWithPose_010();
	Forward_IVRSystem_GetEventTypeNameFromEnum();
	Forward_IVRSystem_GetHiddenAreaMesh_012();
	Forward_IVRSystem_GetControllerState_012();
	Forward_IVRSystem_GetControllerStateWithPose_012();
	Forward_IVRSystem_TriggerHapticPulse();
	Forward_IVRSystem_GetButtonIdNameFromEnum();
	Forward_IVRSystem_GetControllerAxisTypeNameFromEnum();
	Forward_IVRSystem_CaptureInputFocus();
	Forward_IVRSystem_ReleaseInputFocus();
	Forward_IVRSystem_IsInputFocusCapturedByAnotherProcess();
	Forward_IVRSystem_DriverDebugRequest_017();
	Forward_IVRSystem_PerformFirmwareUpdate_026();
	Forward_IVRSystem_AcknowledgeQuit_Exiting();
	Forward_IVRSystem_AcknowledgeQuit_UserPrompt();
};

}; // namespace xrt::state_trackers::openvr
