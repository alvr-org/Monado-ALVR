// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRScreenshots_001 interface version.
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

class XRTVRScreenshots_001 : public vr::IVRScreenshots_001
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRScreenshots_001(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRScreenshots_001() = default;

	FnTable_IVRScreenshots_001();

	vr::EVRScreenshotError
	RequestScreenshot(vr::ScreenshotHandle_t *pOutScreenshotHandle,
	                  vr::EVRScreenshotType type,
	                  const char *pchPreviewFilename,
	                  const char *pchVRFilename) override;

	vr::EVRScreenshotError
	HookScreenshot(const vr::EVRScreenshotType *pSupportedTypes, int numTypes) override;

	vr::EVRScreenshotType
	GetScreenshotPropertyType(vr::ScreenshotHandle_t screenshotHandle, vr::EVRScreenshotError *pError) override;

	uint32_t
	GetScreenshotPropertyFilename(vr::ScreenshotHandle_t screenshotHandle,
	                              vr::EVRScreenshotPropertyFilenames filenameType,
	                              char *pchFilename,
	                              uint32_t cchFilename,
	                              vr::EVRScreenshotError *pError) override;

	vr::EVRScreenshotError
	UpdateScreenshotProgress(vr::ScreenshotHandle_t screenshotHandle, float flProgress) override;

	vr::EVRScreenshotError
	TakeStereoScreenshot(vr::ScreenshotHandle_t *pOutScreenshotHandle,
	                     const char *pchPreviewFilename,
	                     const char *pchVRFilename) override;

	vr::EVRScreenshotError
	SubmitScreenshot(vr::ScreenshotHandle_t screenshotHandle,
	                 vr::EVRScreenshotType type,
	                 const char *pchSourcePreviewFilename,
	                 const char *pchSourceVRFilename) override;
};

} // namespace xrt::state_trackers::openvr
