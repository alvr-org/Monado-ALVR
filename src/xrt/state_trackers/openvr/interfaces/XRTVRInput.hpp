// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRInput_011 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "XRTVRClientCore.hpp"

#include <string>
#include <unordered_map>


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRInput_011 : public vr::IVRInput_011
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRInput_011(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRInput_011() = default;

	FnTable_IVRInput_011();

	EVRInputError
	SetActionManifestPath(const char *pchActionManifestPath) override;

	EVRInputError
	GetActionSetHandle(const char *pchActionSetName, VRActionSetHandle_t *pHandle) override;

	EVRInputError
	GetActionHandle(const char *pchActionName, VRActionHandle_t *pHandle) override;

	EVRInputError
	GetInputSourceHandle(const char *pchInputSourcePath, VRInputValueHandle_t *pHandle) override;

	EVRInputError
	UpdateActionState(VRActiveActionSet_t *pSets,
	                  uint32_t unSizeOfVRSelectedActionSet_t,
	                  uint32_t unSetCount) override;

	EVRInputError
	GetDigitalActionData(VRActionHandle_t action,
	                     InputDigitalActionData_t *pActionData,
	                     uint32_t unActionDataSize,
	                     VRInputValueHandle_t ulRestrictToDevice) override;

	EVRInputError
	GetAnalogActionData(VRActionHandle_t action,
	                    InputAnalogActionData_t *pActionData,
	                    uint32_t unActionDataSize,
	                    VRInputValueHandle_t ulRestrictToDevice) override;

	EVRInputError
	GetPoseActionDataRelativeToNow(VRActionHandle_t action,
	                               ETrackingUniverseOrigin eOrigin,
	                               float fPredictedSecondsFromNow,
	                               InputPoseActionData_t *pActionData,
	                               uint32_t unActionDataSize,
	                               VRInputValueHandle_t ulRestrictToDevice) override;

	EVRInputError
	GetPoseActionDataForNextFrame(VRActionHandle_t action,
	                              ETrackingUniverseOrigin eOrigin,
	                              InputPoseActionData_t *pActionData,
	                              uint32_t unActionDataSize,
	                              VRInputValueHandle_t ulRestrictToDevice) override;

	EVRInputError
	GetSkeletalActionData(VRActionHandle_t action,
	                      InputSkeletalActionData_t *pActionData,
	                      uint32_t unActionDataSize) override;

	EVRInputError
	GetDominantHand(ETrackedControllerRole *peDominantHand) override;

	EVRInputError
	SetDominantHand(ETrackedControllerRole eDominantHand) override;

	EVRInputError
	GetEyeTrackingDataRelativeToNow(VRActionHandle_t action,
	                                vr::ETrackingUniverseOrigin eOrigin,
	                                float fPredictedSecondsFromNow,
	                                vr::VREyeTrackingData_t *pEyeTrackingData,
	                                uint32_t ulEyeTrackingDataSize) override;

	EVRInputError
	GetEyeTrackingDataForNextFrame(VRActionHandle_t action,
	                               vr::ETrackingUniverseOrigin eOrigin,
	                               vr::VREyeTrackingData_t *pEyeTrackingData,
	                               uint32_t ulEyeTrackingDataSize) override;

	EVRInputError
	GetBoneCount(VRActionHandle_t action, uint32_t *pBoneCount) override;

	EVRInputError
	GetBoneHierarchy(VRActionHandle_t action, BoneIndex_t *pParentIndices, uint32_t unIndexArayCount) override;

	EVRInputError
	GetBoneName(VRActionHandle_t action,
	            BoneIndex_t nBoneIndex,
	            char *pchBoneName,
	            uint32_t unNameBufferSize) override;

	EVRInputError
	GetSkeletalReferenceTransforms(VRActionHandle_t action,
	                               EVRSkeletalTransformSpace eTransformSpace,
	                               EVRSkeletalReferencePose eReferencePose,
	                               VRBoneTransform_t *pTransformArray,
	                               uint32_t unTransformArrayCount) override;

	EVRInputError
	GetSkeletalTrackingLevel(VRActionHandle_t action, EVRSkeletalTrackingLevel *pSkeletalTrackingLevel) override;

	EVRInputError
	GetSkeletalBoneData(VRActionHandle_t action,
	                    EVRSkeletalTransformSpace eTransformSpace,
	                    EVRSkeletalMotionRange eMotionRange,
	                    VRBoneTransform_t *pTransformArray,
	                    uint32_t unTransformArrayCount) override;

	EVRInputError
	GetSkeletalSummaryData(VRActionHandle_t action,
	                       EVRSummaryType eSummaryType,
	                       VRSkeletalSummaryData_t *pSkeletalSummaryData) override;

	EVRInputError
	GetSkeletalBoneDataCompressed(VRActionHandle_t action,
	                              EVRSkeletalMotionRange eMotionRange,
	                              void *pvCompressedData,
	                              uint32_t unCompressedSize,
	                              uint32_t *punRequiredCompressedSize) override;

	EVRInputError
	DecompressSkeletalBoneData(const void *pvCompressedBuffer,
	                           uint32_t unCompressedBufferSize,
	                           EVRSkeletalTransformSpace eTransformSpace,
	                           VRBoneTransform_t *pTransformArray,
	                           uint32_t unTransformArrayCount) override;

	EVRInputError
	TriggerHapticVibrationAction(VRActionHandle_t action,
	                             float fStartSecondsFromNow,
	                             float fDurationSeconds,
	                             float fFrequency,
	                             float fAmplitude,
	                             VRInputValueHandle_t ulRestrictToDevice) override;

	EVRInputError
	GetActionOrigins(VRActionSetHandle_t actionSetHandle,
	                 VRActionHandle_t digitalActionHandle,
	                 VRInputValueHandle_t *originsOut,
	                 uint32_t originOutCount) override;

	EVRInputError
	GetOriginLocalizedName(VRInputValueHandle_t origin,
	                       char *pchNameArray,
	                       uint32_t unNameArraySize,
	                       int32_t unStringSectionsToInclude) override;

	EVRInputError
	GetOriginTrackedDeviceInfo(VRInputValueHandle_t origin,
	                           InputOriginInfo_t *pOriginInfo,
	                           uint32_t unOriginInfoSize) override;

	EVRInputError
	GetActionBindingInfo(VRActionHandle_t action,
	                     InputBindingInfo_t *pOriginInfo,
	                     uint32_t unBindingInfoSize,
	                     uint32_t unBindingInfoCount,
	                     uint32_t *punReturnedBindingInfoCount) override;

	EVRInputError
	ShowActionOrigins(VRActionSetHandle_t actionSetHandle, VRActionHandle_t ulActionHandle) override;

	EVRInputError
	ShowBindingsForActionSet(VRActiveActionSet_t *pSets,
	                         uint32_t unSizeOfVRSelectedActionSet_t,
	                         uint32_t unSetCount,
	                         VRInputValueHandle_t originToHighlight) override;

	EVRInputError
	GetComponentStateForBinding(const char *pchRenderModelName,
	                            const char *pchComponentName,
	                            const InputBindingInfo_t *pOriginInfo,
	                            uint32_t unBindingInfoSize,
	                            uint32_t unBindingInfoCount,
	                            vr::RenderModel_ComponentState_t *pComponentState) override;

	bool
	IsUsingLegacyInput() override;

	EVRInputError
	OpenBindingUI(const char *pchAppKey,
	              VRActionSetHandle_t ulActionSetHandle,
	              VRInputValueHandle_t ulDeviceHandle,
	              bool bShowOnDesktop) override;

	EVRInputError
	GetBindingVariant(vr::VRInputValueHandle_t ulDevicePath,
	                  char *pchVariantArray,
	                  uint32_t unVariantArraySize) override;
};

} // namespace xrt::state_trackers::openvr
