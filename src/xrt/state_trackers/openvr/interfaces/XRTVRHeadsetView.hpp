// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRHeadsetView_001 interface version.
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

class XRTVRHeadsetView_001 : public vr::IVRHeadsetView_001
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRHeadsetView_001(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRHeadsetView_001() = default;

	FnTable_IVRHeadsetView_001();

	void
	SetHeadsetViewSize(uint32_t nWidth, uint32_t nHeight) override;

	void
	GetHeadsetViewSize(uint32_t *pnWidth, uint32_t *pnHeight) override;

	void
	SetHeadsetViewMode(HeadsetViewMode_t eHeadsetViewMode) override;

	HeadsetViewMode_t
	GetHeadsetViewMode() override;

	void
	SetHeadsetViewCropped(bool bCropped) override;

	bool
	GetHeadsetViewCropped() override;

	float
	GetHeadsetViewAspectRatio() override;

	void
	SetHeadsetViewBlendRange(float flStartPct, float flEndPct) override;

	void
	GetHeadsetViewBlendRange(float *pStartPct, float *pEndPct) override;
};

} // namespace xrt::state_trackers::openvr
