// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVROverlayView_003 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVROverlayView.hpp"


namespace xrt::state_trackers::openvr {

XRTVROverlayView_003::XRTVROverlayView_003(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

EVROverlayError
XRTVROverlayView_003::AcquireOverlayView(VROverlayHandle_t ulOverlayHandle,
                                         VRNativeDevice_t *pNativeDevice,
                                         VROverlayView_t *pOverlayView,
                                         uint32_t unOverlayViewSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "AcquireOverlayView(ulOverlayHandle=%llu, pNativeDevice=%p, pOverlayView=%p, "
	                             "unOverlayViewSize=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pNativeDevice), static_cast<void *>(pOverlayView),
	                             static_cast<unsigned long long>(unOverlayViewSize),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlayView_003::ReleaseOverlayView(VROverlayView_t *pOverlayView)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ReleaseOverlayView(pOverlayView=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<void *>(pOverlayView),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

void
XRTVROverlayView_003::PostOverlayEvent(VROverlayHandle_t ulOverlayHandle, const VREvent_t *pvrEvent)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "PostOverlayEvent(ulOverlayHandle=%llu, pvrEvent=%p)",
	                         static_cast<unsigned long long>(ulOverlayHandle), static_cast<const void *>(pvrEvent));
}

bool
XRTVROverlayView_003::IsViewingPermitted(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsViewingPermitted(ulOverlayHandle=%llu) -> %d", false,
	                             static_cast<unsigned long long>(ulOverlayHandle), false);
}

}; // namespace xrt::state_trackers::openvr
