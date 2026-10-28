// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVROverlay_028 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVROverlay.hpp"


namespace xrt::state_trackers::openvr {

XRTVROverlay_028::XRTVROverlay_028(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

EVROverlayError
XRTVROverlay_028::FindOverlay(const char *pchOverlayKey, VROverlayHandle_t *pOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "FindOverlay(pchOverlayKey=%s, pOverlayHandle=%p) -> %llu", static_cast<EVROverlayError>(0),
	    pchOverlayKey != nullptr ? pchOverlayKey : "(null)", static_cast<void *>(pOverlayHandle),
	    static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::CreateOverlay(const char *pchOverlayKey,
                                const char *pchOverlayName,
                                VROverlayHandle_t *pOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "CreateOverlay(pchOverlayKey=%s, pchOverlayName=%s, pOverlayHandle=%p) -> %llu",
	    static_cast<EVROverlayError>(0), pchOverlayKey != nullptr ? pchOverlayKey : "(null)",
	    pchOverlayName != nullptr ? pchOverlayName : "(null)", static_cast<void *>(pOverlayHandle),
	    static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::CreateSubviewOverlay(VROverlayHandle_t parentOverlayHandle,
                                       const char *pchSubviewOverlayKey,
                                       const char *pchSubviewOverlayName,
                                       VROverlayHandle_t *pSubviewOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "CreateSubviewOverlay(parentOverlayHandle=%llu, pchSubviewOverlayKey=%s, "
	                             "pchSubviewOverlayName=%s, pSubviewOverlayHandle=%p) -> %llu",
	                             static_cast<EVROverlayError>(0),
	                             static_cast<unsigned long long>(parentOverlayHandle),
	                             pchSubviewOverlayKey != nullptr ? pchSubviewOverlayKey : "(null)",
	                             pchSubviewOverlayName != nullptr ? pchSubviewOverlayName : "(null)",
	                             static_cast<void *>(pSubviewOverlayHandle),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::DestroyOverlay(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "DestroyOverlay(ulOverlayHandle=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

uint32_t
XRTVROverlay_028::GetOverlayKey(VROverlayHandle_t ulOverlayHandle,
                                char *pchValue,
                                uint32_t unBufferSize,
                                EVROverlayError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetOverlayKey(ulOverlayHandle=%llu, pchValue=%p, unBufferSize=%llu, pError=%p) -> %llu",
	    static_cast<uint32_t>(0), static_cast<unsigned long long>(ulOverlayHandle), static_cast<void *>(pchValue),
	    static_cast<unsigned long long>(unBufferSize), static_cast<void *>(pError),
	    static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

uint32_t
XRTVROverlay_028::GetOverlayName(VROverlayHandle_t ulOverlayHandle,
                                 char *pchValue,
                                 uint32_t unBufferSize,
                                 EVROverlayError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetOverlayName(ulOverlayHandle=%llu, pchValue=%p, unBufferSize=%llu, pError=%p) -> %llu",
	    static_cast<uint32_t>(0), static_cast<unsigned long long>(ulOverlayHandle), static_cast<void *>(pchValue),
	    static_cast<unsigned long long>(unBufferSize), static_cast<void *>(pError),
	    static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayName(VROverlayHandle_t ulOverlayHandle, const char *pchName)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayName(ulOverlayHandle=%llu, pchName=%s) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             pchName != nullptr ? pchName : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayImageData(
    VROverlayHandle_t ulOverlayHandle, void *pvBuffer, uint32_t unBufferSize, uint32_t *punWidth, uint32_t *punHeight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetOverlayImageData(ulOverlayHandle=%llu, pvBuffer=%p, unBufferSize=%llu, "
	                             "punWidth=%p, punHeight=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pvBuffer), static_cast<unsigned long long>(unBufferSize),
	                             static_cast<void *>(punWidth), static_cast<void *>(punHeight),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

const char *
XRTVROverlay_028::GetOverlayErrorNameFromEnum(EVROverlayError error)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayErrorNameFromEnum(error=%llu) -> %s",
	                             "TODO: GetOverlayErrorNameFromEnum", static_cast<unsigned long long>(error),
	                             "TODO: GetOverlayErrorNameFromEnum");
}

EVROverlayError
XRTVROverlay_028::SetOverlayRenderingPid(VROverlayHandle_t ulOverlayHandle, uint32_t unPID)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayRenderingPid(ulOverlayHandle=%llu, unPID=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(unPID),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

uint32_t
XRTVROverlay_028::GetOverlayRenderingPid(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayRenderingPid(ulOverlayHandle=%llu) -> %llu",
	                             static_cast<uint32_t>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayFlag(VROverlayHandle_t ulOverlayHandle, VROverlayFlags eOverlayFlag, bool bEnabled)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayFlag(ulOverlayHandle=%llu, eOverlayFlag=%llu, bEnabled=%d) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(eOverlayFlag), static_cast<int>(bEnabled),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayFlag(VROverlayHandle_t ulOverlayHandle, VROverlayFlags eOverlayFlag, bool *pbEnabled)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetOverlayFlag(ulOverlayHandle=%llu, eOverlayFlag=%llu, pbEnabled=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(eOverlayFlag), static_cast<void *>(pbEnabled),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayFlags(VROverlayHandle_t ulOverlayHandle, uint32_t *pFlags)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayFlags(ulOverlayHandle=%llu, pFlags=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pFlags),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayColor(VROverlayHandle_t ulOverlayHandle, float fRed, float fGreen, float fBlue)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayColor(ulOverlayHandle=%llu, fRed=%f, fGreen=%f, fBlue=%f) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<double>(fRed), static_cast<double>(fGreen), static_cast<double>(fBlue),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayColor(VROverlayHandle_t ulOverlayHandle, float *pfRed, float *pfGreen, float *pfBlue)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetOverlayColor(ulOverlayHandle=%llu, pfRed=%p, pfGreen=%p, pfBlue=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<void *>(pfRed), static_cast<void *>(pfGreen), static_cast<void *>(pfBlue),
	    static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayAlpha(VROverlayHandle_t ulOverlayHandle, float fAlpha)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayAlpha(ulOverlayHandle=%llu, fAlpha=%f) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<double>(fAlpha),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayAlpha(VROverlayHandle_t ulOverlayHandle, float *pfAlpha)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayAlpha(ulOverlayHandle=%llu, pfAlpha=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pfAlpha),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayTexelAspect(VROverlayHandle_t ulOverlayHandle, float fTexelAspect)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayTexelAspect(ulOverlayHandle=%llu, fTexelAspect=%f) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<double>(fTexelAspect),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTexelAspect(VROverlayHandle_t ulOverlayHandle, float *pfTexelAspect)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayTexelAspect(ulOverlayHandle=%llu, pfTexelAspect=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pfTexelAspect),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlaySortOrder(VROverlayHandle_t ulOverlayHandle, uint32_t unSortOrder)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlaySortOrder(ulOverlayHandle=%llu, unSortOrder=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(unSortOrder),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlaySortOrder(VROverlayHandle_t ulOverlayHandle, uint32_t *punSortOrder)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlaySortOrder(ulOverlayHandle=%llu, punSortOrder=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(punSortOrder),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayWidthInMeters(VROverlayHandle_t ulOverlayHandle, float fWidthInMeters)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayWidthInMeters(ulOverlayHandle=%llu, fWidthInMeters=%f) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<double>(fWidthInMeters),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayWidthInMeters(VROverlayHandle_t ulOverlayHandle, float *pfWidthInMeters)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetOverlayWidthInMeters(ulOverlayHandle=%llu, pfWidthInMeters=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<void *>(pfWidthInMeters), static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayCurvature(VROverlayHandle_t ulOverlayHandle, float fCurvature)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayCurvature(ulOverlayHandle=%llu, fCurvature=%f) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<double>(fCurvature),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayCurvature(VROverlayHandle_t ulOverlayHandle, float *pfCurvature)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayCurvature(ulOverlayHandle=%llu, pfCurvature=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pfCurvature),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayPreCurvePitch(VROverlayHandle_t ulOverlayHandle, float fRadians)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayPreCurvePitch(ulOverlayHandle=%llu, fRadians=%f) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<double>(fRadians),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayPreCurvePitch(VROverlayHandle_t ulOverlayHandle, float *pfRadians)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayPreCurvePitch(ulOverlayHandle=%llu, pfRadians=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pfRadians),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayTextureColorSpace(VROverlayHandle_t ulOverlayHandle, EColorSpace eTextureColorSpace)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "SetOverlayTextureColorSpace(ulOverlayHandle=%llu, eTextureColorSpace=%llu) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<unsigned long long>(eTextureColorSpace),
	    static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTextureColorSpace(VROverlayHandle_t ulOverlayHandle, EColorSpace *peTextureColorSpace)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetOverlayTextureColorSpace(ulOverlayHandle=%llu, peTextureColorSpace=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<void *>(peTextureColorSpace), static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayTextureBounds(VROverlayHandle_t ulOverlayHandle,
                                          const VRTextureBounds_t *pOverlayTextureBounds)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayTextureBounds(ulOverlayHandle=%llu, pOverlayTextureBounds=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<const void *>(pOverlayTextureBounds),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTextureBounds(VROverlayHandle_t ulOverlayHandle, VRTextureBounds_t *pOverlayTextureBounds)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetOverlayTextureBounds(ulOverlayHandle=%llu, pOverlayTextureBounds=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pOverlayTextureBounds),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTransformType(VROverlayHandle_t ulOverlayHandle, VROverlayTransformType *peTransformType)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetOverlayTransformType(ulOverlayHandle=%llu, peTransformType=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<void *>(peTransformType), static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayTransformAbsolute(VROverlayHandle_t ulOverlayHandle,
                                              ETrackingUniverseOrigin eTrackingOrigin,
                                              const HmdMatrix34_t *pmatTrackingOriginToOverlayTransform)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayTransformAbsolute(ulOverlayHandle=%llu, eTrackingOrigin=%llu, "
	                             "pmatTrackingOriginToOverlayTransform=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(eTrackingOrigin),
	                             static_cast<const void *>(pmatTrackingOriginToOverlayTransform),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTransformAbsolute(VROverlayHandle_t ulOverlayHandle,
                                              ETrackingUniverseOrigin *peTrackingOrigin,
                                              HmdMatrix34_t *pmatTrackingOriginToOverlayTransform)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetOverlayTransformAbsolute(ulOverlayHandle=%llu, peTrackingOrigin=%p, "
	                             "pmatTrackingOriginToOverlayTransform=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(peTrackingOrigin),
	                             static_cast<void *>(pmatTrackingOriginToOverlayTransform),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayTransformTrackedDeviceRelative(VROverlayHandle_t ulOverlayHandle,
                                                           TrackedDeviceIndex_t unTrackedDevice,
                                                           const HmdMatrix34_t *pmatTrackedDeviceToOverlayTransform)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayTransformTrackedDeviceRelative(ulOverlayHandle=%llu, "
	                             "unTrackedDevice=%llu, pmatTrackedDeviceToOverlayTransform=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(unTrackedDevice),
	                             static_cast<const void *>(pmatTrackedDeviceToOverlayTransform),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTransformTrackedDeviceRelative(VROverlayHandle_t ulOverlayHandle,
                                                           TrackedDeviceIndex_t *punTrackedDevice,
                                                           HmdMatrix34_t *pmatTrackedDeviceToOverlayTransform)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetOverlayTransformTrackedDeviceRelative(ulOverlayHandle=%llu, "
	                             "punTrackedDevice=%p, pmatTrackedDeviceToOverlayTransform=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(punTrackedDevice),
	                             static_cast<void *>(pmatTrackedDeviceToOverlayTransform),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayTransformTrackedDeviceComponent(VROverlayHandle_t ulOverlayHandle,
                                                            TrackedDeviceIndex_t unDeviceIndex,
                                                            const char *pchComponentName)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayTransformTrackedDeviceComponent(ulOverlayHandle=%llu, "
	                             "unDeviceIndex=%llu, pchComponentName=%s) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(unDeviceIndex),
	                             pchComponentName != nullptr ? pchComponentName : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTransformTrackedDeviceComponent(VROverlayHandle_t ulOverlayHandle,
                                                            TrackedDeviceIndex_t *punDeviceIndex,
                                                            char *pchComponentName,
                                                            uint32_t unComponentNameSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetOverlayTransformTrackedDeviceComponent(ulOverlayHandle=%llu, "
	                             "punDeviceIndex=%p, pchComponentName=%p, unComponentNameSize=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(punDeviceIndex), static_cast<void *>(pchComponentName),
	                             static_cast<unsigned long long>(unComponentNameSize),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayTransformCursor(VROverlayHandle_t ulCursorOverlayHandle, const HmdVector2_t *pvHotspot)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "SetOverlayTransformCursor(ulCursorOverlayHandle=%llu, pvHotspot=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulCursorOverlayHandle),
	    static_cast<const void *>(pvHotspot), static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

vr::EVROverlayError
XRTVROverlay_028::GetOverlayTransformCursor(VROverlayHandle_t ulOverlayHandle, HmdVector2_t *pvHotspot)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayTransformCursor(ulOverlayHandle=%llu, pvHotspot=%p) -> %llu",
	                             static_cast<vr::EVROverlayError>(0),
	                             static_cast<unsigned long long>(ulOverlayHandle), static_cast<void *>(pvHotspot),
	                             static_cast<unsigned long long>(static_cast<vr::EVROverlayError>(0)));
}

vr::EVROverlayError
XRTVROverlay_028::SetOverlayTransformProjection(VROverlayHandle_t ulOverlayHandle,
                                                ETrackingUniverseOrigin eTrackingOrigin,
                                                const HmdMatrix34_t *pmatTrackingOriginToOverlayTransform,
                                                const VROverlayProjection_t *pProjection,
                                                vr::EVREye eEye)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayTransformProjection(ulOverlayHandle=%llu, eTrackingOrigin=%llu, "
	                             "pmatTrackingOriginToOverlayTransform=%p, pProjection=%p, eEye=%llu) -> %llu",
	                             static_cast<vr::EVROverlayError>(0),
	                             static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(eTrackingOrigin),
	                             static_cast<const void *>(pmatTrackingOriginToOverlayTransform),
	                             static_cast<const void *>(pProjection), static_cast<unsigned long long>(eEye),
	                             static_cast<unsigned long long>(static_cast<vr::EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetSubviewPosition(VROverlayHandle_t ulOverlayHandle, float fX, float fY)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetSubviewPosition(ulOverlayHandle=%llu, fX=%f, fY=%f) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<double>(fX), static_cast<double>(fY),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::ShowOverlay(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ShowOverlay(ulOverlayHandle=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::HideOverlay(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "HideOverlay(ulOverlayHandle=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

bool
XRTVROverlay_028::IsOverlayVisible(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsOverlayVisible(ulOverlayHandle=%llu) -> %d", false,
	                             static_cast<unsigned long long>(ulOverlayHandle), false);
}

EVROverlayError
XRTVROverlay_028::GetTransformForOverlayCoordinates(VROverlayHandle_t ulOverlayHandle,
                                                    ETrackingUniverseOrigin eTrackingOrigin,
                                                    HmdVector2_t coordinatesInOverlay,
                                                    HmdMatrix34_t *pmatTransform)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetTransformForOverlayCoordinates(ulOverlayHandle=%llu, eTrackingOrigin=%llu, coordinatesInOverlay=%p, "
	    "pmatTransform=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<unsigned long long>(eTrackingOrigin), static_cast<const void *>(&coordinatesInOverlay),
	    static_cast<void *>(pmatTransform), static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::WaitFrameSync(uint32_t nTimeoutMs)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "WaitFrameSync(nTimeoutMs=%llu) -> %llu", static_cast<EVROverlayError>(0),
	                             static_cast<unsigned long long>(nTimeoutMs),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

bool
XRTVROverlay_028::PollNextOverlayEvent(VROverlayHandle_t ulOverlayHandle, VREvent_t *pEvent, uint32_t uncbVREvent)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "PollNextOverlayEvent(ulOverlayHandle=%llu, pEvent=%p, uncbVREvent=%llu) -> %d",
	                             false, static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pEvent), static_cast<unsigned long long>(uncbVREvent), false);
}

EVROverlayError
XRTVROverlay_028::GetOverlayInputMethod(VROverlayHandle_t ulOverlayHandle, VROverlayInputMethod *peInputMethod)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayInputMethod(ulOverlayHandle=%llu, peInputMethod=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(peInputMethod),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayInputMethod(VROverlayHandle_t ulOverlayHandle, VROverlayInputMethod eInputMethod)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayInputMethod(ulOverlayHandle=%llu, eInputMethod=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(eInputMethod),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayMouseScale(VROverlayHandle_t ulOverlayHandle, HmdVector2_t *pvecMouseScale)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetOverlayMouseScale(ulOverlayHandle=%llu, pvecMouseScale=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pvecMouseScale),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayMouseScale(VROverlayHandle_t ulOverlayHandle, const HmdVector2_t *pvecMouseScale)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayMouseScale(ulOverlayHandle=%llu, pvecMouseScale=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<const void *>(pvecMouseScale),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

bool
XRTVROverlay_028::ComputeOverlayIntersection(VROverlayHandle_t ulOverlayHandle,
                                             const VROverlayIntersectionParams_t *pParams,
                                             VROverlayIntersectionResults_t *pResults)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "ComputeOverlayIntersection(ulOverlayHandle=%llu, pParams=%p, pResults=%p) -> %d",
	                             false, static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<const void *>(pParams), static_cast<void *>(pResults), false);
}

bool
XRTVROverlay_028::IsHoverTargetOverlay(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsHoverTargetOverlay(ulOverlayHandle=%llu) -> %d", false,
	                             static_cast<unsigned long long>(ulOverlayHandle), false);
}

EVROverlayError
XRTVROverlay_028::SetOverlayIntersectionMask(VROverlayHandle_t ulOverlayHandle,
                                             VROverlayIntersectionMaskPrimitive_t *pMaskPrimitives,
                                             uint32_t unNumMaskPrimitives,
                                             uint32_t unPrimitiveSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayIntersectionMask(ulOverlayHandle=%llu, pMaskPrimitives=%p, "
	                             "unNumMaskPrimitives=%llu, unPrimitiveSize=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pMaskPrimitives),
	                             static_cast<unsigned long long>(unNumMaskPrimitives),
	                             static_cast<unsigned long long>(unPrimitiveSize),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::TriggerLaserMouseHapticVibration(VROverlayHandle_t ulOverlayHandle,
                                                   float fDurationSeconds,
                                                   float fFrequency,
                                                   float fAmplitude)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "TriggerLaserMouseHapticVibration(ulOverlayHandle=%llu, fDurationSeconds=%f, "
	                             "fFrequency=%f, fAmplitude=%f) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<double>(fDurationSeconds), static_cast<double>(fFrequency),
	                             static_cast<double>(fAmplitude),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayCursor(VROverlayHandle_t ulOverlayHandle, VROverlayHandle_t ulCursorHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayCursor(ulOverlayHandle=%llu, ulCursorHandle=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(ulCursorHandle),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayCursorPositionOverride(VROverlayHandle_t ulOverlayHandle, const HmdVector2_t *pvCursor)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "SetOverlayCursorPositionOverride(ulOverlayHandle=%llu, pvCursor=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<const void *>(pvCursor), static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::ClearOverlayCursorPositionOverride(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ClearOverlayCursorPositionOverride(ulOverlayHandle=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayTexture(VROverlayHandle_t ulOverlayHandle, const Texture_t *pTexture)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayTexture(ulOverlayHandle=%llu, pTexture=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<const void *>(pTexture),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::ClearOverlayTexture(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ClearOverlayTexture(ulOverlayHandle=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayRaw(
    VROverlayHandle_t ulOverlayHandle, void *pvBuffer, uint32_t unWidth, uint32_t unHeight, uint32_t unBytesPerPixel)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetOverlayRaw(ulOverlayHandle=%llu, pvBuffer=%p, unWidth=%llu, unHeight=%llu, "
	                             "unBytesPerPixel=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pvBuffer), static_cast<unsigned long long>(unWidth),
	                             static_cast<unsigned long long>(unHeight),
	                             static_cast<unsigned long long>(unBytesPerPixel),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::SetOverlayFromFile(VROverlayHandle_t ulOverlayHandle, const char *pchFilePath)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetOverlayFromFile(ulOverlayHandle=%llu, pchFilePath=%s) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             pchFilePath != nullptr ? pchFilePath : "(null)",
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTexture(VROverlayHandle_t ulOverlayHandle,
                                    void **pNativeTextureHandle,
                                    void *pNativeTextureRef,
                                    uint32_t *pWidth,
                                    uint32_t *pHeight,
                                    uint32_t *pNativeFormat,
                                    ETextureType *pAPIType,
                                    EColorSpace *pColorSpace,
                                    VRTextureBounds_t *pTextureBounds)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetOverlayTexture(ulOverlayHandle=%llu, pNativeTextureHandle=%p, pNativeTextureRef=%p, pWidth=%p, "
	    "pHeight=%p, pNativeFormat=%p, pAPIType=%p, pColorSpace=%p, pTextureBounds=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<void *>(pNativeTextureHandle), static_cast<void *>(pNativeTextureRef),
	    static_cast<void *>(pWidth), static_cast<void *>(pHeight), static_cast<void *>(pNativeFormat),
	    static_cast<void *>(pAPIType), static_cast<void *>(pColorSpace), static_cast<void *>(pTextureBounds),
	    static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::ReleaseNativeOverlayHandle(VROverlayHandle_t ulOverlayHandle, void *pNativeTextureHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "ReleaseNativeOverlayHandle(ulOverlayHandle=%llu, pNativeTextureHandle=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<void *>(pNativeTextureHandle),
	    static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetOverlayTextureSize(VROverlayHandle_t ulOverlayHandle, uint32_t *pWidth, uint32_t *pHeight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetOverlayTextureSize(ulOverlayHandle=%llu, pWidth=%p, pHeight=%p) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<void *>(pWidth), static_cast<void *>(pHeight),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::CreateDashboardOverlay(const char *pchOverlayKey,
                                         const char *pchOverlayFriendlyName,
                                         VROverlayHandle_t *pMainHandle,
                                         VROverlayHandle_t *pThumbnailHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "CreateDashboardOverlay(pchOverlayKey=%s, pchOverlayFriendlyName=%s, pMainHandle=%p, pThumbnailHandle=%p) "
	    "-> %llu",
	    static_cast<EVROverlayError>(0), pchOverlayKey != nullptr ? pchOverlayKey : "(null)",
	    pchOverlayFriendlyName != nullptr ? pchOverlayFriendlyName : "(null)", static_cast<void *>(pMainHandle),
	    static_cast<void *>(pThumbnailHandle), static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

bool
XRTVROverlay_028::IsDashboardVisible()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsDashboardVisible() -> %d", false, false);
}

bool
XRTVROverlay_028::IsActiveDashboardOverlay(VROverlayHandle_t ulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsActiveDashboardOverlay(ulOverlayHandle=%llu) -> %d", false,
	                             static_cast<unsigned long long>(ulOverlayHandle), false);
}

EVROverlayError
XRTVROverlay_028::SetDashboardOverlaySceneProcess(VROverlayHandle_t ulOverlayHandle, uint32_t unProcessId)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "SetDashboardOverlaySceneProcess(ulOverlayHandle=%llu, unProcessId=%llu) -> %llu",
	                             static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	                             static_cast<unsigned long long>(unProcessId),
	                             static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::GetDashboardOverlaySceneProcess(VROverlayHandle_t ulOverlayHandle, uint32_t *punProcessId)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetDashboardOverlaySceneProcess(ulOverlayHandle=%llu, punProcessId=%p) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<void *>(punProcessId), static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

void
XRTVROverlay_028::ShowDashboard(const char *pchOverlayToShow)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "ShowDashboard(pchOverlayToShow=%s)",
	                         pchOverlayToShow != nullptr ? pchOverlayToShow : "(null)");
}

vr::TrackedDeviceIndex_t
XRTVROverlay_028::GetPrimaryDashboardDevice()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetPrimaryDashboardDevice() -> %llu",
	                             static_cast<vr::TrackedDeviceIndex_t>(0),
	                             static_cast<unsigned long long>(static_cast<vr::TrackedDeviceIndex_t>(0)));
}

EVROverlayError
XRTVROverlay_028::ShowKeyboard(EGamepadTextInputMode eInputMode,
                               EGamepadTextInputLineMode eLineInputMode,
                               uint32_t unFlags,
                               const char *pchDescription,
                               uint32_t unCharMax,
                               const char *pchExistingText,
                               uint64_t uUserValue)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "ShowKeyboard(eInputMode=%llu, eLineInputMode=%llu, unFlags=%llu, pchDescription=%s, unCharMax=%llu, "
	    "pchExistingText=%s, uUserValue=%llu) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(eInputMode),
	    static_cast<unsigned long long>(eLineInputMode), static_cast<unsigned long long>(unFlags),
	    pchDescription != nullptr ? pchDescription : "(null)", static_cast<unsigned long long>(unCharMax),
	    pchExistingText != nullptr ? pchExistingText : "(null)", static_cast<unsigned long long>(uUserValue),
	    static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

EVROverlayError
XRTVROverlay_028::ShowKeyboardForOverlay(VROverlayHandle_t ulOverlayHandle,
                                         EGamepadTextInputMode eInputMode,
                                         EGamepadTextInputLineMode eLineInputMode,
                                         uint32_t unFlags,
                                         const char *pchDescription,
                                         uint32_t unCharMax,
                                         const char *pchExistingText,
                                         uint64_t uUserValue)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "ShowKeyboardForOverlay(ulOverlayHandle=%llu, eInputMode=%llu, eLineInputMode=%llu, unFlags=%llu, "
	    "pchDescription=%s, unCharMax=%llu, pchExistingText=%s, uUserValue=%llu) -> %llu",
	    static_cast<EVROverlayError>(0), static_cast<unsigned long long>(ulOverlayHandle),
	    static_cast<unsigned long long>(eInputMode), static_cast<unsigned long long>(eLineInputMode),
	    static_cast<unsigned long long>(unFlags), pchDescription != nullptr ? pchDescription : "(null)",
	    static_cast<unsigned long long>(unCharMax), pchExistingText != nullptr ? pchExistingText : "(null)",
	    static_cast<unsigned long long>(uUserValue),
	    static_cast<unsigned long long>(static_cast<EVROverlayError>(0)));
}

uint32_t
XRTVROverlay_028::GetKeyboardText(char *pchText, uint32_t cchText)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetKeyboardText(pchText=%p, cchText=%llu) -> %llu",
	                             static_cast<uint32_t>(0), static_cast<void *>(pchText),
	                             static_cast<unsigned long long>(cchText),
	                             static_cast<unsigned long long>(static_cast<uint32_t>(0)));
}

void
XRTVROverlay_028::HideKeyboard()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "HideKeyboard()");
}

void
XRTVROverlay_028::SetKeyboardTransformAbsolute(ETrackingUniverseOrigin eTrackingOrigin,
                                               const HmdMatrix34_t *pmatTrackingOriginToKeyboardTransform)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(
	    logger, "SetKeyboardTransformAbsolute(eTrackingOrigin=%llu, pmatTrackingOriginToKeyboardTransform=%p)",
	    static_cast<unsigned long long>(eTrackingOrigin),
	    static_cast<const void *>(pmatTrackingOriginToKeyboardTransform));
}

void
XRTVROverlay_028::SetKeyboardPositionForOverlay(VROverlayHandle_t ulOverlayHandle, HmdRect2_t avoidRect)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetKeyboardPositionForOverlay(ulOverlayHandle=%llu, avoidRect=%p)",
	                         static_cast<unsigned long long>(ulOverlayHandle),
	                         static_cast<const void *>(&avoidRect));
}

VRMessageOverlayResponse
XRTVROverlay_028::ShowMessageOverlay(const char *pchText,
                                     const char *pchCaption,
                                     const char *pchButton0Text,
                                     const char *pchButton1Text,
                                     const char *pchButton2Text,
                                     const char *pchButton3Text)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "ShowMessageOverlay(pchText=%s, pchCaption=%s, pchButton0Text=%s, "
	                             "pchButton1Text=%s, pchButton2Text=%s, pchButton3Text=%s) -> %llu",
	                             static_cast<VRMessageOverlayResponse>(0), pchText != nullptr ? pchText : "(null)",
	                             pchCaption != nullptr ? pchCaption : "(null)",
	                             pchButton0Text != nullptr ? pchButton0Text : "(null)",
	                             pchButton1Text != nullptr ? pchButton1Text : "(null)",
	                             pchButton2Text != nullptr ? pchButton2Text : "(null)",
	                             pchButton3Text != nullptr ? pchButton3Text : "(null)",
	                             static_cast<unsigned long long>(static_cast<VRMessageOverlayResponse>(0)));
}

void
XRTVROverlay_028::CloseMessageOverlay()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "CloseMessageOverlay()");
}

}; // namespace xrt::state_trackers::openvr
