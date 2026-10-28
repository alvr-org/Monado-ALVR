// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRHeadsetView_001 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRHeadsetView.hpp"


namespace xrt::state_trackers::openvr {

XRTVRHeadsetView_001::XRTVRHeadsetView_001(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

void
XRTVRHeadsetView_001::SetHeadsetViewSize(uint32_t nWidth, uint32_t nHeight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetHeadsetViewSize(nWidth=%llu, nHeight=%llu)",
	                         static_cast<unsigned long long>(nWidth), static_cast<unsigned long long>(nHeight));
}

void
XRTVRHeadsetView_001::GetHeadsetViewSize(uint32_t *pnWidth, uint32_t *pnHeight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "GetHeadsetViewSize(pnWidth=%p, pnHeight=%p)", static_cast<void *>(pnWidth),
	                         static_cast<void *>(pnHeight));
}

void
XRTVRHeadsetView_001::SetHeadsetViewMode(HeadsetViewMode_t eHeadsetViewMode)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetHeadsetViewMode(eHeadsetViewMode=%llu)",
	                         static_cast<unsigned long long>(eHeadsetViewMode));
}

HeadsetViewMode_t
XRTVRHeadsetView_001::GetHeadsetViewMode()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetHeadsetViewMode() -> %llu", static_cast<HeadsetViewMode_t>(0),
	                             static_cast<unsigned long long>(static_cast<HeadsetViewMode_t>(0)));
}

void
XRTVRHeadsetView_001::SetHeadsetViewCropped(bool bCropped)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetHeadsetViewCropped(bCropped=%d)", static_cast<int>(bCropped));
}

bool
XRTVRHeadsetView_001::GetHeadsetViewCropped()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetHeadsetViewCropped() -> %d", false, false);
}

float
XRTVRHeadsetView_001::GetHeadsetViewAspectRatio()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetHeadsetViewAspectRatio() -> %f", 0.0f, 0.0);
}

void
XRTVRHeadsetView_001::SetHeadsetViewBlendRange(float flStartPct, float flEndPct)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetHeadsetViewBlendRange(flStartPct=%f, flEndPct=%f)",
	                         static_cast<double>(flStartPct), static_cast<double>(flEndPct));
}

void
XRTVRHeadsetView_001::GetHeadsetViewBlendRange(float *pStartPct, float *pEndPct)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "GetHeadsetViewBlendRange(pStartPct=%p, pEndPct=%p)",
	                         static_cast<void *>(pStartPct), static_cast<void *>(pEndPct));
}

}; // namespace xrt::state_trackers::openvr
