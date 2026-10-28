// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRRenderModels interface version 001.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRRenderModels.hpp"
#include "XRTVRRenderModels_002.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRRenderModels_001 : public XRTVRRenderModels_002, public vr::IVRRenderModels_001
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRRenderModels_001(XRTVRClientCore_003 *core) : XRTVRRenderModels_002(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRRenderModels_001.
		return nullptr;
	}

	ForwardDeclareBase_IVRRenderModels(XRTVRRenderModels_002);

	bool
	LoadRenderModel(const char *pchRenderModelName, RenderModel_t *pRenderModel) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LoadRenderModel(pchRenderModelName=%s, pRenderModel=%p) -> false",
		                             false, pchRenderModelName, static_cast<const void *>(pRenderModel));
	}

	Forward_IVRRenderModels_FreeRenderModel();
	Forward_IVRRenderModels_GetRenderModelName();
	Forward_IVRRenderModels_GetRenderModelCount();
	Forward_IVRRenderModels_GetComponentCount();
	Forward_IVRRenderModels_GetComponentName();
	Forward_IVRRenderModels_GetComponentButtonMask();
	Forward_IVRRenderModels_GetComponentRenderModelName();

	bool
	GetComponentState(const char *pchRenderModelName,
	                  const char *pchComponentName,
	                  vr::VRControllerState_t controllerState,
	                  ComponentState_t *pComponentState) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetComponentState(pchRenderModelName=%s, pchComponentName=%s, "
		                             "controllerState=?, pComponentState=%p) -> false",
		                             false, pchRenderModelName, pchComponentName,
		                             static_cast<const void *>(pComponentState));
	}
};

}; // namespace xrt::state_trackers::openvr
