// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRDriverManager_001 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRDriverManager.hpp"


namespace xrt::state_trackers::openvr {

XRTVRDriverManager_001::XRTVRDriverManager_001(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

uint32_t
XRTVRDriverManager_001::GetDriverCount() const
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetDriverCount() -> %llu", static_cast<uint32_t>(0),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVRDriverManager_001::GetDriverName(vr::DriverId_t nDriver, char *pchValue, uint32_t unBufferSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetDriverName(nDriver=%llu, pchValue=%p, unBufferSize=%llu) -> %llu",
	                             static_cast<uint32_t>(0), static_cast<unsigned long long>(nDriver),
	                             static_cast<void *>(pchValue), static_cast<unsigned long long>(unBufferSize),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

DriverHandle_t
XRTVRDriverManager_001::GetDriverHandle(const char *pchDriverName)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetDriverHandle(pchDriverName=%s) -> %llu",
	                             static_cast<DriverHandle_t>(0),
	                             pchDriverName != nullptr ? pchDriverName : "(null)",
	                             static_cast<unsigned long long>(static_cast<DriverHandle_t>(0)));
}

bool
XRTVRDriverManager_001::IsEnabled(vr::DriverId_t nDriver) const
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsEnabled(nDriver=%llu) -> %d", false,
	                             static_cast<unsigned long long>(nDriver), false);
}

}; // namespace xrt::state_trackers::openvr
