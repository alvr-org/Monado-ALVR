// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRDriverManager_001 interface version.
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

class XRTVRDriverManager_001 : public vr::IVRDriverManager_001
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRDriverManager_001(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRDriverManager_001() = default;

	FnTable_IVRDriverManager_001();

	uint32_t
	GetDriverCount() const override;

	uint32_t
	GetDriverName(vr::DriverId_t nDriver, char *pchValue, uint32_t unBufferSize) override;

	DriverHandle_t
	GetDriverHandle(const char *pchDriverName) override;

	bool
	IsEnabled(vr::DriverId_t nDriver) const override;
};

} // namespace xrt::state_trackers::openvr
