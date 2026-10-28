// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRCompositor interface version 006.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRCompositor.hpp"
#include "XRTVRCompositor_007.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRCompositor_006 : public XRTVRCompositor_007, public vr::IVRCompositor_006
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRCompositor_006(XRTVRClientCore_003 *core) : XRTVRCompositor_007(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRCompositor_006.
		return nullptr;
	}

	ForwardDeclareBase_IVRCompositor(XRTVRCompositor_007);

	Forward_IVRCompositor_GetLastError();
	Forward_IVRCompositor_SetVSync();
	Forward_IVRCompositor_GetVSync();
	Forward_IVRCompositor_SetGamma();
	Forward_IVRCompositor_GetGamma();

	void
	SetGraphicsDevice(Compositor_DeviceType eType, void *pDevice) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetGraphicsDevice(eType=%d, pDevice=%p)", static_cast<int>(eType),
		                         static_cast<const void *>(pDevice));
	}

	Forward_IVRCompositor_WaitGetPoses_008();

	VRCompositorError
	Submit(Hmd_Eye eEye, void *pTexture, VRTextureBounds_t *pBounds) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		// @todo Track what graphics API they choose in @ref SetGraphicsDevice and use that to submit for the
		//       right texture type. For now we don't really care about IVRCompositor_006 until some apps hits
		//       it.

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "Submit(eEye=%d, pTexture=%p, pBounds=%p) -> VRCompositorError_RequestFailed",
		    VRCompositorError::VRCompositorError_RequestFailed, static_cast<int>(eEye),
		    static_cast<const void *>(pTexture), static_cast<const void *>(pBounds));
	}

	Forward_IVRCompositor_ClearLastSubmittedFrame();
	Forward_IVRCompositor_GetFrameTiming();
	Forward_IVRCompositor_FadeToColor();
	Forward_IVRCompositor_FadeGrid_026();
	Forward_IVRCompositor_CompositorBringToFront();
	Forward_IVRCompositor_CompositorGoToBack();
	Forward_IVRCompositor_CompositorQuit();
	Forward_IVRCompositor_IsFullscreen();
	Forward_IVRCompositor_SetTrackingSpace_008();
	Forward_IVRCompositor_GetTrackingSpace_008();
	Forward_IVRCompositor_GetCurrentSceneFocusProcess();
	Forward_IVRCompositor_CanRenderScene();
};

}; // namespace xrt::state_trackers::openvr
