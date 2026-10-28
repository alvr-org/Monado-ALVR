// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRClientCore interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRApplications.hpp"
#include "XRTVRChaperone.hpp"
#include "XRTVRChaperoneSetup.hpp"
#include "XRTVRClientCore.hpp"
#include "XRTVRCompositor.hpp"
#include "XRTVRControlPanel.hpp"
#include "XRTVRDebug.hpp"
#include "XRTVRDriverManager.hpp"
#include "XRTVRExtendedDisplay.hpp"
#include "XRTVRHeadsetView.hpp"
#include "XRTVRInput.hpp"
#include "XRTVRIOBuffer.hpp"
#include "XRTVRNotifications.hpp"
#include "XRTVROverlay.hpp"
#include "XRTVROverlayView.hpp"
#include "XRTVRRenderModels.hpp"
#include "XRTVRResources.hpp"
#include "XRTVRScreenshots.hpp"
#include "XRTVRSettings.hpp"
#include "XRTVRSpatialAnchors.hpp"
#include "XRTVRSystem.hpp"
#include "XRTVRTrackedCamera.hpp"

#include "old/IVRApplications.hpp"
#include "old/IVRChaperone.hpp"
#include "old/IVRChaperoneSetup.hpp"
#include "old/IVRCompositor.hpp"
#include "old/IVRInput.hpp"
#include "old/IVRIOBuffer.hpp"
#include "old/IVRIPCResourceManagerClient.hpp"
#include "old/IVRNotifications.hpp"
#include "old/IVROverlay.hpp"
#include "old/IVRRenderModels.hpp"
#include "old/IVRSettings.hpp"
#include "old/IVRSystem.hpp"
#include "old/IVRTrackedCamera.hpp"

#include <cstring>


namespace xrt::state_trackers::openvr {

vr::EVRInitError
XRTVRClientCore_003::Init(vr::EVRApplicationType eApplicationType, const char *pStartupInfo)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	xrt_result_t xret;

	switch (eApplicationType) {
	case vr::EVRApplicationType::VRApplication_Background:
	case vr::EVRApplicationType::VRApplication_Scene: break;
	default: return vr::EVRInitError::VRInitError_Init_InvalidApplicationType;
	}

	this->application_type = eApplicationType;

	xrt_instance_info i_info = {
	    .app_info =
	        {
	            .application_name = "OpenVR client",
	            .immediate_disconnect = false,
	            .ext_hand_tracking_enabled = false,
	            .ext_hand_tracking_data_source_enabled = false,
	            .ext_eye_gaze_interaction_enabled = false,
	            .ext_future_enabled = false,
	            .ext_hand_interaction_enabled = false,
	            .htc_facial_tracking_enabled = false,
	            .fb_body_tracking_enabled = false,
	            .fb_face_tracking2_enabled = false,
	            .meta_body_tracking_full_body_enabled = false,
	            .meta_body_tracking_calibration_enabled = false,
	            .meta_body_tracking_fidelity_enabled = false,
	            .android_face_tracking_enabled = false,
	            .view_configuration_views_change_supported = true,
	        },
	    .platform_info = {0},
	};

	xret = xrt_instance_create(&i_info, &this->xinst);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to create xrt_instance", xret);
		return xretToInitError(xret);
	}

	bool is_available;
	xret = xrt_instance_is_system_available(this->xinst, &is_available);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to check if system is available", xret);
		return xretToInitError(xret);
	}

	if (!is_available) {
		return vr::EVRInitError::VRInitError_Init_HmdNotFound;
	}

	xret = xrt_instance_create_app_instance(this->xinst, &this->xainst);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to create app instance.", xret);
		return xretToInitError(xret);
	}

	// @note We need an xsysc here since `getVulkanOutputDevice` must work in a headless session.
	xret = xrt_instance_create_system(this->xinst, &this->xsys, &this->xsysd, &this->xso, &this->xsysc);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to create xrt_system", xret);
		return xretToInitError(xret);
	}

	xret = xrt_app_instance_create_app_system(this->xainst, this->xsys, &this->xasys);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to create app system.", xret);
		return xretToInitError(xret);
	}

	xrt_session_info xsi = {
	    .is_overlay = false,
	    .flags = 0,
	    .z_order = 0,
	};
	xret = xrt_system_create_session(this->xsys, &xsi, &this->xs, this->isHeadless() ? nullptr : &this->xcn);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to create xrt_session", xret);
		return xretToInitError(xret);
	}

	this->events = std::make_shared<Events>(this->xs);
	this->compositor = std::make_shared<Compositor>(this->xsysd, this->xcn, this->xsysc, this->events);
	this->devices = std::make_shared<Devices>(logger, this->xso, this->xsysd, this->events);
	this->input = std::make_shared<Input>();

	return vr::EVRInitError::VRInitError_None;
}

void
XRTVRClientCore_003::Cleanup()
{
	// Clear all the interfaces first, to invalidate them.
#define CLEAR_INTERFACE(name, version_suffix) this->name##version_suffix.reset();
	SUPPORTED_INTERFACES(CLEAR_INTERFACE)
#undef CLEAR_INTERFACE

	this->devices.reset();
	this->compositor.reset();
	this->events.reset();

	if (this->xs) {
		xrt_session_destroy(&this->xs);
	}

	if (this->xsysc) {
		xrt_syscomp_destroy(&this->xsysc);
	}

	if (this->xso) {
		xrt_space_overseer_destroy(&this->xso);
	}

	if (this->xsysd) {
		xrt_system_devices_destroy(&this->xsysd);
	}

	if (this->xasys) {
		xrt_app_system_destroy(&this->xasys);
	}

	if (this->xsys) {
		xrt_system_destroy(&this->xsys);
	}

	if (this->xainst) {
		xrt_app_instance_destroy(&this->xainst);
	}

	if (this->xinst) {
		xrt_instance_destroy(&this->xinst);
	}

	return;
}

vr::EVRInitError
XRTVRClientCore_003::IsInterfaceVersionValid(const char *pchInterfaceVersion)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_TRACE(logger, "IsInterfaceVersionValid called with '%s'", pchInterfaceVersion);

#define CHECK_INTERFACE_VERSION(name, version)                                                                         \
	if (std::strcmp(pchInterfaceVersion, vr::I##name##version##_Version) == 0) {                                   \
		return vr::EVRInitError::VRInitError_None;                                                             \
	}
	SUPPORTED_CORE_INTERFACES(CHECK_INTERFACE_VERSION)
#undef CHECK_INTERFACE_VERSION

#define CHECK_INTERFACE_VERSION(name, version_suffix)                                                                  \
	if (std::strcmp(pchInterfaceVersion, vr::I##name##version_suffix##_Version) == 0) {                            \
		return vr::EVRInitError::VRInitError_None;                                                             \
	}
	SUPPORTED_INTERFACES(CHECK_INTERFACE_VERSION)
#undef CHECK_INTERFACE_VERSION

	OPENVR_LOG_WARN(logger, "Requested version '%s' is not supported.", pchInterfaceVersion);

	return vr::EVRInitError::VRInitError_Init_InterfaceNotFound;
}

void *
XRTVRClientCore_003::GetGenericInterface(const char *pchNameAndVersion, vr::EVRInitError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(peError, vr::EVRInitError::VRInitError_None);

	std::string interface_name(pchNameAndVersion);

	OPENVR_LOG_TRACE(logger, "GetGenericInterface called with '%s'", interface_name.c_str());

	static const std::string fntable_prefix = "FnTable:";

	bool fntable = interface_name.starts_with(fntable_prefix);

	// Trim the C prefix
	if (fntable) {
		interface_name = pchNameAndVersion + fntable_prefix.length();
	}

#define GET_CORE_INTERFACE(name, version)                                                                              \
	if (std::strcmp(interface_name.c_str(), vr::I##name##version##_Version) == 0) {                                \
		return static_cast<vr::I##name##version *>(this);                                                      \
	}
	SUPPORTED_CORE_INTERFACES(GET_CORE_INTERFACE)
#undef GET_CORE_INTERFACE

#define GET_INTERFACE(name, version_suffix)                                                                            \
	if (std::strcmp(interface_name.c_str(), vr::I##name##version_suffix##_Version) == 0) {                         \
		if (!this->name##version_suffix) {                                                                     \
			this->name##version_suffix = std::make_shared<XRT##name##version_suffix>(this);                \
		}                                                                                                      \
		if (fntable) {                                                                                         \
			return static_cast<XRT##name##version_suffix *>(this->name##version_suffix.get())              \
			    ->getFnTable();                                                                            \
		}                                                                                                      \
		return this->name##version_suffix.get();                                                               \
	}
	SUPPORTED_INTERFACES(GET_INTERFACE)
#undef GET_INTERFACE

	SET_ERROR(peError, vr::EVRInitError::VRInitError_Init_InterfaceNotFound);

	OPENVR_LOG_WARN(logger, "Requested interface '%s' is not supported.", interface_name.c_str());

	return nullptr;
}

bool
XRTVRClientCore_003::BIsHmdPresent()
{
	xrt_result_t xret;

	// If we have a created system then shortcut this to `true`. Because Climbey is polling this synchronously every
	// single frame while the game runs. On the main thread.
	if (this->xsys != nullptr) {
		return true;
	}

	xrt_instance_info i_info = {
	    .app_info =
	        {
	            .application_name = "OpenVR client (temporary instance for BIsHmdPresent)",
	            .immediate_disconnect = false,
	            .ext_hand_tracking_enabled = false,
	            .ext_hand_tracking_data_source_enabled = false,
	            .ext_eye_gaze_interaction_enabled = false,
	            .ext_future_enabled = false,
	            .ext_hand_interaction_enabled = false,
	            .htc_facial_tracking_enabled = false,
	            .fb_body_tracking_enabled = false,
	            .fb_face_tracking2_enabled = false,
	            .meta_body_tracking_full_body_enabled = false,
	            .meta_body_tracking_calibration_enabled = false,
	            .meta_body_tracking_fidelity_enabled = false,
	            .android_face_tracking_enabled = false,
	            .view_configuration_views_change_supported = false,
	        },
	    .platform_info = {0},
	};

	xrt_instance *xinst;
	xret = xrt_instance_create(&i_info, &xinst);
	if (xret != XRT_SUCCESS) {
		return false;
	}

	bool is_available;
	xret = xrt_instance_is_system_available(xinst, &is_available);
	if (xret != XRT_SUCCESS) {
		// Error, but nothing we can do.
		is_available = false;
	}

	xrt_instance_destroy(&xinst);

	return is_available;
}

const char *
XRTVRClientCore_003::GetEnglishStringForHmdError(vr::EVRInitError eError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_TRACE(logger, "GetEnglishStringForHmdError called with error code %d", static_cast<int>(eError));

	return "TODO: GetEnglishStringForHmdError";
}

const char *
XRTVRClientCore_003::GetIDForVRInitError(vr::EVRInitError eError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_TRACE(logger, "GetIDForVRInitError called with error code %d", static_cast<int>(eError));

	return "TODO: GetIDForVRInitError";
}

}; // namespace xrt::state_trackers::openvr
