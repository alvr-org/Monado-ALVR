// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRClientCore interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "xrt/xrt_instance.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_session.h"
#include "xrt/xrt_compositor.h"

#include "openvr_devices.hpp"
#include "openvr_events.hpp"
#include "openvr_input.hpp"

#include "compositor/openvr_compositor.hpp"

#include "openvr_interfaces_unified.h"
#include "IVRClientCore_003.h"

#include <memory>


namespace xrt::state_trackers::openvr {

#define SUPPORTED_CORE_INTERFACES(_) _(VRClientCore, _003)
#define SUPPORTED_INTERFACES(_)                                                                                        \
	_(VRApplications, _001)                                                                                        \
	_(VRApplications, _002)                                                                                        \
	_(VRApplications, _003)                                                                                        \
	_(VRApplications, _004)                                                                                        \
	_(VRApplications, _005)                                                                                        \
	_(VRApplications, _006)                                                                                        \
	_(VRApplications, _007)                                                                                        \
	_(VRApplications, _008)                                                                                        \
	_(VRChaperone, _002)                                                                                           \
	_(VRChaperone, _003)                                                                                           \
	_(VRChaperone, _004)                                                                                           \
	_(VRChaperoneSetup, _004)                                                                                      \
	_(VRChaperoneSetup, _005)                                                                                      \
	_(VRChaperoneSetup, _006)                                                                                      \
	_(VRCompositor, _005)                                                                                          \
	_(VRCompositor, _006)                                                                                          \
	_(VRCompositor, _007)                                                                                          \
	_(VRCompositor, _008)                                                                                          \
	_(VRCompositor, _009)                                                                                          \
	_(VRCompositor, _010)                                                                                          \
	_(VRCompositor, _011)                                                                                          \
	_(VRCompositor, _012)                                                                                          \
	_(VRCompositor, _013)                                                                                          \
	_(VRCompositor, _014)                                                                                          \
	_(VRCompositor, _015)                                                                                          \
	_(VRCompositor, _016)                                                                                          \
	_(VRCompositor, _018)                                                                                          \
	_(VRCompositor, _019)                                                                                          \
	_(VRCompositor, _020)                                                                                          \
	_(VRCompositor, _021)                                                                                          \
	_(VRCompositor, _022)                                                                                          \
	_(VRCompositor, _024)                                                                                          \
	_(VRCompositor, _026)                                                                                          \
	_(VRCompositor, _027)                                                                                          \
	_(VRCompositor, _028)                                                                                          \
	_(VRCompositor, _029)                                                                                          \
	_(VRControlPanel, _001)                                                                                        \
	_(VRDebug, _001)                                                                                               \
	_(VRDriverManager, _001)                                                                                       \
	_(VRExtendedDisplay, _001)                                                                                     \
	_(VRHeadsetView, _001)                                                                                         \
	_(VRInput, _003)                                                                                               \
	_(VRInput, _004)                                                                                               \
	_(VRInput, _005)                                                                                               \
	_(VRInput, _006)                                                                                               \
	_(VRInput, _007)                                                                                               \
	_(VRInput, _010)                                                                                               \
	_(VRInput, _011)                                                                                               \
	_(VRIOBuffer, _001)                                                                                            \
	_(VRIOBuffer, _002)                                                                                            \
	_(VRIPCResourceManagerClient, _001)                                                                            \
	_(VRIPCResourceManagerClient, _002)                                                                            \
	_(VRNotifications, _001)                                                                                       \
	_(VRNotifications, _002)                                                                                       \
	_(VROverlay, _001)                                                                                             \
	_(VROverlay, _002)                                                                                             \
	_(VROverlay, _003)                                                                                             \
	_(VROverlay, _004)                                                                                             \
	_(VROverlay, _005)                                                                                             \
	_(VROverlay, _006)                                                                                             \
	_(VROverlay, _007)                                                                                             \
	_(VROverlay, _008)                                                                                             \
	_(VROverlay, _010)                                                                                             \
	_(VROverlay, _011)                                                                                             \
	_(VROverlay, _012)                                                                                             \
	_(VROverlay, _013)                                                                                             \
	_(VROverlay, _014)                                                                                             \
	_(VROverlay, _016)                                                                                             \
	_(VROverlay, _017)                                                                                             \
	_(VROverlay, _018)                                                                                             \
	_(VROverlay, _019)                                                                                             \
	_(VROverlay, _020)                                                                                             \
	_(VROverlay, _021)                                                                                             \
	_(VROverlay, _022)                                                                                             \
	_(VROverlay, _024)                                                                                             \
	_(VROverlay, _025)                                                                                             \
	_(VROverlay, _026)                                                                                             \
	_(VROverlay, _027)                                                                                             \
	_(VROverlay, _028)                                                                                             \
	_(VROverlayView, _003)                                                                                         \
	_(VRRenderModels, _001)                                                                                        \
	_(VRRenderModels, _002)                                                                                        \
	_(VRRenderModels, _004)                                                                                        \
	_(VRRenderModels, _005)                                                                                        \
	_(VRRenderModels, _006)                                                                                        \
	_(VRResources, _001)                                                                                           \
	_(VRScreenshots, _001)                                                                                         \
	_(VRSettings, _001)                                                                                            \
	_(VRSettings, _002)                                                                                            \
	_(VRSettings, _003)                                                                                            \
	_(VRSpatialAnchors, _001)                                                                                      \
	_(VRSystem, _003)                                                                                              \
	_(VRSystem, _004)                                                                                              \
	_(VRSystem, _005)                                                                                              \
	_(VRSystem, _006)                                                                                              \
	_(VRSystem, _007)                                                                                              \
	_(VRSystem, _009)                                                                                              \
	_(VRSystem, _010)                                                                                              \
	_(VRSystem, _011)                                                                                              \
	_(VRSystem, _012)                                                                                              \
	_(VRSystem, _014)                                                                                              \
	_(VRSystem, _015)                                                                                              \
	_(VRSystem, _016)                                                                                              \
	_(VRSystem, _017)                                                                                              \
	_(VRSystem, _019)                                                                                              \
	_(VRSystem, _020)                                                                                              \
	_(VRSystem, _021)                                                                                              \
	_(VRSystem, _022)                                                                                              \
	_(VRSystem, _023)                                                                                              \
	_(VRSystem, _026)                                                                                              \
	_(VRTrackedCamera, _001)                                                                                       \
	_(VRTrackedCamera, _002)                                                                                       \
	_(VRTrackedCamera, _003)                                                                                       \
	_(VRTrackedCamera, _004)                                                                                       \
	_(VRTrackedCamera, _005)                                                                                       \
	_(VRTrackedCamera, _006)

class XRTVRClientCore_003 : public vr::IVRClientCore_003
{
public: // Fields
	xrt_instance *xinst{nullptr};
	xrt_app_instance *xainst{nullptr};

	xrt_system *xsys{nullptr};
	xrt_app_system *xasys{nullptr};
	xrt_system_devices *xsysd{nullptr};
	xrt_space_overseer *xso{nullptr};
	xrt_system_compositor *xsysc{nullptr};

	xrt_session *xs{nullptr};
	xrt_compositor_native *xcn{nullptr};

	std::shared_ptr<Compositor> compositor{};
	std::shared_ptr<Devices> devices{};
	std::shared_ptr<Events> events{};
	std::shared_ptr<Input> input{};

	vr::EVRApplicationType application_type{};

#define XRTVR_INTERFACE_MEMBER(name, version) std::shared_ptr<vr::I##name##version> name##version{nullptr};
	SUPPORTED_INTERFACES(XRTVR_INTERFACE_MEMBER)
#undef XRTVR_INTERFACE_MEMBER

private: // Methods
	bool
	isHeadless()
	{
		return this->application_type == vr::EVRApplicationType::VRApplication_Background;
	}

public: // Methods
	vr::EVRInitError
	Init(vr::EVRApplicationType eApplicationType, const char *pStartupInfo) override;

	void
	Cleanup() override;

	vr::EVRInitError
	IsInterfaceVersionValid(const char *pchInterfaceVersion) override;

	void *
	GetGenericInterface(const char *pchNameAndVersion, vr::EVRInitError *peError) override;

	bool
	BIsHmdPresent() override;

	const char *
	GetEnglishStringForHmdError(vr::EVRInitError eError) override;

	const char *
	GetIDForVRInitError(vr::EVRInitError eError) override;
};

}; // namespace xrt::state_trackers::openvr
