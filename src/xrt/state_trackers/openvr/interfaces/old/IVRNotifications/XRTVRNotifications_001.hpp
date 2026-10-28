// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRNotifications interface version 001.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRNotifications.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRNotifications_001 : public XRTVRNotifications_002, public vr::IVRNotifications_001
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRNotifications_001(XRTVRClientCore_003 *core) : XRTVRNotifications_002(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRNotifications_001.
		return nullptr;
	}

	ForwardDeclareBase_IVRNotifications(XRTVRNotifications_002);

	uint32_t
	GetErrorString(NotificationError_t error, VR_OUT_STRING() char *pchBuffer, uint32_t unBufferSize) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetErrorString(error=?, pchBuffer=%p, unBufferSize=%llu) -> 0", 0,
		                             static_cast<const void *>(pchBuffer),
		                             static_cast<unsigned long long>(unBufferSize));
	}

	NotificationError_t
	CreateNotification(VROverlayHandle_t ulOverlayHandle,
	                   uint64_t ulUserValue,
	                   const char *strType,
	                   const char *strText,
	                   const char *strCategory,
	                   const NotificationBitmap *photo,
	                   vr::VRNotificationId *notificationId) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "CreateNotification(ulOverlayHandle=%llu, ulUserValue=%llu, strType=%s, "
		    "strText=%s, strCategory=%s, photo=%p, notificationId=%p) -> k_ENotificationError_OK",
		    NotificationError_t::k_ENotificationError_OK, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<unsigned long long>(ulUserValue), strType, strText, strCategory,
		    static_cast<const void *>(photo), static_cast<const void *>(notificationId));
	}

	NotificationError_t
	DismissNotification(vr::VRNotificationId notificationId) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "DismissNotification(notificationId=%llu) -> k_ENotificationError_OK",
		    NotificationError_t::k_ENotificationError_OK, static_cast<unsigned long long>(notificationId));
	}
};

}; // namespace xrt::state_trackers::openvr
