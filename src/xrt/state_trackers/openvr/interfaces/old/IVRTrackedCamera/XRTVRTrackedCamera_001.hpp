// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRTrackedCamera interface version 001.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRTrackedCamera.hpp"
#include "XRTVRTrackedCamera_002.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRTrackedCamera_001 : public XRTVRTrackedCamera_002, public vr::IVRTrackedCamera_001
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRTrackedCamera_001(XRTVRClientCore_003 *core) : XRTVRTrackedCamera_002(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRTrackedCamera_001.
		return nullptr;
	}

	ForwardDeclareBase_IVRTrackedCamera(XRTVRTrackedCamera_002);

	bool
	HasCamera(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "HasCamera(nDeviceIndex=%llu)", false,
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	bool
	GetCameraFirmwareDescription(vr::TrackedDeviceIndex_t nDeviceIndex, char *pBuffer, uint32_t nBufferLen) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetCameraFirmwareDescription(nDeviceIndex=%llu, pBuffer=%p, nBufferLen=%llu)", false,
		    static_cast<unsigned long long>(nDeviceIndex), static_cast<const void *>(pBuffer),
		    static_cast<unsigned long long>(nBufferLen));
	}

	bool
	GetCameraFrameDimensions(vr::TrackedDeviceIndex_t nDeviceIndex,
	                         vr::ECameraVideoStreamFormat nVideoStreamFormat,
	                         uint32_t *pWidth,
	                         uint32_t *pHeight) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetCameraFrameDimensions(nDeviceIndex=%llu, nVideoStreamFormat=?, pWidth=%p, pHeight=%p)",
		    false, static_cast<unsigned long long>(nDeviceIndex), static_cast<const void *>(pWidth),
		    static_cast<const void *>(pHeight));
	}

	bool
	SetCameraVideoStreamFormat(vr::TrackedDeviceIndex_t nDeviceIndex,
	                           vr::ECameraVideoStreamFormat nVideoStreamFormat) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "SetCameraVideoStreamFormat(nDeviceIndex=%llu, nVideoStreamFormat=?)",
		                             false, static_cast<unsigned long long>(nDeviceIndex));
	}

	vr::ECameraVideoStreamFormat
	GetCameraVideoStreamFormat(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCameraVideoStreamFormat(nDeviceIndex=%llu)", {},
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	bool
	EnableCameraForStreaming(vr::TrackedDeviceIndex_t nDeviceIndex, bool bEnable) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "EnableCameraForStreaming(nDeviceIndex=%llu, bEnable=%d)", false,
		                             static_cast<unsigned long long>(nDeviceIndex), static_cast<int>(bEnable));
	}

	bool
	StartVideoStream(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "StartVideoStream(nDeviceIndex=%llu)", false,
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	bool
	StopVideoStream(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "StopVideoStream(nDeviceIndex=%llu)", false,
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	bool
	IsVideoStreamActive(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsVideoStreamActive(nDeviceIndex=%llu)", false,
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	float
	GetVideoStreamElapsedTime(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetVideoStreamElapsedTime(nDeviceIndex=%llu)", {},
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	const vr::CameraVideoStreamFrame_t *
	GetVideoStreamFrame(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetVideoStreamFrame(nDeviceIndex=%llu)", {},
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	bool
	ReleaseVideoStreamFrame(vr::TrackedDeviceIndex_t nDeviceIndex,
	                        const vr::CameraVideoStreamFrame_t *pFrameImage) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ReleaseVideoStreamFrame(nDeviceIndex=%llu, pFrameImage=%p)",
		                             false, static_cast<unsigned long long>(nDeviceIndex),
		                             static_cast<const void *>(pFrameImage));
	}

	bool
	SetAutoExposure(vr::TrackedDeviceIndex_t nDeviceIndex, bool bEnable) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetAutoExposure(nDeviceIndex=%llu, bEnable=%d)", false,
		                             static_cast<unsigned long long>(nDeviceIndex), static_cast<int>(bEnable));
	}

	bool
	PauseVideoStream(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "PauseVideoStream(nDeviceIndex=%llu)", false,
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	bool
	ResumeVideoStream(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ResumeVideoStream(nDeviceIndex=%llu)", false,
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	bool
	IsVideoStreamPaused(vr::TrackedDeviceIndex_t nDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsVideoStreamPaused(nDeviceIndex=%llu)", false,
		                             static_cast<unsigned long long>(nDeviceIndex));
	}

	bool
	GetCameraDistortion(vr::TrackedDeviceIndex_t nDeviceIndex,
	                    float flInputU,
	                    float flInputV,
	                    float *pflOutputU,
	                    float *pflOutputV) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetCameraDistortion(nDeviceIndex=%llu, flInputU=%f, flInputV=%f, pflOutputU=%p, pflOutputV=%p)",
		    false, static_cast<unsigned long long>(nDeviceIndex), flInputU, flInputV,
		    static_cast<const void *>(pflOutputU), static_cast<const void *>(pflOutputV));
	}

	bool
	GetCameraProjection(vr::TrackedDeviceIndex_t nDeviceIndex,
	                    float flWidthPixels,
	                    float flHeightPixels,
	                    float flZNear,
	                    float flZFar,
	                    vr::HmdMatrix44_t *pProjection) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetCameraProjection(nDeviceIndex=%llu, flWidthPixels=%f, "
		                             "flHeightPixels=%f, flZNear=%f, flZFar=%f, pProjection=%p)",
		                             false, static_cast<unsigned long long>(nDeviceIndex), flWidthPixels,
		                             flHeightPixels, flZNear, flZFar, static_cast<const void *>(pProjection));
	}
};

}; // namespace xrt::state_trackers::openvr
