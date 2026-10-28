// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRSystem interface version 007.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRSystem.hpp"
#include "XRTVRSystem_010.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRSystem_007 : public XRTVRSystem_010, public vr::IVRSystem_007
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRSystem_007(XRTVRClientCore_003 *core) : XRTVRSystem_010(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRSystem_007.
		return nullptr;
	}

	ForwardDeclareBase_IVRSystem(XRTVRSystem_010);

	void
	GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		// i guess this is fine?

		(*pnX) = 0;
		(*pnY) = 0;

		ForwardBase_IVRSystem::GetRecommendedRenderTargetSize(pnWidth, pnHeight);

		(*pnWidth) *= 2;
	}

	Forward_IVRSystem_GetRecommendedRenderTargetSize();

	void
	GetEyeOutputViewport(Hmd_Eye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		(*pnX) = 0;
		(*pnY) = 0;

		ForwardBase_IVRSystem::GetRecommendedRenderTargetSize(pnWidth, pnHeight);

		(*pnWidth) *= 2;

		if (eEye != Hmd_Eye::Eye_Left) {
			// Half the window width over
			(*pnX) += (*pnWidth) / 2;
		}

		OPENVR_LOG_UNIMPLEMENTED(logger,
		                         "GetEyeOutputViewport(eEye=?, pnX=%p, pnY=%p, pnWidth=%p, pnHeight=%p)",
		                         static_cast<const void *>(pnX), static_cast<const void *>(pnY),
		                         static_cast<const void *>(pnWidth), static_cast<const void *>(pnHeight));
	}

	Forward_IVRSystem_GetProjectionMatrix_014();
	Forward_IVRSystem_GetProjectionRaw_026();
	Forward_IVRSystem_ComputeDistortion_012();
	Forward_IVRSystem_GetEyeToHeadTransform_026();
	Forward_IVRSystem_GetTimeSinceLastVsync();
	Forward_IVRSystem_GetD3D9AdapterIndex();

	void
	GetDXGIOutputInfo(int32_t *pnAdapterIndex, int32_t *pnAdapterOutputIndex) override
	{
		int32_t adapterIndex;
		ForwardBase_IVRSystem::GetDXGIOutputInfo(&adapterIndex);

		(*pnAdapterIndex) = adapterIndex;
		(*pnAdapterOutputIndex) = adapterIndex;
	}

	bool
	AttachToWindow(void *hWnd) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "AttachToWindow(hWnd=%p) -> true", true,
		                             static_cast<const void *>(hWnd));
	}

	Forward_IVRSystem_GetDeviceToAbsoluteTrackingPose_026();
	Forward_IVRSystem_ResetSeatedZeroPose();
	Forward_IVRSystem_GetSeatedZeroPoseToStandingAbsoluteTrackingPose();
	Forward_IVRSystem_GetRawZeroPoseToStandingAbsoluteTrackingPose();
	Forward_IVRSystem_GetSortedTrackedDeviceIndicesOfClass_026();
	Forward_IVRSystem_GetTrackedDeviceActivityLevel();

	void
	ApplyTransform(TrackedDevicePose_t *pOutputPose,
	               const TrackedDevicePose_t &trackedDevicePose,
	               const HmdMatrix34_t &transform) override
	{
		return ForwardBase_IVRSystem::ApplyTransform(pOutputPose, &trackedDevicePose, &transform);
	}

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
	Forward_IVRSystem_IsDisplayOnDesktop();
	Forward_IVRSystem_SetDisplayVisibility();
};

}; // namespace xrt::state_trackers::openvr
