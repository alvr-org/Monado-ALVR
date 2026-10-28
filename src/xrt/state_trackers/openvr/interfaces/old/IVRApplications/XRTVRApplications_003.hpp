// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRApplications interface version 003.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRApplications.hpp"
#include "XRTVRApplications_005.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRApplications_003 : public XRTVRApplications_005, public vr::IVRApplications_003
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRApplications_003(XRTVRClientCore_003 *core) : XRTVRApplications_005(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRApplications_003.
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
	Forward_IVRApplications_GetApplicationPropertyUint64();
	Forward_IVRApplications_SetApplicationAutoLaunch();
	Forward_IVRApplications_GetApplicationAutoLaunch();
	Forward_IVRApplications_GetStartingApplication_005();
	Forward_IVRApplications_GetTransitionState();
	Forward_IVRApplications_PerformApplicationPrelaunchCheck();
	Forward_IVRApplications_GetApplicationsTransitionStateNameFromEnum();
	Forward_IVRApplications_IsQuitUserPromptRequested();
};

}; // namespace xrt::state_trackers::openvr
