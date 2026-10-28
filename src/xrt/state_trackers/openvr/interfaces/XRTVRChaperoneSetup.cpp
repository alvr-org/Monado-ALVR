// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRChaperoneSetup interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRChaperoneSetup.hpp"

#include <cstring>


namespace xrt::state_trackers::openvr {

XRTVRChaperoneSetup_006::XRTVRChaperoneSetup_006(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

bool
XRTVRChaperoneSetup_006::CommitWorkingCopy(vr::EChaperoneConfigFile configFile)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "CommitWorkingCopy(configFile=%d) -> %d", false,
	                             static_cast<int>(configFile), false);
}

void
XRTVRChaperoneSetup_006::RevertWorkingCopy()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "RevertWorkingCopy()");
}

bool
XRTVRChaperoneSetup_006::GetWorkingPlayAreaSize(float *pSizeX, float *pSizeZ)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetWorkingPlayAreaSize(pSizeX=%p, pSizeZ=%p) -> %d", false,
	                             static_cast<void *>(pSizeX), static_cast<void *>(pSizeZ), false);
}

bool
XRTVRChaperoneSetup_006::GetWorkingPlayAreaRect(vr::HmdQuad_t *rect)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetWorkingPlayAreaRect(rect=%p) -> %d", false, static_cast<void *>(rect),
	                             false);
}

bool
XRTVRChaperoneSetup_006::GetWorkingCollisionBoundsInfo(VR_OUT_ARRAY_COUNT(punQuadsCount) vr::HmdQuad_t *pQuadsBuffer,
                                                       uint32_t *punQuadsCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetWorkingCollisionBoundsInfo(pQuadsBuffer=%p, punQuadsCount=%p) -> %d",
	                             false, static_cast<void *>(pQuadsBuffer), static_cast<void *>(punQuadsCount),
	                             false);
}

bool
XRTVRChaperoneSetup_006::GetLiveCollisionBoundsInfo(VR_OUT_ARRAY_COUNT(punQuadsCount) vr::HmdQuad_t *pQuadsBuffer,
                                                    uint32_t *punQuadsCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetLiveCollisionBoundsInfo(pQuadsBuffer=%p, punQuadsCount=%p) -> %d",
	                             false, static_cast<void *>(pQuadsBuffer), static_cast<void *>(punQuadsCount),
	                             false);
}

bool
XRTVRChaperoneSetup_006::GetWorkingSeatedZeroPoseToRawTrackingPose(
    vr::HmdMatrix34_t *pmatSeatedZeroPoseToRawTrackingPose)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetWorkingSeatedZeroPoseToRawTrackingPose(pmatSeatedZeroPoseToRawTrackingPose=%p) -> %d", false,
	    static_cast<void *>(pmatSeatedZeroPoseToRawTrackingPose), false);
}

bool
XRTVRChaperoneSetup_006::GetWorkingStandingZeroPoseToRawTrackingPose(
    vr::HmdMatrix34_t *pmatStandingZeroPoseToRawTrackingPose)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetWorkingStandingZeroPoseToRawTrackingPose(pmatStandingZeroPoseToRawTrackingPose=%p) -> %d",
	    false, static_cast<void *>(pmatStandingZeroPoseToRawTrackingPose), false);
}

void
XRTVRChaperoneSetup_006::SetWorkingPlayAreaSize(float sizeX, float sizeZ)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetWorkingPlayAreaSize(sizeX=%f, sizeZ=%f)", sizeX, sizeZ);
}

void
XRTVRChaperoneSetup_006::SetWorkingCollisionBoundsInfo(VR_ARRAY_COUNT(unQuadsCount) vr::HmdQuad_t *pQuadsBuffer,
                                                       uint32_t unQuadsCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetWorkingCollisionBoundsInfo(pQuadsBuffer=%p, unQuadsCount=%u)",
	                         static_cast<void *>(pQuadsBuffer), static_cast<unsigned int>(unQuadsCount));
}

void
XRTVRChaperoneSetup_006::SetWorkingPerimeter(VR_ARRAY_COUNT(unPointCount) const vr::HmdVector2_t *pPointBuffer,
                                             uint32_t unPointCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetWorkingPerimeter(pPointBuffer=%p, unPointCount=%u)",
	                         static_cast<const void *>(pPointBuffer), static_cast<unsigned int>(unPointCount));
}

void
XRTVRChaperoneSetup_006::SetWorkingSeatedZeroPoseToRawTrackingPose(
    const vr::HmdMatrix34_t *pMatSeatedZeroPoseToRawTrackingPose)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger,
	                         "SetWorkingSeatedZeroPoseToRawTrackingPose(pMatSeatedZeroPoseToRawTrackingPose=%p)",
	                         static_cast<const void *>(pMatSeatedZeroPoseToRawTrackingPose));
}

void
XRTVRChaperoneSetup_006::SetWorkingStandingZeroPoseToRawTrackingPose(
    const vr::HmdMatrix34_t *pMatStandingZeroPoseToRawTrackingPose)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(
	    logger, "SetWorkingStandingZeroPoseToRawTrackingPose(pMatStandingZeroPoseToRawTrackingPose=%p)",
	    static_cast<const void *>(pMatStandingZeroPoseToRawTrackingPose));
}

void
XRTVRChaperoneSetup_006::ReloadFromDisk(vr::EChaperoneConfigFile configFile)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "ReloadFromDisk(configFile=%d)", static_cast<int>(configFile));
}

bool
XRTVRChaperoneSetup_006::GetLiveSeatedZeroPoseToRawTrackingPose(vr::HmdMatrix34_t *pmatSeatedZeroPoseToRawTrackingPose)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetLiveSeatedZeroPoseToRawTrackingPose(pmatSeatedZeroPoseToRawTrackingPose=%p) -> %d", false,
	    static_cast<void *>(pmatSeatedZeroPoseToRawTrackingPose), false);
}

bool
XRTVRChaperoneSetup_006::ExportLiveToBuffer(VR_OUT_STRING() char *pBuffer, uint32_t *pnBufferLength)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ExportLiveToBuffer(pBuffer=%p, pnBufferLength=%p) -> %d", false,
	                             static_cast<void *>(pBuffer), static_cast<void *>(pnBufferLength), false);
}

bool
XRTVRChaperoneSetup_006::ImportFromBufferToWorking(const char *pBuffer, uint32_t nImportFlags)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ImportFromBufferToWorking(pBuffer=%p, nImportFlags=%u) -> %d", false,
	                             static_cast<const void *>(pBuffer), static_cast<unsigned int>(nImportFlags),
	                             false);
}

void
XRTVRChaperoneSetup_006::ShowWorkingSetPreview()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "ShowWorkingSetPreview()");
}

void
XRTVRChaperoneSetup_006::HideWorkingSetPreview()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "HideWorkingSetPreview()");
}

void
XRTVRChaperoneSetup_006::RoomSetupStarting()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "RoomSetupStarting()");
}

}; // namespace xrt::state_trackers::openvr
