// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRTrackedCamera_006 interface version.
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

class XRTVRTrackedCamera_006 : public vr::IVRTrackedCamera_006
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRTrackedCamera_006(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRTrackedCamera_006() = default;

	FnTable_IVRTrackedCamera_006();

	const char *
	GetCameraErrorNameFromEnum(vr::EVRTrackedCameraError eCameraError) override;

	vr::EVRTrackedCameraError
	HasCamera(vr::TrackedDeviceIndex_t nDeviceIndex, bool *pHasCamera) override;

	vr::EVRTrackedCameraError
	GetCameraFrameSize(vr::TrackedDeviceIndex_t nDeviceIndex,
	                   vr::EVRTrackedCameraFrameType eFrameType,
	                   uint32_t *pnWidth,
	                   uint32_t *pnHeight,
	                   uint32_t *pnFrameBufferSize) override;

	vr::EVRTrackedCameraError
	GetCameraIntrinsics(vr::TrackedDeviceIndex_t nDeviceIndex,
	                    uint32_t nCameraIndex,
	                    vr::EVRTrackedCameraFrameType eFrameType,
	                    vr::HmdVector2_t *pFocalLength,
	                    vr::HmdVector2_t *pCenter) override;

	vr::EVRTrackedCameraError
	GetCameraProjection(vr::TrackedDeviceIndex_t nDeviceIndex,
	                    uint32_t nCameraIndex,
	                    vr::EVRTrackedCameraFrameType eFrameType,
	                    float flZNear,
	                    float flZFar,
	                    vr::HmdMatrix44_t *pProjection) override;

	vr::EVRTrackedCameraError
	AcquireVideoStreamingService(vr::TrackedDeviceIndex_t nDeviceIndex,
	                             vr::TrackedCameraHandle_t *pHandle) override;

	vr::EVRTrackedCameraError
	ReleaseVideoStreamingService(vr::TrackedCameraHandle_t hTrackedCamera) override;

	vr::EVRTrackedCameraError
	GetVideoStreamFrameBuffer(vr::TrackedCameraHandle_t hTrackedCamera,
	                          vr::EVRTrackedCameraFrameType eFrameType,
	                          void *pFrameBuffer,
	                          uint32_t nFrameBufferSize,
	                          vr::CameraVideoStreamFrameHeader_t *pFrameHeader,
	                          uint32_t nFrameHeaderSize) override;

	vr::EVRTrackedCameraError
	GetVideoStreamTextureSize(vr::TrackedDeviceIndex_t nDeviceIndex,
	                          vr::EVRTrackedCameraFrameType eFrameType,
	                          vr::VRTextureBounds_t *pTextureBounds,
	                          uint32_t *pnWidth,
	                          uint32_t *pnHeight) override;

	vr::EVRTrackedCameraError
	GetVideoStreamTextureD3D11(vr::TrackedCameraHandle_t hTrackedCamera,
	                           vr::EVRTrackedCameraFrameType eFrameType,
	                           void *pD3D11DeviceOrResource,
	                           void **ppD3D11ShaderResourceView,
	                           vr::CameraVideoStreamFrameHeader_t *pFrameHeader,
	                           uint32_t nFrameHeaderSize) override;

	vr::EVRTrackedCameraError
	GetVideoStreamTextureGL(vr::TrackedCameraHandle_t hTrackedCamera,
	                        vr::EVRTrackedCameraFrameType eFrameType,
	                        vr::glUInt_t *pglTextureId,
	                        vr::CameraVideoStreamFrameHeader_t *pFrameHeader,
	                        uint32_t nFrameHeaderSize) override;

	vr::EVRTrackedCameraError
	ReleaseVideoStreamTextureGL(vr::TrackedCameraHandle_t hTrackedCamera, vr::glUInt_t glTextureId) override;

	void
	SetCameraTrackingSpace(vr::ETrackingUniverseOrigin eUniverse) override;

	vr::ETrackingUniverseOrigin
	GetCameraTrackingSpace() override;
};

} // namespace xrt::state_trackers::openvr
