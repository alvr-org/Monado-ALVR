// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVROverlay interface version 006.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVROverlay.hpp"
#include "XRTVROverlay_008.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVROverlay_006 : public XRTVROverlay_008, public vr::IVROverlay_006
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVROverlay_006(XRTVRClientCore_003 *core) : XRTVROverlay_008(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVROverlay_006.
		return nullptr;
	}

	ForwardDeclareBase_IVROverlay(XRTVROverlay_008);

	VROverlayError
	FindOverlay(const char *pchOverlayKey, VROverlayHandle_t *pOverlayHandle) override
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
	              VROverlayHandle_t *pOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "CreateOverlay(pchOverlayKey=%s, pchOverlayFriendlyName=%s, "
		                             "pOverlayHandle=%p)-> VROverlayError_None",
		                             VROverlayError::VROverlayError_None, pchOverlayKey, pchOverlayFriendlyName,
		                             static_cast<const void *>(pOverlayHandle));
	}

	VROverlayError
	DestroyOverlay(VROverlayHandle_t ulOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "DestroyOverlay(ulOverlayHandle=%llu)-> VROverlayError_None",
		                             VROverlayError::VROverlayError_None,
		                             static_cast<unsigned long long>(ulOverlayHandle));
	}

	VROverlayError
	SetHighQualityOverlay(VROverlayHandle_t ulOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetHighQualityOverlay(ulOverlayHandle=%llu)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle));
	}

	Forward_IVROverlay_GetHighQualityOverlay();

	uint32_t
	GetOverlayKey(VROverlayHandle_t ulOverlayHandle,
	              VR_OUT_STRING() char *pchValue,
	              uint32_t unBufferSize,
	              VROverlayError *pError = 0L) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetOverlayKey(ulOverlayHandle=%llu, pchValue=%p, unBufferSize=%llu, pError=%p)-> "
		    "VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pchValue), static_cast<unsigned long long>(unBufferSize),
		    static_cast<const void *>(pError));
	}

	uint32_t
	GetOverlayName(VROverlayHandle_t ulOverlayHandle,
	               VR_OUT_STRING() char *pchValue,
	               uint32_t unBufferSize,
	               VROverlayError *pError = 0L) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetOverlayName(ulOverlayHandle=%llu, pchValue=%p, unBufferSize=%llu, pError=%p)-> "
		    "VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pchValue), static_cast<unsigned long long>(unBufferSize),
		    static_cast<const void *>(pError));
	}

	VROverlayError
	GetOverlayImageData(VROverlayHandle_t ulOverlayHandle,
	                    void *pvBuffer,
	                    uint32_t unBufferSize,
	                    uint32_t *punWidth,
	                    uint32_t *punHeight) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetOverlayImageData(ulOverlayHandle=%llu, pvBuffer=%p, "
		                             "unBufferSize=%llu, punWidth=%p, punHeight=%p)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<const void *>(pvBuffer),
		                             static_cast<unsigned long long>(unBufferSize),
		                             static_cast<const void *>(punWidth), static_cast<const void *>(punHeight));
	}

	const char *
	GetOverlayErrorNameFromEnum(VROverlayError error) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayErrorNameFromEnum(error=%d)",
		                             "TODO: GetOverlayErrorNameFromEnum", static_cast<int>(error));
	}

	VROverlayError
	SetOverlayFlag(VROverlayHandle_t ulOverlayHandle, VROverlayFlags eOverlayFlag, bool bEnabled) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayFlag(ulOverlayHandle=%llu, eOverlayFlag=?, bEnabled=%d)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<int>(bEnabled));
	}

	VROverlayError
	GetOverlayFlag(VROverlayHandle_t ulOverlayHandle, VROverlayFlags eOverlayFlag, bool *pbEnabled) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetOverlayFlag(ulOverlayHandle=%llu, eOverlayFlag=?, pbEnabled=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pbEnabled));
	}

	VROverlayError
	SetOverlayColor(VROverlayHandle_t ulOverlayHandle, float fRed, float fGreen, float fBlue) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayColor(ulOverlayHandle=%llu, fRed=%f, fGreen=%f, fBlue=%f)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle), fRed, fGreen,
		    fBlue);
	}

	VROverlayError
	GetOverlayColor(VROverlayHandle_t ulOverlayHandle, float *pfRed, float *pfGreen, float *pfBlue) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetOverlayColor(ulOverlayHandle=%llu, pfRed=%p, pfGreen=%p, pfBlue=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pfRed), static_cast<const void *>(pfGreen),
		    static_cast<const void *>(pfBlue));
	}

	VROverlayError
	SetOverlayAlpha(VROverlayHandle_t ulOverlayHandle, float fAlpha) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayAlpha(ulOverlayHandle=%llu, fAlpha=%f)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle), fAlpha);
	}

	VROverlayError
	GetOverlayAlpha(VROverlayHandle_t ulOverlayHandle, float *pfAlpha) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetOverlayAlpha(ulOverlayHandle=%llu, pfAlpha=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pfAlpha));
	}

	VROverlayError
	SetOverlayGamma(VROverlayHandle_t ulOverlayHandle, float fGamma) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayGamma(ulOverlayHandle=%llu, fGamma=%f)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle), fGamma);
	}

	VROverlayError
	GetOverlayGamma(VROverlayHandle_t ulOverlayHandle, float *pfGamma) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetOverlayGamma(ulOverlayHandle=%llu, pfGamma=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pfGamma));
	}

	VROverlayError
	SetOverlayWidthInMeters(VROverlayHandle_t ulOverlayHandle, float fWidthInMeters) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayWidthInMeters(ulOverlayHandle=%llu, fWidthInMeters=%f)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle), fWidthInMeters);
	}

	VROverlayError
	GetOverlayWidthInMeters(VROverlayHandle_t ulOverlayHandle, float *pfWidthInMeters) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetOverlayWidthInMeters(ulOverlayHandle=%llu, pfWidthInMeters=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pfWidthInMeters));
	}

	VROverlayError
	SetOverlayAutoCurveDistanceRangeInMeters(VROverlayHandle_t ulOverlayHandle,
	                                         float fMinDistanceInMeters,
	                                         float fMaxDistanceInMeters) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "SetOverlayAutoCurveDistanceRangeInMeters(ulOverlayHandle=%llu, "
		                             "fMinDistanceInMeters=%f, fMaxDistanceInMeters=%f)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle), fMinDistanceInMeters,
		                             fMaxDistanceInMeters);
	}

	VROverlayError
	GetOverlayAutoCurveDistanceRangeInMeters(VROverlayHandle_t ulOverlayHandle,
	                                         float *pfMinDistanceInMeters,
	                                         float *pfMaxDistanceInMeters) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetOverlayAutoCurveDistanceRangeInMeters(ulOverlayHandle=%llu, "
		                             "pfMinDistanceInMeters=%p, pfMaxDistanceInMeters=%p)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<const void *>(pfMinDistanceInMeters),
		                             static_cast<const void *>(pfMaxDistanceInMeters));
	}

	VROverlayError
	SetOverlayTextureBounds(VROverlayHandle_t ulOverlayHandle,
	                        const VRTextureBounds_t *pOverlayTextureBounds) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "SetOverlayTextureBounds(ulOverlayHandle=%llu, pOverlayTextureBounds=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pOverlayTextureBounds));
	}

	VROverlayError
	GetOverlayTextureBounds(VROverlayHandle_t ulOverlayHandle, VRTextureBounds_t *pOverlayTextureBounds) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetOverlayTextureBounds(ulOverlayHandle=%llu, pOverlayTextureBounds=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pOverlayTextureBounds));
	}

	VROverlayError
	GetOverlayTransformType(VROverlayHandle_t ulOverlayHandle, VROverlayTransformType *peTransformType) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetOverlayTransformType(ulOverlayHandle=%llu, peTransformType=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(peTransformType));
	}

	VROverlayError
	SetOverlayTransformAbsolute(VROverlayHandle_t ulOverlayHandle,
	                            TrackingUniverseOrigin eTrackingOrigin,
	                            const HmdMatrix34_t *pmatTrackingOriginToOverlayTransform) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "SetOverlayTransformAbsolute(ulOverlayHandle=%llu, eTrackingOrigin=?, "
		                             "pmatTrackingOriginToOverlayTransform=%p)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<const void *>(pmatTrackingOriginToOverlayTransform));
	}

	VROverlayError
	GetOverlayTransformAbsolute(VROverlayHandle_t ulOverlayHandle,
	                            TrackingUniverseOrigin *peTrackingOrigin,
	                            HmdMatrix34_t *pmatTrackingOriginToOverlayTransform) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "GetOverlayTransformAbsolute(ulOverlayHandle=%llu, peTrackingOrigin=%p, "
		                             "pmatTrackingOriginToOverlayTransform=%p)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<const void *>(peTrackingOrigin),
		                             static_cast<const void *>(pmatTrackingOriginToOverlayTransform));
	}

	VROverlayError
	SetOverlayTransformTrackedDeviceRelative(VROverlayHandle_t ulOverlayHandle,
	                                         TrackedDeviceIndex_t unTrackedDevice,
	                                         const HmdMatrix34_t *pmatTrackedDeviceToOverlayTransform) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "SetOverlayTransformTrackedDeviceRelative(ulOverlayHandle=%llu, unTrackedDevice=%llu, "
		    "pmatTrackedDeviceToOverlayTransform=%p)",
		    {}, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<unsigned long long>(unTrackedDevice),
		    static_cast<const void *>(pmatTrackedDeviceToOverlayTransform));
	}

	VROverlayError
	GetOverlayTransformTrackedDeviceRelative(VROverlayHandle_t ulOverlayHandle,
	                                         TrackedDeviceIndex_t *punTrackedDevice,
	                                         HmdMatrix34_t *pmatTrackedDeviceToOverlayTransform) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetOverlayTransformTrackedDeviceRelative(ulOverlayHandle=%llu, punTrackedDevice=%p, "
		    "pmatTrackedDeviceToOverlayTransform=%p)",
		    {}, static_cast<unsigned long long>(ulOverlayHandle), static_cast<const void *>(punTrackedDevice),
		    static_cast<const void *>(pmatTrackedDeviceToOverlayTransform));
	}

	VROverlayError
	ShowOverlay(VROverlayHandle_t ulOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ShowOverlay(ulOverlayHandle=%llu)-> VROverlayError_None",
		                             VROverlayError::VROverlayError_None,
		                             static_cast<unsigned long long>(ulOverlayHandle));
	}

	VROverlayError
	HideOverlay(VROverlayHandle_t ulOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "HideOverlay(ulOverlayHandle=%llu)-> VROverlayError_None",
		                             VROverlayError::VROverlayError_None,
		                             static_cast<unsigned long long>(ulOverlayHandle));
	}

	Forward_IVROverlay_IsOverlayVisible();
	Forward_IVROverlay_PollNextOverlayEvent_008();

	VROverlayError
	GetOverlayInputMethod(VROverlayHandle_t ulOverlayHandle, VROverlayInputMethod *peInputMethod) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayInputMethod(ulOverlayHandle=%llu, peInputMethod=%p)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<const void *>(peInputMethod));
	}

	VROverlayError
	SetOverlayInputMethod(VROverlayHandle_t ulOverlayHandle, VROverlayInputMethod eInputMethod) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayInputMethod(ulOverlayHandle=%llu, eInputMethod=?)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle));
	}

	VROverlayError
	GetOverlayMouseScale(VROverlayHandle_t ulOverlayHandle, HmdVector2_t *pvecMouseScale) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayMouseScale(ulOverlayHandle=%llu, pvecMouseScale=%p)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<const void *>(pvecMouseScale));
	}

	VROverlayError
	SetOverlayMouseScale(VROverlayHandle_t ulOverlayHandle, const HmdVector2_t *pvecMouseScale) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayMouseScale(ulOverlayHandle=%llu, pvecMouseScale=%p)",
		                             {}, static_cast<unsigned long long>(ulOverlayHandle),
		                             static_cast<const void *>(pvecMouseScale));
	}

	Forward_IVROverlay_ComputeOverlayIntersection();
	Forward_IVROverlay_HandleControllerOverlayInteractionAsMouse_017();
	Forward_IVROverlay_IsHoverTargetOverlay();
	Forward_IVROverlay_GetGamepadFocusOverlay();

	VROverlayError
	SetGamepadFocusOverlay(VROverlayHandle_t ulNewFocusOverlay) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetGamepadFocusOverlay(ulNewFocusOverlay=%llu)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulNewFocusOverlay));
	}

	VROverlayError
	SetOverlayNeighbor(EOverlayDirection eDirection, VROverlayHandle_t ulFrom, VROverlayHandle_t ulTo) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayNeighbor(eDirection=?, ulFrom=%llu, ulTo=%llu)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulFrom),
		    static_cast<unsigned long long>(ulTo));
	}

	VROverlayError
	MoveGamepadFocusToNeighbor(EOverlayDirection eDirection, VROverlayHandle_t ulFrom) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "MoveGamepadFocusToNeighbor(eDirection=?, ulFrom=%llu)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulFrom));
	}

	VROverlayError
	SetOverlayTexture(VROverlayHandle_t ulOverlayHandle,
	                  GraphicsAPIConvention eTextureType,
	                  void *pTexture) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "SetOverlayTexture(ulOverlayHandle=%llu, eTextureType=?, pTexture=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pTexture));
	}

	VROverlayError
	ClearOverlayTexture(VROverlayHandle_t ulOverlayHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ClearOverlayTexture(ulOverlayHandle=%llu)-> VROverlayError_None",
		                             VROverlayError::VROverlayError_None,
		                             static_cast<unsigned long long>(ulOverlayHandle));
	}

	VROverlayError
	SetOverlayRaw(VROverlayHandle_t ulOverlayHandle,
	              void *pvBuffer,
	              uint32_t unWidth,
	              uint32_t unHeight,
	              uint32_t unDepth) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "SetOverlayRaw(ulOverlayHandle=%llu, pvBuffer=%p, unWidth=%llu, unHeight=%llu, unDepth=%llu)-> "
		    "VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(pvBuffer), static_cast<unsigned long long>(unWidth),
		    static_cast<unsigned long long>(unHeight), static_cast<unsigned long long>(unDepth));
	}

	VROverlayError
	SetOverlayFromFile(VROverlayHandle_t ulOverlayHandle, const char *pchFilePath) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "SetOverlayFromFile(ulOverlayHandle=%llu, pchFilePath=%s)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle), pchFilePath);
	}

	VROverlayError
	CreateDashboardOverlay(const char *pchOverlayKey,
	                       const char *pchOverlayFriendlyName,
	                       VROverlayHandle_t *pMainHandle,
	                       VROverlayHandle_t *pThumbnailHandle) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "CreateDashboardOverlay(pchOverlayKey=%s, pchOverlayFriendlyName=%s, "
		                             "pMainHandle=%p, pThumbnailHandle=%p)",
		                             {}, pchOverlayKey, pchOverlayFriendlyName,
		                             static_cast<const void *>(pMainHandle),
		                             static_cast<const void *>(pThumbnailHandle));
	}

	Forward_IVROverlay_IsDashboardVisible();
	Forward_IVROverlay_IsActiveDashboardOverlay();

	VROverlayError
	SetDashboardOverlaySceneProcess(VROverlayHandle_t ulOverlayHandle, uint32_t unProcessId) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "SetDashboardOverlaySceneProcess(ulOverlayHandle=%llu, unProcessId=%llu)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<unsigned long long>(unProcessId));
	}

	VROverlayError
	GetDashboardOverlaySceneProcess(VROverlayHandle_t ulOverlayHandle, uint32_t *punProcessId) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "GetDashboardOverlaySceneProcess(ulOverlayHandle=%llu, punProcessId=%p)-> VROverlayError_None",
		    VROverlayError::VROverlayError_None, static_cast<unsigned long long>(ulOverlayHandle),
		    static_cast<const void *>(punProcessId));
	}

	Forward_IVROverlay_ShowDashboard();

	VROverlayError
	ShowKeyboard(EGamepadTextInputMode eInputMode,
	             EGamepadTextInputLineMode eLineInputMode,
	             const char *pchDescription,
	             uint32_t unCharMax,
	             const char *pchExistingText,
	             bool bUseMinimalMode,
	             uint64_t uUserValue) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger,
		                             "ShowKeyboard(eInputMode=?, eLineInputMode=?, pchDescription=%s, "
		                             "unCharMax=%llu, pchExistingText=%s, bUseMinimalMode=%d, uUserValue=%llu)",
		                             {}, pchDescription, static_cast<unsigned long long>(unCharMax),
		                             pchExistingText, static_cast<int>(bUseMinimalMode),
		                             static_cast<unsigned long long>(uUserValue));
	}

	VROverlayError
	ShowKeyboardForOverlay(VROverlayHandle_t ulOverlayHandle,
	                       EGamepadTextInputMode eInputMode,
	                       EGamepadTextInputLineMode eLineInputMode,
	                       const char *pchDescription,
	                       uint32_t unCharMax,
	                       const char *pchExistingText,
	                       bool bUseMinimalMode,
	                       uint64_t uUserValue) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "ShowKeyboardForOverlay(ulOverlayHandle=%llu, eInputMode=?, eLineInputMode=?, pchDescription=%s, "
		    "unCharMax=%llu, pchExistingText=%s, bUseMinimalMode=%d, uUserValue=%llu)",
		    {}, static_cast<unsigned long long>(ulOverlayHandle), pchDescription,
		    static_cast<unsigned long long>(unCharMax), pchExistingText, static_cast<int>(bUseMinimalMode),
		    static_cast<unsigned long long>(uUserValue));
	}

	Forward_IVROverlay_GetKeyboardText();
	Forward_IVROverlay_HideKeyboard();
};

}; // namespace xrt::state_trackers::openvr
