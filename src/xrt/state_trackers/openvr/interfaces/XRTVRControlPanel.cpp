// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRControlPanel_001 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRControlPanel.hpp"


namespace xrt::state_trackers::openvr {

XRTVRControlPanel_001::XRTVRControlPanel_001(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

uint32_t
XRTVRControlPanel_001::GetDriverCount()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetDriverCount() -> %llu", static_cast<uint32_t>(0),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVRControlPanel_001::GetDriverId(uint32_t unDriverIndex, char *pchBuffer, uint32_t unBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetDriverId(unDriverIndex=%llu, pchBuffer=%p, unBufferLen=%llu) -> %llu",
	                             static_cast<uint32_t>(0), static_cast<unsigned long long>(unDriverIndex),
	                             static_cast<void *>(pchBuffer), static_cast<unsigned long long>(unBufferLen),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVRControlPanel_001::GetDriverDisplayCount(const char *pchDriverId)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetDriverDisplayCount(pchDriverId=%s) -> %llu", static_cast<uint32_t>(0),
	                             pchDriverId != nullptr ? pchDriverId : "(null)",
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVRControlPanel_001::GetDriverDisplayId(const char *pchDriverId,
                                          uint32_t unDisplayIndex,
                                          char *pchBuffer,
                                          uint32_t unBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetDriverDisplayId(pchDriverId=%s, unDisplayIndex=%llu, pchBuffer=%p, unBufferLen=%llu) -> %llu",
	    static_cast<uint32_t>(0), pchDriverId != nullptr ? pchDriverId : "(null)",
	    static_cast<unsigned long long>(unDisplayIndex), static_cast<void *>(pchBuffer),
	    static_cast<unsigned long long>(unBufferLen), static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVRControlPanel_001::GetDriverDisplayModelNumber(const char *pchDriverId,
                                                   const char *pchDisplayId,
                                                   char *pchBuffer,
                                                   uint32_t unBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetDriverDisplayModelNumber(pchDriverId=%s, pchDisplayId=%s, pchBuffer=%p, unBufferLen=%llu) -> %llu",
	    static_cast<uint32_t>(0), pchDriverId != nullptr ? pchDriverId : "(null)",
	    pchDisplayId != nullptr ? pchDisplayId : "(null)", static_cast<void *>(pchBuffer),
	    static_cast<unsigned long long>(unBufferLen), static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVRControlPanel_001::GetDriverDisplaySerialNumber(const char *pchDriverId,
                                                    const char *pchDisplayId,
                                                    char *pchBuffer,
                                                    uint32_t unBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetDriverDisplaySerialNumber(pchDriverId=%s, pchDisplayId=%s, pchBuffer=%p, unBufferLen=%llu) -> %llu",
	    static_cast<uint32_t>(0), pchDriverId != nullptr ? pchDriverId : "(null)",
	    pchDisplayId != nullptr ? pchDisplayId : "(null)", static_cast<void *>(pchBuffer),
	    static_cast<unsigned long long>(unBufferLen), static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

class IVRSystem *
XRTVRControlPanel_001::GetCurrentDisplayInterface(const char *pchHmdInterfaceVersion)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCurrentDisplayInterface(pchHmdInterfaceVersion=%s) -> %p", nullptr,
	                             pchHmdInterfaceVersion != nullptr ? pchHmdInterfaceVersion : "(null)",
	                             static_cast<const void *>(nullptr));
}

uint32_t
XRTVRControlPanel_001::LoadSharedResource(const char *pchResourceName, char *pchBuffer, uint32_t unBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "LoadSharedResource(pchResourceName=%s, pchBuffer=%p, unBufferLen=%llu) -> %llu",
	                             static_cast<uint32_t>(0), pchResourceName != nullptr ? pchResourceName : "(null)",
	                             static_cast<void *>(pchBuffer), static_cast<unsigned long long>(unBufferLen),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

float
XRTVRControlPanel_001::GetIPD()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetIPD() -> %f", 0.0f, 0.0);
}

void
XRTVRControlPanel_001::SetIPD(float fIPD)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetIPD(fIPD=%f)", static_cast<double>(fIPD));
}

class vr::IVRCompositor *
XRTVRControlPanel_001::GetCurrentCompositorInterface(const char *pchInterfaceVersion)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCurrentCompositorInterface(pchInterfaceVersion=%s) -> %p", nullptr,
	                             pchInterfaceVersion != nullptr ? pchInterfaceVersion : "(null)",
	                             static_cast<const void *>(nullptr));
}

bool
XRTVRControlPanel_001::QuitProcess(uint32_t pidProcessToQuit)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "QuitProcess(pidProcessToQuit=%llu) -> %d", false,
	                             static_cast<unsigned long long>(pidProcessToQuit), false);
}

uint32_t
XRTVRControlPanel_001::StartVRProcess(const char *pchExecutable,
                                      const char **pchArguments,
                                      uint32_t unArgumentCount,
                                      const char *pchWorkingDirectory)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "StartVRProcess(pchExecutable=%s, pchArguments=%p, unArgumentCount=%llu, pchWorkingDirectory=%s) -> %llu",
	    static_cast<uint32_t>(0), pchExecutable != nullptr ? pchExecutable : "(null)",
	    static_cast<const void *>(pchArguments), static_cast<unsigned long long>(unArgumentCount),
	    pchWorkingDirectory != nullptr ? pchWorkingDirectory : "(null)",
	    static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

void
XRTVRControlPanel_001::SetMasterProcessToThis()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetMasterProcessToThis()");
}

void
XRTVRControlPanel_001::StartAutolaunchOverlays()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "StartAutolaunchOverlays()");
}

bool
XRTVRControlPanel_001::ForceQuitProcess(uint32_t pidProcessToQuit)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ForceQuitProcess(pidProcessToQuit=%llu) -> %d", false,
	                             static_cast<unsigned long long>(pidProcessToQuit), false);
}

void
XRTVRControlPanel_001::AbortTransition()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "AbortTransition()");
}

}; // namespace xrt::state_trackers::openvr
