// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRApplications_008 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRApplications.hpp"


namespace xrt::state_trackers::openvr {

XRTVRApplications_008::XRTVRApplications_008(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

EVRApplicationError
XRTVRApplications_008::AddApplicationManifest(const char *pchApplicationManifestFullPath, bool bTemporary)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "AddApplicationManifest(pchApplicationManifestFullPath=%s, bTemporary=%d) -> %llu",
	    static_cast<EVRApplicationError>(0),
	    pchApplicationManifestFullPath != nullptr ? pchApplicationManifestFullPath : "(null)",
	    static_cast<int>(bTemporary), static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

EVRApplicationError
XRTVRApplications_008::RemoveApplicationManifest(const char *pchApplicationManifestFullPath)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "RemoveApplicationManifest(pchApplicationManifestFullPath=%s) -> %llu",
	                             static_cast<EVRApplicationError>(0),
	                             pchApplicationManifestFullPath != nullptr ? pchApplicationManifestFullPath
	                                                                       : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

bool
XRTVRApplications_008::IsApplicationInstalled(const char *pchAppKey)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsApplicationInstalled(pchAppKey=%s) -> %d", false,
	                             pchAppKey != nullptr ? pchAppKey : "(null)", false);
}

uint32_t
XRTVRApplications_008::GetApplicationCount()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetApplicationCount() -> %llu", static_cast<uint32_t>(0),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

EVRApplicationError
XRTVRApplications_008::GetApplicationKeyByIndex(uint32_t unApplicationIndex,
                                                char *pchAppKeyBuffer,
                                                uint32_t unAppKeyBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetApplicationKeyByIndex(unApplicationIndex=%llu, pchAppKeyBuffer=%p, unAppKeyBufferLen=%llu) -> %llu",
	    static_cast<EVRApplicationError>(0), static_cast<unsigned long long>(unApplicationIndex),
	    static_cast<void *>(pchAppKeyBuffer), static_cast<unsigned long long>(unAppKeyBufferLen),
	    static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

EVRApplicationError
XRTVRApplications_008::GetApplicationKeyByProcessId(uint32_t unProcessId,
                                                    char *pchAppKeyBuffer,
                                                    uint32_t unAppKeyBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetApplicationKeyByProcessId(unProcessId=%llu, pchAppKeyBuffer=%p, unAppKeyBufferLen=%llu) -> %llu",
	    static_cast<EVRApplicationError>(0), static_cast<unsigned long long>(unProcessId),
	    static_cast<void *>(pchAppKeyBuffer), static_cast<unsigned long long>(unAppKeyBufferLen),
	    static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

EVRApplicationError
XRTVRApplications_008::LaunchApplication(const char *pchAppKey)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LaunchApplication(pchAppKey=%s) -> %llu",
	                             static_cast<EVRApplicationError>(0), pchAppKey != nullptr ? pchAppKey : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

EVRApplicationError
XRTVRApplications_008::LaunchTemplateApplication(const char *pchTemplateAppKey,
                                                 const char *pchNewAppKey,
                                                 const AppOverrideKeys_t *pKeys,
                                                 uint32_t unKeys)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "LaunchTemplateApplication(pchTemplateAppKey=%s, pchNewAppKey=%s, pKeys=%p, unKeys=%llu) -> %llu",
	    static_cast<EVRApplicationError>(0), pchTemplateAppKey != nullptr ? pchTemplateAppKey : "(null)",
	    pchNewAppKey != nullptr ? pchNewAppKey : "(null)", static_cast<const void *>(pKeys),
	    static_cast<unsigned long long>(unKeys),
	    static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

vr::EVRApplicationError
XRTVRApplications_008::LaunchApplicationFromMimeType(const char *pchMimeType, const char *pchArgs)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LaunchApplicationFromMimeType(pchMimeType=%s, pchArgs=%s) -> %llu",
	                             static_cast<vr::EVRApplicationError>(0),
	                             pchMimeType != nullptr ? pchMimeType : "(null)",
	                             pchArgs != nullptr ? pchArgs : "(null)",
	                             static_cast<unsigned long long>(static_cast<vr::EVRApplicationError>(0)));
}

EVRApplicationError
XRTVRApplications_008::LaunchDashboardOverlay(const char *pchAppKey)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LaunchDashboardOverlay(pchAppKey=%s) -> %llu",
	                             static_cast<EVRApplicationError>(0), pchAppKey != nullptr ? pchAppKey : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

bool
XRTVRApplications_008::CancelApplicationLaunch(const char *pchAppKey)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "CancelApplicationLaunch(pchAppKey=%s) -> %d", false,
	                             pchAppKey != nullptr ? pchAppKey : "(null)", false);
}

EVRApplicationError
XRTVRApplications_008::IdentifyApplication(uint32_t unProcessId, const char *pchAppKey)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IdentifyApplication(unProcessId=%llu, pchAppKey=%s) -> %llu",
	                             static_cast<EVRApplicationError>(0), static_cast<unsigned long long>(unProcessId),
	                             pchAppKey != nullptr ? pchAppKey : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

uint32_t
XRTVRApplications_008::GetApplicationProcessId(const char *pchAppKey)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetApplicationProcessId(pchAppKey=%s) -> %llu", static_cast<uint32_t>(0),
	                             pchAppKey != nullptr ? pchAppKey : "(null)",
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

const char *
XRTVRApplications_008::GetApplicationsErrorNameFromEnum(EVRApplicationError error)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetApplicationsErrorNameFromEnum(error=%llu) -> %s",
	                             "TODO: GetApplicationsErrorNameFromEnum", static_cast<unsigned long long>(error),
	                             "TODO: GetApplicationsErrorNameFromEnum");
}

uint32_t
XRTVRApplications_008::GetApplicationPropertyString(const char *pchAppKey,
                                                    EVRApplicationProperty eProperty,
                                                    char *pchPropertyValueBuffer,
                                                    uint32_t unPropertyValueBufferLen,
                                                    EVRApplicationError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetApplicationPropertyString(pchAppKey=%s, eProperty=%llu, pchPropertyValueBuffer=%p, "
	    "unPropertyValueBufferLen=%llu, peError=%p) -> %llu",
	    static_cast<uint32_t>(0), pchAppKey != nullptr ? pchAppKey : "(null)",
	    static_cast<unsigned long long>(eProperty), static_cast<void *>(pchPropertyValueBuffer),
	    static_cast<unsigned long long>(unPropertyValueBufferLen), static_cast<void *>(peError),
	    static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

bool
XRTVRApplications_008::GetApplicationPropertyBool(const char *pchAppKey,
                                                  EVRApplicationProperty eProperty,
                                                  EVRApplicationError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetApplicationPropertyBool(pchAppKey=%s, eProperty=%llu, peError=%p) -> %d",
	                             false, pchAppKey != nullptr ? pchAppKey : "(null)",
	                             static_cast<unsigned long long>(eProperty), static_cast<void *>(peError), false);
}

uint64_t
XRTVRApplications_008::GetApplicationPropertyUint64(const char *pchAppKey,
                                                    EVRApplicationProperty eProperty,
                                                    EVRApplicationError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetApplicationPropertyUint64(pchAppKey=%s, eProperty=%llu, peError=%p) -> %llu",
	                             static_cast<uint64_t>(0), pchAppKey != nullptr ? pchAppKey : "(null)",
	                             static_cast<unsigned long long>(eProperty), static_cast<void *>(peError),
	                             static_cast<unsigned long long>(static_cast<uint64_t>(0)));
}

EVRApplicationError
XRTVRApplications_008::SetApplicationAutoLaunch(const char *pchAppKey, bool bAutoLaunch)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetApplicationAutoLaunch(pchAppKey=%s, bAutoLaunch=%d) -> %llu",
	                             static_cast<EVRApplicationError>(0), pchAppKey != nullptr ? pchAppKey : "(null)",
	                             static_cast<int>(bAutoLaunch),
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

bool
XRTVRApplications_008::GetApplicationAutoLaunch(const char *pchAppKey)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetApplicationAutoLaunch(pchAppKey=%s) -> %d", false,
	                             pchAppKey != nullptr ? pchAppKey : "(null)", false);
}

EVRApplicationError
XRTVRApplications_008::SetDefaultApplicationForMimeType(const char *pchAppKey, const char *pchMimeType)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetDefaultApplicationForMimeType(pchAppKey=%s, pchMimeType=%s) -> %llu",
	                             static_cast<EVRApplicationError>(0), pchAppKey != nullptr ? pchAppKey : "(null)",
	                             pchMimeType != nullptr ? pchMimeType : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

bool
XRTVRApplications_008::GetDefaultApplicationForMimeType(const char *pchMimeType,
                                                        char *pchAppKeyBuffer,
                                                        uint32_t unAppKeyBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetDefaultApplicationForMimeType(pchMimeType=%s, pchAppKeyBuffer=%p, unAppKeyBufferLen=%llu) -> %d", false,
	    pchMimeType != nullptr ? pchMimeType : "(null)", static_cast<void *>(pchAppKeyBuffer),
	    static_cast<unsigned long long>(unAppKeyBufferLen), false);
}

bool
XRTVRApplications_008::GetApplicationSupportedMimeTypes(const char *pchAppKey,
                                                        char *pchMimeTypesBuffer,
                                                        uint32_t unMimeTypesBuffer)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetApplicationSupportedMimeTypes(pchAppKey=%s, pchMimeTypesBuffer=%p, unMimeTypesBuffer=%llu) -> %d",
	    false, pchAppKey != nullptr ? pchAppKey : "(null)", static_cast<void *>(pchMimeTypesBuffer),
	    static_cast<unsigned long long>(unMimeTypesBuffer), false);
}

uint32_t
XRTVRApplications_008::GetApplicationsThatSupportMimeType(const char *pchMimeType,
                                                          char *pchAppKeysThatSupportBuffer,
                                                          uint32_t unAppKeysThatSupportBuffer)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetApplicationsThatSupportMimeType(pchMimeType=%s, "
	                             "pchAppKeysThatSupportBuffer=%p, unAppKeysThatSupportBuffer=%llu) -> %llu",
	                             static_cast<uint32_t>(0), pchMimeType != nullptr ? pchMimeType : "(null)",
	                             static_cast<void *>(pchAppKeysThatSupportBuffer),
	                             static_cast<unsigned long long>(unAppKeysThatSupportBuffer),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVRApplications_008::GetApplicationLaunchArguments(uint32_t unHandle, char *pchArgs, uint32_t unArgs)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetApplicationLaunchArguments(unHandle=%llu, pchArgs=%p, unArgs=%llu) -> %llu",
	    static_cast<uint32_t>(0), static_cast<unsigned long long>(unHandle), static_cast<void *>(pchArgs),
	    static_cast<unsigned long long>(unArgs), static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

EVRApplicationError
XRTVRApplications_008::GetStartingApplication(char *pchAppKeyBuffer, uint32_t unAppKeyBufferLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetStartingApplication(pchAppKeyBuffer=%p, unAppKeyBufferLen=%llu) -> %llu",
	                             static_cast<EVRApplicationError>(0), static_cast<void *>(pchAppKeyBuffer),
	                             static_cast<unsigned long long>(unAppKeyBufferLen),
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

EVRSceneApplicationState
XRTVRApplications_008::GetSceneApplicationState()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetSceneApplicationState() -> %llu",
	                             static_cast<EVRSceneApplicationState>(0),
	                             static_cast<unsigned long long>(static_cast<EVRSceneApplicationState>(0)));
}

EVRApplicationError
XRTVRApplications_008::PerformApplicationPrelaunchCheck(const char *pchAppKey)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "PerformApplicationPrelaunchCheck(pchAppKey=%s) -> %llu",
	                             static_cast<EVRApplicationError>(0), pchAppKey != nullptr ? pchAppKey : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

const char *
XRTVRApplications_008::GetSceneApplicationStateNameFromEnum(EVRSceneApplicationState state)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetSceneApplicationStateNameFromEnum(state=%llu) -> %s",
	                             "TODO: GetSceneApplicationStateNameFromEnum",
	                             static_cast<unsigned long long>(state),
	                             "TODO: GetSceneApplicationStateNameFromEnum");
}

EVRApplicationError
XRTVRApplications_008::LaunchInternalProcess(const char *pchBinaryPath,
                                             const char *pchArguments,
                                             const char *pchWorkingDirectory)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "LaunchInternalProcess(pchBinaryPath=%s, pchArguments=%s, pchWorkingDirectory=%s) -> %llu",
	    static_cast<EVRApplicationError>(0), pchBinaryPath != nullptr ? pchBinaryPath : "(null)",
	    pchArguments != nullptr ? pchArguments : "(null)",
	    pchWorkingDirectory != nullptr ? pchWorkingDirectory : "(null)",
	    static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

EVRApplicationError
XRTVRApplications_008::RegisterSubprocess(uint32_t nPid)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "RegisterSubprocess(nPid=%llu) -> %llu",
	                             static_cast<EVRApplicationError>(0), static_cast<unsigned long long>(nPid),
	                             static_cast<unsigned long long>(static_cast<EVRApplicationError>(0)));
}

uint32_t
XRTVRApplications_008::GetCurrentSceneProcessId()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCurrentSceneProcessId() -> %llu", static_cast<uint32_t>(0),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

}; // namespace xrt::state_trackers::openvr
