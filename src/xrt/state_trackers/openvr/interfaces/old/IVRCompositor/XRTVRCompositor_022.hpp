// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRCompositor interface version 022.
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

class XRTVRCompositor_022 : public XRTVRCompositor_029, public vr::IVRCompositor_022
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRCompositor_022(XRTVRClientCore_003 *core) : XRTVRCompositor_029(core) {};
	// clang-format on

	FnTable_IVRCompositor_022();

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
	Forward_IVRCompositor_GetFrameTimings_029();
	Forward_IVRCompositor_GetFrameTimeRemaining();
	Forward_IVRCompositor_GetCumulativeStats();
	Forward_IVRCompositor_FadeToColor();
	Forward_IVRCompositor_GetCurrentFadeColor();
	Forward_IVRCompositor_FadeGrid_026();
	Forward_IVRCompositor_GetCurrentGridAlpha();
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
	Forward_IVRCompositor_ForceInterleavedReprojectionOn();
	Forward_IVRCompositor_ForceReconnectProcess();
	Forward_IVRCompositor_SuspendRendering();
	Forward_IVRCompositor_GetMirrorTextureD3D11();
	Forward_IVRCompositor_ReleaseMirrorTextureD3D11();
	Forward_IVRCompositor_GetMirrorTextureGL();
	Forward_IVRCompositor_ReleaseSharedGLTexture();
	Forward_IVRCompositor_LockGLSharedTextureForAccess();
	Forward_IVRCompositor_UnlockGLSharedTextureForAccess();
	Forward_IVRCompositor_GetVulkanInstanceExtensionsRequired();
	Forward_IVRCompositor_GetVulkanDeviceExtensionsRequired();
	Forward_IVRCompositor_SetExplicitTimingMode_029();
	Forward_IVRCompositor_SubmitExplicitTimingData();
	Forward_IVRCompositor_IsMotionSmoothingEnabled();
	Forward_IVRCompositor_IsMotionSmoothingSupported();
	Forward_IVRCompositor_IsCurrentSceneFocusAppLoading();
};

}; // namespace xrt::state_trackers::openvr
