// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRDebug_001 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRDebug.hpp"


namespace xrt::state_trackers::openvr {

XRTVRDebug_001::XRTVRDebug_001(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

EVRDebugError
XRTVRDebug_001::EmitVrProfilerEvent(const char *pchMessage)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "EmitVrProfilerEvent(pchMessage=%s) -> %llu",
	                             static_cast<EVRDebugError>(0), pchMessage != nullptr ? pchMessage : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRDebugError>(0)));
}

EVRDebugError
XRTVRDebug_001::BeginVrProfilerEvent(VrProfilerEventHandle_t *pHandleOut)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "BeginVrProfilerEvent(pHandleOut=%p) -> %llu",
	                             static_cast<EVRDebugError>(0), static_cast<void *>(pHandleOut),
	                             static_cast<unsigned long long>(static_cast<EVRDebugError>(0)));
}

EVRDebugError
XRTVRDebug_001::FinishVrProfilerEvent(VrProfilerEventHandle_t hHandle, const char *pchMessage)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "FinishVrProfilerEvent(hHandle=%llu, pchMessage=%s) -> %llu",
	                             static_cast<EVRDebugError>(0), static_cast<unsigned long long>(hHandle),
	                             pchMessage != nullptr ? pchMessage : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRDebugError>(0)));
}

uint32_t
XRTVRDebug_001::DriverDebugRequest(vr::TrackedDeviceIndex_t unDeviceIndex,
                                   const char *pchRequest,
                                   char *pchResponseBuffer,
                                   uint32_t unResponseBufferSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "DriverDebugRequest(unDeviceIndex=%llu, pchRequest=%s, pchResponseBuffer=%p, "
	                             "unResponseBufferSize=%llu) -> %llu",
	                             static_cast<uint32_t>(0), static_cast<unsigned long long>(unDeviceIndex),
	                             pchRequest != nullptr ? pchRequest : "(null)",
	                             static_cast<void *>(pchResponseBuffer),
	                             static_cast<unsigned long long>(unResponseBufferSize),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

}; // namespace xrt::state_trackers::openvr
