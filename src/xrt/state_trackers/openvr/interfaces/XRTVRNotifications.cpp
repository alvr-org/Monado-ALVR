// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRNotifications_002 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRNotifications.hpp"


namespace xrt::state_trackers::openvr {

XRTVRNotifications_002::XRTVRNotifications_002(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

EVRNotificationError
XRTVRNotifications_002::CreateNotification(VROverlayHandle_t ulOverlayHandle,
                                           uint64_t ulUserValue,
                                           EVRNotificationType type,
                                           const char *pchText,
                                           EVRNotificationStyle style,
                                           const NotificationBitmap_t *pImage,
                                           VRNotificationId *pNotificationId)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "CreateNotification(ulOverlayHandle=%llu, ulUserValue=%llu, type=%llu, pchText=%s, style=%llu, pImage=%p, "
	    "pNotificationId=%p) -> %llu",
	    static_cast<EVRNotificationError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<unsigned long long>(ulUserValue), static_cast<unsigned long long>(type),
	    pchText != nullptr ? pchText : "(null)", static_cast<unsigned long long>(style),
	    static_cast<const void *>(pImage), static_cast<void *>(pNotificationId),
	    static_cast<unsigned long long>(static_cast<EVRNotificationError>(0)));
}

EVRNotificationError
XRTVRNotifications_002::RemoveNotification(VRNotificationId notificationId)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "RemoveNotification(notificationId=%llu) -> %llu",
	                             static_cast<EVRNotificationError>(0),
	                             static_cast<unsigned long long>(notificationId),
	                             static_cast<unsigned long long>(static_cast<EVRNotificationError>(0)));
}

}; // namespace xrt::state_trackers::openvr
