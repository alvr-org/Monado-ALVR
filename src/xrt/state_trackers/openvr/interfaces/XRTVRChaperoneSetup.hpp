// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRChaperoneSetup interface version.
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

class XRTVRChaperoneSetup_006 : public vr::IVRChaperoneSetup_006
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRChaperoneSetup_006(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRChaperoneSetup_006() = default;

	FnTable_IVRChaperoneSetup_006();

	bool
	CommitWorkingCopy(vr::EChaperoneConfigFile configFile) override;

	void
	RevertWorkingCopy() override;

	bool
	GetWorkingPlayAreaSize(float *pSizeX, float *pSizeZ) override;

	bool
	GetWorkingPlayAreaRect(vr::HmdQuad_t *rect) override;

	bool
	GetWorkingCollisionBoundsInfo(VR_OUT_ARRAY_COUNT(punQuadsCount) vr::HmdQuad_t *pQuadsBuffer,
	                              uint32_t *punQuadsCount) override;

	bool
	GetLiveCollisionBoundsInfo(VR_OUT_ARRAY_COUNT(punQuadsCount) vr::HmdQuad_t *pQuadsBuffer,
	                           uint32_t *punQuadsCount) override;

	bool
	GetWorkingSeatedZeroPoseToRawTrackingPose(vr::HmdMatrix34_t *pmatSeatedZeroPoseToRawTrackingPose) override;

	bool
	GetWorkingStandingZeroPoseToRawTrackingPose(vr::HmdMatrix34_t *pmatStandingZeroPoseToRawTrackingPose) override;

	void
	SetWorkingPlayAreaSize(float sizeX, float sizeZ) override;

	void
	SetWorkingCollisionBoundsInfo(VR_ARRAY_COUNT(unQuadsCount) vr::HmdQuad_t *pQuadsBuffer,
	                              uint32_t unQuadsCount) override;

	void
	SetWorkingPerimeter(VR_ARRAY_COUNT(unPointCount) const vr::HmdVector2_t *pPointBuffer,
	                    uint32_t unPointCount) override;

	void
	SetWorkingSeatedZeroPoseToRawTrackingPose(
	    const vr::HmdMatrix34_t *pMatSeatedZeroPoseToRawTrackingPose) override;

	void
	SetWorkingStandingZeroPoseToRawTrackingPose(
	    const vr::HmdMatrix34_t *pMatStandingZeroPoseToRawTrackingPose) override;

	void
	ReloadFromDisk(vr::EChaperoneConfigFile configFile) override;

	bool
	GetLiveSeatedZeroPoseToRawTrackingPose(vr::HmdMatrix34_t *pmatSeatedZeroPoseToRawTrackingPose) override;

	bool
	ExportLiveToBuffer(VR_OUT_STRING() char *pBuffer, uint32_t *pnBufferLength) override;
	bool
	ImportFromBufferToWorking(const char *pBuffer, uint32_t nImportFlags) override;

	void
	ShowWorkingSetPreview() override;

	void
	HideWorkingSetPreview() override;

	void
	RoomSetupStarting() override;
};

} // namespace xrt::state_trackers::openvr
