// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRApplications interface version 005.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"
#include "common/openvr_error.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRApplications.hpp"
#include "XRTVRApplications_006.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRApplications_005 : public XRTVRApplications_006, public vr::IVRApplications_005
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRApplications_005(XRTVRClientCore_003 *core) : XRTVRApplications_006(core) {};
	// clang-format on

	FnTable_IVRApplications_005();

	ForwardDeclareBase_IVRApplications(XRTVRApplications_006);

	Forward_IVRApplications_AddApplicationManifest();
	Forward_IVRApplications_RemoveApplicationManifest();
	Forward_IVRApplications_IsApplicationInstalled();
	Forward_IVRApplications_GetApplicationCount();

	EVRApplicationError
	GetApplicationKeyByIndex(uint32_t unApplicationIndex,
	                         char *pchAppKeyBuffer,
	                         uint32_t unAppKeyBufferLen) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetApplicationKeyByIndex(unApplicationIndex=%llu, pchAppKeyBuffer=%p, "
		    "unAppKeyBufferLen=%llu) -> VRApplicationError_None",
		    EVRApplicationError::VRApplicationError_None, static_cast<unsigned long long>(unApplicationIndex),
		    static_cast<const void *>(pchAppKeyBuffer), static_cast<unsigned long long>(unAppKeyBufferLen));
	}

	EVRApplicationError
	GetApplicationKeyByProcessId(uint32_t unProcessId, char *pchAppKeyBuffer, uint32_t unAppKeyBufferLen) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetApplicationKeyByProcessId(unProcessId=%llu, pchAppKeyBuffer=%p, "
		    "unAppKeyBufferLen=%llu) -> VRApplicationError_None",
		    EVRApplicationError::VRApplicationError_None, static_cast<unsigned long long>(unProcessId),
		    static_cast<const void *>(pchAppKeyBuffer), static_cast<unsigned long long>(unAppKeyBufferLen));
	}

	Forward_IVRApplications_LaunchApplication();
	Forward_IVRApplications_LaunchTemplateApplication();
	Forward_IVRApplications_LaunchDashboardOverlay();
	Forward_IVRApplications_CancelApplicationLaunch();
	Forward_IVRApplications_IdentifyApplication();
	Forward_IVRApplications_GetApplicationProcessId();
	Forward_IVRApplications_GetApplicationsErrorNameFromEnum();

	uint32_t
	GetApplicationPropertyString(const char *pchAppKey,
	                             EVRApplicationProperty eProperty,
	                             char *pchPropertyValueBuffer,
	                             uint32_t unPropertyValueBufferLen,
	                             EVRApplicationError *peError = nullptr) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		SET_ERROR(peError, EVRApplicationError::VRApplicationError_None);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetApplicationPropertyString(pchAppKey=%s, eProperty=?, "
		    "pchPropertyValueBuffer=%p, unPropertyValueBufferLen=%llu, peError=%p) -> 0",
		    0, pchAppKey, static_cast<const void *>(pchPropertyValueBuffer),
		    static_cast<unsigned long long>(unPropertyValueBufferLen), static_cast<const void *>(peError));
	}

	Forward_IVRApplications_GetApplicationPropertyBool();
	Forward_IVRApplications_GetApplicationPropertyUint64();
	Forward_IVRApplications_SetApplicationAutoLaunch();
	Forward_IVRApplications_GetApplicationAutoLaunch();

	EVRApplicationError
	GetStartingApplication(char *pchAppKeyBuffer, uint32_t unAppKeyBufferLen) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetStartingApplication(pchAppKeyBuffer=%p, unAppKeyBufferLen=%llu) -> VRApplicationError_None",
		    EVRApplicationError::VRApplicationError_None, static_cast<const void *>(pchAppKeyBuffer),
		    static_cast<unsigned long long>(unAppKeyBufferLen));
	}

	Forward_IVRApplications_GetTransitionState();
	Forward_IVRApplications_PerformApplicationPrelaunchCheck();
	Forward_IVRApplications_GetApplicationsTransitionStateNameFromEnum();
	Forward_IVRApplications_IsQuitUserPromptRequested();
	Forward_IVRApplications_LaunchInternalProcess();
};

}; // namespace xrt::state_trackers::openvr
