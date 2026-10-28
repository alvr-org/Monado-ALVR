// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRCompositor interface version 012.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRCompositor.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRCompositor_012 : public XRTVRCompositor_029, public vr::IVRCompositor_012
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRCompositor_012(XRTVRClientCore_003 *core) : XRTVRCompositor_029(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRCompositor_012.
		return nullptr;
	}

	ForwardDeclareBase_IVRCompositor(XRTVRCompositor_029);

	Forward_IVRCompositor_SetTrackingSpace_029();
	Forward_IVRCompositor_GetTrackingSpace_029();
	Forward_IVRCompositor_WaitGetPoses_029();
	Forward_IVRCompositor_GetLastPoses_029();
	Forward_IVRCompositor_GetLastPoseForTrackedDeviceIndex();
	Forward_IVRCompositor_Submit_029();
	Forward_IVRCompositor_ClearLastSubmittedFrame();
	Forward_IVRCompositor_PostPresentHandoff();
	Forward_IVRCompositor_GetFrameTiming();
	Forward_IVRCompositor_GetFrameTimeRemaining();
	Forward_IVRCompositor_FadeToColor();
	Forward_IVRCompositor_FadeGrid_026();
	Forward_IVRCompositor_SetSkyboxOverride_029();
	Forward_IVRCompositor_ClearSkyboxOverride();
	Forward_IVRCompositor_CompositorBringToFront();
	Forward_IVRCompositor_CompositorGoToBack();
	Forward_IVRCompositor_CompositorQuit();
	Forward_IVRCompositor_IsFullscreen();
	Forward_IVRCompositor_GetCurrentSceneFocusProcess();
	Forward_IVRCompositor_GetLastFrameRenderer();
	Forward_IVRCompositor_CanRenderScene();
	Forward_IVRCompositor_ShowMirrorWindow();
	Forward_IVRCompositor_HideMirrorWindow();
	Forward_IVRCompositor_IsMirrorWindowVisible();
	Forward_IVRCompositor_CompositorDumpImages();
	Forward_IVRCompositor_ShouldAppRenderWithLowResources();
};

}; // namespace xrt::state_trackers::openvr
