// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVROverlay interface version 027.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVROverlay.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVROverlay_027 : public XRTVROverlay_028, public vr::IVROverlay_027
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVROverlay_027(XRTVRClientCore_003 *core) : XRTVROverlay_028(core) {};
	// clang-format on

	FnTable_IVROverlay_027();

	ForwardDeclareBase_IVROverlay(XRTVROverlay_028);

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
	Forward_IVROverlay_SetOverlayPreCurvePitch();
	Forward_IVROverlay_GetOverlayPreCurvePitch();
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
	Forward_IVROverlay_GetOverlayTransformTrackedDeviceComponent_028();
	Forward_IVROverlay_SetOverlayTransformCursor();
	Forward_IVROverlay_GetOverlayTransformCursor();
	Forward_IVROverlay_SetOverlayTransformProjection();
	Forward_IVROverlay_ShowOverlay_028();
	Forward_IVROverlay_HideOverlay_028();
	Forward_IVROverlay_IsOverlayVisible();
	Forward_IVROverlay_GetTransformForOverlayCoordinates();
	Forward_IVROverlay_WaitFrameSync();
	Forward_IVROverlay_PollNextOverlayEvent_028();
	Forward_IVROverlay_GetOverlayInputMethod_028();
	Forward_IVROverlay_SetOverlayInputMethod_028();
	Forward_IVROverlay_GetOverlayMouseScale_028();
	Forward_IVROverlay_SetOverlayMouseScale_028();
	Forward_IVROverlay_ComputeOverlayIntersection();
	Forward_IVROverlay_IsHoverTargetOverlay();
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
	Forward_IVROverlay_ShowKeyboard_028();
	Forward_IVROverlay_ShowKeyboardForOverlay_028();
	Forward_IVROverlay_GetKeyboardText();
	Forward_IVROverlay_HideKeyboard();
	Forward_IVROverlay_SetKeyboardTransformAbsolute();
	Forward_IVROverlay_SetKeyboardPositionForOverlay();
	Forward_IVROverlay_ShowMessageOverlay();
	Forward_IVROverlay_CloseMessageOverlay();
};

}; // namespace xrt::state_trackers::openvr
