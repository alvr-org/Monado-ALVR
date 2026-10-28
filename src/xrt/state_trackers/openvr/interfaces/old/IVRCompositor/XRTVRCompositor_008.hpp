// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRCompositor interface version 008.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRCompositor.hpp"
#include "XRTVRCompositor_015.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRCompositor_008 : public XRTVRCompositor_015, public vr::IVRCompositor_008
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRCompositor_008(XRTVRClientCore_003 *core) : XRTVRCompositor_015(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRCompositor_008.
		return nullptr;
	}

	ForwardDeclareBase_IVRCompositor(XRTVRCompositor_015);

	uint32_t
	GetLastError(VR_OUT_STRING() char *pchBuffer, uint32_t unBufferSize) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetLastError(pchBuffer=%p, unBufferSize=%llu) -> 0", 0,
		                             static_cast<const void *>(pchBuffer),
		                             static_cast<unsigned long long>(unBufferSize));
	}

	void
	SetVSync(bool bVSync) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetVSync(bVSync=%d)", static_cast<int>(bVSync));
	}

	bool
	GetVSync() override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetVSync() -> false", false);
	}

	void
	SetGamma(float fGamma) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetGamma(fGamma=%f)", fGamma);
	}

	float
	GetGamma() override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetGamma() -> 0.0f", 0.0f);
	}

	Forward_IVRCompositor_WaitGetPoses_029();

	VRCompositorError
	Submit(Hmd_Eye eEye,
	       GraphicsAPIConvention eTextureType,
	       void *pTexture,
	       const VRTextureBounds_t *pBounds,
	       VRSubmitFlags_t nSubmitFlags = Submit_Default) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		// @todo implement this, it appears to be `void*` is a GLuint for an OpenGL texture ID, unsure for DX
		//       for now. Use eTextureType to determine

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "Submit(eEye=%d, eTextureType=%d, pTexture=%p, pBounds=%p, "
		                             "nSubmitFlags=%d) -> VRCompositorError_RequestFailed",
		                             EVRCompositorError::VRCompositorError_RequestFailed,
		                             static_cast<int>(eEye), static_cast<int>(eTextureType),
		                             static_cast<const void *>(pTexture), static_cast<const void *>(pBounds),
		                             static_cast<int>(nSubmitFlags));
	}

	Forward_IVRCompositor_ClearLastSubmittedFrame();
	Forward_IVRCompositor_GetFrameTiming();
	Forward_IVRCompositor_FadeToColor();
	Forward_IVRCompositor_FadeGrid_026();

	void
	SetSkyboxOverride(GraphicsAPIConvention eTextureType,
	                  void *pFront,
	                  void *pBack,
	                  void *pLeft,
	                  void *pRight,
	                  void *pTop,
	                  void *pBottom) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(
		    logger,
		    "SetSkyboxOverride(eTextureType=%d, pFront=%p, pBack=%p, pLeft=%p, pRight=%p, pTop=%p, pBottom=%p)",
		    static_cast<int>(eTextureType), static_cast<const void *>(pFront), static_cast<const void *>(pBack),
		    static_cast<const void *>(pLeft), static_cast<const void *>(pRight),
		    static_cast<const void *>(pTop), static_cast<const void *>(pBottom));
	}

	Forward_IVRCompositor_ClearSkyboxOverride();
	Forward_IVRCompositor_CompositorBringToFront();
	Forward_IVRCompositor_CompositorGoToBack();
	Forward_IVRCompositor_CompositorQuit();
	Forward_IVRCompositor_IsFullscreen();
	Forward_IVRCompositor_SetTrackingSpace_029();
	Forward_IVRCompositor_GetTrackingSpace_029();
	Forward_IVRCompositor_GetCurrentSceneFocusProcess();
	Forward_IVRCompositor_CanRenderScene();
	Forward_IVRCompositor_ShowMirrorWindow();
	Forward_IVRCompositor_HideMirrorWindow();
	Forward_IVRCompositor_CompositorDumpImages();
	Forward_IVRCompositor_GetFrameTimeRemaining();
	Forward_IVRCompositor_GetLastFrameRenderer();

	Forward_IVRCompositor_GetLastPoses_029();

	Forward_IVRCompositor_PostPresentHandoff();
};

}; // namespace xrt::state_trackers::openvr
