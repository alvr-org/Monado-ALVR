// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRTrackedCamera interface version 004.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRTrackedCamera.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRTrackedCamera_004 : public XRTVRTrackedCamera_006, public vr::IVRTrackedCamera_004
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRTrackedCamera_004(XRTVRClientCore_003 *core) : XRTVRTrackedCamera_006(core) {};
	// clang-format on

	FnTable_IVRTrackedCamera_004();

	ForwardDeclareBase_IVRTrackedCamera(XRTVRTrackedCamera_006);

	Forward_IVRTrackedCamera_GetCameraErrorNameFromEnum();
	Forward_IVRTrackedCamera_HasCamera_006();
	Forward_IVRTrackedCamera_GetCameraFrameSize();

	vr::EVRTrackedCameraError
	GetCameraIntrinsics(vr::TrackedDeviceIndex_t nDeviceIndex,
	                    vr::EVRTrackedCameraFrameType eFrameType,
	                    vr::HmdVector2_t *pFocalLength,
	                    vr::HmdVector2_t *pCenter) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetCameraIntrinsics(nDeviceIndex=%llu, eFrameType=%d, pFocalLength=%p, pCenter=%p)", {},
		    static_cast<unsigned long long>(nDeviceIndex), static_cast<int>(eFrameType),
		    static_cast<const void *>(pFocalLength), static_cast<const void *>(pCenter));
	}

	vr::EVRTrackedCameraError
	GetCameraProjection(vr::TrackedDeviceIndex_t nDeviceIndex,
	                    vr::EVRTrackedCameraFrameType eFrameType,
	                    float flZNear,
	                    float flZFar,
	                    vr::HmdMatrix44_t *pProjection) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetCameraProjection(nDeviceIndex=%llu, eFrameType=%d, flZNear=%f, flZFar=%f, pProjection=%p)", {},
		    static_cast<unsigned long long>(nDeviceIndex), static_cast<int>(eFrameType), flZNear, flZFar,
		    static_cast<const void *>(pProjection));
	}

	Forward_IVRTrackedCamera_AcquireVideoStreamingService();
	Forward_IVRTrackedCamera_ReleaseVideoStreamingService();
	Forward_IVRTrackedCamera_GetVideoStreamFrameBuffer();
	Forward_IVRTrackedCamera_GetVideoStreamTextureSize();
	Forward_IVRTrackedCamera_GetVideoStreamTextureD3D11();
	Forward_IVRTrackedCamera_GetVideoStreamTextureGL();
	Forward_IVRTrackedCamera_ReleaseVideoStreamTextureGL();
};

}; // namespace xrt::state_trackers::openvr
