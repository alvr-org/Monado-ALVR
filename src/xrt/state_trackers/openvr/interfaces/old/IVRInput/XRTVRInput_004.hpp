// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRInput interface version 004.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRInput.hpp"
#include "XRTVRInput_005.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRInput_004 : public XRTVRInput_005, public vr::IVRInput_004
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRInput_004(XRTVRClientCore_003 *core) : XRTVRInput_005(core) {};
	// clang-format on

	FnTable_IVRInput_004();

	ForwardDeclareBase_IVRInput(XRTVRInput_005);

	Forward_IVRInput_SetActionManifestPath();
	Forward_IVRInput_GetActionSetHandle();
	Forward_IVRInput_GetActionHandle();
	Forward_IVRInput_GetInputSourceHandle();
	Forward_IVRInput_UpdateActionState();
	Forward_IVRInput_GetDigitalActionData_011();
	Forward_IVRInput_GetAnalogActionData_011();
	Forward_IVRInput_GetPoseActionData_005();

	EVRInputError
	GetSkeletalActionData(VRActionHandle_t action,
	                      InputSkeletalActionData_t *pActionData,
	                      uint32_t unActionDataSize,
	                      VRInputValueHandle_t ulRestrictToDevice) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetSkeletalActionData(action=%llu, pActionData=%p, "
		                             "unActionDataSize=%llu, ulRestrictToDevice=%llu) -> VRInputError_None",
		                             EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		                             static_cast<const void *>(pActionData),
		                             static_cast<unsigned long long>(unActionDataSize),
		                             static_cast<unsigned long long>(ulRestrictToDevice));
	}

	EVRInputError
	GetSkeletalBoneData(VRActionHandle_t action,
	                    EVRSkeletalTransformSpace eTransformSpace,
	                    EVRSkeletalMotionRange eMotionRange,
	                    VR_ARRAY_COUNT(unTransformArrayCount) VRBoneTransform_t *pTransformArray,
	                    uint32_t unTransformArrayCount,
	                    VRInputValueHandle_t ulRestrictToDevice) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetSkeletalBoneData(action=%llu, eTransformSpace=?, eMotionRange=%d, "
		    "pTransformArray=%p, unTransformArrayCount=%llu, ulRestrictToDevice=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		    static_cast<int>(eMotionRange), static_cast<const void *>(pTransformArray),
		    static_cast<unsigned long long>(unTransformArrayCount),
		    static_cast<unsigned long long>(ulRestrictToDevice));
	}

	EVRInputError
	GetSkeletalBoneDataCompressed(VRActionHandle_t action,
	                              EVRSkeletalTransformSpace eTransformSpace,
	                              EVRSkeletalMotionRange eMotionRange,
	                              VR_OUT_BUFFER_COUNT(unCompressedSize) void *pvCompressedData,
	                              uint32_t unCompressedSize,
	                              uint32_t *punRequiredCompressedSize,
	                              VRInputValueHandle_t ulRestrictToDevice) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetSkeletalBoneDataCompressed(action=%llu, eTransformSpace=?, "
		    "eMotionRange=%d, pvCompressedData=%p, unCompressedSize=%llu, "
		    "punRequiredCompressedSize=%p, ulRestrictToDevice=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		    static_cast<int>(eMotionRange), static_cast<const void *>(pvCompressedData),
		    static_cast<unsigned long long>(unCompressedSize),
		    static_cast<const void *>(punRequiredCompressedSize),
		    static_cast<unsigned long long>(ulRestrictToDevice));
	}

	EVRInputError
	DecompressSkeletalBoneData(void *pvCompressedBuffer,
	                           uint32_t unCompressedBufferSize,
	                           EVRSkeletalTransformSpace *peTransformSpace,
	                           VR_ARRAY_COUNT(unTransformArrayCount) VRBoneTransform_t *pTransformArray,
	                           uint32_t unTransformArrayCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "DecompressSkeletalBoneData(pvCompressedBuffer=%p, unCompressedBufferSize=%llu, "
		    "peTransformSpace=%p, pTransformArray=%p, unTransformArrayCount=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<const void *>(pvCompressedBuffer),
		    static_cast<unsigned long long>(unCompressedBufferSize),
		    static_cast<const void *>(peTransformSpace), static_cast<const void *>(pTransformArray),
		    static_cast<unsigned long long>(unTransformArrayCount));
	}

	Forward_IVRInput_TriggerHapticVibrationAction_011();
	Forward_IVRInput_GetActionOrigins();

	EVRInputError
	GetOriginLocalizedName(VRInputValueHandle_t origin,
	                       VR_OUT_STRING() char *pchNameArray,
	                       uint32_t unNameArraySize) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetOriginLocalizedName(origin=%llu, pchNameArray=%p, unNameArraySize=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(origin),
		    static_cast<const void *>(pchNameArray), static_cast<unsigned long long>(unNameArraySize));
	}

	Forward_IVRInput_GetOriginTrackedDeviceInfo();
	Forward_IVRInput_ShowActionOrigins();
	Forward_IVRInput_ShowBindingsForActionSet();
};

}; // namespace xrt::state_trackers::openvr
