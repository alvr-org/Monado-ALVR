// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRDebug_001 interface version.
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

class XRTVRDebug_001 : public vr::IVRDebug_001
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRDebug_001(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRDebug_001() = default;

	FnTable_IVRDebug_001();

	EVRDebugError
	EmitVrProfilerEvent(const char *pchMessage) override;

	EVRDebugError
	BeginVrProfilerEvent(VrProfilerEventHandle_t *pHandleOut) override;

	EVRDebugError
	FinishVrProfilerEvent(VrProfilerEventHandle_t hHandle, const char *pchMessage) override;

	uint32_t
	DriverDebugRequest(vr::TrackedDeviceIndex_t unDeviceIndex,
	                   const char *pchRequest,
	                   char *pchResponseBuffer,
	                   uint32_t unResponseBufferSize) override;
};

} // namespace xrt::state_trackers::openvr
