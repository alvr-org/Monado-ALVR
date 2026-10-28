// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRRenderModels interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRRenderModels.hpp"

#include <cstring>


namespace xrt::state_trackers::openvr {

XRTVRRenderModels_006::XRTVRRenderModels_006(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

vr::EVRRenderModelError
XRTVRRenderModels_006::LoadRenderModel_Async(const char *pchRenderModelName, vr::RenderModel_t **ppRenderModel)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LoadRenderModel_Async(pchRenderModelName=%s, ppRenderModel=%p) -> %d",
	                             vr::EVRRenderModelError::VRRenderModelError_NotSupported,
	                             pchRenderModelName != nullptr ? pchRenderModelName : "(null)",
	                             static_cast<void *>(ppRenderModel),
	                             static_cast<int>(vr::EVRRenderModelError::VRRenderModelError_NotSupported));
}

void
XRTVRRenderModels_006::FreeRenderModel(vr::RenderModel_t *pRenderModel)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "FreeRenderModel(pRenderModel=%p)", static_cast<void *>(pRenderModel));
}

vr::EVRRenderModelError
XRTVRRenderModels_006::LoadTexture_Async(vr::TextureID_t textureId, vr::RenderModel_TextureMap_t **ppTexture)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LoadTexture_Async(textureId=%d, ppTexture=%p) -> %d",
	                             vr::EVRRenderModelError::VRRenderModelError_NotSupported,
	                             static_cast<int>(textureId), static_cast<void *>(ppTexture),
	                             static_cast<int>(vr::EVRRenderModelError::VRRenderModelError_NotSupported));
}

void
XRTVRRenderModels_006::FreeTexture(vr::RenderModel_TextureMap_t *pTexture)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "FreeTexture(pTexture=%p)", static_cast<void *>(pTexture));
}

vr::EVRRenderModelError
XRTVRRenderModels_006::LoadTextureD3D11_Async(vr::TextureID_t textureId, void *pD3D11Device, void **ppD3D11Texture2D)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "LoadTextureD3D11_Async(textureId=%d, pD3D11Device=%p, ppD3D11Texture2D=%p) -> %d",
	                             vr::EVRRenderModelError::VRRenderModelError_NotSupported,
	                             static_cast<int>(textureId), pD3D11Device, static_cast<void *>(ppD3D11Texture2D),
	                             static_cast<int>(vr::EVRRenderModelError::VRRenderModelError_NotSupported));
}

vr::EVRRenderModelError
XRTVRRenderModels_006::LoadIntoTextureD3D11_Async(vr::TextureID_t textureId, void *pDstTexture)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "LoadIntoTextureD3D11_Async(textureId=%d, pDstTexture=%p) -> %d",
	                             vr::EVRRenderModelError::VRRenderModelError_NotSupported,
	                             static_cast<int>(textureId), pDstTexture,
	                             static_cast<int>(vr::EVRRenderModelError::VRRenderModelError_NotSupported));
}

void
XRTVRRenderModels_006::FreeTextureD3D11(void *pD3D11Texture2D)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "FreeTextureD3D11(pD3D11Texture2D=%p)", pD3D11Texture2D);
}

uint32_t
XRTVRRenderModels_006::GetRenderModelName(uint32_t unRenderModelIndex,
                                          VR_OUT_STRING() char *pchRenderModelName,
                                          uint32_t unRenderModelNameLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (pchRenderModelName == nullptr) {
		return 0;
	}

	if (unRenderModelNameLen > 0) {
		pchRenderModelName[0] = '\0';
	}

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetRenderModelName(unRenderModelIndex=%u, pchRenderModelName=%p, "
	                             "unRenderModelNameLen=%u) -> %u",
	                             0U, static_cast<unsigned int>(unRenderModelIndex),
	                             static_cast<void *>(pchRenderModelName),
	                             static_cast<unsigned int>(unRenderModelNameLen), 0U);
}

uint32_t
XRTVRRenderModels_006::GetRenderModelCount()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetRenderModelCount() -> %u", 0U, 0U);
}


uint32_t
XRTVRRenderModels_006::GetComponentCount(const char *pchRenderModelName)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetComponentCount(pchRenderModelName=%s) -> %u", 0U,
	                             pchRenderModelName != nullptr ? pchRenderModelName : "(null)", 0U);
}

uint32_t
XRTVRRenderModels_006::GetComponentName(const char *pchRenderModelName,
                                        uint32_t unComponentIndex,
                                        VR_OUT_STRING() char *pchComponentName,
                                        uint32_t unComponentNameLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (unComponentNameLen > 0) {
		pchComponentName[0] = '\0';
	}

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetComponentName(pchRenderModelName=%s, unComponentIndex=%u, "
	                             "pchComponentName=%p, unComponentNameLen=%u) -> %u",
	                             0U, pchRenderModelName != nullptr ? pchRenderModelName : "(null)",
	                             static_cast<unsigned int>(unComponentIndex), static_cast<void *>(pchComponentName),
	                             static_cast<unsigned int>(unComponentNameLen), 0U);
}

uint64_t
XRTVRRenderModels_006::GetComponentButtonMask(const char *pchRenderModelName, const char *pchComponentName)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetComponentButtonMask(pchRenderModelName=%s, pchComponentName=%s) "
	                             "-> %llu",
	                             0ULL, pchRenderModelName != nullptr ? pchRenderModelName : "(null)",
	                             pchComponentName != nullptr ? pchComponentName : "(null)", 0ULL);
}

uint32_t
XRTVRRenderModels_006::GetComponentRenderModelName(const char *pchRenderModelName,
                                                   const char *pchComponentName,
                                                   VR_OUT_STRING() char *pchComponentRenderModelName,
                                                   uint32_t unComponentRenderModelNameLen)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (unComponentRenderModelNameLen > 0) {
		pchComponentRenderModelName[0] = '\0';
	}

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetComponentRenderModelName(pchRenderModelName=%s, pchComponentName=%s, "
	                             "pchComponentRenderModelName=%p, unComponentRenderModelNameLen=%u) -> %u",
	                             0U, pchRenderModelName != nullptr ? pchRenderModelName : "(null)",
	                             pchComponentName != nullptr ? pchComponentName : "(null)",
	                             static_cast<void *>(pchComponentRenderModelName),
	                             static_cast<unsigned int>(unComponentRenderModelNameLen), 0U);
}

bool
XRTVRRenderModels_006::GetComponentStateForDevicePath(const char *pchRenderModelName,
                                                      const char *pchComponentName,
                                                      vr::VRInputValueHandle_t devicePath,
                                                      const vr::RenderModel_ControllerMode_State_t *pState,
                                                      vr::RenderModel_ComponentState_t *pComponentState)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(pComponentState, 0, sizeof(*pComponentState));
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetComponentStateForDevicePath(pchRenderModelName=%s, pchComponentName=%s, devicePath=%llu, "
	    "pState=%p, pComponentState=%p) -> %d",
	    false, pchRenderModelName != nullptr ? pchRenderModelName : "(null)",
	    pchComponentName != nullptr ? pchComponentName : "(null)", static_cast<unsigned long long>(devicePath),
	    static_cast<const void *>(pState), static_cast<void *>(pComponentState), 0);
}

bool
XRTVRRenderModels_006::GetComponentState(const char *pchRenderModelName,
                                         const char *pchComponentName,
                                         const vr::VRControllerState_t *pControllerState,
                                         const vr::RenderModel_ControllerMode_State_t *pState,
                                         vr::RenderModel_ComponentState_t *pComponentState)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(pComponentState, 0, sizeof(*pComponentState));
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetComponentState(pchRenderModelName=%s, pchComponentName=%s, pControllerState=%p, "
	    "pState=%p, pComponentState=%p) -> %d",
	    false, pchRenderModelName != nullptr ? pchRenderModelName : "(null)",
	    pchComponentName != nullptr ? pchComponentName : "(null)", static_cast<const void *>(pControllerState),
	    static_cast<const void *>(pState), static_cast<void *>(pComponentState), 0);
}

bool
XRTVRRenderModels_006::RenderModelHasComponent(const char *pchRenderModelName, const char *pchComponentName)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "RenderModelHasComponent(pchRenderModelName=%s, pchComponentName=%s) -> %d", false,
	                             pchRenderModelName != nullptr ? pchRenderModelName : "(null)",
	                             pchComponentName != nullptr ? pchComponentName : "(null)", 0);
}

uint32_t
XRTVRRenderModels_006::GetRenderModelThumbnailURL(const char *pchRenderModelName,
                                                  VR_OUT_STRING() char *pchThumbnailURL,
                                                  uint32_t unThumbnailURLLen,
                                                  vr::EVRRenderModelError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(peError, vr::EVRRenderModelError::VRRenderModelError_NotSupported);
	if (unThumbnailURLLen > 0) {
		pchThumbnailURL[0] = '\0';
	}

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetRenderModelThumbnailURL(pchRenderModelName=%s, pchThumbnailURL=%p, unThumbnailURLLen=%u, "
	    "peError=%p) -> %u",
	    0U, pchRenderModelName != nullptr ? pchRenderModelName : "(null)", static_cast<void *>(pchThumbnailURL),
	    static_cast<unsigned int>(unThumbnailURLLen), static_cast<void *>(peError), 0U);
}

uint32_t
XRTVRRenderModels_006::GetRenderModelOriginalPath(const char *pchRenderModelName,
                                                  VR_OUT_STRING() char *pchOriginalPath,
                                                  uint32_t unOriginalPathLen,
                                                  vr::EVRRenderModelError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(peError, vr::EVRRenderModelError::VRRenderModelError_NotSupported);
	if (unOriginalPathLen > 0) {
		pchOriginalPath[0] = '\0';
	}

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetRenderModelOriginalPath(pchRenderModelName=%s, pchOriginalPath=%p, unOriginalPathLen=%u, "
	    "peError=%p) -> %u",
	    0U, pchRenderModelName != nullptr ? pchRenderModelName : "(null)", static_cast<void *>(pchOriginalPath),
	    static_cast<unsigned int>(unOriginalPathLen), static_cast<void *>(peError), 0U);
}

const char *
XRTVRRenderModels_006::GetRenderModelErrorNameFromEnum(vr::EVRRenderModelError error)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetRenderModelErrorNameFromEnum(error=%d) -> %s",
	                             "TODO: GetRenderModelErrorNameFromEnum", static_cast<int>(error),
	                             "TODO: GetRenderModelErrorNameFromEnum");
}

}; // namespace xrt::state_trackers::openvr
