// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRApplications interface version 001.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRApplications.hpp"
#include "XRTVRApplications_005.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRApplications_001 : public XRTVRApplications_005, public vr::IVRApplications_001
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRApplications_001(XRTVRClientCore_003 *core) : XRTVRApplications_005(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRApplications_001.
		return nullptr;
	}

	ForwardDeclareBase_IVRApplications(XRTVRApplications_005);

	Forward_IVRApplications_AddApplicationManifest();
	Forward_IVRApplications_RemoveApplicationManifest();
	Forward_IVRApplications_IsApplicationInstalled();
	Forward_IVRApplications_GetApplicationCount();
	Forward_IVRApplications_GetApplicationKeyByIndex_005();
	Forward_IVRApplications_GetApplicationKeyByProcessId_005();
	Forward_IVRApplications_LaunchApplication();
	Forward_IVRApplications_LaunchDashboardOverlay();
	Forward_IVRApplications_IdentifyApplication();
	Forward_IVRApplications_GetApplicationProcessId();
	Forward_IVRApplications_GetApplicationsErrorNameFromEnum();
	Forward_IVRApplications_GetApplicationPropertyString_005();
	Forward_IVRApplications_GetApplicationPropertyBool();

	EVRApplicationError
	GetHomeApplication(char *pchAppKeyBuffer, uint32_t unAppKeyBufferLen) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetHomeApplication(pchAppKeyBuffer=%p, unAppKeyBufferLen=%llu) -> VRApplicationError_None",
		    EVRApplicationError::VRApplicationError_None, static_cast<const void *>(pchAppKeyBuffer),
		    static_cast<unsigned long long>(unAppKeyBufferLen));
	}

	EVRApplicationError
	SetHomeApplication(const char *pchAppKey) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetHomeApplication(pchAppKey=%s) -> VRApplicationError_None",
		                             EVRApplicationError::VRApplicationError_None, pchAppKey);
	}

	Forward_IVRApplications_SetApplicationAutoLaunch();
	Forward_IVRApplications_GetApplicationAutoLaunch();
	Forward_IVRApplications_GetStartingApplication_005();
	Forward_IVRApplications_GetTransitionState();
	Forward_IVRApplications_PerformApplicationPrelaunchCheck();
	Forward_IVRApplications_GetApplicationsTransitionStateNameFromEnum();
};

}; // namespace xrt::state_trackers::openvr
