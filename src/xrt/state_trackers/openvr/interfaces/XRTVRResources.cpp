// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRResources_001 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRResources.hpp"


namespace xrt::state_trackers::openvr {

XRTVRResources_001::XRTVRResources_001(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

uint32_t
XRTVRResources_001::LoadSharedResource(const char *pchResourceName, char *pchBuffer, uint32_t unBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "LoadSharedResource(pchResourceName=%s, pchBuffer=%p, unBufferLen=%llu) -> %llu",
	                             static_cast<uint32_t>(0), pchResourceName != nullptr ? pchResourceName : "(null)",
	                             static_cast<void *>(pchBuffer), static_cast<unsigned long long>(unBufferLen),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVRResources_001::GetResourceFullPath(const char *pchResourceName,
                                        const char *pchResourceTypeDirectory,
                                        char *pchPathBuffer,
                                        uint32_t unBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetResourceFullPath(pchResourceName=%s, pchResourceTypeDirectory=%s, "
	                             "pchPathBuffer=%p, unBufferLen=%llu) -> %llu",
	                             static_cast<uint32_t>(0), pchResourceName != nullptr ? pchResourceName : "(null)",
	                             pchResourceTypeDirectory != nullptr ? pchResourceTypeDirectory : "(null)",
	                             static_cast<void *>(pchPathBuffer), static_cast<unsigned long long>(unBufferLen),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

}; // namespace xrt::state_trackers::openvr
