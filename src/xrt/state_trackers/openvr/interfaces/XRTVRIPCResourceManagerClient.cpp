// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRIPCResourceManagerClient_003 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRIPCResourceManagerClient.hpp"


namespace xrt::state_trackers::openvr {

XRTVRIPCResourceManagerClient_003::XRTVRIPCResourceManagerClient_003(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

bool
XRTVRIPCResourceManagerClient_003::NewSharedVulkanImage(uint32_t nImageFormat,
                                                        uint32_t nWidth,
                                                        uint32_t nHeight,
                                                        bool bRenderable,
                                                        bool bMappable,
                                                        bool bComputeAccess,
                                                        uint32_t unMipLevels,
                                                        uint32_t unArrayLayerCount,
                                                        uint32_t unAdditionalVkCreateFlags,
                                                        uint32_t unAdditionalVkUsageFlags,
                                                        vr::SharedTextureHandle_t *pSharedHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "NewSharedVulkanImage(nImageFormat=%llu, nWidth=%llu, nHeight=%llu, bRenderable=%d, bMappable=%d, "
	    "bComputeAccess=%d, unMipLevels=%llu, unArrayLayerCount=%llu, unAdditionalVkCreateFlags=%llu, "
	    "unAdditionalVkUsageFlags=%llu, pSharedHandle=%p) -> %d",
	    false, static_cast<unsigned long long>(nImageFormat), static_cast<unsigned long long>(nWidth),
	    static_cast<unsigned long long>(nHeight), static_cast<int>(bRenderable), static_cast<int>(bMappable),
	    static_cast<int>(bComputeAccess), static_cast<unsigned long long>(unMipLevels),
	    static_cast<unsigned long long>(unArrayLayerCount),
	    static_cast<unsigned long long>(unAdditionalVkCreateFlags),
	    static_cast<unsigned long long>(unAdditionalVkUsageFlags), static_cast<void *>(pSharedHandle), false);
}

bool
XRTVRIPCResourceManagerClient_003::NewSharedVulkanBuffer(uint32_t nSize,
                                                         uint32_t nUsageFlags,
                                                         vr::SharedTextureHandle_t *pSharedHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "NewSharedVulkanBuffer(nSize=%llu, nUsageFlags=%llu, pSharedHandle=%p) -> %d", false,
	    static_cast<unsigned long long>(nSize), static_cast<unsigned long long>(nUsageFlags),
	    static_cast<void *>(pSharedHandle), false);
}

bool
XRTVRIPCResourceManagerClient_003::NewSharedVulkanSemaphore(bool bCounting, vr::SharedTextureHandle_t *pSharedHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "NewSharedVulkanSemaphore(bCounting=%d, pSharedHandle=%p) -> %d", false,
	                             static_cast<int>(bCounting), static_cast<void *>(pSharedHandle), false);
}

bool
XRTVRIPCResourceManagerClient_003::RefResource(vr::SharedTextureHandle_t hSharedHandle, uint64_t *pNewIpcHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "RefResource(hSharedHandle=%llu, pNewIpcHandle=%p) -> %d", false,
	                             static_cast<unsigned long long>(hSharedHandle), static_cast<void *>(pNewIpcHandle),
	                             false);
}

bool
XRTVRIPCResourceManagerClient_003::UnrefResource(vr::SharedTextureHandle_t hSharedHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "UnrefResource(hSharedHandle=%llu) -> %d", false,
	                             static_cast<unsigned long long>(hSharedHandle), false);
}

bool
XRTVRIPCResourceManagerClient_003::GetDmabufFormats(uint32_t *pOutFormatCount, uint32_t *pOutFormats)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetDmabufFormats(pOutFormatCount=%p, pOutFormats=%p) -> %d", false,
	                             static_cast<void *>(pOutFormatCount), static_cast<void *>(pOutFormats), false);
}

bool
XRTVRIPCResourceManagerClient_003::GetDmabufModifiers(vr::EVRApplicationType eApplicationType,
                                                      uint32_t unDRMFormat,
                                                      uint32_t *pOutModifierCount,
                                                      uint64_t *pOutModifiers)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetDmabufModifiers(eApplicationType=%llu, unDRMFormat=%llu, pOutModifierCount=%p, pOutModifiers=%p) -> %d",
	    false, static_cast<unsigned long long>(eApplicationType), static_cast<unsigned long long>(unDRMFormat),
	    static_cast<void *>(pOutModifierCount), static_cast<void *>(pOutModifiers), false);
}

bool
XRTVRIPCResourceManagerClient_003::ImportDmabuf(vr::EVRApplicationType eApplicationType,
                                                vr::DmabufAttributes_t *pDmabufAttributes,
                                                vr::SharedTextureHandle_t *pSharedHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "ImportDmabuf(eApplicationType=%llu, pDmabufAttributes=%p, pSharedHandle=%p) -> %d", false,
	    static_cast<unsigned long long>(eApplicationType), static_cast<void *>(pDmabufAttributes),
	    static_cast<void *>(pSharedHandle), false);
}

bool
XRTVRIPCResourceManagerClient_003::ReceiveSharedFd(uint64_t ulIpcHandle, int *pOutFd)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ReceiveSharedFd(ulIpcHandle=%llu, pOutFd=%p) -> %d", false,
	                             static_cast<unsigned long long>(ulIpcHandle), static_cast<void *>(pOutFd), false);
}

}; // namespace xrt::state_trackers::openvr
