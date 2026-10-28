// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRRenderModels interface version 004.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRRenderModels.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRRenderModels_004 : public XRTVRRenderModels_006, public vr::IVRRenderModels_004
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRRenderModels_004(XRTVRClientCore_003 *core) : XRTVRRenderModels_006(core) {};
	// clang-format on

	FnTable_IVRRenderModels_004();

	ForwardDeclareBase_IVRRenderModels(XRTVRRenderModels_006);

	Forward_IVRRenderModels_LoadRenderModel_Async();
	Forward_IVRRenderModels_FreeRenderModel();
	Forward_IVRRenderModels_LoadTexture_Async();
	Forward_IVRRenderModels_FreeTexture();
	Forward_IVRRenderModels_LoadTextureD3D11_Async();
	Forward_IVRRenderModels_FreeTextureD3D11();
	Forward_IVRRenderModels_GetRenderModelName();
	Forward_IVRRenderModels_GetRenderModelCount();
	Forward_IVRRenderModels_GetComponentCount();
	Forward_IVRRenderModels_GetComponentName();
	Forward_IVRRenderModels_GetComponentButtonMask();
	Forward_IVRRenderModels_GetComponentRenderModelName();
	Forward_IVRRenderModels_GetComponentState_006();
	Forward_IVRRenderModels_RenderModelHasComponent();
};

}; // namespace xrt::state_trackers::openvr
