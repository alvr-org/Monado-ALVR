// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRInput interface version 006.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRInput.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRInput_006 : public XRTVRInput_011, public vr::IVRInput_006
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRInput_006(XRTVRClientCore_003 *core) : XRTVRInput_011(core) {};
	// clang-format on

	FnTable_IVRInput_006();

	ForwardDeclareBase_IVRInput(XRTVRInput_011);

	Forward_IVRInput_SetActionManifestPath();
	Forward_IVRInput_GetActionSetHandle();
	Forward_IVRInput_GetActionHandle();
	Forward_IVRInput_GetInputSourceHandle();
	Forward_IVRInput_UpdateActionState();
	Forward_IVRInput_GetDigitalActionData_011();
	Forward_IVRInput_GetAnalogActionData_011();
	Forward_IVRInput_GetPoseActionDataRelativeToNow();
	Forward_IVRInput_GetPoseActionDataForNextFrame();
	Forward_IVRInput_GetSkeletalActionData_011();
	Forward_IVRInput_GetBoneCount();
	Forward_IVRInput_GetBoneHierarchy();
	Forward_IVRInput_GetBoneName();
	Forward_IVRInput_GetSkeletalReferenceTransforms();
	Forward_IVRInput_GetSkeletalTrackingLevel();
	Forward_IVRInput_GetSkeletalBoneData_011();
	Forward_IVRInput_GetSkeletalSummaryData_011();
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
