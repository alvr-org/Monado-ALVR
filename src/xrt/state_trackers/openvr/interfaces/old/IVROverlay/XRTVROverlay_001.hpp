// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVROverlay interface version 001.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVROverlay.hpp"
#include "XRTVROverlay_005.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVROverlay_001 : public XRTVROverlay_005, public vr::IVROverlay_001
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVROverlay_001(XRTVRClientCore_003 *core) : XRTVROverlay_005(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVROverlay_001.
		return nullptr;
	}

	ForwardDeclareBase_IVROverlay(XRTVROverlay_005);

	VROverlayError
	FindOverlay(const char *pchOverlayKey, vr::VROverlayHandle_t *pOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "FindOverlay(pchOverlayKey=%s, pOverlayHandle=%p) -> VROverlayError_None",
		    VROverlayError::VROverlayError_None, pchOverlayKey, static_cast<const void *>(pOverlayHandle));
	}

	VROverlayError
	CreateOverlay(const char *pchOverlayKey,
	              const char *pchOverlayFriendlyName,
	              vr::VROverlayHandle_t *pOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "CreateOverlay(pchOverlayKey=%s, pchOverlayFriendlyName=%s, "
		                             "pOverlayHandle=%p) -> VROverlayError_None",
		                             VROverlayError::VROverlayError_None, pchOverlayKey, pchOverlayFriendlyName,
		                             static_cast<const void *>(pOverlayHandle));
	}

	Forward_IVROverlay_DestroyOverlay_006();
	Forward_IVROverlay_SetHighQualityOverlay_006();
	Forward_IVROverlay_GetHighQualityOverlay();
	Forward_IVROverlay_GetOverlayErrorNameFromEnum_006();
	Forward_IVROverlay_SetOverlayFlag_006();
	Forward_IVROverlay_GetOverlayFlag_006();
	Forward_IVROverlay_SetOverlayAlpha_006();
	Forward_IVROverlay_GetOverlayAlpha_006();
	Forward_IVROverlay_SetOverlayGamma();
	Forward_IVROverlay_GetOverlayGamma();
	Forward_IVROverlay_SetOverlayWidthInMeters_006();
	Forward_IVROverlay_GetOverlayWidthInMeters_006();
	Forward_IVROverlay_SetOverlayTextureBounds_006();
	Forward_IVROverlay_GetOverlayTextureBounds_006();
	Forward_IVROverlay_GetOverlayTransformType_006();
	Forward_IVROverlay_SetOverlayTransformAbsolute_006();
	Forward_IVROverlay_GetOverlayTransformAbsolute_006();
	Forward_IVROverlay_SetOverlayTransformTrackedDeviceRelative_006();
	Forward_IVROverlay_GetOverlayTransformTrackedDeviceRelative_006();

	VROverlayError
	GetOverlayVisibility(VROverlayHandle_t ulOverlayHandle, VROverlayVisibility *peOverlayVisibility) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetOverlayVisibility(ulOverlayHandle=%llu, peOverlayVisibility=%p) -> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(peOverlayVisibility));
	}

	VROverlayError
	SetOverlayVisibility(VROverlayHandle_t ulOverlayHandle, VROverlayVisibility eOverlayVisibility) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayVisibility(ulOverlayHandle=%llu, eOverlayVisibility=?) -> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle));
	}

	Forward_IVROverlay_ShowOverlay_006();
	Forward_IVROverlay_HideOverlay_006();
	Forward_IVROverlay_IsOverlayVisible();
	Forward_IVROverlay_PollNextOverlayEvent_008();
	Forward_IVROverlay_GetOverlayInputMethod_006();
	Forward_IVROverlay_SetOverlayInputMethod_006();
	Forward_IVROverlay_GetOverlayMouseScale_006();
	Forward_IVROverlay_SetOverlayMouseScale_006();
	Forward_IVROverlay_ComputeOverlayIntersection();

	bool
	HandleControllerOverlayInteractionAsMouse(VROverlayHandle_t ulOverlayHandle,
	                                          vr::TrackedDeviceIndex_t unControllerDeviceIndex) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "HandleControllerOverlayInteractionAsMouse(ulOverlayHandle=%llu, "
		                             "unControllerDeviceIndex=%llu) -> false",
		                             false, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<unsigned long long>(unControllerDeviceIndex));
	}

	VROverlayError
	SetOverlayTexture(VROverlayHandle_t ulOverlayHandle, void *pTexture) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayTexture(ulOverlayHandle=%llu, pTexture=%p) -> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pTexture));
	}

	Forward_IVROverlay_SetOverlayRaw_006();
	Forward_IVROverlay_SetOverlayFromFile_006();

	bool
	IsSystemOverlayVisible() override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsSystemOverlayVisible() -> false", false);
	}

	bool
	IsActiveSystemOverlay(VROverlayHandle_t ulOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsActiveSystemOverlay(ulOverlayHandle=%llu) -> false", false,
		                             static_cast<unsigned long long>(ulOverlayHandle));
	}

	VROverlayError
	SetSystemOverlaySceneProcess(VROverlayHandle_t ulOverlayHandle, uint32_t unProcessId) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "SetSystemOverlaySceneProcess(ulOverlayHandle=%llu, unProcessId=%llu) -> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<unsigned long long>(unProcessId));
	}

	VROverlayError
	GetSystemOverlaySceneProcess(VROverlayHandle_t ulOverlayHandle, uint32_t *punProcessId) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetSystemOverlaySceneProcess(ulOverlayHandle=%llu, punProcessId=%p) -> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(punProcessId));
	}
};

}; // namespace xrt::state_trackers::openvr
