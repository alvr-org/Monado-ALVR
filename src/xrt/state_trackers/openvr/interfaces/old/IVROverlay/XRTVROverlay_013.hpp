// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVROverlay interface version 013.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVROverlay.hpp"
#include "XRTVROverlay_017.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVROverlay_013 : public XRTVROverlay_017, public vr::IVROverlay_013
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVROverlay_013(XRTVRClientCore_003 *core) : XRTVROverlay_017(core) {};
	// clang-format on

	FnTable_IVROverlay_013();

	ForwardDeclareBase_IVROverlay(XRTVROverlay_017);

	Forward_IVROverlay_FindOverlay_028();
	Forward_IVROverlay_CreateOverlay_014();
	Forward_IVROverlay_DestroyOverlay_028();
	Forward_IVROverlay_SetHighQualityOverlay_019();
	Forward_IVROverlay_GetHighQualityOverlay();
	Forward_IVROverlay_GetOverlayKey_028();
	Forward_IVROverlay_GetOverlayName_028();
	Forward_IVROverlay_GetOverlayImageData_028();
	Forward_IVROverlay_GetOverlayErrorNameFromEnum_028();
	Forward_IVROverlay_SetOverlayRenderingPid();
	Forward_IVROverlay_GetOverlayRenderingPid();
	Forward_IVROverlay_SetOverlayFlag_028();
	Forward_IVROverlay_GetOverlayFlag_028();
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
	Forward_IVROverlay_SetOverlayAutoCurveDistanceRangeInMeters_020();
	Forward_IVROverlay_GetOverlayAutoCurveDistanceRangeInMeters_020();
	Forward_IVROverlay_SetOverlayTextureColorSpace();
	Forward_IVROverlay_GetOverlayTextureColorSpace();
	Forward_IVROverlay_SetOverlayTextureBounds_028();
	Forward_IVROverlay_GetOverlayTextureBounds_028();
	Forward_IVROverlay_GetOverlayTransformType_028();
	Forward_IVROverlay_SetOverlayTransformAbsolute_028();
	Forward_IVROverlay_GetOverlayTransformAbsolute_028();
	Forward_IVROverlay_SetOverlayTransformTrackedDeviceRelative_028();
	Forward_IVROverlay_GetOverlayTransformTrackedDeviceRelative_028();
	Forward_IVROverlay_SetOverlayTransformTrackedDeviceComponent();
	Forward_IVROverlay_GetOverlayTransformTrackedDeviceComponent_017();
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
	Forward_IVROverlay_HandleControllerOverlayInteractionAsMouse_017();
	Forward_IVROverlay_IsHoverTargetOverlay();
	Forward_IVROverlay_GetGamepadFocusOverlay();
	Forward_IVROverlay_SetGamepadFocusOverlay_020();
	Forward_IVROverlay_SetOverlayNeighbor_020();
	Forward_IVROverlay_MoveGamepadFocusToNeighbor_020();
	Forward_IVROverlay_SetOverlayTexture_028();
	Forward_IVROverlay_ClearOverlayTexture_028();
	Forward_IVROverlay_SetOverlayRaw_021();
	Forward_IVROverlay_SetOverlayFromFile_028();

	EVROverlayError
	GetOverlayTexture(VROverlayHandle_t ulOverlayHandle,
	                  void **pNativeTextureHandle,
	                  void *pNativeTextureRef,
	                  uint32_t *pWidth,
	                  uint32_t *pHeight,
	                  uint32_t *pNativeFormat,
	                  EGraphicsAPIConvention *pAPI,
	                  EColorSpace *pColorSpace) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetOverlayTexture(ulOverlayHandle=%llu, pNativeTextureHandle=%p, pNativeTextureRef=%p, pWidth=%p, "
		    "pHeight=%p, pNativeFormat=%p, pAPI=%p, pColorSpace=%p) -> VROverlayError_None",
		    EVROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pNativeTextureHandle), static_cast<const void *>(pNativeTextureRef),
		    static_cast<const void *>(pWidth), static_cast<const void *>(pHeight),
		    static_cast<const void *>(pNativeFormat), static_cast<const void *>(pAPI),
		    static_cast<const void *>(pColorSpace));
	}

	Forward_IVROverlay_ReleaseNativeOverlayHandle();
	Forward_IVROverlay_GetOverlayTextureSize();
	Forward_IVROverlay_CreateDashboardOverlay_028();
	Forward_IVROverlay_IsDashboardVisible();
	Forward_IVROverlay_IsActiveDashboardOverlay();
	Forward_IVROverlay_SetDashboardOverlaySceneProcess_028();
	Forward_IVROverlay_GetDashboardOverlaySceneProcess_028();
	Forward_IVROverlay_ShowDashboard();
	Forward_IVROverlay_GetPrimaryDashboardDevice();
	Forward_IVROverlay_ShowKeyboard_022();
	Forward_IVROverlay_ShowKeyboardForOverlay_022();
	Forward_IVROverlay_GetKeyboardText();
	Forward_IVROverlay_HideKeyboard();
	Forward_IVROverlay_SetKeyboardTransformAbsolute();
	Forward_IVROverlay_SetKeyboardPositionForOverlay();
	Forward_IVROverlay_SetOverlayIntersectionMask();
};

}; // namespace xrt::state_trackers::openvr
