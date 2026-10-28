// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRChaperone interface version.
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

class XRTVRChaperone_004 : public vr::IVRChaperone_004
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRChaperone_004(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRChaperone_004() = default;

	FnTable_IVRChaperone_004();

	vr::ChaperoneCalibrationState
	GetCalibrationState() override;

	bool
	GetPlayAreaSize(float *pSizeX, float *pSizeZ) override;

	bool
	GetPlayAreaRect(vr::HmdQuad_t *rect) override;

	void
	ReloadInfo(void) override;

	void
	SetSceneColor(vr::HmdColor_t color) override;

	void
	GetBoundsColor(vr::HmdColor_t *pOutputColorArray,
	               int nNumOutputColors,
	               float flCollisionBoundsFadeDistance,
	               vr::HmdColor_t *pOutputCameraColor) override;

	bool
	AreBoundsVisible() override;

	void
	ForceBoundsVisible(bool bForce) override;

	void
	ResetZeroPose(vr::ETrackingUniverseOrigin eTrackingUniverseOrigin) override;
};

} // namespace xrt::state_trackers::openvr
