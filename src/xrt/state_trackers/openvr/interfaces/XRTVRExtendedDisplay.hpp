// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRExtendedDisplay_001 interface version.
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

class XRTVRExtendedDisplay_001 : public vr::IVRExtendedDisplay_001
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRExtendedDisplay_001(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRExtendedDisplay_001() = default;

	FnTable_IVRExtendedDisplay_001();

	void
	GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override;

	void
	GetEyeOutputViewport(EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override;

	void
	GetDXGIOutputInfo(int32_t *pnAdapterIndex, int32_t *pnAdapterOutputIndex) override;
};

} // namespace xrt::state_trackers::openvr
