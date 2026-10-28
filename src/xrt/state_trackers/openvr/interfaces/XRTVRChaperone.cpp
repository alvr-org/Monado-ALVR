// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRChaperone interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRChaperone.hpp"

#include <cstring>


namespace xrt::state_trackers::openvr {

XRTVRChaperone_004::XRTVRChaperone_004(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

vr::ChaperoneCalibrationState
XRTVRChaperone_004::GetCalibrationState()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetCalibrationState() -> %d",
	                             vr::ChaperoneCalibrationState::ChaperoneCalibrationState_OK,
	                             vr::ChaperoneCalibrationState::ChaperoneCalibrationState_OK);
}

bool
XRTVRChaperone_004::GetPlayAreaSize(float *pSizeX, float *pSizeZ)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetPlayAreaSize(pSizeX=%p, pSizeZ=%p) -> %d", false,
	                             static_cast<void *>(pSizeX), static_cast<void *>(pSizeZ), false);
}

bool
XRTVRChaperone_004::GetPlayAreaRect(vr::HmdQuad_t *rect)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetPlayAreaRect(rect=%p) -> %d", false, static_cast<void *>(rect), false);
}

void
XRTVRChaperone_004::ReloadInfo(void)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "ReloadInfo()");
}

void
XRTVRChaperone_004::SetSceneColor(vr::HmdColor_t color)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetSceneColor(color={r=%f, g=%f, b=%f, a=%f})", color.r, color.g, color.b,
	                         color.a);
}

void
XRTVRChaperone_004::GetBoundsColor(vr::HmdColor_t *pOutputColorArray,
                                   int nNumOutputColors,
                                   float flCollisionBoundsFadeDistance,
                                   vr::HmdColor_t *pOutputCameraColor)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(
	    logger,
	    "GetBoundsColor(pOutputColorArray=%p, nNumOutputColors=%d, flCollisionBoundsFadeDistance=%f, "
	    "pOutputCameraColor=%p)",
	    static_cast<void *>(pOutputColorArray), nNumOutputColors, flCollisionBoundsFadeDistance,
	    static_cast<void *>(pOutputCameraColor));
}

bool
XRTVRChaperone_004::AreBoundsVisible()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "AreBoundsVisible() -> %d", false, false);
}

void
XRTVRChaperone_004::ForceBoundsVisible(bool bForce)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "ForceBoundsVisible(bForce=%d)", static_cast<int>(bForce));
}

void
XRTVRChaperone_004::ResetZeroPose(vr::ETrackingUniverseOrigin eTrackingUniverseOrigin)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "ResetZeroPose(eTrackingUniverseOrigin=%d)",
	                         static_cast<int>(eTrackingUniverseOrigin));
}

}; // namespace xrt::state_trackers::openvr
