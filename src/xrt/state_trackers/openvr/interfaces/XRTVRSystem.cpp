// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRSystem interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_instance.h"
#include "xrt/xrt_tracking.h"

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"
#include "common/openvr_math.hpp"

#include "XRTVRSystem.hpp"
#include "XRTVRClientCore.hpp"

#include <cstring>
#include <cmath>
#include <array>


namespace xrt::state_trackers::openvr {

XRTVRSystem_026::XRTVRSystem_026(XRTVRClientCore_003 *core) : core(core)
{
	// Nothing yet
}

void
XRTVRSystem_026::GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	// Get the head device
	auto maybe_device = this->core->devices->getDevice(vr::k_unTrackedDeviceIndex_Hmd);
	if (!maybe_device) {
		OPENVR_LOG_ERROR(logger, "GetProjectionRaw: HMD device not found");
		XRT_DEBUGBREAK();

		// We're in a bad situation, pick something that will prevent a crash/invalid memory stuff.
		*pnWidth = 1024;
		*pnHeight = 1024;

		return;
	}

	auto xdev = (*maybe_device)->xdev;

	// We unconditionally use view zero, since OpenVR doesn't support per-eye render target sizes.
	xrt_recommended_view_config recommended_view_config;
	xrt_result_t xret = xrt_app_system_get_recommended_view_configuration( //
	    this->core->xasys,                                                 //
	    XRT_VIEW_TYPE_STEREO,                                              //
	    &recommended_view_config);                                         //
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(
		    logger,
		    "Failed to get app system view configuration, this is likely fatal? Trying xrt_system_compositor",
		    xret);
	} else if (recommended_view_config.valid) {
		// We got a recommendation
		*pnWidth = recommended_view_config.views[0].width_pixels;
		*pnHeight = recommended_view_config.views[0].height_pixels;
		return;
	}

	xrt_view_config view_config;
	xret = xrt_syscomp_get_view_config(this->core->xsysc, XRT_VIEW_TYPE_STEREO, &view_config);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to get view configuration, this is likely fatal, using backup.",
		                      xret);

		*pnWidth = xdev->hmd->views[0].display.w_pixels;
		*pnHeight = xdev->hmd->views[0].display.h_pixels;
		return;
	}

	// Fallback for no recommendation.
	*pnWidth = view_config.views[0].recommended.width_pixels;
	*pnHeight = view_config.views[0].recommended.height_pixels;
}

vr::HmdMatrix44_t
XRTVRSystem_026::GetProjectionMatrix(vr::EVREye eEye, float fNearZ, float fFarZ)
{
	float fLeft, fRight, fTop, fBottom;
	this->GetProjectionRaw(eEye, &fLeft, &fRight, &fTop, &fBottom);

	// https://github.com/ValveSoftware/openvr/wiki/IVRSystem::GetProjectionRaw
	float idx = 1.0f / (fRight - fLeft);
	float idy = 1.0f / (fBottom - fTop);
	float idz = 1.0f / (fFarZ - fNearZ);
	float sx = fRight + fLeft;
	float sy = fBottom + fTop;

	vr::HmdMatrix44_t mat;
	// @todo Move this to just the matrix definition when clang-format is updated in CI
	// clang-format off
	float (*p)[4] = mat.m;

	p[0][0] = 2*idx; p[0][1] = 0;     p[0][2] = sx*idx;     p[0][3] = 0;
	p[1][0] = 0;     p[1][1] = 2*idy; p[1][2] = sy*idy;     p[1][3] = 0;
	p[2][0] = 0;     p[2][1] = 0;     p[2][2] = -fFarZ*idz; p[2][3] = -fFarZ*fNearZ*idz;
	p[3][0] = 0;     p[3][1] = 0;     p[3][2] = -1.0f;      p[3][3] = 0;
	// clang-format on

	return mat;
}

void
XRTVRSystem_026::GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	auto maybe_device = this->core->devices->getDevice(vr::k_unTrackedDeviceIndex_Hmd);
	if (!maybe_device) {
		OPENVR_LOG_ERROR(logger, "GetProjectionRaw: HMD device not found");
		XRT_DEBUGBREAK();

		*pfLeft = 0;
		*pfRight = 0;
		*pfTop = 0;
		*pfBottom = 0;

		return;
	}

	std::array<xrt_fov, 2> fovs;
	std::array<xrt_pose, 2> poses;
	xrt_pose head_relation;
	xrt_result_t xret = this->core->compositor->getFrameRenderState(fovs, poses, head_relation);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "GetProjectionRaw: Failed to get output data", xret);
		XRT_DEBUGBREAK();

		*pfLeft = 0;
		*pfRight = 0;
		*pfTop = 0;
		*pfBottom = 0;

		return;
	}

	*pfLeft = std::tan(fovs[eEye].angle_left);
	*pfRight = std::tan(fovs[eEye].angle_right);
	// OpenVR convention: pfTop is negative (geometric bottom), pfBottom is positive (geometric top)
	*pfTop = std::tan(fovs[eEye].angle_down);
	*pfBottom = std::tan(fovs[eEye].angle_up);
}

bool
XRTVRSystem_026::ComputeDistortion(vr::EVREye eEye,
                                   float fU,
                                   float fV,
                                   vr::DistortionCoordinates_t *pDistortionCoordinates)
{
	if (fU < 0.f || fU > 1.f || fV < 0.f || fV > 1.f) {
		(*pDistortionCoordinates) = {
		    .rfRed = {0.f, 0.f},
		    .rfGreen = {0.f, 0.f},
		    .rfBlue = {0.f, 0.f},
		};

		return false;
	}

	// Direct passthrough for identity distortion, since we don't support pre-distorted rendering.
	(*pDistortionCoordinates) = {
	    .rfRed = {fU, fV},
	    .rfGreen = {fU, fV},
	    .rfBlue = {fU, fV},
	};

	return true;
}

bool
XRTVRSystem_026::ComputeDistortionSet(vr::EVREye eEye,
                                      vr::EVRDistortionChannel eChannel,
                                      bool bAsNormalizedDeviceCoordinates,
                                      uint32_t nNumCoordinates,
                                      const vr::DistortionCoordinate_t *pInput,
                                      vr::DistortionCoordinate_t *pOutput)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (nNumCoordinates == 0) {
		return true;
	}

	// @note: SteamVR does not check for nullptr for either of these, and segfaults if they're null.
	if (pInput == nullptr) {
		OPENVR_LOG_ERROR(logger, "ComputeDistortionSet: pInput == nullptr");
		return false;
	}

	if (pOutput == nullptr) {
		OPENVR_LOG_ERROR(logger, "ComputeDistortionSet: pOutput == nullptr");
		return false;
	}

	for (uint32_t i = 0; i < nNumCoordinates; i++) {
		(*pOutput) = *pInput;
	}

	return true;
}

vr::HmdMatrix34_t
XRTVRSystem_026::GetEyeToHeadTransform(vr::EVREye eEye)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	auto maybe_device = this->core->devices->getDevice(vr::k_unTrackedDeviceIndex_Hmd);
	if (!maybe_device) {
		OPENVR_LOG_ERROR(logger, "GetProjectionRaw: HMD device not found");
		XRT_DEBUGBREAK();

		vr::HmdMatrix34_t mat;
		hmdMatrix34Identity(mat);
		return mat;
	}

	std::array<xrt_fov, 2> fovs;
	std::array<xrt_pose, 2> T_head_eyes;
	xrt_pose head_relation;
	xrt_result_t xret = this->core->compositor->getFrameRenderState(fovs, T_head_eyes, head_relation);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "GetEyeToHeadTransform: Failed to get output data", xret);
		XRT_DEBUGBREAK();

		vr::HmdMatrix34_t mat;
		hmdMatrix34Identity(mat);
		return mat;
	}

	vr::HmdMatrix34_t mat;
	xrtPoseToHmdMatrix34(T_head_eyes[eEye], mat);
	return mat;
}

bool
XRTVRSystem_026::GetTimeSinceLastVsync(float *pfSecondsSinceLastVsync, uint64_t *pulFrameCounter)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	*pfSecondsSinceLastVsync = 0;
	*pulFrameCounter = 0;
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetTimeSinceLastVsync(pfSecondsSinceLastVsync=%p, pulFrameCounter=%p) -> %d", false,
	    static_cast<void *>(pfSecondsSinceLastVsync), static_cast<void *>(pulFrameCounter), 0);
}

int32_t
XRTVRSystem_026::GetD3D9AdapterIndex()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetD3D9AdapterIndex() -> %d", -1, -1);
}

void
XRTVRSystem_026::GetDXGIOutputInfo(int32_t *pnAdapterIndex)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "GetDXGIOutputInfo(pnAdapterIndex=%p)", static_cast<void *>(pnAdapterIndex));

	*pnAdapterIndex = -1;
}

void
XRTVRSystem_026::GetOutputDevice(uint64_t *pnDevice, vr::ETextureType textureType, VkInstance_T *pInstance)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	this->core->compositor->getOutputDevice(logger, pnDevice, textureType, pInstance);
}

/* Display Mode methods */

bool
XRTVRSystem_026::IsDisplayOnDesktop()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsDisplayOnDesktop() -> %d", false, 0);
}

bool
XRTVRSystem_026::SetDisplayVisibility(bool bIsVisibleOnDesktop)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "SetDisplayVisibility(bIsVisibleOnDesktop=%d) -> %d", false,
	                             static_cast<int>(bIsVisibleOnDesktop), 0);
}

/* Tracking Methods */

void
XRTVRSystem_026::GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin eOrigin,
                                                 float fPredictedSecondsToPhotonsFromNow,
                                                 VR_ARRAY_COUNT(unTrackedDevicePoseArrayCount)
                                                     vr::TrackedDevicePose_t *pTrackedDevicePoseArray,
                                                 uint32_t unTrackedDevicePoseArrayCount)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger,
	                         "GetDeviceToAbsoluteTrackingPose(eOrigin=%d, fPredictedSecondsToPhotonsFromNow=%f, "
	                         "pTrackedDevicePoseArray=%p, unTrackedDevicePoseArrayCount=%u)",
	                         static_cast<int>(eOrigin), fPredictedSecondsToPhotonsFromNow,
	                         static_cast<void *>(pTrackedDevicePoseArray),
	                         static_cast<unsigned int>(unTrackedDevicePoseArrayCount));

	std::memset(pTrackedDevicePoseArray, 0, sizeof(*pTrackedDevicePoseArray) * unTrackedDevicePoseArrayCount);
}

vr::HmdMatrix34_t
XRTVRSystem_026::GetSeatedZeroPoseToStandingAbsoluteTrackingPose()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	vr::HmdMatrix34_t mat;
	hmdMatrix34Identity(mat);
	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetSeatedZeroPoseToStandingAbsoluteTrackingPose() -> %p", mat,
	                             static_cast<void *>(&mat));
}

vr::HmdMatrix34_t
XRTVRSystem_026::GetRawZeroPoseToStandingAbsoluteTrackingPose()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	vr::HmdMatrix34_t mat;
	hmdMatrix34Identity(mat);
	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetRawZeroPoseToStandingAbsoluteTrackingPose() -> %p", mat,
	                             static_cast<void *>(&mat));
}

uint32_t
XRTVRSystem_026::GetSortedTrackedDeviceIndicesOfClass(vr::ETrackedDeviceClass eTrackedDeviceClass,
                                                      VR_ARRAY_COUNT(unTrackedDeviceIndexArrayCount)
                                                          vr::TrackedDeviceIndex_t *punTrackedDeviceIndexArray,
                                                      uint32_t unTrackedDeviceIndexArrayCount,
                                                      vr::TrackedDeviceIndex_t unRelativeToTrackedDeviceIndex)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(punTrackedDeviceIndexArray, 0,
	            sizeof(*punTrackedDeviceIndexArray) * unTrackedDeviceIndexArrayCount);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetSortedTrackedDeviceIndicesOfClass(eTrackedDeviceClass=%d, punTrackedDeviceIndexArray=%p, "
	    "unTrackedDeviceIndexArrayCount=%u, unRelativeToTrackedDeviceIndex=%u) -> %u",
	    0U, static_cast<int>(eTrackedDeviceClass), static_cast<void *>(punTrackedDeviceIndexArray),
	    static_cast<unsigned int>(unTrackedDeviceIndexArrayCount),
	    static_cast<unsigned int>(unRelativeToTrackedDeviceIndex), 0U);
}

vr::EDeviceActivityLevel
XRTVRSystem_026::GetTrackedDeviceActivityLevel(vr::TrackedDeviceIndex_t unDeviceId)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	vr::EDeviceActivityLevel activity_level;
	if (!this->core->devices->getTrackedDeviceActivityLevel(logger, unDeviceId, activity_level)) {
		return vr::EDeviceActivityLevel::k_EDeviceActivityLevel_Unknown;
	}

	return activity_level;
}

void
XRTVRSystem_026::ApplyTransform(vr::TrackedDevicePose_t *pOutputPose,
                                const vr::TrackedDevicePose_t *pTrackedDevicePose,
                                const vr::HmdMatrix34_t *pTransform)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "ApplyTransform(pOutputPose=%p, pTrackedDevicePose=%p, pTransform=%p)",
	                         static_cast<void *>(pOutputPose), static_cast<const void *>(pTrackedDevicePose),
	                         static_cast<const void *>(pTransform));

	std::memset(pOutputPose, 0, sizeof(*pOutputPose));
}

vr::TrackedDeviceIndex_t
XRTVRSystem_026::GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole unDeviceType)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetTrackedDeviceIndexForControllerRole(unDeviceType=%d) -> %u",
	                             vr::k_unTrackedDeviceIndexInvalid, static_cast<int>(unDeviceType),
	                             static_cast<unsigned int>(vr::k_unTrackedDeviceIndexInvalid));
}

vr::ETrackedControllerRole
XRTVRSystem_026::GetControllerRoleForTrackedDeviceIndex(vr::TrackedDeviceIndex_t unDeviceIndex)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetControllerRoleForTrackedDeviceIndex(unDeviceIndex=%u) -> %d",
	                             vr::ETrackedControllerRole::TrackedControllerRole_Invalid,
	                             static_cast<unsigned int>(unDeviceIndex),
	                             static_cast<int>(vr::ETrackedControllerRole::TrackedControllerRole_Invalid));
}

/* Property methods */

vr::ETrackedDeviceClass
XRTVRSystem_026::GetTrackedDeviceClass(vr::TrackedDeviceIndex_t unDeviceIndex)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	auto maybe_device = this->core->devices->getDevice(unDeviceIndex);
	if (!maybe_device) {
		OPENVR_LOG_ERROR(logger, "GetTrackedDeviceClass: device %u not found", unDeviceIndex);
		return vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid;
	}

	return (*maybe_device)->tracked_class;
}

bool
XRTVRSystem_026::IsTrackedDeviceConnected(vr::TrackedDeviceIndex_t unDeviceIndex)
{
	return this->core->devices->getDevice(unDeviceIndex).has_value();
}

bool
XRTVRSystem_026::GetBoolTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
                                              vr::ETrackedDeviceProperty prop,
                                              vr::ETrackedPropertyError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(pError, vr::TrackedProp_UnknownProperty);
	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetBoolTrackedDeviceProperty(unDeviceIndex=%u, prop=%d, pError=%p) -> %d",
	                             false, static_cast<unsigned int>(unDeviceIndex), static_cast<int>(prop),
	                             static_cast<void *>(pError), 0);
}

float
XRTVRSystem_026::GetFloatTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
                                               vr::ETrackedDeviceProperty prop,
                                               vr::ETrackedPropertyError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(pError, vr::TrackedProp_UnknownProperty);
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetFloatTrackedDeviceProperty(unDeviceIndex=%u, prop=%d, pError=%p) -> %f", 0.0f,
	    static_cast<unsigned int>(unDeviceIndex), static_cast<int>(prop), static_cast<void *>(pError), 0.0);
}

int32_t
XRTVRSystem_026::GetInt32TrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
                                               vr::ETrackedDeviceProperty prop,
                                               vr::ETrackedPropertyError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(pError, vr::TrackedProp_UnknownProperty);
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetInt32TrackedDeviceProperty(unDeviceIndex=%u, prop=%d, pError=%p) -> %d", 0,
	    static_cast<unsigned int>(unDeviceIndex), static_cast<int>(prop), static_cast<void *>(pError), 0);
}

uint64_t
XRTVRSystem_026::GetUint64TrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
                                                vr::ETrackedDeviceProperty prop,
                                                vr::ETrackedPropertyError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(pError, vr::TrackedProp_UnknownProperty);
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetUint64TrackedDeviceProperty(unDeviceIndex=%u, prop=%d, pError=%p) -> %llu", 0ULL,
	    static_cast<unsigned int>(unDeviceIndex), static_cast<int>(prop), static_cast<void *>(pError), 0ULL);
}

vr::HmdMatrix34_t
XRTVRSystem_026::GetMatrix34TrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
                                                  vr::ETrackedDeviceProperty prop,
                                                  vr::ETrackedPropertyError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(pError, vr::TrackedProp_UnknownProperty);

	vr::HmdMatrix34_t mat;
	hmdMatrix34Identity(mat);
	OPENVR_LOG_UNIMPLEMENTED_RET(logger,
	                             "GetMatrix34TrackedDeviceProperty(unDeviceIndex=%u, prop=%d, pError=%p) -> %p",
	                             mat, static_cast<unsigned int>(unDeviceIndex), static_cast<int>(prop),
	                             static_cast<void *>(pError), static_cast<void *>(&mat));
}

uint32_t
XRTVRSystem_026::GetArrayTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
                                               vr::ETrackedDeviceProperty prop,
                                               vr::PropertyTypeTag_t propType,
                                               void *pBuffer,
                                               uint32_t unBufferSize,
                                               vr::ETrackedPropertyError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	SET_ERROR(pError, vr::TrackedProp_UnknownProperty);
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetArrayTrackedDeviceProperty(unDeviceIndex=%u, prop=%d, propType=%u, pBuffer=%p, "
	    "unBufferSize=%u, pError=%p) -> %u",
	    0U, static_cast<unsigned int>(unDeviceIndex), static_cast<int>(prop), static_cast<unsigned int>(propType),
	    pBuffer, static_cast<unsigned int>(unBufferSize), static_cast<void *>(pError), 0U);
}

uint32_t
XRTVRSystem_026::GetStringTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex,
                                                vr::ETrackedDeviceProperty prop,
                                                VR_OUT_STRING() char *pchValue,
                                                uint32_t unBufferSize,
                                                vr::ETrackedPropertyError *pError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_TRACE(logger, "GetStringTrackedDeviceProperty(%d)", static_cast<int>(prop));

	std::string str;
	if (!this->core->devices->getDeviceStringProperty(logger, unDeviceIndex, prop, str, pError)) {
		return 0;
	}

	size_t str_size = str.length() + 1;

	if (str_size > unBufferSize) {
		SET_ERROR(pError, vr::ETrackedPropertyError::TrackedProp_BufferTooSmall);
		return str_size;
	}

	SET_ERROR(pError, vr::ETrackedPropertyError::TrackedProp_Success);
	snprintf(pchValue, unBufferSize, "%s", str.c_str());

	return str_size;
}

const char *
XRTVRSystem_026::GetPropErrorNameFromEnum(vr::ETrackedPropertyError error)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetPropErrorNameFromEnum(error=%d) -> %s",
	                             "TODO: GetPropErrorNameFromEnum", static_cast<int>(error),
	                             "TODO: GetPropErrorNameFromEnum");
}

/* Event methods */

bool
XRTVRSystem_026::PollNextEvent(vr::VREvent_t *pEvent, uint32_t uncbVREvent)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	bool ret = this->core->events->pollEvent(logger, *pEvent, uncbVREvent);

	// No event
	if (!ret) {
		pEvent->eventType = vr::VREvent_None;
	}

	return ret;
}

bool
XRTVRSystem_026::PollNextEventWithPose(vr::ETrackingUniverseOrigin eOrigin,
                                       vr::VREvent_t *pEvent,
                                       uint32_t uncbVREvent,
                                       vr::TrackedDevicePose_t *pTrackedDevicePose)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(pTrackedDevicePose, 0, sizeof(*pTrackedDevicePose));
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "PollNextEventWithPose(eOrigin=%d, pEvent=%p, uncbVREvent=%u, pTrackedDevicePose=%p) -> %d", false,
	    static_cast<int>(eOrigin), static_cast<void *>(pEvent), static_cast<unsigned int>(uncbVREvent),
	    static_cast<void *>(pTrackedDevicePose), 0);
}

bool
XRTVRSystem_026::PollNextEventWithPoseAndOverlays(vr::ETrackingUniverseOrigin eOrigin,
                                                  vr::VREvent_t *pEvent,
                                                  uint32_t uncbVREvent,
                                                  vr::TrackedDevicePose_t *pTrackedDevicePose,
                                                  vr::VROverlayHandle_t *pulOverlayHandle)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(pTrackedDevicePose, 0, sizeof(*pTrackedDevicePose));
	*pulOverlayHandle = vr::k_ulOverlayHandleInvalid;
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "PollNextEventWithPoseAndOverlays(eOrigin=%d, pEvent=%p, uncbVREvent=%u, pTrackedDevicePose=%p, "
	    "pulOverlayHandle=%p) -> %d",
	    false, static_cast<int>(eOrigin), static_cast<void *>(pEvent), static_cast<unsigned int>(uncbVREvent),
	    static_cast<void *>(pTrackedDevicePose), static_cast<void *>(pulOverlayHandle), 0);
}

const char *
XRTVRSystem_026::GetEventTypeNameFromEnum(vr::EVREventType eType)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetEventTypeNameFromEnum(eType=%d) -> %s",
	                             "TODO: GetEventTypeNameFromEnum", static_cast<int>(eType),
	                             "TODO: GetEventTypeNameFromEnum");
}

/* Rendering helper methods */

vr::HiddenAreaMesh_t
XRTVRSystem_026::GetHiddenAreaMesh(vr::EVREye eEye, vr::EHiddenAreaMeshType type)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	vr::HiddenAreaMesh_t mesh;
	std::memset(&mesh, 0, sizeof(mesh));
	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetHiddenAreaMesh(eEye=%d, type=%d) -> %p", mesh, static_cast<int>(eEye),
	                             static_cast<int>(type), static_cast<void *>(&mesh));
}

bool
XRTVRSystem_026::GetEyeTrackedFoveationCenter(vr::HmdVector2_t *pNdcLeft, vr::HmdVector2_t *pNdcRight)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(pNdcLeft, 0, sizeof(*pNdcLeft));
	std::memset(pNdcRight, 0, sizeof(*pNdcRight));
	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetEyeTrackedFoveationCenter(pNdcLeft=%p, pNdcRight=%p) -> %d", false,
	                             static_cast<void *>(pNdcLeft), static_cast<void *>(pNdcRight), 0);
}

bool
XRTVRSystem_026::GetEyeTrackedFoveationCenterForProjection(const vr::HmdMatrix44_t *pProjMat, vr::HmdVector2_t *pNdc)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(pNdc, 0, sizeof(*pNdc));
	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetEyeTrackedFoveationCenterForProjection(pProjMat=%p, pNdc=%p) -> %d",
	                             false, static_cast<const void *>(pProjMat), static_cast<void *>(pNdc), 0);
}

/* Controller methods */

bool
XRTVRSystem_026::GetControllerState(vr::TrackedDeviceIndex_t unControllerDeviceIndex,
                                    vr::VRControllerState_t *pControllerState,
                                    uint32_t unControllerStateSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(pControllerState, 0, sizeof(*pControllerState));
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetControllerState(unControllerDeviceIndex=%u, pControllerState=%p, unControllerStateSize=%u) -> %d",
	    false, static_cast<unsigned int>(unControllerDeviceIndex), static_cast<void *>(pControllerState),
	    static_cast<unsigned int>(unControllerStateSize), 0);
}

bool
XRTVRSystem_026::GetControllerStateWithPose(vr::ETrackingUniverseOrigin eOrigin,
                                            vr::TrackedDeviceIndex_t unControllerDeviceIndex,
                                            vr::VRControllerState_t *pControllerState,
                                            uint32_t unControllerStateSize,
                                            vr::TrackedDevicePose_t *pTrackedDevicePose)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	std::memset(pControllerState, 0, sizeof(*pControllerState));
	std::memset(pTrackedDevicePose, 0, sizeof(*pTrackedDevicePose));
	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger,
	    "GetControllerStateWithPose(eOrigin=%d, unControllerDeviceIndex=%u, pControllerState=%p, "
	    "unControllerStateSize=%u, pTrackedDevicePose=%p) -> %d",
	    false, static_cast<int>(eOrigin), static_cast<unsigned int>(unControllerDeviceIndex),
	    static_cast<void *>(pControllerState), static_cast<unsigned int>(unControllerStateSize),
	    static_cast<void *>(pTrackedDevicePose), 0);
}

void
XRTVRSystem_026::TriggerHapticPulse(vr::TrackedDeviceIndex_t unControllerDeviceIndex,
                                    uint32_t unAxisId,
                                    unsigned short usDurationMicroSec)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger,
	                         "TriggerHapticPulse(unControllerDeviceIndex=%u, unAxisId=%u, usDurationMicroSec=%u)",
	                         static_cast<unsigned int>(unControllerDeviceIndex),
	                         static_cast<unsigned int>(unAxisId), static_cast<unsigned int>(usDurationMicroSec));
}

const char *
XRTVRSystem_026::GetButtonIdNameFromEnum(vr::EVRButtonId eButtonId)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetButtonIdNameFromEnum(eButtonId=%d) -> %s",
	                             "TODO: GetButtonIdNameFromEnum", static_cast<int>(eButtonId),
	                             "TODO: GetButtonIdNameFromEnum");
}

const char *
XRTVRSystem_026::GetControllerAxisTypeNameFromEnum(vr::EVRControllerAxisType eAxisType)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetControllerAxisTypeNameFromEnum(eAxisType=%d) -> %s",
	                             "TODO: GetControllerAxisTypeNameFromEnum", static_cast<int>(eAxisType),
	                             "TODO: GetControllerAxisTypeNameFromEnum");
}

bool
XRTVRSystem_026::IsInputAvailable()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsInputAvailable() -> %d", false, 0);
}

bool
XRTVRSystem_026::IsSteamVRDrawingControllers()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "IsSteamVRDrawingControllers() -> %d", false, 0);
}

bool
XRTVRSystem_026::ShouldApplicationPause()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ShouldApplicationPause() -> %d", false, 0);
}

bool
XRTVRSystem_026::ShouldApplicationReduceRenderingWork()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "ShouldApplicationReduceRenderingWork() -> %d", false, 0);
}

/* Firmware methods */

vr::EVRFirmwareError
XRTVRSystem_026::PerformFirmwareUpdate(vr::TrackedDeviceIndex_t unDeviceIndex)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "PerformFirmwareUpdate(unDeviceIndex=%u) -> %d", vr::EVRFirmwareError::VRFirmwareError_None,
	    static_cast<unsigned int>(unDeviceIndex), static_cast<int>(vr::EVRFirmwareError::VRFirmwareError_None));
}

/* Application life cycle methods */

void
XRTVRSystem_026::AcknowledgeQuit_Exiting()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);
	OPENVR_LOG_UNIMPLEMENTED(logger, "AcknowledgeQuit_Exiting()");
}

/* App container sandbox methods */

uint32_t
XRTVRSystem_026::GetAppContainerFilePaths(VR_OUT_STRING() char *pchBuffer, uint32_t unBufferSize)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	if (unBufferSize > 0) {
		pchBuffer[0] = '\0';
	}

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetAppContainerFilePaths(pchBuffer=%p, unBufferSize=%u) -> %u", 0U,
	                             static_cast<void *>(pchBuffer), static_cast<unsigned int>(unBufferSize), 0U);
}

/* System methods */

const char *
XRTVRSystem_026::GetRuntimeVersion()
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetRuntimeVersion() -> %s", "TODO: GetRuntimeVersion",
	                             "TODO: GetRuntimeVersion");
}

vr::EVRInitError
XRTVRSystem_026::SetSDKVersion(uint32_t nVersionMajor, uint32_t nVersionMinor, uint32_t nVersionBuild)
{
	// We don't need any special handling of this right now, so let's just ignore it.
	return vr::EVRInitError::VRInitError_None;
}

}; // namespace xrt::state_trackers::openvr
