// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRSpatialAnchors_001 interface version.
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

class XRTVRSpatialAnchors_001 : public vr::IVRSpatialAnchors_001
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRSpatialAnchors_001(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRSpatialAnchors_001() = default;

	FnTable_IVRSpatialAnchors_001();

	EVRSpatialAnchorError
	CreateSpatialAnchorFromDescriptor(const char *pchDescriptor, SpatialAnchorHandle_t *pHandleOut) override;

	EVRSpatialAnchorError
	CreateSpatialAnchorFromPose(TrackedDeviceIndex_t unDeviceIndex,
	                            ETrackingUniverseOrigin eOrigin,
	                            SpatialAnchorPose_t *pPose,
	                            SpatialAnchorHandle_t *pHandleOut) override;

	EVRSpatialAnchorError
	GetSpatialAnchorPose(SpatialAnchorHandle_t unHandle,
	                     ETrackingUniverseOrigin eOrigin,
	                     SpatialAnchorPose_t *pPoseOut) override;

	EVRSpatialAnchorError
	GetSpatialAnchorDescriptor(SpatialAnchorHandle_t unHandle,
	                           char *pchDescriptorOut,
	                           uint32_t *punDescriptorBufferLenInOut) override;
};

} // namespace xrt::state_trackers::openvr
