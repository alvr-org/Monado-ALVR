// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRCompositor interface version 007.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRCompositor.hpp"
#include "XRTVRCompositor_008.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRCompositor_007 : public XRTVRCompositor_008, public vr::IVRCompositor_007
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRCompositor_007(XRTVRClientCore_003 *core) : XRTVRCompositor_008(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRCompositor_007.
		return nullptr;
	}

	ForwardDeclareBase_IVRCompositor(XRTVRCompositor_008);

	Forward_IVRCompositor_GetLastError();
	Forward_IVRCompositor_SetVSync();
	Forward_IVRCompositor_GetVSync();
	Forward_IVRCompositor_SetGamma();
	Forward_IVRCompositor_GetGamma();
	Forward_IVRCompositor_WaitGetPoses_008();

	VRCompositorError
	Submit(Hmd_Eye eEye,
	       GraphicsAPIConvention eTextureType,
	       void *pTexture,
	       const VRTextureBounds_t *pBounds) override
	{
		return ForwardBase_IVRCompositor::Submit(eEye, eTextureType, pTexture, pBounds);
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
