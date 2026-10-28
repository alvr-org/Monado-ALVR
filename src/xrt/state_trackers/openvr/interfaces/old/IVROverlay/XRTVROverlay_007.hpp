// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVROverlay interface version 007.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVROverlay.hpp"
#include "XRTVROverlay_008.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVROverlay_007 : public XRTVROverlay_008, public vr::IVROverlay_007
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVROverlay_007(XRTVRClientCore_003 *core) : XRTVROverlay_008(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVROverlay_007.
		return nullptr;
	}

	ForwardDeclareBase_IVROverlay(XRTVROverlay_008);

	Forward_IVROverlay_FindOverlay_028();
	Forward_IVROverlay_CreateOverlay_014();
	Forward_IVROverlay_DestroyOverlay_028();
	Forward_IVROverlay_SetHighQualityOverlay_019();
	Forward_IVROverlay_GetHighQualityOverlay();
	Forward_IVROverlay_GetOverlayKey_028();
	Forward_IVROverlay_GetOverlayName_028();
	Forward_IVROverlay_GetOverlayImageData_028();
	Forward_IVROverlay_GetOverlayErrorNameFromEnum_028();
	Forward_IVROverlay_SetOverlayFlag_028();
	Forward_IVROverlay_GetOverlayFlag_028();
	Forward_IVROverlay_SetOverlayColor_028();
	Forward_IVROverlay_GetOverlayColor_028();
	Forward_IVROverlay_SetOverlayAlpha_028();
	Forward_IVROverlay_GetOverlayAlpha_028();
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
	Forward_IVROverlay_ShowOverlay_028();
	Forward_IVROverlay_HideOverlay_028();
	Forward_IVROverlay_IsOverlayVisible();
	Forward_IVROverlay_PollNextOverlayEvent_008();
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
	Forward_IVROverlay_CreateDashboardOverlay_028();
	Forward_IVROverlay_IsDashboardVisible();
	Forward_IVROverlay_IsActiveDashboardOverlay();
	Forward_IVROverlay_SetDashboardOverlaySceneProcess_028();
	Forward_IVROverlay_GetDashboardOverlaySceneProcess_028();
	Forward_IVROverlay_ShowDashboard();
	Forward_IVROverlay_ShowKeyboard_022();
	Forward_IVROverlay_ShowKeyboardForOverlay_022();
	Forward_IVROverlay_GetKeyboardText();
	Forward_IVROverlay_HideKeyboard();
};

}; // namespace xrt::state_trackers::openvr
