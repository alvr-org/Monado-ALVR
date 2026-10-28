// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRInput_011 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRInput.hpp"


namespace xrt::state_trackers::openvr {

XRTVRInput_011::XRTVRInput_011(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

EVRInputError
XRTVRInput_011::SetActionManifestPath(const char *pchActionManifestPath)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetActionManifestPath(pchActionManifestPath=%s) -> %llu",
	                             static_cast<EVRInputError>(0),
	                             pchActionManifestPath != nullptr ? pchActionManifestPath : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetActionSetHandle(const char *pchActionSetName, VRActionSetHandle_t *pHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (pchActionSetName == nullptr || pHandle == nullptr) {
		return vr::EVRInputError::VRInputError_InvalidParam;
	}

	std::string name(pchActionSetName);

	auto ret = this->core->input->getActionSetHandle(name, *pHandle);

	OPENVR_LOG_TRACE(logger, "GetActionSetHandle(pchActionSetName=%s, pHandle=%p) -> %llu", pchActionSetName,
	                 static_cast<void *>(pHandle), static_cast<unsigned long long>(*pHandle));

	return ret;
}

EVRInputError
XRTVRInput_011::GetActionHandle(const char *pchActionName, VRActionHandle_t *pHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (pchActionName == nullptr || pHandle == nullptr) {
		return vr::EVRInputError::VRInputError_InvalidParam;
	}

	std::string name(pchActionName);

	auto ret = this->core->input->getActionHandle(name, *pHandle);

	OPENVR_LOG_TRACE(logger, "GetActionHandle(pchActionName=%s, pHandle=%p) -> %llu", pchActionName,
	                 static_cast<void *>(pHandle), static_cast<unsigned long long>(*pHandle));

	return ret;
}

EVRInputError
XRTVRInput_011::GetInputSourceHandle(const char *pchInputSourcePath, VRInputValueHandle_t *pHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (pchInputSourcePath == nullptr || pHandle == nullptr) {
		return vr::EVRInputError::VRInputError_InvalidParam;
	}

	std::string path(pchInputSourcePath);

	auto ret = this->core->input->getInputSourceHandle(path, *pHandle);

	OPENVR_LOG_TRACE(logger, "GetInputSourceHandle(pchInputSourcePath=%s, pHandle=%p) -> %llu", pchInputSourcePath,
	                 static_cast<void *>(pHandle), static_cast<unsigned long long>(*pHandle));

	return ret;
}

EVRInputError
XRTVRInput_011::UpdateActionState(VRActiveActionSet_t *pSets,
                                  uint32_t unSizeOfVRSelectedActionSet_t,
                                  uint32_t unSetCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "UpdateActionState(pSets=%p, unSizeOfVRSelectedActionSet_t=%llu, unSetCount=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<void *>(pSets),
	    static_cast<unsigned long long>(unSizeOfVRSelectedActionSet_t), static_cast<unsigned long long>(unSetCount),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetDigitalActionData(VRActionHandle_t action,
                                     InputDigitalActionData_t *pActionData,
                                     uint32_t unActionDataSize,
                                     VRInputValueHandle_t ulRestrictToDevice)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetDigitalActionData(action=%llu, pActionData=%p, unActionDataSize=%llu, ulRestrictToDevice=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action), static_cast<void *>(pActionData),
	    static_cast<unsigned long long>(unActionDataSize), static_cast<unsigned long long>(ulRestrictToDevice),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetAnalogActionData(VRActionHandle_t action,
                                    InputAnalogActionData_t *pActionData,
                                    uint32_t unActionDataSize,
                                    VRInputValueHandle_t ulRestrictToDevice)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetAnalogActionData(action=%llu, pActionData=%p, unActionDataSize=%llu, ulRestrictToDevice=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action), static_cast<void *>(pActionData),
	    static_cast<unsigned long long>(unActionDataSize), static_cast<unsigned long long>(ulRestrictToDevice),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetPoseActionDataRelativeToNow(VRActionHandle_t action,
                                               ETrackingUniverseOrigin eOrigin,
                                               float fPredictedSecondsFromNow,
                                               InputPoseActionData_t *pActionData,
                                               uint32_t unActionDataSize,
                                               VRInputValueHandle_t ulRestrictToDevice)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetPoseActionDataRelativeToNow(action=%llu, eOrigin=%llu, fPredictedSecondsFromNow=%f, pActionData=%p, "
	    "unActionDataSize=%llu, ulRestrictToDevice=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	    static_cast<unsigned long long>(eOrigin), static_cast<double>(fPredictedSecondsFromNow),
	    static_cast<void *>(pActionData), static_cast<unsigned long long>(unActionDataSize),
	    static_cast<unsigned long long>(ulRestrictToDevice),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetPoseActionDataForNextFrame(VRActionHandle_t action,
                                              ETrackingUniverseOrigin eOrigin,
                                              InputPoseActionData_t *pActionData,
                                              uint32_t unActionDataSize,
                                              VRInputValueHandle_t ulRestrictToDevice)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetPoseActionDataForNextFrame(action=%llu, eOrigin=%llu, pActionData=%p, "
	                             "unActionDataSize=%llu, ulRestrictToDevice=%llu) -> %llu",
	                             static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	                             static_cast<unsigned long long>(eOrigin), static_cast<void *>(pActionData),
	                             static_cast<unsigned long long>(unActionDataSize),
	                             static_cast<unsigned long long>(ulRestrictToDevice),
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetSkeletalActionData(VRActionHandle_t action,
                                      InputSkeletalActionData_t *pActionData,
                                      uint32_t unActionDataSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetSkeletalActionData(action=%llu, pActionData=%p, unActionDataSize=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action), static_cast<void *>(pActionData),
	    static_cast<unsigned long long>(unActionDataSize),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetDominantHand(ETrackedControllerRole *peDominantHand)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	// @todo verify this is the right error
	if (peDominantHand == nullptr) {
		OPENVR_LOG_TRACE(logger, "GetDominantHand(peDominantHand=nullptr) -> VRInputError_InvalidParam");
		return EVRInputError::VRInputError_InvalidParam;
	}

	auto ret = this->core->input->getDominantHand(*peDominantHand);

	OPENVR_LOG_TRACE(logger, "GetDominantHand(peDominantHand=%d) -> %d", static_cast<int>(*peDominantHand),
	                 static_cast<int>(ret));

	return ret;
}

EVRInputError
XRTVRInput_011::SetDominantHand(ETrackedControllerRole eDominantHand)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	auto ret = this->core->input->setDominantHand(eDominantHand);

	OPENVR_LOG_TRACE(logger, "SetDominantHand(eDominantHand=%d) -> %d", static_cast<int>(eDominantHand),
	                 static_cast<int>(ret));

	return ret;
}

EVRInputError
XRTVRInput_011::GetEyeTrackingDataRelativeToNow(VRActionHandle_t action,
                                                vr::ETrackingUniverseOrigin eOrigin,
                                                float fPredictedSecondsFromNow,
                                                vr::VREyeTrackingData_t *pEyeTrackingData,
                                                uint32_t ulEyeTrackingDataSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetEyeTrackingDataRelativeToNow(action=%llu, eOrigin=%llu, fPredictedSecondsFromNow=%f, "
	    "pEyeTrackingData=%p, ulEyeTrackingDataSize=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	    static_cast<unsigned long long>(eOrigin), static_cast<double>(fPredictedSecondsFromNow),
	    static_cast<void *>(pEyeTrackingData), static_cast<unsigned long long>(ulEyeTrackingDataSize),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetEyeTrackingDataForNextFrame(VRActionHandle_t action,
                                               vr::ETrackingUniverseOrigin eOrigin,
                                               vr::VREyeTrackingData_t *pEyeTrackingData,
                                               uint32_t ulEyeTrackingDataSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetEyeTrackingDataForNextFrame(action=%llu, eOrigin=%llu, pEyeTrackingData=%p, "
	                             "ulEyeTrackingDataSize=%llu) -> %llu",
	                             static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	                             static_cast<unsigned long long>(eOrigin), static_cast<void *>(pEyeTrackingData),
	                             static_cast<unsigned long long>(ulEyeTrackingDataSize),
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetBoneCount(VRActionHandle_t action, uint32_t *pBoneCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetBoneCount(action=%llu, pBoneCount=%p) -> %llu",
	                             static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	                             static_cast<void *>(pBoneCount),
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetBoneHierarchy(VRActionHandle_t action, BoneIndex_t *pParentIndices, uint32_t unIndexArayCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetBoneHierarchy(action=%llu, pParentIndices=%p, unIndexArayCount=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action), static_cast<void *>(pParentIndices),
	    static_cast<unsigned long long>(unIndexArayCount),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetBoneName(VRActionHandle_t action,
                            BoneIndex_t nBoneIndex,
                            char *pchBoneName,
                            uint32_t unNameBufferSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetBoneName(action=%llu, nBoneIndex=%llu, pchBoneName=%p, unNameBufferSize=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	    static_cast<unsigned long long>(nBoneIndex), static_cast<void *>(pchBoneName),
	    static_cast<unsigned long long>(unNameBufferSize),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetSkeletalReferenceTransforms(VRActionHandle_t action,
                                               EVRSkeletalTransformSpace eTransformSpace,
                                               EVRSkeletalReferencePose eReferencePose,
                                               VRBoneTransform_t *pTransformArray,
                                               uint32_t unTransformArrayCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetSkeletalReferenceTransforms(action=%llu, eTransformSpace=%llu, eReferencePose=%llu, "
	    "pTransformArray=%p, unTransformArrayCount=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	    static_cast<unsigned long long>(eTransformSpace), static_cast<unsigned long long>(eReferencePose),
	    static_cast<void *>(pTransformArray), static_cast<unsigned long long>(unTransformArrayCount),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetSkeletalTrackingLevel(VRActionHandle_t action, EVRSkeletalTrackingLevel *pSkeletalTrackingLevel)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetSkeletalTrackingLevel(action=%llu, pSkeletalTrackingLevel=%p) -> %llu",
	                             static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	                             static_cast<void *>(pSkeletalTrackingLevel),
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetSkeletalBoneData(VRActionHandle_t action,
                                    EVRSkeletalTransformSpace eTransformSpace,
                                    EVRSkeletalMotionRange eMotionRange,
                                    VRBoneTransform_t *pTransformArray,
                                    uint32_t unTransformArrayCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetSkeletalBoneData(action=%llu, eTransformSpace=%llu, eMotionRange=%llu, pTransformArray=%p, "
	    "unTransformArrayCount=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	    static_cast<unsigned long long>(eTransformSpace), static_cast<unsigned long long>(eMotionRange),
	    static_cast<void *>(pTransformArray), static_cast<unsigned long long>(unTransformArrayCount),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetSkeletalSummaryData(VRActionHandle_t action,
                                       EVRSummaryType eSummaryType,
                                       VRSkeletalSummaryData_t *pSkeletalSummaryData)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetSkeletalSummaryData(action=%llu, eSummaryType=%llu, pSkeletalSummaryData=%p) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	    static_cast<unsigned long long>(eSummaryType), static_cast<void *>(pSkeletalSummaryData),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetSkeletalBoneDataCompressed(VRActionHandle_t action,
                                              EVRSkeletalMotionRange eMotionRange,
                                              void *pvCompressedData,
                                              uint32_t unCompressedSize,
                                              uint32_t *punRequiredCompressedSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetSkeletalBoneDataCompressed(action=%llu, eMotionRange=%llu, pvCompressedData=%p, unCompressedSize=%llu, "
	    "punRequiredCompressedSize=%p) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	    static_cast<unsigned long long>(eMotionRange), static_cast<void *>(pvCompressedData),
	    static_cast<unsigned long long>(unCompressedSize), static_cast<void *>(punRequiredCompressedSize),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::DecompressSkeletalBoneData(const void *pvCompressedBuffer,
                                           uint32_t unCompressedBufferSize,
                                           EVRSkeletalTransformSpace eTransformSpace,
                                           VRBoneTransform_t *pTransformArray,
                                           uint32_t unTransformArrayCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "DecompressSkeletalBoneData(pvCompressedBuffer=%p, unCompressedBufferSize=%llu, eTransformSpace=%llu, "
	    "pTransformArray=%p, unTransformArrayCount=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<const void *>(pvCompressedBuffer),
	    static_cast<unsigned long long>(unCompressedBufferSize), static_cast<unsigned long long>(eTransformSpace),
	    static_cast<void *>(pTransformArray), static_cast<unsigned long long>(unTransformArrayCount),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::TriggerHapticVibrationAction(VRActionHandle_t action,
                                             float fStartSecondsFromNow,
                                             float fDurationSeconds,
                                             float fFrequency,
                                             float fAmplitude,
                                             VRInputValueHandle_t ulRestrictToDevice)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "TriggerHapticVibrationAction(action=%llu, fStartSecondsFromNow=%f, fDurationSeconds=%f, fFrequency=%f, "
	    "fAmplitude=%f, ulRestrictToDevice=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action),
	    static_cast<double>(fStartSecondsFromNow), static_cast<double>(fDurationSeconds),
	    static_cast<double>(fFrequency), static_cast<double>(fAmplitude),
	    static_cast<unsigned long long>(ulRestrictToDevice),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetActionOrigins(VRActionSetHandle_t actionSetHandle,
                                 VRActionHandle_t digitalActionHandle,
                                 VRInputValueHandle_t *originsOut,
                                 uint32_t originOutCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetActionOrigins(actionSetHandle=%llu, digitalActionHandle=%llu, originsOut=%p, "
	                             "originOutCount=%llu) -> %llu",
	                             static_cast<EVRInputError>(0), static_cast<unsigned long long>(actionSetHandle),
	                             static_cast<unsigned long long>(digitalActionHandle),
	                             static_cast<void *>(originsOut), static_cast<unsigned long long>(originOutCount),
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetOriginLocalizedName(VRInputValueHandle_t origin,
                                       char *pchNameArray,
                                       uint32_t unNameArraySize,
                                       int32_t unStringSectionsToInclude)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetOriginLocalizedName(origin=%llu, pchNameArray=%p, unNameArraySize=%llu, "
	                             "unStringSectionsToInclude=%llu) -> %llu",
	                             static_cast<EVRInputError>(0), static_cast<unsigned long long>(origin),
	                             static_cast<void *>(pchNameArray),
	                             static_cast<unsigned long long>(unNameArraySize),
	                             static_cast<unsigned long long>(unStringSectionsToInclude),
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetOriginTrackedDeviceInfo(VRInputValueHandle_t origin,
                                           InputOriginInfo_t *pOriginInfo,
                                           uint32_t unOriginInfoSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetOriginTrackedDeviceInfo(origin=%llu, pOriginInfo=%p, unOriginInfoSize=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(origin), static_cast<void *>(pOriginInfo),
	    static_cast<unsigned long long>(unOriginInfoSize),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetActionBindingInfo(VRActionHandle_t action,
                                     InputBindingInfo_t *pOriginInfo,
                                     uint32_t unBindingInfoSize,
                                     uint32_t unBindingInfoCount,
                                     uint32_t *punReturnedBindingInfoCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetActionBindingInfo(action=%llu, pOriginInfo=%p, unBindingInfoSize=%llu, unBindingInfoCount=%llu, "
	    "punReturnedBindingInfoCount=%p) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(action), static_cast<void *>(pOriginInfo),
	    static_cast<unsigned long long>(unBindingInfoSize), static_cast<unsigned long long>(unBindingInfoCount),
	    static_cast<void *>(punReturnedBindingInfoCount),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::ShowActionOrigins(VRActionSetHandle_t actionSetHandle, VRActionHandle_t ulActionHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ShowActionOrigins(actionSetHandle=%llu, ulActionHandle=%llu) -> %llu",
	                             static_cast<EVRInputError>(0), static_cast<unsigned long long>(actionSetHandle),
	                             static_cast<unsigned long long>(ulActionHandle),
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::ShowBindingsForActionSet(VRActiveActionSet_t *pSets,
                                         uint32_t unSizeOfVRSelectedActionSet_t,
                                         uint32_t unSetCount,
                                         VRInputValueHandle_t originToHighlight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "ShowBindingsForActionSet(pSets=%p, unSizeOfVRSelectedActionSet_t=%llu, "
	                             "unSetCount=%llu, originToHighlight=%llu) -> %llu",
	                             static_cast<EVRInputError>(0), static_cast<void *>(pSets),
	                             static_cast<unsigned long long>(unSizeOfVRSelectedActionSet_t),
	                             static_cast<unsigned long long>(unSetCount),
	                             static_cast<unsigned long long>(originToHighlight),
	                             static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetComponentStateForBinding(const char *pchRenderModelName,
                                            const char *pchComponentName,
                                            const InputBindingInfo_t *pOriginInfo,
                                            uint32_t unBindingInfoSize,
                                            uint32_t unBindingInfoCount,
                                            vr::RenderModel_ComponentState_t *pComponentState)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetComponentStateForBinding(pchRenderModelName=%s, pchComponentName=%s, pOriginInfo=%p, "
	    "unBindingInfoSize=%llu, unBindingInfoCount=%llu, pComponentState=%p) -> %llu",
	    static_cast<EVRInputError>(0), pchRenderModelName != nullptr ? pchRenderModelName : "(null)",
	    pchComponentName != nullptr ? pchComponentName : "(null)", static_cast<const void *>(pOriginInfo),
	    static_cast<unsigned long long>(unBindingInfoSize), static_cast<unsigned long long>(unBindingInfoCount),
	    static_cast<void *>(pComponentState), static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

bool
XRTVRInput_011::IsUsingLegacyInput()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsUsingLegacyInput() -> %d", false, false);
}

EVRInputError
XRTVRInput_011::OpenBindingUI(const char *pchAppKey,
                              VRActionSetHandle_t ulActionSetHandle,
                              VRInputValueHandle_t ulDeviceHandle,
                              bool bShowOnDesktop)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "OpenBindingUI(pchAppKey=%s, ulActionSetHandle=%llu, ulDeviceHandle=%llu, bShowOnDesktop=%d) -> %llu",
	    static_cast<EVRInputError>(0), pchAppKey != nullptr ? pchAppKey : "(null)",
	    static_cast<unsigned long long>(ulActionSetHandle), static_cast<unsigned long long>(ulDeviceHandle),
	    static_cast<int>(bShowOnDesktop), static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

EVRInputError
XRTVRInput_011::GetBindingVariant(vr::VRInputValueHandle_t ulDevicePath,
                                  char *pchVariantArray,
                                  uint32_t unVariantArraySize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetBindingVariant(ulDevicePath=%llu, pchVariantArray=%p, unVariantArraySize=%llu) -> %llu",
	    static_cast<EVRInputError>(0), static_cast<unsigned long long>(ulDevicePath),
	    static_cast<void *>(pchVariantArray), static_cast<unsigned long long>(unVariantArraySize),
	    static_cast<unsigned long long>(static_cast<EVRInputError>(0)));
}

}; // namespace xrt::state_trackers::openvr
