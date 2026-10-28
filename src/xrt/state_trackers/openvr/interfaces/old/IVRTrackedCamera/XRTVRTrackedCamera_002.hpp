// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRTrackedCamera interface version 002.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRTrackedCamera.hpp"
#include "XRTVRTrackedCamera_004.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRTrackedCamera_002 : public XRTVRTrackedCamera_004, public vr::IVRTrackedCamera_002
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRTrackedCamera_002(XRTVRClientCore_003 *core) : XRTVRTrackedCamera_004(core) {};
	// clang-format on

	FnTable_IVRTrackedCamera_002();

	ForwardDeclareBase_IVRTrackedCamera(XRTVRTrackedCamera_004);

	Forward_IVRTrackedCamera_GetCameraErrorNameFromEnum();
	Forward_IVRTrackedCamera_HasCamera_006();
	Forward_IVRTrackedCamera_GetCameraFrameSize();

	vr::EVRTrackedCameraError
	GetCameraIntrinisics(vr::TrackedDeviceIndex_t nDeviceIndex,
	                     vr::EVRTrackedCameraFrameType eFrameType,
	                     vr::HmdVector2_t *pFocalLength,
	                     vr::HmdVector2_t *pCenter) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetCameraIntrinisics(nDeviceIndex=%llu, eFrameType=%d, pFocalLength=%p, pCenter=%p)", {},
		    static_cast<unsigned long long>(nDeviceIndex), static_cast<int>(eFrameType),
		    static_cast<const void *>(pFocalLength), static_cast<const void *>(pCenter));
	}

	Forward_IVRTrackedCamera_GetCameraProjection_004();
	Forward_IVRTrackedCamera_AcquireVideoStreamingService();
	Forward_IVRTrackedCamera_ReleaseVideoStreamingService();
	Forward_IVRTrackedCamera_GetVideoStreamFrameBuffer();
};

}; // namespace xrt::state_trackers::openvr
