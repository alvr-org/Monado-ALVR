// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the OpenVR negotiation interface.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_negotiate
 */

#define VR_API_EXPORT
#include "openvr.h"

#include "interfaces/XRTVRClientCore.hpp"

#include <cstring>
#include <memory>


namespace xrt::state_trackers::openvr {

// The main entrypoint for OpenVR clients.
VR_INTERFACE void *
VRClientCoreFactory(const char *pInterfaceName, int *pReturnCode)
{
	if (std::strcmp(pInterfaceName, vr::IVRClientCore_003_Version) == 0) {
		static XRTVRClientCore_003 clientCore{};
		return &clientCore;
	}

	*pReturnCode = static_cast<int>(vr::VRInitError_Init_InterfaceNotFound);
	return NULL;
}

VR_INTERFACE void *
HmdSystemFactory(const char *pInterfaceName, int *pReturnCode)
{
	// Implemented in SteamVR as a passthrough to VRClientCoreFactory, so we do the same here.
	return VRClientCoreFactory(pInterfaceName, pReturnCode);
}

}; // namespace xrt::state_trackers::openvr
