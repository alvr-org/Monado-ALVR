// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRSystem interface version 003.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRSystem.hpp"
#include "XRTVRSystem_007.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRSystem_003 : public XRTVRSystem_007, public vr::IVRSystem_003
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRSystem_003(XRTVRClientCore_003 *core) : XRTVRSystem_007(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRSystem_003.
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

	bool
	LoadRenderModel(const char *pchRenderModelName, RenderModel_t *pRenderModel) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LoadRenderModel(pchRenderModelName=%s, pRenderModel=%p) -> false",
		                             false, pchRenderModelName, static_cast<const void *>(pRenderModel));
	}

	void
	FreeRenderModel(RenderModel_t *pRenderModel) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "FreeRenderModel(pRenderModel=%p)",
		                         static_cast<const void *>(pRenderModel));
	}

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

	bool
	HandleControllerOverlayInteractionAsMouse(const vr::Compositor_OverlaySettings &overlaySettings,
	                                          vr::HmdVector2_t vecWindowClientPositionOnScreen,
	                                          vr::HmdVector2_t vecWindowClientSize,
	                                          vr::TrackedDeviceIndex_t unControllerDeviceIndex,
	                                          vr::EVRControllerEventOutputType eOutputType) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "HandleControllerOverlayInteractionAsMouse(overlaySettings=?, vecWindowClientPositionOnScreen=?, "
		    "vecWindowClientSize=?, unControllerDeviceIndex=%llu, eOutputType=%d) -> false",
		    false, static_cast<unsigned long long>(unControllerDeviceIndex), static_cast<int>(eOutputType));
	}

	Forward_IVRSystem_CaptureInputFocus();
	Forward_IVRSystem_ReleaseInputFocus();
	Forward_IVRSystem_IsInputFocusCapturedByAnotherProcess();
};

}; // namespace xrt::state_trackers::openvr
