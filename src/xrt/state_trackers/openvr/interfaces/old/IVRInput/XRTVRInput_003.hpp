// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRInput interface version 003.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRInput.hpp"
#include "XRTVRInput_004.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRInput_003 : public XRTVRInput_004, public vr::IVRInput_003
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRInput_003(XRTVRClientCore_003 *core) : XRTVRInput_004(core) {};
	// clang-format on

	FnTable_IVRInput_003();

	ForwardDeclareBase_IVRInput(XRTVRInput_004);

	Forward_IVRInput_SetActionManifestPath();
	Forward_IVRInput_GetActionSetHandle();
	Forward_IVRInput_GetActionHandle();
	Forward_IVRInput_GetInputSourceHandle();
	Forward_IVRInput_UpdateActionState();

	EVRInputError
	GetDigitalActionData(VRActionHandle_t action,
	                     InputDigitalActionData_t *pActionData,
	                     uint32_t unActionDataSize) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetDigitalActionData(action=%llu, pActionData=%p, unActionDataSize=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		    static_cast<const void *>(pActionData), static_cast<unsigned long long>(unActionDataSize));
	}

	EVRInputError
	GetAnalogActionData(VRActionHandle_t action,
	                    InputAnalogActionData_t *pActionData,
	                    uint32_t unActionDataSize) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetAnalogActionData(action=%llu, pActionData=%p, unActionDataSize=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		    static_cast<const void *>(pActionData), static_cast<unsigned long long>(unActionDataSize));
	}

	EVRInputError
	GetPoseActionData(VRActionHandle_t action,
	                  ETrackingUniverseOrigin eOrigin,
	                  float fPredictedSecondsFromNow,
	                  InputPoseActionData_t *pActionData,
	                  uint32_t unActionDataSize) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetPoseActionData(action=%llu, eOrigin=%d, fPredictedSecondsFromNow=%f, "
		                             "pActionData=%p, unActionDataSize=%llu) -> VRInputError_None",
		                             EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		                             static_cast<int>(eOrigin), fPredictedSecondsFromNow,
		                             static_cast<const void *>(pActionData),
		                             static_cast<unsigned long long>(unActionDataSize));
	}

	EVRInputError
	GetSkeletalActionData(VRActionHandle_t action,
	                      EVRSkeletalTransformSpace eBoneParent,
	                      float fPredictedSecondsFromNow,
	                      InputSkeletonActionData_t *pActionData,
	                      uint32_t unActionDataSize,
	                      VR_ARRAY_COUNT(unTransformArrayCount) VRBoneTransform_t *pTransformArray,
	                      uint32_t unTransformArrayCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetSkeletalActionData(action=%llu, eBoneParent=?, fPredictedSecondsFromNow=%f, pActionData=%p, "
		    "unActionDataSize=%llu, pTransformArray=%p, unTransformArrayCount=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(action), fPredictedSecondsFromNow,
		    static_cast<const void *>(pActionData), static_cast<unsigned long long>(unActionDataSize),
		    static_cast<const void *>(pTransformArray), static_cast<unsigned long long>(unTransformArrayCount));
	}

	EVRInputError
	GetSkeletalActionDataCompressed(VRActionHandle_t action,
	                                EVRSkeletalTransformSpace eBoneParent,
	                                float fPredictedSecondsFromNow,
	                                VR_OUT_BUFFER_COUNT(unCompressedSize) void *pvCompressedData,
	                                uint32_t unCompressedSize,
	                                uint32_t *punRequiredCompressedSize) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetSkeletalActionDataCompressed(action=%llu, eBoneParent=?, fPredictedSecondsFromNow=%f, "
		    "pvCompressedData=%p, unCompressedSize=%llu, punRequiredCompressedSize=%p) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(action), fPredictedSecondsFromNow,
		    static_cast<const void *>(pvCompressedData), static_cast<unsigned long long>(unCompressedSize),
		    static_cast<const void *>(punRequiredCompressedSize));
	}

	EVRInputError
	UncompressSkeletalActionData(void *pvCompressedBuffer,
	                             uint32_t unCompressedBufferSize,
	                             EVRSkeletalTransformSpace *peBoneParent,
	                             VR_ARRAY_COUNT(unTransformArrayCount) VRBoneTransform_t *pTransformArray,
	                             uint32_t unTransformArrayCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "UncompressSkeletalActionData(pvCompressedBuffer=%p, unCompressedBufferSize=%llu, peBoneParent=%p, "
		    "pTransformArray=%p, unTransformArrayCount=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<const void *>(pvCompressedBuffer),
		    static_cast<unsigned long long>(unCompressedBufferSize), static_cast<const void *>(peBoneParent),
		    static_cast<const void *>(pTransformArray), static_cast<unsigned long long>(unTransformArrayCount));
	}

	EVRInputError
	TriggerHapticVibrationAction(VRActionHandle_t action,
	                             float fStartSecondsFromNow,
	                             float fDurationSeconds,
	                             float fFrequency,
	                             float fAmplitude) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "TriggerHapticVibrationAction(action=%llu, fStartSecondsFromNow=%f, "
		                             "fDurationSeconds=%f, fFrequency=%f, fAmplitude=%f) -> VRInputError_None",
		                             EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		                             fStartSecondsFromNow, fDurationSeconds, fFrequency, fAmplitude);
	}

	Forward_IVRInput_GetActionOrigins();
	Forward_IVRInput_GetOriginLocalizedName_004();
	Forward_IVRInput_GetOriginTrackedDeviceInfo();
	Forward_IVRInput_ShowActionOrigins();
	Forward_IVRInput_ShowBindingsForActionSet();
};

}; // namespace xrt::state_trackers::openvr
