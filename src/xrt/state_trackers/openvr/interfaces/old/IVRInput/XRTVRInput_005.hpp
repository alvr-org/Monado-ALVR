// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRInput interface version 005.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRInput.hpp"
#include "XRTVRInput_007.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRInput_005 : public XRTVRInput_007, public vr::IVRInput_005
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRInput_005(XRTVRClientCore_003 *core) : XRTVRInput_007(core) {};
	// clang-format on

	FnTable_IVRInput_005();

	ForwardDeclareBase_IVRInput(XRTVRInput_007);

	Forward_IVRInput_SetActionManifestPath();
	Forward_IVRInput_GetActionSetHandle();
	Forward_IVRInput_GetActionHandle();
	Forward_IVRInput_GetInputSourceHandle();
	Forward_IVRInput_UpdateActionState();
	Forward_IVRInput_GetDigitalActionData_011();
	Forward_IVRInput_GetAnalogActionData_011();

	EVRInputError
	GetPoseActionData(VRActionHandle_t action,
	                  ETrackingUniverseOrigin eOrigin,
	                  float fPredictedSecondsFromNow,
	                  InputPoseActionData_t *pActionData,
	                  uint32_t unActionDataSize,
	                  VRInputValueHandle_t ulRestrictToDevice) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetPoseActionData(action=%llu, eOrigin=%d, fPredictedSecondsFromNow=%f, "
		    "pActionData=%p, unActionDataSize=%llu, ulRestrictToDevice=%llu) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		    static_cast<int>(eOrigin), fPredictedSecondsFromNow, static_cast<const void *>(pActionData),
		    static_cast<unsigned long long>(unActionDataSize),
		    static_cast<unsigned long long>(ulRestrictToDevice));
	}

	Forward_IVRInput_GetSkeletalActionData_011();
	Forward_IVRInput_GetBoneCount();
	Forward_IVRInput_GetBoneHierarchy();
	Forward_IVRInput_GetBoneName();
	Forward_IVRInput_GetSkeletalReferenceTransforms();
	Forward_IVRInput_GetSkeletalTrackingLevel();
	Forward_IVRInput_GetSkeletalBoneData_011();

	EVRInputError
	GetSkeletalSummaryData(VRActionHandle_t action, VRSkeletalSummaryData_t *pSkeletalSummaryData) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetSkeletalSummaryData(action=%llu, pSkeletalSummaryData=%p) -> VRInputError_None",
		    EVRInputError::VRInputError_None, static_cast<unsigned long long>(action),
		    static_cast<const void *>(pSkeletalSummaryData));
	}

	Forward_IVRInput_GetSkeletalBoneDataCompressed_011();
	Forward_IVRInput_DecompressSkeletalBoneData_011();
	Forward_IVRInput_TriggerHapticVibrationAction_011();
	Forward_IVRInput_GetActionOrigins();
	Forward_IVRInput_GetOriginLocalizedName_011();
	Forward_IVRInput_GetOriginTrackedDeviceInfo();
	Forward_IVRInput_ShowActionOrigins();
	Forward_IVRInput_ShowBindingsForActionSet();
	Forward_IVRInput_IsUsingLegacyInput();
};

}; // namespace xrt::state_trackers::openvr
