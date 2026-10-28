// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRIPCResourceManagerClient interface version 002.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRIPCResourceManagerClient.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRIPCResourceManagerClient_002 : public XRTVRIPCResourceManagerClient_003,
                                          public vr::IVRIPCResourceManagerClient_002
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRIPCResourceManagerClient_002(XRTVRClientCore_003 *core) : XRTVRIPCResourceManagerClient_003(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRIPCResourceManagerClient_002.
		return nullptr;
	}

	ForwardDeclareBase_IVRIPCResourceManagerClient(XRTVRIPCResourceManagerClient_003);

	bool
	NewSharedVulkanImage(uint32_t nImageFormat,
	                     uint32_t nWidth,
	                     uint32_t nHeight,
	                     bool bRenderable,
	                     bool bMappable,
	                     bool bComputeAccess,
	                     uint32_t unMipLevels,
	                     uint32_t unArrayLayerCount,
	                     vr::SharedTextureHandle_t *pSharedHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "NewSharedVulkanImage(nImageFormat=%llu, nWidth=%llu, nHeight=%llu, bRenderable=%d, bMappable=%d, "
		    "bComputeAccess=%d, unMipLevels=%llu, unArrayLayerCount=%llu, pSharedHandle=%p) -> false",
		    false, static_cast<unsigned long long>(nImageFormat), static_cast<unsigned long long>(nWidth),
		    static_cast<unsigned long long>(nHeight), static_cast<int>(bRenderable),
		    static_cast<int>(bMappable), static_cast<int>(bComputeAccess),
		    static_cast<unsigned long long>(unMipLevels), static_cast<unsigned long long>(unArrayLayerCount),
		    static_cast<const void *>(pSharedHandle));
	}

	Forward_IVRIPCResourceManagerClient_NewSharedVulkanBuffer();
	Forward_IVRIPCResourceManagerClient_NewSharedVulkanSemaphore_003();
	Forward_IVRIPCResourceManagerClient_RefResource();
	Forward_IVRIPCResourceManagerClient_UnrefResource();
	Forward_IVRIPCResourceManagerClient_GetDmabufFormats();
	Forward_IVRIPCResourceManagerClient_GetDmabufModifiers();
	Forward_IVRIPCResourceManagerClient_ImportDmabuf();
	Forward_IVRIPCResourceManagerClient_ReceiveSharedFd();
};

}; // namespace xrt::state_trackers::openvr
