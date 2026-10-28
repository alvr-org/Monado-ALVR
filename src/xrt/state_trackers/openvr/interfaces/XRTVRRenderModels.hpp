// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRRenderModels interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "XRTVRClientCore.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRRenderModels_006 : public vr::IVRRenderModels_006
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRRenderModels_006(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRRenderModels_006() = default;

	FnTable_IVRRenderModels_006();

	vr::EVRRenderModelError
	LoadRenderModel_Async(const char *pchRenderModelName, vr::RenderModel_t **ppRenderModel) override;

	void
	FreeRenderModel(vr::RenderModel_t *pRenderModel) override;

	vr::EVRRenderModelError
	LoadTexture_Async(vr::TextureID_t textureId, vr::RenderModel_TextureMap_t **ppTexture) override;

	void
	FreeTexture(vr::RenderModel_TextureMap_t *pTexture) override;

	vr::EVRRenderModelError
	LoadTextureD3D11_Async(vr::TextureID_t textureId, void *pD3D11Device, void **ppD3D11Texture2D) override;

	vr::EVRRenderModelError
	LoadIntoTextureD3D11_Async(vr::TextureID_t textureId, void *pDstTexture) override;

	void
	FreeTextureD3D11(void *pD3D11Texture2D) override;

	uint32_t
	GetRenderModelName(uint32_t unRenderModelIndex,
	                   VR_OUT_STRING() char *pchRenderModelName,
	                   uint32_t unRenderModelNameLen) override;

	uint32_t
	GetRenderModelCount() override;


	uint32_t
	GetComponentCount(const char *pchRenderModelName) override;

	uint32_t
	GetComponentName(const char *pchRenderModelName,
	                 uint32_t unComponentIndex,
	                 VR_OUT_STRING() char *pchComponentName,
	                 uint32_t unComponentNameLen) override;

	uint64_t
	GetComponentButtonMask(const char *pchRenderModelName, const char *pchComponentName) override;

	uint32_t
	GetComponentRenderModelName(const char *pchRenderModelName,
	                            const char *pchComponentName,
	                            VR_OUT_STRING() char *pchComponentRenderModelName,
	                            uint32_t unComponentRenderModelNameLen) override;

	bool
	GetComponentStateForDevicePath(const char *pchRenderModelName,
	                               const char *pchComponentName,
	                               vr::VRInputValueHandle_t devicePath,
	                               const vr::RenderModel_ControllerMode_State_t *pState,
	                               vr::RenderModel_ComponentState_t *pComponentState) override;

	bool
	GetComponentState(const char *pchRenderModelName,
	                  const char *pchComponentName,
	                  const vr::VRControllerState_t *pControllerState,
	                  const vr::RenderModel_ControllerMode_State_t *pState,
	                  vr::RenderModel_ComponentState_t *pComponentState) override;

	bool
	RenderModelHasComponent(const char *pchRenderModelName, const char *pchComponentName) override;

	uint32_t
	GetRenderModelThumbnailURL(const char *pchRenderModelName,
	                           VR_OUT_STRING() char *pchThumbnailURL,
	                           uint32_t unThumbnailURLLen,
	                           vr::EVRRenderModelError *peError) override;

	uint32_t
	GetRenderModelOriginalPath(const char *pchRenderModelName,
	                           VR_OUT_STRING() char *pchOriginalPath,
	                           uint32_t unOriginalPathLen,
	                           vr::EVRRenderModelError *peError) override;

	const char *
	GetRenderModelErrorNameFromEnum(vr::EVRRenderModelError error) override;
};

} // namespace xrt::state_trackers::openvr
