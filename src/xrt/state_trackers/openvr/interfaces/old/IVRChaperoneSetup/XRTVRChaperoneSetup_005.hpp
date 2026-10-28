// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRChaperoneSetup interface version 005.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRChaperoneSetup.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRChaperoneSetup_005 : public XRTVRChaperoneSetup_006, public vr::IVRChaperoneSetup_005
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRChaperoneSetup_005(XRTVRClientCore_003 *core) : XRTVRChaperoneSetup_006(core) {};
	// clang-format on

	FnTable_IVRChaperoneSetup_005();

	ForwardDeclareBase_IVRChaperoneSetup(XRTVRChaperoneSetup_006);

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
	SetWorkingCollisionBoundsTagsInfo(VR_ARRAY_COUNT(unTagCount) uint8_t *pTagsBuffer, uint32_t unTagCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetWorkingCollisionBoundsTagsInfo(pTagsBuffer=%p, unTagCount=%llu)",
		                         static_cast<const void *>(pTagsBuffer),
		                         static_cast<unsigned long long>(unTagCount));
	}

	bool
	GetLiveCollisionBoundsTagsInfo(VR_OUT_ARRAY_COUNT(punTagCount) uint8_t *pTagsBuffer,
	                               uint32_t *punTagCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetLiveCollisionBoundsTagsInfo(pTagsBuffer=%p, punTagCount=%p) -> false", false,
		    static_cast<const void *>(pTagsBuffer), static_cast<const void *>(punTagCount));
	}

	bool
	SetWorkingPhysicalBoundsInfo(VR_ARRAY_COUNT(unQuadsCount) HmdQuad_t *pQuadsBuffer,
	                             uint32_t unQuadsCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetWorkingPhysicalBoundsInfo(pQuadsBuffer=%p, unQuadsCount=%llu) -> false", false,
		    static_cast<const void *>(pQuadsBuffer), static_cast<unsigned long long>(unQuadsCount));
	}

	bool
	GetLivePhysicalBoundsInfo(VR_OUT_ARRAY_COUNT(punQuadsCount) HmdQuad_t *pQuadsBuffer,
	                          uint32_t *punQuadsCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetLivePhysicalBoundsInfo(pQuadsBuffer=%p, punQuadsCount=%p) -> false", false,
		    static_cast<const void *>(pQuadsBuffer), static_cast<const void *>(punQuadsCount));
	}

	Forward_IVRChaperoneSetup_ExportLiveToBuffer();
	Forward_IVRChaperoneSetup_ImportFromBufferToWorking();
};

}; // namespace xrt::state_trackers::openvr
