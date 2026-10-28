// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRSpatialAnchors_001 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRSpatialAnchors.hpp"


namespace xrt::state_trackers::openvr {

XRTVRSpatialAnchors_001::XRTVRSpatialAnchors_001(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

EVRSpatialAnchorError
XRTVRSpatialAnchors_001::CreateSpatialAnchorFromDescriptor(const char *pchDescriptor, SpatialAnchorHandle_t *pHandleOut)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "CreateSpatialAnchorFromDescriptor(pchDescriptor=%s, pHandleOut=%p) -> %llu",
	    static_cast<EVRSpatialAnchorError>(0), pchDescriptor != nullptr ? pchDescriptor : "(null)",
	    static_cast<void *>(pHandleOut), static_cast<unsigned long long>(static_cast<EVRSpatialAnchorError>(0)));
}

EVRSpatialAnchorError
XRTVRSpatialAnchors_001::CreateSpatialAnchorFromPose(TrackedDeviceIndex_t unDeviceIndex,
                                                     ETrackingUniverseOrigin eOrigin,
                                                     SpatialAnchorPose_t *pPose,
                                                     SpatialAnchorHandle_t *pHandleOut)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "CreateSpatialAnchorFromPose(unDeviceIndex=%llu, eOrigin=%llu, pPose=%p, pHandleOut=%p) -> %llu",
	    static_cast<EVRSpatialAnchorError>(0), static_cast<unsigned long long>(unDeviceIndex),
	    static_cast<unsigned long long>(eOrigin), static_cast<void *>(pPose), static_cast<void *>(pHandleOut),
	    static_cast<unsigned long long>(static_cast<EVRSpatialAnchorError>(0)));
}

EVRSpatialAnchorError
XRTVRSpatialAnchors_001::GetSpatialAnchorPose(SpatialAnchorHandle_t unHandle,
                                              ETrackingUniverseOrigin eOrigin,
                                              SpatialAnchorPose_t *pPoseOut)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetSpatialAnchorPose(unHandle=%llu, eOrigin=%llu, pPoseOut=%p) -> %llu",
	                             static_cast<EVRSpatialAnchorError>(0), static_cast<unsigned long long>(unHandle),
	                             static_cast<unsigned long long>(eOrigin), static_cast<void *>(pPoseOut),
	                             static_cast<unsigned long long>(static_cast<EVRSpatialAnchorError>(0)));
}

EVRSpatialAnchorError
XRTVRSpatialAnchors_001::GetSpatialAnchorDescriptor(SpatialAnchorHandle_t unHandle,
                                                    char *pchDescriptorOut,
                                                    uint32_t *punDescriptorBufferLenInOut)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetSpatialAnchorDescriptor(unHandle=%llu, pchDescriptorOut=%p, punDescriptorBufferLenInOut=%p) -> %llu",
	    static_cast<EVRSpatialAnchorError>(0), static_cast<unsigned long long>(unHandle),
	    static_cast<void *>(pchDescriptorOut), static_cast<void *>(punDescriptorBufferLenInOut),
	    static_cast<unsigned long long>(static_cast<EVRSpatialAnchorError>(0)));
}

}; // namespace xrt::state_trackers::openvr
