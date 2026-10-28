// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRApplications_008 interface version.
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

class XRTVRApplications_008 : public vr::IVRApplications_008
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRApplications_008(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRApplications_008() = default;

	FnTable_IVRApplications_008();

	EVRApplicationError
	AddApplicationManifest(const char *pchApplicationManifestFullPath, bool bTemporary) override;

	EVRApplicationError
	RemoveApplicationManifest(const char *pchApplicationManifestFullPath) override;

	bool
	IsApplicationInstalled(const char *pchAppKey) override;

	uint32_t
	GetApplicationCount() override;

	EVRApplicationError
	GetApplicationKeyByIndex(uint32_t unApplicationIndex,
	                         char *pchAppKeyBuffer,
	                         uint32_t unAppKeyBufferLen) override;

	EVRApplicationError
	GetApplicationKeyByProcessId(uint32_t unProcessId, char *pchAppKeyBuffer, uint32_t unAppKeyBufferLen) override;

	EVRApplicationError
	LaunchApplication(const char *pchAppKey) override;

	EVRApplicationError
	LaunchTemplateApplication(const char *pchTemplateAppKey,
	                          const char *pchNewAppKey,
	                          const AppOverrideKeys_t *pKeys,
	                          uint32_t unKeys) override;

	vr::EVRApplicationError
	LaunchApplicationFromMimeType(const char *pchMimeType, const char *pchArgs) override;

	EVRApplicationError
	LaunchDashboardOverlay(const char *pchAppKey) override;

	bool
	CancelApplicationLaunch(const char *pchAppKey) override;

	EVRApplicationError
	IdentifyApplication(uint32_t unProcessId, const char *pchAppKey) override;

	uint32_t
	GetApplicationProcessId(const char *pchAppKey) override;

	const char *
	GetApplicationsErrorNameFromEnum(EVRApplicationError error) override;

	uint32_t
	GetApplicationPropertyString(const char *pchAppKey,
	                             EVRApplicationProperty eProperty,
	                             char *pchPropertyValueBuffer,
	                             uint32_t unPropertyValueBufferLen,
	                             EVRApplicationError *peError) override;

	bool
	GetApplicationPropertyBool(const char *pchAppKey,
	                           EVRApplicationProperty eProperty,
	                           EVRApplicationError *peError) override;

	uint64_t
	GetApplicationPropertyUint64(const char *pchAppKey,
	                             EVRApplicationProperty eProperty,
	                             EVRApplicationError *peError) override;

	EVRApplicationError
	SetApplicationAutoLaunch(const char *pchAppKey, bool bAutoLaunch) override;

	bool
	GetApplicationAutoLaunch(const char *pchAppKey) override;

	EVRApplicationError
	SetDefaultApplicationForMimeType(const char *pchAppKey, const char *pchMimeType) override;

	bool
	GetDefaultApplicationForMimeType(const char *pchMimeType,
	                                 char *pchAppKeyBuffer,
	                                 uint32_t unAppKeyBufferLen) override;

	bool
	GetApplicationSupportedMimeTypes(const char *pchAppKey,
	                                 char *pchMimeTypesBuffer,
	                                 uint32_t unMimeTypesBuffer) override;

	uint32_t
	GetApplicationsThatSupportMimeType(const char *pchMimeType,
	                                   char *pchAppKeysThatSupportBuffer,
	                                   uint32_t unAppKeysThatSupportBuffer) override;

	uint32_t
	GetApplicationLaunchArguments(uint32_t unHandle, char *pchArgs, uint32_t unArgs) override;

	EVRApplicationError
	GetStartingApplication(char *pchAppKeyBuffer, uint32_t unAppKeyBufferLen) override;

	EVRSceneApplicationState
	GetSceneApplicationState() override;

	EVRApplicationError
	PerformApplicationPrelaunchCheck(const char *pchAppKey) override;

	const char *
	GetSceneApplicationStateNameFromEnum(EVRSceneApplicationState state) override;

	EVRApplicationError
	LaunchInternalProcess(const char *pchBinaryPath,
	                      const char *pchArguments,
	                      const char *pchWorkingDirectory) override;

	EVRApplicationError
	RegisterSubprocess(uint32_t nPid) override;

	uint32_t
	GetCurrentSceneProcessId() override;
};

} // namespace xrt::state_trackers::openvr
