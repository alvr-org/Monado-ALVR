// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRNotifications_002 interface version.
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

class XRTVRNotifications_002 : public vr::IVRNotifications_002
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRNotifications_002(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRNotifications_002() = default;

	FnTable_IVRNotifications_002();

	EVRNotificationError
	CreateNotification(VROverlayHandle_t ulOverlayHandle,
	                   uint64_t ulUserValue,
	                   EVRNotificationType type,
	                   const char *pchText,
	                   EVRNotificationStyle style,
	                   const NotificationBitmap_t *pImage,
	                   VRNotificationId *pNotificationId) override;

	EVRNotificationError
	RemoveNotification(VRNotificationId notificationId) override;
};

} // namespace xrt::state_trackers::openvr
