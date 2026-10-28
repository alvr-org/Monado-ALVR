// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRIPCResourceManagerClient interface version 001.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRIPCResourceManagerClient.hpp"
#include "XRTVRIPCResourceManagerClient_002.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRIPCResourceManagerClient_001 : public XRTVRIPCResourceManagerClient_002,
                                          public vr::IVRIPCResourceManagerClient_001
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRIPCResourceManagerClient_001(XRTVRClientCore_003 *core) : XRTVRIPCResourceManagerClient_002(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRIPCResourceManagerClient_001.
		return nullptr;
	}

	ForwardDeclareBase_IVRIPCResourceManagerClient(XRTVRIPCResourceManagerClient_002);

	Forward_IVRIPCResourceManagerClient_NewSharedVulkanImage_002();
	Forward_IVRIPCResourceManagerClient_NewSharedVulkanBuffer();

	bool
	NewSharedVulkanSemaphore(vr::SharedTextureHandle_t *pSharedHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "NewSharedVulkanSemaphore(pSharedHandle=%p) -> false", false,
		                             static_cast<const void *>(pSharedHandle));
	}

	Forward_IVRIPCResourceManagerClient_RefResource();
	Forward_IVRIPCResourceManagerClient_UnrefResource();
	Forward_IVRIPCResourceManagerClient_GetDmabufFormats();
	Forward_IVRIPCResourceManagerClient_GetDmabufModifiers();
	Forward_IVRIPCResourceManagerClient_ImportDmabuf();
	Forward_IVRIPCResourceManagerClient_ReceiveSharedFd();
};

}; // namespace xrt::state_trackers::openvr
