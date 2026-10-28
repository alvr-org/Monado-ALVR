// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRCompositor interface version 021.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRCompositor.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRCompositor_021 : public XRTVRCompositor_029, public vr::IVRCompositor_021
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRCompositor_021(XRTVRClientCore_003 *core) : XRTVRCompositor_029(core) {};
	// clang-format on

	FnTable_IVRCompositor_021();

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

	uint32_t
	GetFrameTimings(Compositor_FrameTiming *pTiming, uint32_t nFrames) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetFrameTimings(pTiming=%p, nFrames=%llu) -> 0", 0,
		                             static_cast<const void *>(pTiming),
		                             static_cast<unsigned long long>(nFrames));
	}

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

	void
	SetExplicitTimingMode(bool bExplicitTimingMode) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetExplicitTimingMode(bExplicitTimingMode=%d)",
		                         static_cast<int>(bExplicitTimingMode));
	}

	Forward_IVRCompositor_SubmitExplicitTimingData();
};

}; // namespace xrt::state_trackers::openvr
