// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRCompositor interface version 005.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRCompositor.hpp"
#include "XRTVRCompositor_007.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

// @note: We want to base this off 006, but we can't because Submit has the same signature with only differing return
//        types. C++ moment that I cannot find a workaround to.
class XRTVRCompositor_005 : public XRTVRCompositor_007, public vr::IVRCompositor_005
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRCompositor_005(XRTVRClientCore_003 *core) : XRTVRCompositor_007(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRCompositor_005.
		return nullptr;
	}

	ForwardDeclareBase_IVRCompositor(XRTVRCompositor_007);

	Forward_IVRCompositor_GetLastError();
	Forward_IVRCompositor_SetVSync();
	Forward_IVRCompositor_GetVSync();
	Forward_IVRCompositor_SetGamma();
	Forward_IVRCompositor_GetGamma();

	void
	SetGraphicsDevice(Compositor_DeviceType eType, void *pDevice) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetGraphicsDevice(eType=%d, pDevice=%p)", static_cast<int>(eType),
		                         static_cast<const void *>(pDevice));
	}

	void
	WaitGetPoses(VR_ARRAY_COUNT(unPoseArrayCount) TrackedDevicePose_t *pPoseArray,
	             uint32_t unPoseArrayCount) override
	{
		// @todo This needs to save the error to be passed back by GetLastError
		(void)ForwardBase_IVRCompositor::WaitGetPoses(pPoseArray, unPoseArrayCount, nullptr, 0);
	}

	void
	Submit(Hmd_Eye eEye, void *pTexture, Compositor_TextureBounds *pBounds) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		// @todo Track what graphics API they choose in @ref SetGraphicsDevice and use that to submit for the
		//       right texture type. For now we don't really care about IVRCompositor_005 until some apps hits
		//       it.

		OPENVR_LOG_UNIMPLEMENTED(logger, "Submit(eEye=%d, pTexture=%p, pBounds=%p)", static_cast<int>(eEye),
		                         static_cast<const void *>(pTexture), static_cast<const void *>(pBounds));
	}

	Forward_IVRCompositor_ClearLastSubmittedFrame();

	void
	GetOverlayDefaults(Compositor_OverlaySettings *pSettings) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		// Zero out for now
		U_ZERO(pSettings);

		OPENVR_LOG_UNIMPLEMENTED(logger, "GetOverlayDefaults(pSettings=%p)",
		                         static_cast<const void *>(pSettings));
	}

	void
	SetOverlay(void *pTexture, Compositor_OverlaySettings *pSettings) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetOverlay(pTexture=%p, pSettings=%p)",
		                         static_cast<const void *>(pTexture), static_cast<const void *>(pSettings));
	}

	void
	SetOverlayRaw(void *buffer,
	              uint32_t width,
	              uint32_t height,
	              uint32_t depth,
	              Compositor_OverlaySettings *pSettings) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger,
		                         "SetOverlayRaw(buffer=%p, width=%llu, height=%llu, depth=%llu, pSettings=%p)",
		                         static_cast<const void *>(buffer), static_cast<unsigned long long>(width),
		                         static_cast<unsigned long long>(height),
		                         static_cast<unsigned long long>(depth), static_cast<const void *>(pSettings));
	}

	void
	SetOverlayFromFile(const char *pchFilePath, Compositor_OverlaySettings *pSettings) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "SetOverlayFromFile(pchFilePath=%s, pSettings=%p)", pchFilePath,
		                         static_cast<const void *>(pSettings));
	}

	void
	ClearOverlay() override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "ClearOverlay()");
	}

	Forward_IVRCompositor_GetFrameTiming();
	Forward_IVRCompositor_FadeToColor();
	Forward_IVRCompositor_FadeGrid_026();
	Forward_IVRCompositor_CompositorBringToFront();
	Forward_IVRCompositor_CompositorGoToBack();
	Forward_IVRCompositor_CompositorQuit();
	Forward_IVRCompositor_IsFullscreen();

	bool
	ComputeOverlayIntersection(const Compositor_OverlaySettings *pSettings,
	                           float fAspectRatio,
	                           vr::TrackingUniverseOrigin eOrigin,
	                           vr::HmdVector3_t vSource,
	                           vr::HmdVector3_t vDirection,
	                           vr::HmdVector2_t *pvecIntersectionUV,
	                           vr::HmdVector3_t *pvecIntersectionTrackingSpace) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger,
		    "ComputeOverlayIntersection(pSettings=%p, fAspectRatio=%f, eOrigin=%d, vSource=?, vDirection=?, "
		    "pvecIntersectionUV=%p, pvecIntersectionTrackingSpace=%p) -> false",
		    false, static_cast<const void *>(pSettings), fAspectRatio, static_cast<int>(eOrigin),
		    static_cast<const void *>(pvecIntersectionUV),
		    static_cast<const void *>(pvecIntersectionTrackingSpace));
	}

	Forward_IVRCompositor_SetTrackingSpace_008();
	Forward_IVRCompositor_GetTrackingSpace_008();
};

}; // namespace xrt::state_trackers::openvr
