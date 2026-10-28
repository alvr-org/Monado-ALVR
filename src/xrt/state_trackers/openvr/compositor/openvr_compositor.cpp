// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of compositor-related functionality for OpenVR.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#include "os/os_time.h"

#include "util/u_pretty_print.h"

#include "math/m_api.h"

#include "common/openvr_error.hpp"

#include "openvr_compositor.hpp"


namespace xrt::state_trackers::openvr {

/*
 *
 * SwapchainCache implementations
 *
 */

xrt_result_t
SwapchainCache::ensureSwapchain(xrt_compositor *xc,
                                uint32_t storage_format,
                                uint32_t sample_format,
                                uint32_t width,
                                uint32_t height,
                                xrt_swapchain_usage_bits usage_bits,
                                xrt_swapchain_create_flags flags)
{
	if (this->xsc != nullptr && this->storage_format == storage_format && this->sample_format == sample_format &&
	    this->width == width && this->height == height) {
		// Cache hit
		return XRT_SUCCESS;
	}

	// OpenVR apps will sometimes do silly things, this stops that
	width = MAX(width, 1);
	height = MAX(height, 1);

	if (width > xc->info.max_texture_size || height > xc->info.max_texture_size) {
		return XRT_ERROR_ALLOCATION;
	}

	// Free the existing swapchain if it exists, since the cache failed
	if (this->xsc != nullptr) {
		this->width = 0;
		this->height = 0;
		this->storage_format = 0;
		this->sample_format = 0;

		xrt_swapchain_reference(&this->xsc, nullptr);
	}

	if (storage_format != sample_format) {
		assert(flags & XRT_SWAPCHAIN_CREATE_SAMPLE_AS_SRGB);
		assert(usage_bits & XRT_SWAPCHAIN_USAGE_MUTABLE_FORMAT);

		if ((flags & XRT_SWAPCHAIN_CREATE_SAMPLE_AS_SRGB) == 0 ||
		    (usage_bits & XRT_SWAPCHAIN_USAGE_MUTABLE_FORMAT) == 0) {
			return XRT_ERROR_INVALID_ARGUMENT;
		}
	}

	struct xrt_swapchain_create_info swapchain_create_info = {
	    .create = flags,
	    .bits = usage_bits,
	    .format = storage_format,
	    .sample_count = 1,
	    .width = width,
	    .height = height,
	    .face_count = 1,
	    .array_size = 1,
	    .mip_count = 1,
	    .format_count = static_cast<uint32_t>((flags & XRT_SWAPCHAIN_CREATE_SAMPLE_AS_SRGB) ? 2 : 0),
	    .formats = {storage_format, sample_format},
	};

	xrt_result_t xret = xrt_comp_create_swapchain(xc, &swapchain_create_info, &this->xsc);
	if (xret != XRT_SUCCESS) {
		// Failed to create swapchain
		return xret;
	}

	this->width = width;
	this->height = height;
	this->storage_format = storage_format;
	this->sample_format = sample_format;

	return XRT_SUCCESS;
}

void
SwapchainCache::reset()
{
	this->width = 0;
	this->height = 0;
	this->storage_format = 0;
	this->sample_format = 0;

	if (this->xsc != nullptr) {
		xrt_swapchain_reference(&this->xsc, nullptr);
	}
}

SwapchainCache::~SwapchainCache()
{
	this->reset();
}

/*
 *
 * Compositor implementations
 *
 */

Compositor::Compositor(xrt_system_devices *xsysd,
                       xrt_compositor_native *xcn,
                       xrt_system_compositor *xsysc,
                       std::shared_ptr<Events> &events)
    : xsysd(xsysd), xcn(xcn), xsysc(xsysc), events(events)
{}

Compositor::~Compositor()
{
	// Wipe the swapchain caches, since we're about to destroy the compositor, and we want to make sure the
	// swapchains are released before the compositor is destroyed.
	this->swapchain_caches[0].reset();
	this->swapchain_caches[1].reset();

#ifdef XRT_HAVE_VULKAN
	this->destroyVulkanResources();
#endif

	if (this->active_compositor) {
		xrt_comp_destroy(&this->active_compositor);
	}
}

vr::EVRCompositorError
Compositor::waitBeginFrame(openvr_logger &logger)
{
	xrt_result_t xret;

	if (this->active_compositor == nullptr) {
		return vr::EVRCompositorError::VRCompositorError_DoNotHaveFocus;
	}

	if (this->active_frame.has_value()) {
		xret = xrt_comp_discard_frame(this->active_compositor, this->active_frame->frame_id);
		if (xret != XRT_SUCCESS) {
			return xretToCompositorError(xret);
		}

		this->active_frame.reset();

		this->frame_eye_states[0].reset();
		this->frame_eye_states[1].reset();
	}

	FrameState frame_state{};

	xret = xrt_comp_wait_frame(this->active_compositor, &frame_state.frame_id, &frame_state.predicted_display_time,
	                           &frame_state.predicted_display_period);
	if (xret != XRT_SUCCESS) {
		return xretToCompositorError(xret);
	}

	xret = xrt_comp_begin_frame(this->active_compositor, frame_state.frame_id);
	if (xret != XRT_SUCCESS) {
		return xretToCompositorError(xret);
	}

	// We got a frame, we can set it as the active one, now
	this->active_frame = frame_state;

	this->last_predicted_display_time = frame_state.predicted_display_time;
	this->last_predicted_display_period = frame_state.predicted_display_period;

	this->render_state_cache.reset();

	std::array<xrt_fov, 2> fovs;
	std::array<xrt_pose, 2> poses;
	xrt_pose head_relation;
	xret = this->getFrameRenderState(fovs, poses, head_relation);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to cache output data for frame", xret);
		return xretToCompositorError(xret);
	}

	this->render_state_cache = {
	    .fovs = fovs,
	    .T_head_eyes = poses,
	    .head_relation = head_relation,
	};

	// Handle session events after begin frame
	xret = this->events->handleSessionEvents(logger);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to handle session events, non-fatal so just continuing.", xret);
	}

	return vr::EVRCompositorError::VRCompositorError_None;
}

vr::EVRCompositorError
Compositor::submit(openvr_logger &logger,
                   vr::EVREye eye,
                   const vr::Texture_t &texture,
                   uint32_t array_index,
                   vr::VRTextureBounds_t bounds,
                   vr::EVRSubmitFlags nSubmitFlags)
{
	// @todo return DoNotHaveFocus if the application doesn't have focus
	// @todo return TextureIsOnWrongDevice when we detect the texture isn't using the device from GetOutputInfo
	// @todo return SharedTexturesNotSupported when the app doesn't use CreateDXGIFactory1 or later
	// @todo return TextureUsesUnsupportedFormat when the app uses a DXGI non-sharable format

	// SteamVR returns InvalidTexture first
	if (texture.handle == nullptr) {
		return vr::EVRCompositorError::VRCompositorError_InvalidTexture;
	}

	// We don't have a compositor eg. we're not a scene application.
	if (this->xsysc == nullptr) {
		return vr::EVRCompositorError::VRCompositorError_IsNotSceneApplication;
	}

	// Eye has already been submit
	if (this->frame_eye_states[eye].has_value()) {
		OPENVR_LOG_ERROR(logger, "Already submitted for eye %d in this frame.", eye);
		return vr::EVRCompositorError::VRCompositorError_AlreadySubmitted;
	}

	bounds.uMin = MAX(0.0f, bounds.uMin);
	bounds.vMin = MAX(0.0f, bounds.vMin);
	bounds.uMax = MIN(1.0f, bounds.uMax);
	bounds.vMax = MIN(1.0f, bounds.vMax);

	switch (texture.eType) {
#ifdef XRT_HAVE_VULKAN
	case vr::ETextureType::TextureType_Vulkan: {
		// Supported.
		auto error = this->submitVulkan(logger, eye, *static_cast<vr::VRVulkanTextureData_t *>(texture.handle),
		                                array_index, texture.eColorSpace, bounds);

		if (error != vr::EVRCompositorError::VRCompositorError_None) {
			return error;
		}

		break;
	}
#endif
	default: {
		OPENVR_LOG_ERROR(logger, "Unsupported texture type %d.", texture.eType);

		// Unsupported.
		return vr::EVRCompositorError::VRCompositorError_TextureUsesUnsupportedFormat;
	}
	}

	// We have a value for both eyes, a frame is completed
	if (this->frame_eye_states[0].has_value() && this->frame_eye_states[1].has_value()) {
		this->completeFrame(logger);
	}

	return vr::EVRCompositorError::VRCompositorError_None;
}

xrt_layer_projection_view_data
Compositor::getProjectionLayerDataForEye(vr::EVREye eye, const EyeState &eye_state)
{
	xrt_pose T_origin_head = this->render_state_cache->head_relation;
	xrt_pose T_head_eye = this->render_state_cache->T_head_eyes[eye];

	xrt_pose T_origin_eye;
	math_pose_transform(&T_origin_head, &T_head_eye, &T_origin_eye);

	return {
	    .sub =
	        {
	            .image_index = eye_state.texture.swapchain_index,
	            .array_index = 0,
	            .rect = eye_state.texture.bounds,
	            .norm_rect =
	                {
	                    .x = eye_state.texture.bounds.offset.w / (float)eye_state.texture.width,
	                    .y = eye_state.texture.bounds.offset.h / (float)eye_state.texture.height,
	                    .w = eye_state.texture.bounds.extent.w / (float)eye_state.texture.width,
	                    .h = eye_state.texture.bounds.extent.h / (float)eye_state.texture.height,
	                },
	        },
	    .fov = this->render_state_cache->fovs[eye],
	    .pose = T_origin_eye,
	};
}

vr::EVRCompositorError
Compositor::completeFrame(openvr_logger &logger)
{
	xrt_result_t xret;

	if (!this->active_frame.has_value()) {
		this->frame_eye_states[0].reset();
		this->frame_eye_states[1].reset();

		// Do nothing, we have a compositor now, but we haven't been able to wait and begin a frame.
		return vr::EVRCompositorError::VRCompositorError_None;
	}

	auto &active_frame = this->active_frame.value();

	struct xrt_layer_frame_data frame_data = {
	    .frame_id = active_frame.frame_id,
	    .display_time_ns = active_frame.predicted_display_time,
	    .env_blend_mode = XRT_BLEND_MODE_OPAQUE,
	};
	xret = xrt_comp_layer_begin(this->active_compositor, &frame_data);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to begin layer submission", xret);
		return xretToCompositorError(xret);
	}

	// @todo push the projection layer here

	xrt_swapchain *swapchains[2] = {
	    this->frame_eye_states[0]->texture.xsc,
	    this->frame_eye_states[1]->texture.xsc,
	};

	xrt_layer_data projection_layer_data = {
	    .type = XRT_LAYER_PROJECTION,
	    .name = XRT_INPUT_GENERIC_HEAD_POSE,
	    .timestamp = this->getTimeForPredictions(),
	    .flags = (enum xrt_layer_composition_flags)0,
	    .depth_test = {},
	    .flip_y = false,
	    .color_scale = {},
	    .color_bias = {},
	    .advanced_blend = {},
	    .proj =
	        {
	            .v =
	                {
	                    getProjectionLayerDataForEye(vr::EVREye::Eye_Left, this->frame_eye_states[0].value()),
	                    getProjectionLayerDataForEye(vr::EVREye::Eye_Right, this->frame_eye_states[1].value()),
	                },
	            .chroma_key = {},
	        },
	    .view_count = 2,
	};
	xret = xrt_comp_layer_projection(this->active_compositor, this->xsysd->static_roles.head, swapchains,
	                                 &projection_layer_data);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to submit projection layer", xret);
		return xretToCompositorError(xret);
	}

	// Submit the layers
	xret = xrt_comp_layer_commit(this->active_compositor, XRT_GRAPHICS_SYNC_HANDLE_INVALID);
	if (xret != XRT_SUCCESS) {
		OPENVR_LOG_ERROR_XRET(logger, "Failed to commit layer submission", xret);
		return xretToCompositorError(xret);
	}

	this->frame_eye_states[0].reset();
	this->frame_eye_states[1].reset();

	this->active_frame.reset();

	return vr::EVRCompositorError::VRCompositorError_None;
}

void
Compositor::getOutputDevice(openvr_logger &logger,
                            uint64_t *out_device,
                            vr::ETextureType texture_type,
                            VkInstance pInstance)
{
	switch (texture_type) {
#ifdef XRT_HAVE_VULKAN
	case vr::ETextureType::TextureType_Vulkan: {
		// Supported.
		this->getVulkanOutputDevice(logger, out_device, pInstance);
		return;
	}
#endif
	default: {
		// Unsupported.
		*out_device = 0;
		return;
	}
	}
}

timepoint_ns
Compositor::getTimeForPredictions()
{
	// If we have a predicted display time from the runtime, use that for predictions, otherwise fall back to the
	// current time. This allows us to still provide some level of functionality even before we have successfully
	// gotten timing information from the compositor, which is something OpenVR expects to be able to do.
	return this->last_predicted_display_time.value_or(os_monotonic_get_ns());
}

time_duration_ns
Compositor::getFramePeriod()
{
	// Use the compositor-predicted
	return this->last_predicted_display_period.value_or(
	    this->xsysd->static_roles.head->hmd->screens[0].nominal_frame_interval_ns);
}

xrt_result_t
Compositor::getFrameRenderState(std::array<xrt_fov, 2> &fovs,
                                std::array<xrt_pose, 2> &T_head_eyes,
                                xrt_pose &head_relation)
{
	const xrt_vec3 default_eye_relation = {.x = 0.0635f, .y = 0.0f, .z = 0.0f};
	xrt_space_relation head_relation_space;
	xrt_result_t xret = xrt_device_get_view_poses(this->xsysd->static_roles.head, &default_eye_relation,
	                                              this->getTimeForPredictions(), XRT_VIEW_TYPE_STEREO, 2,
	                                              &head_relation_space, fovs.data(), T_head_eyes.data());
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	head_relation = head_relation_space.pose;

	if (this->render_state_cache.has_value()) {
		fovs = this->render_state_cache->fovs;
		T_head_eyes = this->render_state_cache->T_head_eyes;
		head_relation = this->render_state_cache->head_relation;
	}

	return XRT_SUCCESS;
}

}; // namespace xrt::state_trackers::openvr
