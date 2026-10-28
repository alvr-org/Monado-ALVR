// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRCompositor interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "XRTVRClientCore.hpp"

#include <array>


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRCompositor_029 : public vr::IVRCompositor_029
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRCompositor_029(XRTVRClientCore_003 *core);

	virtual ~XRTVRCompositor_029() = default;

	FnTable_IVRCompositor_029();

	void
	SetTrackingSpace(vr::ETrackingUniverseOrigin eOrigin) override;

	vr::ETrackingUniverseOrigin
	GetTrackingSpace() override;

	vr::EVRCompositorError
	WaitGetPoses(VR_ARRAY_COUNT(unRenderPoseArrayCount) vr::TrackedDevicePose_t *pRenderPoseArray,
	             uint32_t unRenderPoseArrayCount,
	             VR_ARRAY_COUNT(unGamePoseArrayCount) vr::TrackedDevicePose_t *pGamePoseArray,
	             uint32_t unGamePoseArrayCount) override;

	vr::EVRCompositorError
	GetLastPoses(VR_ARRAY_COUNT(unRenderPoseArrayCount) vr::TrackedDevicePose_t *pRenderPoseArray,
	             uint32_t unRenderPoseArrayCount,
	             VR_ARRAY_COUNT(unGamePoseArrayCount) vr::TrackedDevicePose_t *pGamePoseArray,
	             uint32_t unGamePoseArrayCount) override;

	vr::EVRCompositorError
	GetLastPoseForTrackedDeviceIndex(vr::TrackedDeviceIndex_t unDeviceIndex,
	                                 vr::TrackedDevicePose_t *pOutputPose,
	                                 vr::TrackedDevicePose_t *pOutputGamePose) override;

	vr::EVRCompositorError
	GetSubmitTexture(vr::Texture_t *pOutTexture,
	                 bool *pNeedsFlush,
	                 vr::EVRCompositorTextureUsage eUsage,
	                 const vr::Texture_t *pTexture,
	                 const vr::VRTextureBounds_t *pBounds = 0,
	                 vr::EVRSubmitFlags nSubmitFlags = vr::Submit_Default) override;

	vr::EVRCompositorError
	Submit(vr::EVREye eEye,
	       const vr::Texture_t *pTexture,
	       const vr::VRTextureBounds_t *pBounds = 0,
	       vr::EVRSubmitFlags nSubmitFlags = vr::Submit_Default) override;

	vr::EVRCompositorError
	SubmitWithArrayIndex(vr::EVREye eEye,
	                     const vr::Texture_t *pTexture,
	                     uint32_t unTextureArrayIndex,
	                     const vr::VRTextureBounds_t *pBounds = 0,
	                     vr::EVRSubmitFlags nSubmitFlags = vr::Submit_Default) override;

	void
	ClearLastSubmittedFrame() override;

	void
	PostPresentHandoff() override;

	bool
	GetFrameTiming(vr::Compositor_FrameTiming *pTiming, uint32_t unFramesAgo = 0) override;

	uint32_t
	GetFrameTimings(VR_ARRAY_COUNT(nFrames) vr::Compositor_FrameTiming *pTiming, uint32_t nFrames) override;

	float
	GetFrameTimeRemaining() override;

	void
	GetCumulativeStats(vr::Compositor_CumulativeStats *pStats, uint32_t nStatsSizeInBytes) override;

	void
	FadeToColor(
	    float fSeconds, float fRed, float fGreen, float fBlue, float fAlpha, bool bBackground = false) override;

	vr::HmdColor_t
	GetCurrentFadeColor(bool bBackground = false) override;

	void
	FadeGrid(float fSeconds, bool bFadeGridIn) override;

	float
	GetCurrentGridAlpha() override;

	vr::EVRCompositorError
	SetSkyboxOverride(VR_ARRAY_COUNT(unTextureCount) const vr::Texture_t *pTextures,
	                  uint32_t unTextureCount) override;

	void
	ClearSkyboxOverride() override;

	void
	CompositorBringToFront() override;

	void
	CompositorGoToBack() override;

	void
	CompositorQuit() override;

	bool
	IsFullscreen() override;

	uint32_t
	GetCurrentSceneFocusProcess() override;

	uint32_t
	GetLastFrameRenderer() override;

	bool
	CanRenderScene() override;

	void
	ShowMirrorWindow() override;

	void
	HideMirrorWindow() override;

	bool
	IsMirrorWindowVisible() override;

	void
	CompositorDumpImages() override;

	bool
	ShouldAppRenderWithLowResources() override;

	void
	ForceInterleavedReprojectionOn(bool bOverride) override;

	void
	ForceReconnectProcess() override;

	void
	SuspendRendering(bool bSuspend) override;

	vr::EVRCompositorError
	GetMirrorTextureD3D11(vr::EVREye eEye, void *pD3D11DeviceOrResource, void **ppD3D11ShaderResourceView) override;
	void
	ReleaseMirrorTextureD3D11(void *pD3D11ShaderResourceView) override;

	/** Access to mirror textures from OpenGL. */
	vr::EVRCompositorError
	GetMirrorTextureGL(vr::EVREye eEye,
	                   vr::glUInt_t *pglTextureId,
	                   vr::glSharedTextureHandle_t *pglSharedTextureHandle) override;
	bool
	ReleaseSharedGLTexture(vr::glUInt_t glTextureId, vr::glSharedTextureHandle_t glSharedTextureHandle) override;
	void
	LockGLSharedTextureForAccess(vr::glSharedTextureHandle_t glSharedTextureHandle) override;
	void
	UnlockGLSharedTextureForAccess(vr::glSharedTextureHandle_t glSharedTextureHandle) override;

	uint32_t
	GetVulkanInstanceExtensionsRequired(VR_OUT_STRING() char *pchValue, uint32_t unBufferSize) override;

	uint32_t
	GetVulkanDeviceExtensionsRequired(VkPhysicalDevice_T *pPhysicalDevice,
	                                  VR_OUT_STRING() char *pchValue,
	                                  uint32_t unBufferSize) override;

	void
	SetExplicitTimingMode(vr::EVRCompositorTimingMode eTimingMode) override;

	vr::EVRCompositorError
	SubmitExplicitTimingData() override;

	bool
	IsMotionSmoothingEnabled() override;

	bool
	IsMotionSmoothingSupported() override;

	bool
	IsCurrentSceneFocusAppLoading() override;

	vr::EVRCompositorError
	SetStageOverride_Async(const char *pchRenderModelPath,
	                       const vr::HmdMatrix34_t *pTransform = 0,
	                       const vr::Compositor_StageRenderSettings *pRenderSettings = 0,
	                       uint32_t nSizeOfRenderSettings = 0) override;

	void
	ClearStageOverride() override;

	bool
	GetCompositorBenchmarkResults(vr::Compositor_BenchmarkResults *pBenchmarkResults,
	                              uint32_t nSizeOfBenchmarkResults) override;

	vr::EVRCompositorError
	GetLastPosePredictionIDs(uint32_t *pRenderPosePredictionID, uint32_t *pGamePosePredictionID) override;

	vr::EVRCompositorError
	GetPosesForFrame(uint32_t unPosePredictionID,
	                 VR_ARRAY_COUNT(unPoseArrayCount) vr::TrackedDevicePose_t *pPoseArray,
	                 uint32_t unPoseArrayCount) override;
};

} // namespace xrt::state_trackers::openvr
