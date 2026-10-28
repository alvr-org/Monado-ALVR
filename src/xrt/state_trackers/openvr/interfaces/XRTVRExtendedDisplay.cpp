// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRExtendedDisplay_001 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRExtendedDisplay.hpp"


namespace xrt::state_trackers::openvr {

XRTVRExtendedDisplay_001::XRTVRExtendedDisplay_001(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

void
XRTVRExtendedDisplay_001::GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "GetWindowBounds(pnX=%p, pnY=%p, pnWidth=%p, pnHeight=%p)",
	                         static_cast<void *>(pnX), static_cast<void *>(pnY), static_cast<void *>(pnWidth),
	                         static_cast<void *>(pnHeight));
}

void
XRTVRExtendedDisplay_001::GetEyeOutputViewport(
    EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "GetEyeOutputViewport(eEye=%llu, pnX=%p, pnY=%p, pnWidth=%p, pnHeight=%p)",
	                         static_cast<unsigned long long>(eEye), static_cast<void *>(pnX),
	                         static_cast<void *>(pnY), static_cast<void *>(pnWidth), static_cast<void *>(pnHeight));
}

void
XRTVRExtendedDisplay_001::GetDXGIOutputInfo(int32_t *pnAdapterIndex, int32_t *pnAdapterOutputIndex)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "GetDXGIOutputInfo(pnAdapterIndex=%p, pnAdapterOutputIndex=%p)",
	                         static_cast<void *>(pnAdapterIndex), static_cast<void *>(pnAdapterOutputIndex));
}

}; // namespace xrt::state_trackers::openvr
