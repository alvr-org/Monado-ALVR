// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRSystem interface version 006.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRSystem.hpp"
#include "XRTVRSystem_007.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRSystem_006 : public XRTVRSystem_007, public vr::IVRSystem_006
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRSystem_006(XRTVRClientCore_003 *core) : XRTVRSystem_007(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRSystem_006.
		return nullptr;
	}

	ForwardDeclareBase_IVRSystem(XRTVRSystem_007);

	Forward_IVRSystem_GetWindowBounds();
	Forward_IVRSystem_GetRecommendedRenderTargetSize();
	Forward_IVRSystem_GetEyeOutputViewport();
	Forward_IVRSystem_GetProjectionMatrix_007();
	Forward_IVRSystem_GetProjectionRaw_007();
	Forward_IVRSystem_ComputeDistortion_007();
	Forward_IVRSystem_GetEyeToHeadTransform_007();
	Forward_IVRSystem_GetTimeSinceLastVsync();
	Forward_IVRSystem_GetD3D9AdapterIndex();
	Forward_IVRSystem_GetDXGIOutputInfo_007();
	Forward_IVRSystem_AttachToWindow();
	Forward_IVRSystem_GetDeviceToAbsoluteTrackingPose_007();
	Forward_IVRSystem_ResetSeatedZeroPose();
	Forward_IVRSystem_GetSeatedZeroPoseToStandingAbsoluteTrackingPose();
	Forward_IVRSystem_GetRawZeroPoseToStandingAbsoluteTrackingPose();
	Forward_IVRSystem_GetSortedTrackedDeviceIndicesOfClass_007();
	Forward_IVRSystem_GetTrackedDeviceActivityLevel();
	Forward_IVRSystem_GetTrackedDeviceClass_007();
	Forward_IVRSystem_IsTrackedDeviceConnected();
	Forward_IVRSystem_GetBoolTrackedDeviceProperty_007();
	Forward_IVRSystem_GetFloatTrackedDeviceProperty_007();
	Forward_IVRSystem_GetInt32TrackedDeviceProperty_007();
	Forward_IVRSystem_GetUint64TrackedDeviceProperty_007();
	Forward_IVRSystem_GetMatrix34TrackedDeviceProperty_007();
	Forward_IVRSystem_GetStringTrackedDeviceProperty_007();
	Forward_IVRSystem_GetPropErrorNameFromEnum_007();
	Forward_IVRSystem_PollNextEvent_010();
	Forward_IVRSystem_PollNextEventWithPose_007();
	Forward_IVRSystem_GetEventTypeNameFromEnum();
	Forward_IVRSystem_GetHiddenAreaMesh_007();
	Forward_IVRSystem_GetControllerState_012();
	Forward_IVRSystem_GetControllerStateWithPose_007();
	Forward_IVRSystem_TriggerHapticPulse();
	Forward_IVRSystem_GetButtonIdNameFromEnum();
	Forward_IVRSystem_GetControllerAxisTypeNameFromEnum();
	Forward_IVRSystem_CaptureInputFocus();
	Forward_IVRSystem_ReleaseInputFocus();
	Forward_IVRSystem_IsInputFocusCapturedByAnotherProcess();
	Forward_IVRSystem_DriverDebugRequest_017();
	Forward_IVRSystem_PerformFirmwareUpdate_007();
	Forward_IVRSystem_IsDisplayOnDesktop();
	Forward_IVRSystem_SetDisplayVisibility();
};

}; // namespace xrt::state_trackers::openvr
