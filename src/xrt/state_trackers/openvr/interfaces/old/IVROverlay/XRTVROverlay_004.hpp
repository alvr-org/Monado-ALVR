// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVROverlay interface version 004.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVROverlay.hpp"
#include "XRTVROverlay_005.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVROverlay_004 : public XRTVROverlay_005, public vr::IVROverlay_004
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVROverlay_004(XRTVRClientCore_003 *core) : XRTVROverlay_005(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVROverlay_004.
		return nullptr;
	}

	ForwardDeclareBase_IVROverlay(XRTVROverlay_005);

	Forward_IVROverlay_FindOverlay_006();
	Forward_IVROverlay_CreateOverlay_006();
	Forward_IVROverlay_DestroyOverlay_006();
	Forward_IVROverlay_SetHighQualityOverlay_006();
	Forward_IVROverlay_GetHighQualityOverlay();
	Forward_IVROverlay_GetOverlayKey_006();
	Forward_IVROverlay_GetOverlayName_006();
	Forward_IVROverlay_GetOverlayImageData_006();
	Forward_IVROverlay_GetOverlayErrorNameFromEnum_006();
	Forward_IVROverlay_SetOverlayFlag_006();
	Forward_IVROverlay_GetOverlayFlag_006();
	Forward_IVROverlay_SetOverlayColor_006();
	Forward_IVROverlay_GetOverlayColor_006();
	Forward_IVROverlay_SetOverlayAlpha_006();
	Forward_IVROverlay_GetOverlayAlpha_006();
	Forward_IVROverlay_SetOverlayGamma();
	Forward_IVROverlay_GetOverlayGamma();
	Forward_IVROverlay_SetOverlayWidthInMeters_006();
	Forward_IVROverlay_GetOverlayWidthInMeters_006();
	Forward_IVROverlay_SetOverlayAutoCurveDistanceRangeInMeters_006();
	Forward_IVROverlay_GetOverlayAutoCurveDistanceRangeInMeters_006();
	Forward_IVROverlay_SetOverlayTextureBounds_006();
	Forward_IVROverlay_GetOverlayTextureBounds_006();
	Forward_IVROverlay_GetOverlayTransformType_006();
	Forward_IVROverlay_SetOverlayTransformAbsolute_006();
	Forward_IVROverlay_GetOverlayTransformAbsolute_006();
	Forward_IVROverlay_SetOverlayTransformTrackedDeviceRelative_006();
	Forward_IVROverlay_GetOverlayTransformTrackedDeviceRelative_006();
	Forward_IVROverlay_ShowOverlay_006();
	Forward_IVROverlay_HideOverlay_006();
	Forward_IVROverlay_IsOverlayVisible();
	Forward_IVROverlay_PollNextOverlayEvent_008();
	Forward_IVROverlay_GetOverlayInputMethod_006();
	Forward_IVROverlay_SetOverlayInputMethod_006();
	Forward_IVROverlay_GetOverlayMouseScale_006();
	Forward_IVROverlay_SetOverlayMouseScale_006();
	Forward_IVROverlay_ComputeOverlayIntersection();
	Forward_IVROverlay_HandleControllerOverlayInteractionAsMouse_017();
	Forward_IVROverlay_SetOverlayTexture_006();
	Forward_IVROverlay_ClearOverlayTexture_006();
	Forward_IVROverlay_SetOverlayRaw_006();
	Forward_IVROverlay_SetOverlayFromFile_006();
	Forward_IVROverlay_CreateDashboardOverlay_006();
	Forward_IVROverlay_IsDashboardVisible();
	Forward_IVROverlay_IsActiveDashboardOverlay();
	Forward_IVROverlay_SetDashboardOverlaySceneProcess_006();
	Forward_IVROverlay_GetDashboardOverlaySceneProcess_006();
	Forward_IVROverlay_ShowDashboard();
};

}; // namespace xrt::state_trackers::openvr
