// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRScreenshots_001 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRScreenshots.hpp"


namespace xrt::state_trackers::openvr {

XRTVRScreenshots_001::XRTVRScreenshots_001(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

vr::EVRScreenshotError
XRTVRScreenshots_001::RequestScreenshot(vr::ScreenshotHandle_t *pOutScreenshotHandle,
                                        vr::EVRScreenshotType type,
                                        const char *pchPreviewFilename,
                                        const char *pchVRFilename)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "RequestScreenshot(pOutScreenshotHandle=%p, type=%llu, pchPreviewFilename=%s, pchVRFilename=%s) -> %llu",
	    static_cast<vr::EVRScreenshotError>(0), static_cast<void *>(pOutScreenshotHandle),
	    static_cast<unsigned long long>(type), pchPreviewFilename != nullptr ? pchPreviewFilename : "(null)",
	    pchVRFilename != nullptr ? pchVRFilename : "(null)",
	    static_cast<unsigned long long>(static_cast<vr::EVRScreenshotError>(0)));
}

vr::EVRScreenshotError
XRTVRScreenshots_001::HookScreenshot(const vr::EVRScreenshotType *pSupportedTypes, int numTypes)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "HookScreenshot(pSupportedTypes=%p, numTypes=%llu) -> %llu",
	                             static_cast<vr::EVRScreenshotError>(0), static_cast<const void *>(pSupportedTypes),
	                             static_cast<unsigned long long>(numTypes),
	                             static_cast<unsigned long long>(static_cast<vr::EVRScreenshotError>(0)));
}

vr::EVRScreenshotType
XRTVRScreenshots_001::GetScreenshotPropertyType(vr::ScreenshotHandle_t screenshotHandle, vr::EVRScreenshotError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetScreenshotPropertyType(screenshotHandle=%llu, pError=%p) -> %llu",
	                             static_cast<vr::EVRScreenshotType>(0),
	                             static_cast<unsigned long long>(screenshotHandle), static_cast<void *>(pError),
	                             static_cast<unsigned long long>(static_cast<vr::EVRScreenshotType>(0)));
}

uint32_t
XRTVRScreenshots_001::GetScreenshotPropertyFilename(vr::ScreenshotHandle_t screenshotHandle,
                                                    vr::EVRScreenshotPropertyFilenames filenameType,
                                                    char *pchFilename,
                                                    uint32_t cchFilename,
                                                    vr::EVRScreenshotError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetScreenshotPropertyFilename(screenshotHandle=%llu, filenameType=%llu, "
	                             "pchFilename=%p, cchFilename=%llu, pError=%p) -> %llu",
	                             static_cast<uint32_t>(0), static_cast<unsigned long long>(screenshotHandle),
	                             static_cast<unsigned long long>(filenameType), static_cast<void *>(pchFilename),
	                             static_cast<unsigned long long>(cchFilename), static_cast<void *>(pError),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

vr::EVRScreenshotError
XRTVRScreenshots_001::UpdateScreenshotProgress(vr::ScreenshotHandle_t screenshotHandle, float flProgress)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "UpdateScreenshotProgress(screenshotHandle=%llu, flProgress=%f) -> %llu",
	                             static_cast<vr::EVRScreenshotError>(0),
	                             static_cast<unsigned long long>(screenshotHandle), static_cast<double>(flProgress),
	                             static_cast<unsigned long long>(static_cast<vr::EVRScreenshotError>(0)));
}

vr::EVRScreenshotError
XRTVRScreenshots_001::TakeStereoScreenshot(vr::ScreenshotHandle_t *pOutScreenshotHandle,
                                           const char *pchPreviewFilename,
                                           const char *pchVRFilename)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "TakeStereoScreenshot(pOutScreenshotHandle=%p, pchPreviewFilename=%s, pchVRFilename=%s) -> %llu",
	    static_cast<vr::EVRScreenshotError>(0), static_cast<void *>(pOutScreenshotHandle),
	    pchPreviewFilename != nullptr ? pchPreviewFilename : "(null)",
	    pchVRFilename != nullptr ? pchVRFilename : "(null)",
	    static_cast<unsigned long long>(static_cast<vr::EVRScreenshotError>(0)));
}

vr::EVRScreenshotError
XRTVRScreenshots_001::SubmitScreenshot(vr::ScreenshotHandle_t screenshotHandle,
                                       vr::EVRScreenshotType type,
                                       const char *pchSourcePreviewFilename,
                                       const char *pchSourceVRFilename)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SubmitScreenshot(screenshotHandle=%llu, type=%llu, pchSourcePreviewFilename=%s, "
	                             "pchSourceVRFilename=%s) -> %llu",
	                             static_cast<vr::EVRScreenshotError>(0),
	                             static_cast<unsigned long long>(screenshotHandle),
	                             static_cast<unsigned long long>(type),
	                             pchSourcePreviewFilename != nullptr ? pchSourcePreviewFilename : "(null)",
	                             pchSourceVRFilename != nullptr ? pchSourceVRFilename : "(null)",
	                             static_cast<unsigned long long>(static_cast<vr::EVRScreenshotError>(0)));
}

}; // namespace xrt::state_trackers::openvr
