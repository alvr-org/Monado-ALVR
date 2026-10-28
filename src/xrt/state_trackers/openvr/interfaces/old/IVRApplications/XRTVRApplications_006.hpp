// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRApplications interface version 006.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRApplications.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRApplications_006 : public XRTVRApplications_008, public vr::IVRApplications_006
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRApplications_006(XRTVRClientCore_003 *core) : XRTVRApplications_008(core) {};
	// clang-format on

	FnTable_IVRApplications_006();

	ForwardDeclareBase_IVRApplications(XRTVRApplications_008);

	Forward_IVRApplications_AddApplicationManifest();
	Forward_IVRApplications_RemoveApplicationManifest();
	Forward_IVRApplications_IsApplicationInstalled();
	Forward_IVRApplications_GetApplicationCount();
	Forward_IVRApplications_GetApplicationKeyByIndex_008();
	Forward_IVRApplications_GetApplicationKeyByProcessId_008();
	Forward_IVRApplications_LaunchApplication();
	Forward_IVRApplications_LaunchTemplateApplication();
	Forward_IVRApplications_LaunchApplicationFromMimeType();
	Forward_IVRApplications_LaunchDashboardOverlay();
	Forward_IVRApplications_CancelApplicationLaunch();
	Forward_IVRApplications_IdentifyApplication();
	Forward_IVRApplications_GetApplicationProcessId();
	Forward_IVRApplications_GetApplicationsErrorNameFromEnum();
	Forward_IVRApplications_GetApplicationPropertyString_008();
	Forward_IVRApplications_GetApplicationPropertyBool();
	Forward_IVRApplications_GetApplicationPropertyUint64();
	Forward_IVRApplications_SetApplicationAutoLaunch();
	Forward_IVRApplications_GetApplicationAutoLaunch();
	Forward_IVRApplications_SetDefaultApplicationForMimeType();
	Forward_IVRApplications_GetDefaultApplicationForMimeType();
	Forward_IVRApplications_GetApplicationSupportedMimeTypes();
	Forward_IVRApplications_GetApplicationsThatSupportMimeType();
	Forward_IVRApplications_GetApplicationLaunchArguments();
	Forward_IVRApplications_GetStartingApplication_008();

	EVRApplicationTransitionState
	GetTransitionState() override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetTransitionState() -> VRApplicationTransition_None",
		                             EVRApplicationTransitionState::VRApplicationTransition_None);
	}

	Forward_IVRApplications_PerformApplicationPrelaunchCheck();

	const char *
	GetApplicationsTransitionStateNameFromEnum(EVRApplicationTransitionState state) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetApplicationsTransitionStateNameFromEnum(state=%d)",
		                             "TODO: GetApplicationsTransitionStateNameFromEnum", state);
	}

	bool
	IsQuitUserPromptRequested() override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsQuitUserPromptRequested() -> false", false);
	}

	Forward_IVRApplications_LaunchInternalProcess();
	Forward_IVRApplications_GetCurrentSceneProcessId();
};

}; // namespace xrt::state_trackers::openvr
