// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRIPCResourceManagerClient_003 interface version.
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

class XRTVRIPCResourceManagerClient_003 : public vr::IVRIPCResourceManagerClient_003
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRIPCResourceManagerClient_003(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRIPCResourceManagerClient_003() = default;

	// note: IVRIPCResourceManagerClient_003's generated FnTable calls a DestructIVRIPCResourceManagerClient()
	// member that isn't part of the abstract interface itself, so it can't be implemented through a base-class
	// pointer here; this class is therefore not registered via XRTVRClientCore's SUPPORTED_INTERFACES factory
	// macro.

	bool
	NewSharedVulkanImage(uint32_t nImageFormat,
	                     uint32_t nWidth,
	                     uint32_t nHeight,
	                     bool bRenderable,
	                     bool bMappable,
	                     bool bComputeAccess,
	                     uint32_t unMipLevels,
	                     uint32_t unArrayLayerCount,
	                     uint32_t unAdditionalVkCreateFlags,
	                     uint32_t unAdditionalVkUsageFlags,
	                     vr::SharedTextureHandle_t *pSharedHandle) override;

	bool
	NewSharedVulkanBuffer(uint32_t nSize, uint32_t nUsageFlags, vr::SharedTextureHandle_t *pSharedHandle) override;

	bool
	NewSharedVulkanSemaphore(bool bCounting, vr::SharedTextureHandle_t *pSharedHandle) override;

	bool
	RefResource(vr::SharedTextureHandle_t hSharedHandle, uint64_t *pNewIpcHandle) override;

	bool
	UnrefResource(vr::SharedTextureHandle_t hSharedHandle) override;

	bool
	GetDmabufFormats(uint32_t *pOutFormatCount, uint32_t *pOutFormats) override;

	bool
	GetDmabufModifiers(vr::EVRApplicationType eApplicationType,
	                   uint32_t unDRMFormat,
	                   uint32_t *pOutModifierCount,
	                   uint64_t *pOutModifiers) override;

	bool
	ImportDmabuf(vr::EVRApplicationType eApplicationType,
	             vr::DmabufAttributes_t *pDmabufAttributes,
	             vr::SharedTextureHandle_t *pSharedHandle) override;

	bool
	ReceiveSharedFd(uint64_t ulIpcHandle, int *pOutFd) override;
};

} // namespace xrt::state_trackers::openvr
