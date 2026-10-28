// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRTrackedCamera_006 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRTrackedCamera.hpp"


namespace xrt::state_trackers::openvr {

XRTVRTrackedCamera_006::XRTVRTrackedCamera_006(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

const char *
XRTVRTrackedCamera_006::GetCameraErrorNameFromEnum(vr::EVRTrackedCameraError eCameraError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCameraErrorNameFromEnum(eCameraError=%llu) -> %s",
	                             "TODO: GetCameraErrorNameFromEnum", static_cast<unsigned long long>(eCameraError),
	                             "TODO: GetCameraErrorNameFromEnum");
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::HasCamera(vr::TrackedDeviceIndex_t nDeviceIndex, bool *pHasCamera)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "HasCamera(nDeviceIndex=%llu, pHasCamera=%p) -> %llu",
	                             static_cast<vr::EVRTrackedCameraError>(0),
	                             static_cast<unsigned long long>(nDeviceIndex), static_cast<void *>(pHasCamera),
	                             static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::GetCameraFrameSize(vr::TrackedDeviceIndex_t nDeviceIndex,
                                           vr::EVRTrackedCameraFrameType eFrameType,
                                           uint32_t *pnWidth,
                                           uint32_t *pnHeight,
                                           uint32_t *pnFrameBufferSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetCameraFrameSize(nDeviceIndex=%llu, eFrameType=%llu, pnWidth=%p, pnHeight=%p, "
	                             "pnFrameBufferSize=%p) -> %llu",
	                             static_cast<vr::EVRTrackedCameraError>(0),
	                             static_cast<unsigned long long>(nDeviceIndex),
	                             static_cast<unsigned long long>(eFrameType), static_cast<void *>(pnWidth),
	                             static_cast<void *>(pnHeight), static_cast<void *>(pnFrameBufferSize),
	                             static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::GetCameraIntrinsics(vr::TrackedDeviceIndex_t nDeviceIndex,
                                            uint32_t nCameraIndex,
                                            vr::EVRTrackedCameraFrameType eFrameType,
                                            vr::HmdVector2_t *pFocalLength,
                                            vr::HmdVector2_t *pCenter)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetCameraIntrinsics(nDeviceIndex=%llu, nCameraIndex=%llu, eFrameType=%llu, pFocalLength=%p, pCenter=%p) "
	    "-> %llu",
	    static_cast<vr::EVRTrackedCameraError>(0), static_cast<unsigned long long>(nDeviceIndex),
	    static_cast<unsigned long long>(nCameraIndex), static_cast<unsigned long long>(eFrameType),
	    static_cast<void *>(pFocalLength), static_cast<void *>(pCenter),
	    static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::GetCameraProjection(vr::TrackedDeviceIndex_t nDeviceIndex,
                                            uint32_t nCameraIndex,
                                            vr::EVRTrackedCameraFrameType eFrameType,
                                            float flZNear,
                                            float flZFar,
                                            vr::HmdMatrix44_t *pProjection)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetCameraProjection(nDeviceIndex=%llu, nCameraIndex=%llu, eFrameType=%llu, flZNear=%f, flZFar=%f, "
	    "pProjection=%p) -> %llu",
	    static_cast<vr::EVRTrackedCameraError>(0), static_cast<unsigned long long>(nDeviceIndex),
	    static_cast<unsigned long long>(nCameraIndex), static_cast<unsigned long long>(eFrameType),
	    static_cast<double>(flZNear), static_cast<double>(flZFar), static_cast<void *>(pProjection),
	    static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::AcquireVideoStreamingService(vr::TrackedDeviceIndex_t nDeviceIndex,
                                                     vr::TrackedCameraHandle_t *pHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "AcquireVideoStreamingService(nDeviceIndex=%llu, pHandle=%p) -> %llu",
	                             static_cast<vr::EVRTrackedCameraError>(0),
	                             static_cast<unsigned long long>(nDeviceIndex), static_cast<void *>(pHandle),
	                             static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::ReleaseVideoStreamingService(vr::TrackedCameraHandle_t hTrackedCamera)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ReleaseVideoStreamingService(hTrackedCamera=%llu) -> %llu",
	                             static_cast<vr::EVRTrackedCameraError>(0),
	                             static_cast<unsigned long long>(hTrackedCamera),
	                             static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::GetVideoStreamFrameBuffer(vr::TrackedCameraHandle_t hTrackedCamera,
                                                  vr::EVRTrackedCameraFrameType eFrameType,
                                                  void *pFrameBuffer,
                                                  uint32_t nFrameBufferSize,
                                                  vr::CameraVideoStreamFrameHeader_t *pFrameHeader,
                                                  uint32_t nFrameHeaderSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetVideoStreamFrameBuffer(hTrackedCamera=%llu, eFrameType=%llu, pFrameBuffer=%p, nFrameBufferSize=%llu, "
	    "pFrameHeader=%p, nFrameHeaderSize=%llu) -> %llu",
	    static_cast<vr::EVRTrackedCameraError>(0), static_cast<unsigned long long>(hTrackedCamera),
	    static_cast<unsigned long long>(eFrameType), static_cast<void *>(pFrameBuffer),
	    static_cast<unsigned long long>(nFrameBufferSize), static_cast<void *>(pFrameHeader),
	    static_cast<unsigned long long>(nFrameHeaderSize),
	    static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::GetVideoStreamTextureSize(vr::TrackedDeviceIndex_t nDeviceIndex,
                                                  vr::EVRTrackedCameraFrameType eFrameType,
                                                  vr::VRTextureBounds_t *pTextureBounds,
                                                  uint32_t *pnWidth,
                                                  uint32_t *pnHeight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetVideoStreamTextureSize(nDeviceIndex=%llu, eFrameType=%llu, pTextureBounds=%p, "
	                             "pnWidth=%p, pnHeight=%p) -> %llu",
	                             static_cast<vr::EVRTrackedCameraError>(0),
	                             static_cast<unsigned long long>(nDeviceIndex),
	                             static_cast<unsigned long long>(eFrameType), static_cast<void *>(pTextureBounds),
	                             static_cast<void *>(pnWidth), static_cast<void *>(pnHeight),
	                             static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::GetVideoStreamTextureD3D11(vr::TrackedCameraHandle_t hTrackedCamera,
                                                   vr::EVRTrackedCameraFrameType eFrameType,
                                                   void *pD3D11DeviceOrResource,
                                                   void **ppD3D11ShaderResourceView,
                                                   vr::CameraVideoStreamFrameHeader_t *pFrameHeader,
                                                   uint32_t nFrameHeaderSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetVideoStreamTextureD3D11(hTrackedCamera=%llu, eFrameType=%llu, pD3D11DeviceOrResource=%p, "
	    "ppD3D11ShaderResourceView=%p, pFrameHeader=%p, nFrameHeaderSize=%llu) -> %llu",
	    static_cast<vr::EVRTrackedCameraError>(0), static_cast<unsigned long long>(hTrackedCamera),
	    static_cast<unsigned long long>(eFrameType), static_cast<void *>(pD3D11DeviceOrResource),
	    static_cast<void *>(ppD3D11ShaderResourceView), static_cast<void *>(pFrameHeader),
	    static_cast<unsigned long long>(nFrameHeaderSize),
	    static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::GetVideoStreamTextureGL(vr::TrackedCameraHandle_t hTrackedCamera,
                                                vr::EVRTrackedCameraFrameType eFrameType,
                                                vr::glUInt_t *pglTextureId,
                                                vr::CameraVideoStreamFrameHeader_t *pFrameHeader,
                                                uint32_t nFrameHeaderSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetVideoStreamTextureGL(hTrackedCamera=%llu, eFrameType=%llu, pglTextureId=%p, pFrameHeader=%p, "
	    "nFrameHeaderSize=%llu) -> %llu",
	    static_cast<vr::EVRTrackedCameraError>(0), static_cast<unsigned long long>(hTrackedCamera),
	    static_cast<unsigned long long>(eFrameType), static_cast<void *>(pglTextureId),
	    static_cast<void *>(pFrameHeader), static_cast<unsigned long long>(nFrameHeaderSize),
	    static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

vr::EVRTrackedCameraError
XRTVRTrackedCamera_006::ReleaseVideoStreamTextureGL(vr::TrackedCameraHandle_t hTrackedCamera, vr::glUInt_t glTextureId)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "ReleaseVideoStreamTextureGL(hTrackedCamera=%llu, glTextureId=%llu) -> %llu",
	    static_cast<vr::EVRTrackedCameraError>(0), static_cast<unsigned long long>(hTrackedCamera),
	    static_cast<unsigned long long>(glTextureId),
	    static_cast<unsigned long long>(static_cast<vr::EVRTrackedCameraError>(0)));
}

void
XRTVRTrackedCamera_006::SetCameraTrackingSpace(vr::ETrackingUniverseOrigin eUniverse)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetCameraTrackingSpace(eUniverse=%llu)",
	                         static_cast<unsigned long long>(eUniverse));
}

vr::ETrackingUniverseOrigin
XRTVRTrackedCamera_006::GetCameraTrackingSpace()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCameraTrackingSpace() -> %llu",
	                             static_cast<vr::ETrackingUniverseOrigin>(0),
	                             static_cast<unsigned long long>(static_cast<vr::ETrackingUniverseOrigin>(0)));
}

}; // namespace xrt::state_trackers::openvr
