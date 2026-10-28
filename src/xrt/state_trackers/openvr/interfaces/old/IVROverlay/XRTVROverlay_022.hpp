// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVROverlay interface version 022.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVROverlay.hpp"
#include "XRTVROverlay_026.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVROverlay_022 : public XRTVROverlay_026, public vr::IVROverlay_022
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVROverlay_022(XRTVRClientCore_003 *core) : XRTVROverlay_026(core) {};
	// clang-format on

	FnTable_IVROverlay_022();

	ForwardDeclareBase_IVROverlay(XRTVROverlay_026);

	Forward_IVROverlay_FindOverlay_028();
	Forward_IVROverlay_CreateOverlay_028();
	Forward_IVROverlay_DestroyOverlay_028();
	Forward_IVROverlay_GetOverlayKey_028();
	Forward_IVROverlay_GetOverlayName_028();
	Forward_IVROverlay_SetOverlayName();
	Forward_IVROverlay_GetOverlayImageData_028();
	Forward_IVROverlay_GetOverlayErrorNameFromEnum_028();
	Forward_IVROverlay_SetOverlayRenderingPid();
	Forward_IVROverlay_GetOverlayRenderingPid();
	Forward_IVROverlay_SetOverlayFlag_028();
	Forward_IVROverlay_GetOverlayFlag_028();
	Forward_IVROverlay_GetOverlayFlags();
	Forward_IVROverlay_SetOverlayColor_028();
	Forward_IVROverlay_GetOverlayColor_028();
	Forward_IVROverlay_SetOverlayAlpha_028();
	Forward_IVROverlay_GetOverlayAlpha_028();
	Forward_IVROverlay_SetOverlayTexelAspect();
	Forward_IVROverlay_GetOverlayTexelAspect();
	Forward_IVROverlay_SetOverlaySortOrder();
	Forward_IVROverlay_GetOverlaySortOrder();
	Forward_IVROverlay_SetOverlayWidthInMeters_028();
	Forward_IVROverlay_GetOverlayWidthInMeters_028();
	Forward_IVROverlay_SetOverlayCurvature();
	Forward_IVROverlay_GetOverlayCurvature();
	Forward_IVROverlay_SetOverlayTextureColorSpace();
	Forward_IVROverlay_GetOverlayTextureColorSpace();
	Forward_IVROverlay_SetOverlayTextureBounds_028();
	Forward_IVROverlay_GetOverlayTextureBounds_028();

	uint32_t
	GetOverlayRenderModel(vr::VROverlayHandle_t ulOverlayHandle,
	                      VR_OUT_STRING() char *pchValue,
	                      uint32_t unBufferSize,
	                      HmdColor_t *pColor,
	                      vr::EVROverlayError *pError) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		SET_ERROR(pError, vr::EVROverlayError::VROverlayError_None);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetOverlayRenderModel(ulOverlayHandle=%llu, pchValue=%p, "
		                             "unBufferSize=%llu, pColor=%p, pError=%p) -> 0",
		                             0, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<const void *>(pchValue),
		                             static_cast<unsigned long long>(unBufferSize),
		                             static_cast<const void *>(pColor), static_cast<const void *>(pError));
	}

	vr::EVROverlayError
	SetOverlayRenderModel(vr::VROverlayHandle_t ulOverlayHandle,
	                      const char *pchRenderModel,
	                      const HmdColor_t *pColor) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayRenderModel(ulOverlayHandle=%llu, pchRenderModel=%s, pColor=%p) -> 0",
		    vr::EVROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    pchRenderModel, static_cast<const void *>(pColor));
	}

	Forward_IVROverlay_GetOverlayTransformType_028();
	Forward_IVROverlay_SetOverlayTransformAbsolute_028();
	Forward_IVROverlay_GetOverlayTransformAbsolute_028();
	Forward_IVROverlay_SetOverlayTransformTrackedDeviceRelative_028();
	Forward_IVROverlay_GetOverlayTransformTrackedDeviceRelative_028();
	Forward_IVROverlay_SetOverlayTransformTrackedDeviceComponent();
	Forward_IVROverlay_GetOverlayTransformTrackedDeviceComponent_028();
	Forward_IVROverlay_GetOverlayTransformOverlayRelative();
	Forward_IVROverlay_SetOverlayTransformOverlayRelative();
	Forward_IVROverlay_SetOverlayTransformCursor();
	Forward_IVROverlay_GetOverlayTransformCursor();
	Forward_IVROverlay_ShowOverlay_028();
	Forward_IVROverlay_HideOverlay_028();
	Forward_IVROverlay_IsOverlayVisible();
	Forward_IVROverlay_GetTransformForOverlayCoordinates();
	Forward_IVROverlay_PollNextOverlayEvent_028();
	Forward_IVROverlay_GetOverlayInputMethod_028();
	Forward_IVROverlay_SetOverlayInputMethod_028();
	Forward_IVROverlay_GetOverlayMouseScale_028();
	Forward_IVROverlay_SetOverlayMouseScale_028();
	Forward_IVROverlay_ComputeOverlayIntersection();
	Forward_IVROverlay_IsHoverTargetOverlay();

	EVROverlayError
	SetOverlayDualAnalogTransform(VROverlayHandle_t ulOverlay,
	                              EDualAnalogWhich eWhich,
	                              const HmdVector2_t *pvCenter,
	                              float fRadius) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "SetOverlayDualAnalogTransform(ulOverlay=%llu, eWhich=?, pvCenter=%p, "
		                             "fRadius=%f) -> VROverlayError_None",
		                             EVROverlayError::VROverlayError_None,
		                             static_cast<unsigned long long>(ulOverlay),
		                             static_cast<const void *>(pvCenter), fRadius);
	}

	EVROverlayError
	GetOverlayDualAnalogTransform(VROverlayHandle_t ulOverlay,
	                              EDualAnalogWhich eWhich,
	                              HmdVector2_t *pvCenter,
	                              float *pfRadius) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetOverlayDualAnalogTransform(ulOverlay=%llu, eWhich=?, pvCenter=%p, "
		                             "pfRadius=%p) -> VROverlayError_None",
		                             EVROverlayError::VROverlayError_None,
		                             static_cast<unsigned long long>(ulOverlay),
		                             static_cast<const void *>(pvCenter), static_cast<const void *>(pfRadius));
	}

	Forward_IVROverlay_SetOverlayIntersectionMask();
	Forward_IVROverlay_TriggerLaserMouseHapticVibration();
	Forward_IVROverlay_SetOverlayCursor();
	Forward_IVROverlay_SetOverlayCursorPositionOverride();
	Forward_IVROverlay_ClearOverlayCursorPositionOverride();
	Forward_IVROverlay_SetOverlayTexture_028();
	Forward_IVROverlay_ClearOverlayTexture_028();
	Forward_IVROverlay_SetOverlayRaw_028();
	Forward_IVROverlay_SetOverlayFromFile_028();
	Forward_IVROverlay_GetOverlayTexture_028();
	Forward_IVROverlay_ReleaseNativeOverlayHandle();
	Forward_IVROverlay_GetOverlayTextureSize();
	Forward_IVROverlay_CreateDashboardOverlay_028();
	Forward_IVROverlay_IsDashboardVisible();
	Forward_IVROverlay_IsActiveDashboardOverlay();
	Forward_IVROverlay_SetDashboardOverlaySceneProcess_028();
	Forward_IVROverlay_GetDashboardOverlaySceneProcess_028();
	Forward_IVROverlay_ShowDashboard();
	Forward_IVROverlay_GetPrimaryDashboardDevice();

	EVROverlayError
	ShowKeyboard(EGamepadTextInputMode eInputMode,
	             EGamepadTextInputLineMode eLineInputMode,
	             const char *pchDescription,
	             uint32_t unCharMax,
	             const char *pchExistingText,
	             bool bUseMinimalMode,
	             uint64_t uUserValue) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "ShowKeyboard(eInputMode=?, eLineInputMode=?, pchDescription=%s, "
		    "unCharMax=%llu, pchExistingText=%s, bUseMinimalMode=%d, uUserValue=%llu) -> VROverlayError_None",
		    EVROverlayError::VROverlayError_None, pchDescription, static_cast<unsigned long long>(unCharMax),
		    pchExistingText, static_cast<int>(bUseMinimalMode), static_cast<unsigned long long>(uUserValue));
	}

	EVROverlayError
	ShowKeyboardForOverlay(VROverlayHandle_t ulOverlayHandle,
	                       EGamepadTextInputMode eInputMode,
	                       EGamepadTextInputLineMode eLineInputMode,
	                       const char *pchDescription,
	                       uint32_t unCharMax,
	                       const char *pchExistingText,
	                       bool bUseMinimalMode,
	                       uint64_t uUserValue) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "ShowKeyboardForOverlay(ulOverlayHandle=%llu, eInputMode=?, eLineInputMode=?, pchDescription=%s, "
		    "unCharMax=%llu, pchExistingText=%s, bUseMinimalMode=%d, uUserValue=%llu) -> VROverlayError_None",
		    EVROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    pchDescription, static_cast<unsigned long long>(unCharMax), pchExistingText,
		    static_cast<int>(bUseMinimalMode), static_cast<unsigned long long>(uUserValue));
	}

	Forward_IVROverlay_GetKeyboardText();
	Forward_IVROverlay_HideKeyboard();
	Forward_IVROverlay_SetKeyboardTransformAbsolute();
	Forward_IVROverlay_SetKeyboardPositionForOverlay();
	Forward_IVROverlay_ShowMessageOverlay();
	Forward_IVROverlay_CloseMessageOverlay();
};

}; // namespace xrt::state_trackers::openvr
