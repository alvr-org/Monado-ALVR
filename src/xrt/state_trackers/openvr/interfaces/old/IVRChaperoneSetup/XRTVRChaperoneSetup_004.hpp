// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRChaperoneSetup interface version 004.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRChaperoneSetup.hpp"
#include "XRTVRChaperoneSetup_005.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRChaperoneSetup_004 : public XRTVRChaperoneSetup_005, public vr::IVRChaperoneSetup_004
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRChaperoneSetup_004(XRTVRClientCore_003 *core) : XRTVRChaperoneSetup_005(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRChaperoneSetup_004.
		return nullptr;
	}

	ForwardDeclareBase_IVRChaperoneSetup(XRTVRChaperoneSetup_005);

	Forward_IVRChaperoneSetup_CommitWorkingCopy();
	Forward_IVRChaperoneSetup_RevertWorkingCopy();
	Forward_IVRChaperoneSetup_GetWorkingPlayAreaSize();
	Forward_IVRChaperoneSetup_GetWorkingPlayAreaRect();
	Forward_IVRChaperoneSetup_GetWorkingCollisionBoundsInfo();
	Forward_IVRChaperoneSetup_GetLiveCollisionBoundsInfo();
	Forward_IVRChaperoneSetup_GetWorkingSeatedZeroPoseToRawTrackingPose();
	Forward_IVRChaperoneSetup_GetWorkingStandingZeroPoseToRawTrackingPose();
	Forward_IVRChaperoneSetup_SetWorkingPlayAreaSize();
	Forward_IVRChaperoneSetup_SetWorkingCollisionBoundsInfo();
	Forward_IVRChaperoneSetup_SetWorkingSeatedZeroPoseToRawTrackingPose();
	Forward_IVRChaperoneSetup_SetWorkingStandingZeroPoseToRawTrackingPose();
	Forward_IVRChaperoneSetup_ReloadFromDisk();
	Forward_IVRChaperoneSetup_GetLiveSeatedZeroPoseToRawTrackingPose();

	void
	SetWorkingWallTagInfo(VR_ARRAY_COUNT(unTagCount) uint8_t *pTagsBuffer, uint32_t unTagCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetWorkingWallTagInfo(pTagsBuffer=%p, unTagCount=%llu)",
		                         static_cast<const void *>(pTagsBuffer),
		                         static_cast<unsigned long long>(unTagCount));
	}

	bool
	GetLiveWallTagInfo(VR_OUT_ARRAY_COUNT(punTagCount) uint8_t *pTagsBuffer, uint32_t *punTagCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetLiveWallTagInfo(pTagsBuffer=%p, punTagCount=%p) -> false",
		                             false, static_cast<const void *>(pTagsBuffer),
		                             static_cast<const void *>(punTagCount));
	}
};

}; // namespace xrt::state_trackers::openvr
