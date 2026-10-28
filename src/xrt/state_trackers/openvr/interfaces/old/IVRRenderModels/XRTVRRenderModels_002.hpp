// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRRenderModels interface version 002.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRRenderModels.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRRenderModels_002 : public XRTVRRenderModels_006, public vr::IVRRenderModels_002
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRRenderModels_002(XRTVRClientCore_003 *core) : XRTVRRenderModels_006(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRRenderModels_002.
		return nullptr;
	}

	ForwardDeclareBase_IVRRenderModels(XRTVRRenderModels_006);

	bool
	LoadRenderModel(const char *pchRenderModelName, RenderModel_t **ppRenderModel) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "LoadRenderModel(pchRenderModelName=%s, ppRenderModel=%p) -> false", false,
		                             pchRenderModelName, static_cast<const void *>(ppRenderModel));
	}

	Forward_IVRRenderModels_FreeRenderModel();

	bool
	LoadTexture(TextureID_t textureId, RenderModel_TextureMap_t **ppTexture) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LoadTexture(textureId=%lld, ppTexture=%p) -> false", false,
		                             static_cast<long long>(textureId), static_cast<const void *>(ppTexture));
	}

	Forward_IVRRenderModels_FreeTexture();
	Forward_IVRRenderModels_GetRenderModelName();
	Forward_IVRRenderModels_GetRenderModelCount();
	Forward_IVRRenderModels_GetComponentCount();
	Forward_IVRRenderModels_GetComponentName();
	Forward_IVRRenderModels_GetComponentButtonMask();
	Forward_IVRRenderModels_GetComponentRenderModelName();

	bool
	GetComponentState(const char *pchRenderModelName,
	                  const char *pchComponentName,
	                  const vr::VRControllerState_t *pControllerState,
	                  RenderModel_ComponentState_t *pComponentState) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetComponentState(pchRenderModelName=%s, pchComponentName=%s, "
		                             "pControllerState=%p, pComponentState=%p) -> false",
		                             false, pchRenderModelName, pchComponentName,
		                             static_cast<const void *>(pControllerState),
		                             static_cast<const void *>(pComponentState));
	}

	Forward_IVRRenderModels_RenderModelHasComponent();
};

}; // namespace xrt::state_trackers::openvr
