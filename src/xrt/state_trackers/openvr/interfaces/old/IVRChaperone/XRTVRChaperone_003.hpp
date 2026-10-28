// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRChaperone interface version 003.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRChaperone.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRChaperone_003 : public XRTVRChaperone_004, public vr::IVRChaperone_003
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRChaperone_003(XRTVRClientCore_003 *core) : XRTVRChaperone_004(core) {};
	// clang-format on

	FnTable_IVRChaperone_003();

	ForwardDeclareBase_IVRChaperone(XRTVRChaperone_004);

	Forward_IVRChaperone_GetCalibrationState();
	Forward_IVRChaperone_GetPlayAreaSize();
	Forward_IVRChaperone_GetPlayAreaRect();
	Forward_IVRChaperone_ReloadInfo();
	Forward_IVRChaperone_SetSceneColor();
	Forward_IVRChaperone_GetBoundsColor_004();
	Forward_IVRChaperone_AreBoundsVisible();
	Forward_IVRChaperone_ForceBoundsVisible();
};

}; // namespace xrt::state_trackers::openvr
