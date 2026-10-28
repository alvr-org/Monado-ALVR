// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRResources_001 interface version.
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

class XRTVRResources_001 : public vr::IVRResources_001
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRResources_001(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRResources_001() = default;

	FnTable_IVRResources_001();

	uint32_t
	LoadSharedResource(const char *pchResourceName, char *pchBuffer, uint32_t unBufferLen) override;

	uint32_t
	GetResourceFullPath(const char *pchResourceName,
	                    const char *pchResourceTypeDirectory,
	                    char *pchPathBuffer,
	                    uint32_t unBufferLen) override;
};

} // namespace xrt::state_trackers::openvr
