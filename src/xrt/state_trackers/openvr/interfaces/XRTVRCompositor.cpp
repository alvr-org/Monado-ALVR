// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRCompositor interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "xrt/xrt_gfx_vk.h"

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRCompositor.hpp"

#include <cstring>


namespace xrt::state_trackers::openvr {

XRTVRCompositor_029::XRTVRCompositor_029(XRTVRClientCore_003 *core) : core(core)
{
	// Nothing yet
}

void
XRTVRCompositor_029::SetTrackingSpace(vr::ETrackingUniverseOrigin eOrigin)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	this->core->compositor->current_tracking_universe = eOrigin;
	OPENVR_LOG_TRACE(logger, "SetTrackingSpace: tracking universe set to %d", static_cast<int>(eOrigin));
}

vr::ETrackingUniverseOrigin
XRTVRCompositor_029::GetTrackingSpace()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_TRACE(logger, "GetTrackingSpace: returning current tracking universe %d",
	                 static_cast<int>(this->core->compositor->current_tracking_universe));

	return this->core->compositor->current_tracking_universe;
}

vr::EVRCompositorError
XRTVRCompositor_029::WaitGetPoses(VR_ARRAY_COUNT(unRenderPoseArrayCount) vr::TrackedDevicePose_t *pRenderPoseArray,
                                  uint32_t unRenderPoseArrayCount,
                                  VR_ARRAY_COUNT(unGamePoseArrayCount) vr::TrackedDevicePose_t *pGamePoseArray,
                                  uint32_t unGamePoseArrayCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	auto error = this->core->compositor->waitBeginFrame(logger);
	if (error != vr::EVRCompositorError::VRCompositorError_None) {
		OPENVR_LOG_ERROR(logger, "WaitGetPoses: waitBeginFrame failed with error code %d",
		                 static_cast<int>(error));
		return error;
	}

	timepoint_ns render_time_ns = this->core->compositor->getTimeForPredictions();
	timepoint_ns game_time_ns = render_time_ns + this->core->compositor->getFramePeriod();

	this->core->devices->trackDevices(logger, render_time_ns, this->core->compositor->current_tracking_universe,
	                                  this->core->compositor->last_render_poses.data(),
	                                  static_cast<uint32_t>(this->core->compositor->last_render_poses.size()));

	this->core->devices->trackDevices(logger, game_time_ns, this->core->compositor->current_tracking_universe,
	                                  this->core->compositor->last_game_poses.data(),
	                                  static_cast<uint32_t>(this->core->compositor->last_game_poses.size()));

	return this->GetLastPoses(pRenderPoseArray, unRenderPoseArrayCount, pGamePoseArray, unGamePoseArrayCount);
}

vr::EVRCompositorError
XRTVRCompositor_029::GetLastPoses(VR_ARRAY_COUNT(unRenderPoseArrayCount) vr::TrackedDevicePose_t *pRenderPoseArray,
                                  uint32_t unRenderPoseArrayCount,
                                  VR_ARRAY_COUNT(unGamePoseArrayCount) vr::TrackedDevicePose_t *pGamePoseArray,
                                  uint32_t unGamePoseArrayCount)
{
	// Copy all the render poses in
	std::memcpy(
	    pRenderPoseArray, this->core->compositor->last_render_poses.data(),
	    std::min(unRenderPoseArrayCount, static_cast<uint32_t>(this->core->compositor->last_render_poses.size())) *
	        sizeof(vr::TrackedDevicePose_t));
	// Copy all the game poses in
	std::memcpy(
	    pGamePoseArray, this->core->compositor->last_game_poses.data(),
	    std::min(unGamePoseArrayCount, static_cast<uint32_t>(this->core->compositor->last_game_poses.size())) *
	        sizeof(vr::TrackedDevicePose_t));

	return vr::EVRCompositorError::VRCompositorError_None;
}

vr::EVRCompositorError
XRTVRCompositor_029::GetLastPoseForTrackedDeviceIndex(vr::TrackedDeviceIndex_t unDeviceIndex,
                                                      vr::TrackedDevicePose_t *pOutputPose,
                                                      vr::TrackedDevicePose_t *pOutputGamePose)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetLastPoseForTrackedDeviceIndex(unDeviceIndex=%u, pOutputPose=%p, "
	                             "pOutputGamePose=%p) -> %d",
	                             vr::EVRCompositorError::VRCompositorError_RequestFailed,
	                             static_cast<unsigned int>(unDeviceIndex), static_cast<void *>(pOutputPose),
	                             static_cast<void *>(pOutputGamePose),
	                             static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

vr::EVRCompositorError
XRTVRCompositor_029::GetSubmitTexture(vr::Texture_t *pOutTexture,
                                      bool *pNeedsFlush,
                                      vr::EVRCompositorTextureUsage eUsage,
                                      const vr::Texture_t *pTexture,
                                      const vr::VRTextureBounds_t *pBounds,
                                      vr::EVRSubmitFlags nSubmitFlags)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	// @note: This appears to be an internal OpenVR function, the changelogs even say as such, and not even Proton
	//        supports this function, so we can likely avoid implementing it until either apps start using it anyway
	//        or the API stabilizes.

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetSubmitTexture(pOutTexture=%p, pNeedsFlush=%p, eUsage=%d, pTexture=%p, "
	                             "pBounds=%p, nSubmitFlags=%u) -> %d",
	                             vr::EVRCompositorError::VRCompositorError_RequestFailed,
	                             static_cast<void *>(pOutTexture), static_cast<void *>(pNeedsFlush),
	                             static_cast<int>(eUsage), static_cast<const void *>(pTexture),
	                             static_cast<const void *>(pBounds), static_cast<unsigned int>(nSubmitFlags),
	                             static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

vr::EVRCompositorError
XRTVRCompositor_029::Submit(vr::EVREye eEye,
                            const vr::Texture_t *pTexture,
                            const vr::VRTextureBounds_t *pBounds,
                            vr::EVRSubmitFlags nSubmitFlags)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	return this->SubmitWithArrayIndex(eEye, pTexture, 0, pBounds, nSubmitFlags);
}

vr::EVRCompositorError
XRTVRCompositor_029::SubmitWithArrayIndex(vr::EVREye eEye,
                                          const vr::Texture_t *pTexture,
                                          uint32_t unTextureArrayIndex,
                                          const vr::VRTextureBounds_t *pBounds,
                                          vr::EVRSubmitFlags nSubmitFlags)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (pTexture == nullptr) {
		return vr::EVRCompositorError::VRCompositorError_InvalidTexture;
	}

	vr::VRTextureBounds_t default_bounds = {.uMin = 0.0f, .vMin = 0.0f, .uMax = 1.0f, .vMax = 1.0f};
	if (pBounds == nullptr) {
		pBounds = &default_bounds;
	}

	return this->core->compositor->submit(logger,              //
	                                      eEye,                //
	                                      *pTexture,           //
	                                      unTextureArrayIndex, //
	                                      *pBounds,            //
	                                      nSubmitFlags);       //
}

void
XRTVRCompositor_029::ClearLastSubmittedFrame()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "ClearLastSubmittedFrame()");
}

void
XRTVRCompositor_029::PostPresentHandoff()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "PostPresentHandoff()");
}

bool
XRTVRCompositor_029::GetFrameTiming(vr::Compositor_FrameTiming *pTiming, uint32_t unFramesAgo)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetFrameTiming(pTiming=%p, unFramesAgo=%u) -> %d", false,
	                             static_cast<void *>(pTiming), static_cast<unsigned int>(unFramesAgo), 0);
}

uint32_t
XRTVRCompositor_029::GetFrameTimings(VR_ARRAY_COUNT(nFrames) vr::Compositor_FrameTiming *pTiming, uint32_t nFrames)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetFrameTimings(pTiming=%p, nFrames=%u) -> %u", 0U,
	                             static_cast<void *>(pTiming), static_cast<unsigned int>(nFrames), 0U);
}

float
XRTVRCompositor_029::GetFrameTimeRemaining()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetFrameTimeRemaining() -> %f", 0.0f, 0.0);
}

void
XRTVRCompositor_029::GetCumulativeStats(vr::Compositor_CumulativeStats *pStats, uint32_t nStatsSizeInBytes)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "GetCumulativeStats(pStats=%p, nStatsSizeInBytes=%u)",
	                         static_cast<void *>(pStats), static_cast<unsigned int>(nStatsSizeInBytes));
}

void
XRTVRCompositor_029::FadeToColor(float fSeconds, float fRed, float fGreen, float fBlue, float fAlpha, bool bBackground)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger,
	                         "FadeToColor(fSeconds=%f, fRed=%f, fGreen=%f, fBlue=%f, fAlpha=%f, bBackground=%d)",
	                         fSeconds, fRed, fGreen, fBlue, fAlpha, static_cast<int>(bBackground));
}

vr::HmdColor_t
XRTVRCompositor_029::GetCurrentFadeColor(bool bBackground)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	vr::HmdColor_t color{};

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCurrentFadeColor(bBackground=%d) -> %p", color,
	                             static_cast<int>(bBackground), static_cast<void *>(&color));
}

void
XRTVRCompositor_029::FadeGrid(float fSeconds, bool bFadeGridIn)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "FadeGrid(fSeconds=%f, bFadeGridIn=%d)", fSeconds,
	                         static_cast<int>(bFadeGridIn));
}

float
XRTVRCompositor_029::GetCurrentGridAlpha()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCurrentGridAlpha() -> %f", 0.0f, 0.0);
}

vr::EVRCompositorError
XRTVRCompositor_029::SetSkyboxOverride(VR_ARRAY_COUNT(unTextureCount) const vr::Texture_t *pTextures,
                                       uint32_t unTextureCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetSkyboxOverride(pTextures=%p, unTextureCount=%u) -> %d",
	                             vr::EVRCompositorError::VRCompositorError_RequestFailed,
	                             static_cast<const void *>(pTextures), static_cast<unsigned int>(unTextureCount),
	                             static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

void
XRTVRCompositor_029::ClearSkyboxOverride()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "ClearSkyboxOverride()");
}

void
XRTVRCompositor_029::CompositorBringToFront()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "CompositorBringToFront()");
}

void
XRTVRCompositor_029::CompositorGoToBack()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "CompositorGoToBack()");
}

void
XRTVRCompositor_029::CompositorQuit()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "CompositorQuit()");
}

bool
XRTVRCompositor_029::IsFullscreen()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsFullscreen() -> %d", false, 0);
}

uint32_t
XRTVRCompositor_029::GetCurrentSceneFocusProcess()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCurrentSceneFocusProcess() -> %u", 0U, 0U);
}

uint32_t
XRTVRCompositor_029::GetLastFrameRenderer()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetLastFrameRenderer() -> %u", 0U, 0U);
}

bool
XRTVRCompositor_029::CanRenderScene()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "CanRenderScene() -> %d", true, 1);
}

void
XRTVRCompositor_029::ShowMirrorWindow()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "ShowMirrorWindow()");
}

void
XRTVRCompositor_029::HideMirrorWindow()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "HideMirrorWindow()");
}

bool
XRTVRCompositor_029::IsMirrorWindowVisible()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsMirrorWindowVisible() -> %d", false, 0);
}

void
XRTVRCompositor_029::CompositorDumpImages()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "CompositorDumpImages()");
}

bool
XRTVRCompositor_029::ShouldAppRenderWithLowResources()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ShouldAppRenderWithLowResources() -> %d", false, 0);
}

void
XRTVRCompositor_029::ForceInterleavedReprojectionOn(bool bOverride)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "ForceInterleavedReprojectionOn(bOverride=%d)", static_cast<int>(bOverride));
}

void
XRTVRCompositor_029::ForceReconnectProcess()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "ForceReconnectProcess()");
}

void
XRTVRCompositor_029::SuspendRendering(bool bSuspend)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "SuspendRendering(bSuspend=%d)", static_cast<int>(bSuspend));
}

vr::EVRCompositorError
XRTVRCompositor_029::GetMirrorTextureD3D11(vr::EVREye eEye,
                                           void *pD3D11DeviceOrResource,
                                           void **ppD3D11ShaderResourceView)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetMirrorTextureD3D11(eEye=%d, pD3D11DeviceOrResource=%p, "
	                             "ppD3D11ShaderResourceView=%p) -> %d",
	                             vr::EVRCompositorError::VRCompositorError_RequestFailed, static_cast<int>(eEye),
	                             pD3D11DeviceOrResource, static_cast<void *>(ppD3D11ShaderResourceView),
	                             static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

void
XRTVRCompositor_029::ReleaseMirrorTextureD3D11(void *pD3D11ShaderResourceView)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "ReleaseMirrorTextureD3D11(pD3D11ShaderResourceView=%p)",
	                         pD3D11ShaderResourceView);
}

vr::EVRCompositorError
XRTVRCompositor_029::GetMirrorTextureGL(vr::EVREye eEye,
                                        vr::glUInt_t *pglTextureId,
                                        vr::glSharedTextureHandle_t *pglSharedTextureHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetMirrorTextureGL(eEye=%d, pglTextureId=%p, pglSharedTextureHandle=%p) -> %d",
	                             vr::EVRCompositorError::VRCompositorError_RequestFailed, static_cast<int>(eEye),
	                             static_cast<void *>(pglTextureId), static_cast<void *>(pglSharedTextureHandle),
	                             static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

bool
XRTVRCompositor_029::ReleaseSharedGLTexture(vr::glUInt_t glTextureId, vr::glSharedTextureHandle_t glSharedTextureHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ReleaseSharedGLTexture(glTextureId=%u, glSharedTextureHandle=%p) -> %d",
	                             false, static_cast<unsigned int>(glTextureId), glSharedTextureHandle, 0);
}

void
XRTVRCompositor_029::LockGLSharedTextureForAccess(vr::glSharedTextureHandle_t glSharedTextureHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "LockGLSharedTextureForAccess(glSharedTextureHandle=%p)",
	                         glSharedTextureHandle);
}

void
XRTVRCompositor_029::UnlockGLSharedTextureForAccess(vr::glSharedTextureHandle_t glSharedTextureHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "UnlockGLSharedTextureForAccess(glSharedTextureHandle=%p)",
	                         glSharedTextureHandle);
}

uint32_t
XRTVRCompositor_029::GetVulkanInstanceExtensionsRequired(VR_OUT_STRING() char *pchValue, uint32_t unBufferSize)
{
	const uint32_t neededSize = (uint32_t)(strlen(xrt_gfx_vk_instance_extensions) + 1);

	if (neededSize > unBufferSize) {
		return neededSize;
	}

	snprintf(pchValue, unBufferSize, "%s", xrt_gfx_vk_instance_extensions);
	return neededSize;
}

uint32_t
XRTVRCompositor_029::GetVulkanDeviceExtensionsRequired(VkPhysicalDevice_T *pPhysicalDevice,
                                                       VR_OUT_STRING() char *pchValue,
                                                       uint32_t unBufferSize)
{
	const uint32_t neededSize = (uint32_t)(strlen(xrt_gfx_vk_device_extensions) + 1);

	if (neededSize > unBufferSize) {
		return neededSize;
	}

	snprintf(pchValue, unBufferSize, "%s", xrt_gfx_vk_device_extensions);
	return neededSize;
}

void
XRTVRCompositor_029::SetExplicitTimingMode(vr::EVRCompositorTimingMode eTimingMode)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "SetExplicitTimingMode(eTimingMode=%d)", static_cast<int>(eTimingMode));
}

vr::EVRCompositorError
XRTVRCompositor_029::SubmitExplicitTimingData()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SubmitExplicitTimingData() -> %d",
	                             vr::EVRCompositorError::VRCompositorError_RequestFailed,
	                             static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

bool
XRTVRCompositor_029::IsMotionSmoothingEnabled()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsMotionSmoothingEnabled() -> %d", false, 0);
}

bool
XRTVRCompositor_029::IsMotionSmoothingSupported()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsMotionSmoothingSupported() -> %d", false, 0);
}

bool
XRTVRCompositor_029::IsCurrentSceneFocusAppLoading()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsCurrentSceneFocusAppLoading() -> %d", false, 0);
}

vr::EVRCompositorError
XRTVRCompositor_029::SetStageOverride_Async(const char *pchRenderModelPath,
                                            const vr::HmdMatrix34_t *pTransform,
                                            const vr::Compositor_StageRenderSettings *pRenderSettings,
                                            uint32_t nSizeOfRenderSettings)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetStageOverride_Async(pchRenderModelPath=%s, pTransform=%p, pRenderSettings=%p, "
	                             "nSizeOfRenderSettings=%u) -> %d",
	                             vr::EVRCompositorError::VRCompositorError_RequestFailed,
	                             pchRenderModelPath != nullptr ? pchRenderModelPath : "(null)",
	                             static_cast<const void *>(pTransform), static_cast<const void *>(pRenderSettings),
	                             static_cast<unsigned int>(nSizeOfRenderSettings),
	                             static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

void
XRTVRCompositor_029::ClearStageOverride()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "ClearStageOverride()");
}

bool
XRTVRCompositor_029::GetCompositorBenchmarkResults(vr::Compositor_BenchmarkResults *pBenchmarkResults,
                                                   uint32_t nSizeOfBenchmarkResults)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetCompositorBenchmarkResults(pBenchmarkResults=%p, nSizeOfBenchmarkResults=%u) -> %d", false,
	    static_cast<void *>(pBenchmarkResults), static_cast<unsigned int>(nSizeOfBenchmarkResults), 0);
}

vr::EVRCompositorError
XRTVRCompositor_029::GetLastPosePredictionIDs(uint32_t *pRenderPosePredictionID, uint32_t *pGamePosePredictionID)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetLastPosePredictionIDs(pRenderPosePredictionID=%p, pGamePosePredictionID=%p) -> %d",
	    vr::EVRCompositorError::VRCompositorError_RequestFailed, static_cast<void *>(pRenderPosePredictionID),
	    static_cast<void *>(pGamePosePredictionID),
	    static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

vr::EVRCompositorError
XRTVRCompositor_029::GetPosesForFrame(uint32_t unPosePredictionID,
                                      VR_ARRAY_COUNT(unPoseArrayCount) vr::TrackedDevicePose_t *pPoseArray,
                                      uint32_t unPoseArrayCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetPosesForFrame(unPosePredictionID=%u, pPoseArray=%p, unPoseArrayCount=%u) -> %d",
	    vr::EVRCompositorError::VRCompositorError_RequestFailed, static_cast<unsigned int>(unPosePredictionID),
	    static_cast<void *>(pPoseArray), static_cast<unsigned int>(unPoseArrayCount),
	    static_cast<int>(vr::EVRCompositorError::VRCompositorError_RequestFailed));
}

} // namespace xrt::state_trackers::openvr
