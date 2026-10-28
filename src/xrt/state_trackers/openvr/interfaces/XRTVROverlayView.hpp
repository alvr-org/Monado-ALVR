// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVROverlayView_003 interface version.
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

class XRTVROverlayView_003 : public vr::IVROverlayView_003
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVROverlayView_003(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVROverlayView_003() = default;

	FnTable_IVROverlayView_003();

	EVROverlayError
	AcquireOverlayView(VROverlayHandle_t ulOverlayHandle,
	                   VRNativeDevice_t *pNativeDevice,
	                   VROverlayView_t *pOverlayView,
	                   uint32_t unOverlayViewSize) override;

	EVROverlayError
	ReleaseOverlayView(VROverlayView_t *pOverlayView) override;

	void
	PostOverlayEvent(VROverlayHandle_t ulOverlayHandle, const VREvent_t *pvrEvent) override;

	bool
	IsViewingPermitted(VROverlayHandle_t ulOverlayHandle) override;
};

} // namespace xrt::state_trackers::openvr
