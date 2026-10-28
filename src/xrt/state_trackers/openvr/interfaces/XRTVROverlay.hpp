// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVROverlay_028 interface version.
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

class XRTVROverlay_028 : public vr::IVROverlay_028
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVROverlay_028(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVROverlay_028() = default;

	FnTable_IVROverlay_028();

	EVROverlayError
	FindOverlay(const char *pchOverlayKey, VROverlayHandle_t *pOverlayHandle) override;

	EVROverlayError
	CreateOverlay(const char *pchOverlayKey,
	              const char *pchOverlayName,
	              VROverlayHandle_t *pOverlayHandle) override;

	EVROverlayError
	CreateSubviewOverlay(VROverlayHandle_t parentOverlayHandle,
	                     const char *pchSubviewOverlayKey,
	                     const char *pchSubviewOverlayName,
	                     VROverlayHandle_t *pSubviewOverlayHandle) override;

	EVROverlayError
	DestroyOverlay(VROverlayHandle_t ulOverlayHandle) override;

	uint32_t
	GetOverlayKey(VROverlayHandle_t ulOverlayHandle,
	              char *pchValue,
	              uint32_t unBufferSize,
	              EVROverlayError *pError) override;

	uint32_t
	GetOverlayName(VROverlayHandle_t ulOverlayHandle,
	               char *pchValue,
	               uint32_t unBufferSize,
	               EVROverlayError *pError) override;

	EVROverlayError
	SetOverlayName(VROverlayHandle_t ulOverlayHandle, const char *pchName) override;

	EVROverlayError
	GetOverlayImageData(VROverlayHandle_t ulOverlayHandle,
	                    void *pvBuffer,
	                    uint32_t unBufferSize,
	                    uint32_t *punWidth,
	                    uint32_t *punHeight) override;

	const char *
	GetOverlayErrorNameFromEnum(EVROverlayError error) override;

	EVROverlayError
	SetOverlayRenderingPid(VROverlayHandle_t ulOverlayHandle, uint32_t unPID) override;

	uint32_t
	GetOverlayRenderingPid(VROverlayHandle_t ulOverlayHandle) override;

	EVROverlayError
	SetOverlayFlag(VROverlayHandle_t ulOverlayHandle, VROverlayFlags eOverlayFlag, bool bEnabled) override;

	EVROverlayError
	GetOverlayFlag(VROverlayHandle_t ulOverlayHandle, VROverlayFlags eOverlayFlag, bool *pbEnabled) override;

	EVROverlayError
	GetOverlayFlags(VROverlayHandle_t ulOverlayHandle, uint32_t *pFlags) override;

	EVROverlayError
	SetOverlayColor(VROverlayHandle_t ulOverlayHandle, float fRed, float fGreen, float fBlue) override;

	EVROverlayError
	GetOverlayColor(VROverlayHandle_t ulOverlayHandle, float *pfRed, float *pfGreen, float *pfBlue) override;

	EVROverlayError
	SetOverlayAlpha(VROverlayHandle_t ulOverlayHandle, float fAlpha) override;

	EVROverlayError
	GetOverlayAlpha(VROverlayHandle_t ulOverlayHandle, float *pfAlpha) override;

	EVROverlayError
	SetOverlayTexelAspect(VROverlayHandle_t ulOverlayHandle, float fTexelAspect) override;

	EVROverlayError
	GetOverlayTexelAspect(VROverlayHandle_t ulOverlayHandle, float *pfTexelAspect) override;

	EVROverlayError
	SetOverlaySortOrder(VROverlayHandle_t ulOverlayHandle, uint32_t unSortOrder) override;

	EVROverlayError
	GetOverlaySortOrder(VROverlayHandle_t ulOverlayHandle, uint32_t *punSortOrder) override;

	EVROverlayError
	SetOverlayWidthInMeters(VROverlayHandle_t ulOverlayHandle, float fWidthInMeters) override;

	EVROverlayError
	GetOverlayWidthInMeters(VROverlayHandle_t ulOverlayHandle, float *pfWidthInMeters) override;

	EVROverlayError
	SetOverlayCurvature(VROverlayHandle_t ulOverlayHandle, float fCurvature) override;

	EVROverlayError
	GetOverlayCurvature(VROverlayHandle_t ulOverlayHandle, float *pfCurvature) override;

	EVROverlayError
	SetOverlayPreCurvePitch(VROverlayHandle_t ulOverlayHandle, float fRadians) override;

	EVROverlayError
	GetOverlayPreCurvePitch(VROverlayHandle_t ulOverlayHandle, float *pfRadians) override;

	EVROverlayError
	SetOverlayTextureColorSpace(VROverlayHandle_t ulOverlayHandle, EColorSpace eTextureColorSpace) override;

	EVROverlayError
	GetOverlayTextureColorSpace(VROverlayHandle_t ulOverlayHandle, EColorSpace *peTextureColorSpace) override;

	EVROverlayError
	SetOverlayTextureBounds(VROverlayHandle_t ulOverlayHandle,
	                        const VRTextureBounds_t *pOverlayTextureBounds) override;

	EVROverlayError
	GetOverlayTextureBounds(VROverlayHandle_t ulOverlayHandle, VRTextureBounds_t *pOverlayTextureBounds) override;

	EVROverlayError
	GetOverlayTransformType(VROverlayHandle_t ulOverlayHandle, VROverlayTransformType *peTransformType) override;

	EVROverlayError
	SetOverlayTransformAbsolute(VROverlayHandle_t ulOverlayHandle,
	                            ETrackingUniverseOrigin eTrackingOrigin,
	                            const HmdMatrix34_t *pmatTrackingOriginToOverlayTransform) override;

	EVROverlayError
	GetOverlayTransformAbsolute(VROverlayHandle_t ulOverlayHandle,
	                            ETrackingUniverseOrigin *peTrackingOrigin,
	                            HmdMatrix34_t *pmatTrackingOriginToOverlayTransform) override;

	EVROverlayError
	SetOverlayTransformTrackedDeviceRelative(VROverlayHandle_t ulOverlayHandle,
	                                         TrackedDeviceIndex_t unTrackedDevice,
	                                         const HmdMatrix34_t *pmatTrackedDeviceToOverlayTransform) override;

	EVROverlayError
	GetOverlayTransformTrackedDeviceRelative(VROverlayHandle_t ulOverlayHandle,
	                                         TrackedDeviceIndex_t *punTrackedDevice,
	                                         HmdMatrix34_t *pmatTrackedDeviceToOverlayTransform) override;

	EVROverlayError
	SetOverlayTransformTrackedDeviceComponent(VROverlayHandle_t ulOverlayHandle,
	                                          TrackedDeviceIndex_t unDeviceIndex,
	                                          const char *pchComponentName) override;

	EVROverlayError
	GetOverlayTransformTrackedDeviceComponent(VROverlayHandle_t ulOverlayHandle,
	                                          TrackedDeviceIndex_t *punDeviceIndex,
	                                          char *pchComponentName,
	                                          uint32_t unComponentNameSize) override;

	EVROverlayError
	SetOverlayTransformCursor(VROverlayHandle_t ulCursorOverlayHandle, const HmdVector2_t *pvHotspot) override;

	vr::EVROverlayError
	GetOverlayTransformCursor(VROverlayHandle_t ulOverlayHandle, HmdVector2_t *pvHotspot) override;

	vr::EVROverlayError
	SetOverlayTransformProjection(VROverlayHandle_t ulOverlayHandle,
	                              ETrackingUniverseOrigin eTrackingOrigin,
	                              const HmdMatrix34_t *pmatTrackingOriginToOverlayTransform,
	                              const VROverlayProjection_t *pProjection,
	                              vr::EVREye eEye) override;

	EVROverlayError
	SetSubviewPosition(VROverlayHandle_t ulOverlayHandle, float fX, float fY) override;

	EVROverlayError
	ShowOverlay(VROverlayHandle_t ulOverlayHandle) override;

	EVROverlayError
	HideOverlay(VROverlayHandle_t ulOverlayHandle) override;

	bool
	IsOverlayVisible(VROverlayHandle_t ulOverlayHandle) override;

	EVROverlayError
	GetTransformForOverlayCoordinates(VROverlayHandle_t ulOverlayHandle,
	                                  ETrackingUniverseOrigin eTrackingOrigin,
	                                  HmdVector2_t coordinatesInOverlay,
	                                  HmdMatrix34_t *pmatTransform) override;

	EVROverlayError
	WaitFrameSync(uint32_t nTimeoutMs) override;

	bool
	PollNextOverlayEvent(VROverlayHandle_t ulOverlayHandle, VREvent_t *pEvent, uint32_t uncbVREvent) override;

	EVROverlayError
	GetOverlayInputMethod(VROverlayHandle_t ulOverlayHandle, VROverlayInputMethod *peInputMethod) override;

	EVROverlayError
	SetOverlayInputMethod(VROverlayHandle_t ulOverlayHandle, VROverlayInputMethod eInputMethod) override;

	EVROverlayError
	GetOverlayMouseScale(VROverlayHandle_t ulOverlayHandle, HmdVector2_t *pvecMouseScale) override;

	EVROverlayError
	SetOverlayMouseScale(VROverlayHandle_t ulOverlayHandle, const HmdVector2_t *pvecMouseScale) override;

	bool
	ComputeOverlayIntersection(VROverlayHandle_t ulOverlayHandle,
	                           const VROverlayIntersectionParams_t *pParams,
	                           VROverlayIntersectionResults_t *pResults) override;

	bool
	IsHoverTargetOverlay(VROverlayHandle_t ulOverlayHandle) override;

	EVROverlayError
	SetOverlayIntersectionMask(VROverlayHandle_t ulOverlayHandle,
	                           VROverlayIntersectionMaskPrimitive_t *pMaskPrimitives,
	                           uint32_t unNumMaskPrimitives,
	                           uint32_t unPrimitiveSize) override;

	EVROverlayError
	TriggerLaserMouseHapticVibration(VROverlayHandle_t ulOverlayHandle,
	                                 float fDurationSeconds,
	                                 float fFrequency,
	                                 float fAmplitude) override;

	EVROverlayError
	SetOverlayCursor(VROverlayHandle_t ulOverlayHandle, VROverlayHandle_t ulCursorHandle) override;

	EVROverlayError
	SetOverlayCursorPositionOverride(VROverlayHandle_t ulOverlayHandle, const HmdVector2_t *pvCursor) override;

	EVROverlayError
	ClearOverlayCursorPositionOverride(VROverlayHandle_t ulOverlayHandle) override;

	EVROverlayError
	SetOverlayTexture(VROverlayHandle_t ulOverlayHandle, const Texture_t *pTexture) override;

	EVROverlayError
	ClearOverlayTexture(VROverlayHandle_t ulOverlayHandle) override;

	EVROverlayError
	SetOverlayRaw(VROverlayHandle_t ulOverlayHandle,
	              void *pvBuffer,
	              uint32_t unWidth,
	              uint32_t unHeight,
	              uint32_t unBytesPerPixel) override;

	EVROverlayError
	SetOverlayFromFile(VROverlayHandle_t ulOverlayHandle, const char *pchFilePath) override;

	EVROverlayError
	GetOverlayTexture(VROverlayHandle_t ulOverlayHandle,
	                  void **pNativeTextureHandle,
	                  void *pNativeTextureRef,
	                  uint32_t *pWidth,
	                  uint32_t *pHeight,
	                  uint32_t *pNativeFormat,
	                  ETextureType *pAPIType,
	                  EColorSpace *pColorSpace,
	                  VRTextureBounds_t *pTextureBounds) override;

	EVROverlayError
	ReleaseNativeOverlayHandle(VROverlayHandle_t ulOverlayHandle, void *pNativeTextureHandle) override;

	EVROverlayError
	GetOverlayTextureSize(VROverlayHandle_t ulOverlayHandle, uint32_t *pWidth, uint32_t *pHeight) override;

	EVROverlayError
	CreateDashboardOverlay(const char *pchOverlayKey,
	                       const char *pchOverlayFriendlyName,
	                       VROverlayHandle_t *pMainHandle,
	                       VROverlayHandle_t *pThumbnailHandle) override;

	bool
	IsDashboardVisible() override;

	bool
	IsActiveDashboardOverlay(VROverlayHandle_t ulOverlayHandle) override;

	EVROverlayError
	SetDashboardOverlaySceneProcess(VROverlayHandle_t ulOverlayHandle, uint32_t unProcessId) override;

	EVROverlayError
	GetDashboardOverlaySceneProcess(VROverlayHandle_t ulOverlayHandle, uint32_t *punProcessId) override;

	void
	ShowDashboard(const char *pchOverlayToShow) override;

	vr::TrackedDeviceIndex_t
	GetPrimaryDashboardDevice() override;

	EVROverlayError
	ShowKeyboard(EGamepadTextInputMode eInputMode,
	             EGamepadTextInputLineMode eLineInputMode,
	             uint32_t unFlags,
	             const char *pchDescription,
	             uint32_t unCharMax,
	             const char *pchExistingText,
	             uint64_t uUserValue) override;

	EVROverlayError
	ShowKeyboardForOverlay(VROverlayHandle_t ulOverlayHandle,
	                       EGamepadTextInputMode eInputMode,
	                       EGamepadTextInputLineMode eLineInputMode,
	                       uint32_t unFlags,
	                       const char *pchDescription,
	                       uint32_t unCharMax,
	                       const char *pchExistingText,
	                       uint64_t uUserValue) override;

	uint32_t
	GetKeyboardText(char *pchText, uint32_t cchText) override;

	void
	HideKeyboard() override;

	void
	SetKeyboardTransformAbsolute(ETrackingUniverseOrigin eTrackingOrigin,
	                             const HmdMatrix34_t *pmatTrackingOriginToKeyboardTransform) override;

	void
	SetKeyboardPositionForOverlay(VROverlayHandle_t ulOverlayHandle, HmdRect2_t avoidRect) override;

	VRMessageOverlayResponse
	ShowMessageOverlay(const char *pchText,
	                   const char *pchCaption,
	                   const char *pchButton0Text,
	                   const char *pchButton1Text,
	                   const char *pchButton2Text,
	                   const char *pchButton3Text) override;

	void
	CloseMessageOverlay() override;
};

} // namespace xrt::state_trackers::openvr
