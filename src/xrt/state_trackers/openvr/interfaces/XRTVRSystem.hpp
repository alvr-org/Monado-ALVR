// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRSystem interface version.
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

class XRTVRSystem_026 : public vr::IVRSystem_026
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRSystem_026(XRTVRClientCore_003 *core);

	virtual ~XRTVRSystem_026() = default;

	FnTable_IVRSystem_026();

	/* Display Methods */

	void
	GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight) override;

	vr::HmdMatrix44_t
	GetProjectionMatrix(vr::EVREye eEye, float fNearZ, float fFarZ) override;

	void
	GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom) override;

	bool
	ComputeDistortion(vr::EVREye eEye,
	                  float fU,
	                  float fV,
	                  vr::DistortionCoordinates_t *pDistortionCoordinates) override;

	bool
	ComputeDistortionSet(vr::EVREye eEye,
	                     vr::EVRDistortionChannel eChannel,
	                     bool bAsNormalizedDeviceCoordinates,
	                     uint32_t nNumCoordinates,
	                     const vr::DistortionCoordinate_t *pInput,
	                     vr::DistortionCoordinate_t *pOutput) override;

	vr::HmdMatrix34_t
	GetEyeToHeadTransform(vr::EVREye eEye) override;

	bool
	GetTimeSinceLastVsync(float *pfSecondsSinceLastVsync, uint64_t *pulFrameCounter) override;

	int32_t
	GetD3D9AdapterIndex() override;

	void
	GetDXGIOutputInfo(int32_t *pnAdapterIndex) override;

	void
	GetOutputDevice(uint64_t *pnDevice, vr::ETextureType textureType, VkInstance_T *pInstance = nullptr) override;

	/* Display Mode methods */

	bool
	IsDisplayOnDesktop() override;

	bool
	SetDisplayVisibility(bool bIsVisibleOnDesktop) override;

	/* Tracking Methods */

	void
	GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin eOrigin,
	                                float fPredictedSecondsToPhotonsFromNow,
	                                VR_ARRAY_COUNT(unTrackedDevicePoseArrayCount)
	                                    vr::TrackedDevicePose_t *pTrackedDevicePoseArray,
	                                uint32_t unTrackedDevicePoseArrayCount) override;

	vr::HmdMatrix34_t
	GetSeatedZeroPoseToStandingAbsoluteTrackingPose() override;

	vr::HmdMatrix34_t
	GetRawZeroPoseToStandingAbsoluteTrackingPose() override;

	uint32_t
	GetSortedTrackedDeviceIndicesOfClass(
	    vr::ETrackedDeviceClass eTrackedDeviceClass,
	    VR_ARRAY_COUNT(unTrackedDeviceIndexArrayCount) vr::TrackedDeviceIndex_t *punTrackedDeviceIndexArray,
	    uint32_t unTrackedDeviceIndexArrayCount,
	    vr::TrackedDeviceIndex_t unRelativeToTrackedDeviceIndex = vr::k_unTrackedDeviceIndex_Hmd) override;

	vr::EDeviceActivityLevel
	GetTrackedDeviceActivityLevel(vr::TrackedDeviceIndex_t unDeviceId) override;

	void
	ApplyTransform(vr::TrackedDevicePose_t *pOutputPose,
	               const vr::TrackedDevicePose_t *pTrackedDevicePose,
	               const vr::HmdMatrix34_t *pTransform) override;

	vr::TrackedDeviceIndex_t
	GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole unDeviceType) override;

	vr::ETrackedControllerRole
	GetControllerRoleForTrackedDeviceIndex(vr::TrackedDeviceIndex_t unDeviceIndex) override;

	/* Property methods */

	vr::ETrackedDeviceClass
	GetTrackedDeviceClass(vr::TrackedDeviceIndex_t unDeviceIndex) override;

	bool
	IsTrackedDeviceConnected(vr::TrackedDeviceIndex_t unDeviceIndex) override;

	bool
	GetBoolTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
	                             vr::ETrackedDeviceProperty prop,
	                             vr::ETrackedPropertyError *pError = 0L) override;

	float
	GetFloatTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
	                              vr::ETrackedDeviceProperty prop,
	                              vr::ETrackedPropertyError *pError = 0L) override;

	int32_t
	GetInt32TrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
	                              vr::ETrackedDeviceProperty prop,
	                              vr::ETrackedPropertyError *pError = 0L) override;

	uint64_t
	GetUint64TrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
	                               vr::ETrackedDeviceProperty prop,
	                               vr::ETrackedPropertyError *pError = 0L) override;

	vr::HmdMatrix34_t
	GetMatrix34TrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
	                                 vr::ETrackedDeviceProperty prop,
	                                 vr::ETrackedPropertyError *pError = 0L) override;

	uint32_t
	GetArrayTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
	                              vr::ETrackedDeviceProperty prop,
	                              vr::PropertyTypeTag_t propType,
	                              void *pBuffer,
	                              uint32_t unBufferSize,
	                              vr::ETrackedPropertyError *pError = 0L) override;

	uint32_t
	GetStringTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
	                               vr::ETrackedDeviceProperty prop,
	                               VR_OUT_STRING() char *pchValue,
	                               uint32_t unBufferSize,
	                               vr::ETrackedPropertyError *pError = 0L) override;

	const char *
	GetPropErrorNameFromEnum(vr::ETrackedPropertyError error) override;

	/* Event methods */

	bool
	PollNextEvent(vr::VREvent_t *pEvent, uint32_t uncbVREvent) override;

	bool
	PollNextEventWithPose(vr::ETrackingUniverseOrigin eOrigin,
	                      vr::VREvent_t *pEvent,
	                      uint32_t uncbVREvent,
	                      vr::TrackedDevicePose_t *pTrackedDevicePose) override;

	bool
	PollNextEventWithPoseAndOverlays(vr::ETrackingUniverseOrigin eOrigin,
	                                 vr::VREvent_t *pEvent,
	                                 uint32_t uncbVREvent,
	                                 vr::TrackedDevicePose_t *pTrackedDevicePose,
	                                 vr::VROverlayHandle_t *pulOverlayHandle) override;

	const char *
	GetEventTypeNameFromEnum(vr::EVREventType eType) override;

	/* Rendering helper methods */

	vr::HiddenAreaMesh_t
	GetHiddenAreaMesh(vr::EVREye eEye, vr::EHiddenAreaMeshType type = vr::k_eHiddenAreaMesh_Standard) override;

	bool
	GetEyeTrackedFoveationCenter(vr::HmdVector2_t *pNdcLeft, vr::HmdVector2_t *pNdcRight) override;

	bool
	GetEyeTrackedFoveationCenterForProjection(const vr::HmdMatrix44_t *pProjMat, vr::HmdVector2_t *pNdc) override;

	/* Controller methods */

	bool
	GetControllerState(vr::TrackedDeviceIndex_t unControllerDeviceIndex,
	                   vr::VRControllerState_t *pControllerState,
	                   uint32_t unControllerStateSize) override;

	bool
	GetControllerStateWithPose(vr::ETrackingUniverseOrigin eOrigin,
	                           vr::TrackedDeviceIndex_t unControllerDeviceIndex,
	                           vr::VRControllerState_t *pControllerState,
	                           uint32_t unControllerStateSize,
	                           vr::TrackedDevicePose_t *pTrackedDevicePose) override;

	void
	TriggerHapticPulse(vr::TrackedDeviceIndex_t unControllerDeviceIndex,
	                   uint32_t unAxisId,
	                   unsigned short usDurationMicroSec) override;

	const char *
	GetButtonIdNameFromEnum(vr::EVRButtonId eButtonId) override;

	const char *
	GetControllerAxisTypeNameFromEnum(vr::EVRControllerAxisType eAxisType) override;

	bool
	IsInputAvailable() override;

	bool
	IsSteamVRDrawingControllers() override;

	bool
	ShouldApplicationPause() override;

	bool
	ShouldApplicationReduceRenderingWork() override;

	/* Firmware methods */

	vr::EVRFirmwareError
	PerformFirmwareUpdate(vr::TrackedDeviceIndex_t unDeviceIndex) override;

	/* Application life cycle methods */

	void
	AcknowledgeQuit_Exiting() override;

	/* App container sandbox methods */

	uint32_t
	GetAppContainerFilePaths(VR_OUT_STRING() char *pchBuffer, uint32_t unBufferSize) override;

	/* System methods */

	const char *
	GetRuntimeVersion() override;

	vr::EVRInitError
	SetSDKVersion(uint32_t nVersionMajor, uint32_t nVersionMinor, uint32_t nVersionBuild) override;
};

}; // namespace xrt::state_trackers::openvr
