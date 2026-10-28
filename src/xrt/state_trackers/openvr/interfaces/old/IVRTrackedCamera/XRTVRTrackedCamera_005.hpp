// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRTrackedCamera interface version 005.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRTrackedCamera.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRTrackedCamera_005 : public XRTVRTrackedCamera_006, public vr::IVRTrackedCamera_005
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRTrackedCamera_005(XRTVRClientCore_003 *core) : XRTVRTrackedCamera_006(core) {};
	// clang-format on

	FnTable_IVRTrackedCamera_005();

	ForwardDeclareBase_IVRTrackedCamera(XRTVRTrackedCamera_006);

	Forward_IVRTrackedCamera_GetCameraErrorNameFromEnum();
	Forward_IVRTrackedCamera_HasCamera_006();
	Forward_IVRTrackedCamera_GetCameraFrameSize();
	Forward_IVRTrackedCamera_GetCameraIntrinsics_006();
	Forward_IVRTrackedCamera_GetCameraProjection_006();
	Forward_IVRTrackedCamera_AcquireVideoStreamingService();
	Forward_IVRTrackedCamera_ReleaseVideoStreamingService();
	Forward_IVRTrackedCamera_GetVideoStreamFrameBuffer();
	Forward_IVRTrackedCamera_GetVideoStreamTextureSize();
	Forward_IVRTrackedCamera_GetVideoStreamTextureD3D11();
	Forward_IVRTrackedCamera_GetVideoStreamTextureGL();
	Forward_IVRTrackedCamera_ReleaseVideoStreamTextureGL();
};

}; // namespace xrt::state_trackers::openvr
