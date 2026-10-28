// Copyright 2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of compositor-related functionality for OpenVR.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#pragma once

#include "xrt/xrt_compositor.h"
#include "xrt/xrt_config_build.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_gfx_vk.h"

#ifdef XRT_HAVE_VULKAN
#include "vk/vk_helpers.h"
#include "vk/vk_cmd_pool.h"

#include "render_shaders_interface.h"
#include "render/render_interface.h"
#endif

#include "util/u_time.h"

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"

#include "openvr_events.hpp"

#include <array>
#include <optional>
#include <memory>


namespace xrt::state_trackers::openvr {

struct EyeTexture
{
	//! The swapchain in use
	xrt_swapchain *xsc{nullptr};
	//! The index into the eye's swapchain for the currently held texture.
	uint32_t swapchain_index;

	//! The width of the currently held runtime swapchain texture.
	uint32_t width;
	//! The height of the currently held runtime swapchain texture.
	uint32_t height;

	//! The sub-image of the runtime swapchain texture to submit to the compositor.
	xrt_rect bounds;

	~EyeTexture()
	{
		if (this->xsc != nullptr) {
			xrt_swapchain_release_image(this->xsc, this->swapchain_index);
		}

		xrt_swapchain_reference(&this->xsc, nullptr);
	}
};

struct EyeState
{
	EyeTexture texture;
};

struct FrameState
{
	int64_t frame_id{-1};
	timepoint_ns predicted_display_time{0};
	time_duration_ns predicted_display_period{0};
};

/*!
 * Since eye textures are created by the clients, and not the runtime, we have to do a copy into a runtime-owned
 * swapchain, this structure cached created swapchains to avoid creating a new one every frame.
 */
struct SwapchainCache
{
public: // Fields
	xrt_swapchain *xsc{nullptr};

	uint32_t width{0};
	uint32_t height{0};
	uint32_t storage_format{0};
	uint32_t sample_format{0};

public: // Methods
	//! Ensures that there is a swapchain in the cache with the given properties, creating one if necessary.
	xrt_result_t
	ensureSwapchain(xrt_compositor *xc,
	                uint32_t storage_format,
	                uint32_t sample_format,
	                uint32_t width,
	                uint32_t height,
	                xrt_swapchain_usage_bits usage_bits,
	                xrt_swapchain_create_flags flags);

	//! Resets the internal state of the cache, releasing any swapchain it holds.
	void
	reset();

	~SwapchainCache();
};

struct RenderStateCache
{
	std::array<xrt_fov, 2> fovs;
	std::array<xrt_pose, 2> T_head_eyes;

	xrt_pose head_relation;
};

struct Compositor
{
private: // Fields
	//! The state for the current active frame
	std::optional<FrameState> active_frame{std::nullopt};
	//! The state for each submit eye.
	std::array<std::optional<EyeState>, 2> frame_eye_states{std::nullopt, std::nullopt};
	//! Caches of swapchains for each eye, keyed by the runtime storage/sample formats and eye image size.
	std::array<SwapchainCache, 2> swapchain_caches{};

	//! The active system devices
	xrt_system_devices *xsysd{nullptr};

	//! The current native compositor
	xrt_compositor_native *xcn{nullptr};

	xrt_system_compositor *xsysc{nullptr};

	//! The currently active compositor, implicitly also has an active session.
	xrt_compositor *active_compositor{nullptr};

#ifdef XRT_HAVE_VULKAN
	//! The active Vulkan compositor.
	struct xrt_compositor *xc_vk{nullptr};

	//! The vk_bundle for the compositor.
	vk_bundle *vk{nullptr};

	//! The command buffer pool for doing compositor work.
	vk_cmd_pool *cmd_pool;

	//! The shaders for the compositor.
	render_shaders shaders;

	struct
	{
		//! The sampler to use during the blit operation
		VkSampler sampler{VK_NULL_HANDLE};

		//! Vulkan pipeline cache object passed to vkCreateComputePipelines.
		VkPipelineCache driver_pipeline_cache{VK_NULL_HANDLE};

		//! Get-or-create cache for blit_ms.comp specialization variants.
		struct render_blit_ms_pipeline_cache *pipeline_cache{nullptr};

		//! Pipeline layout for blit pipelines.
		VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};

		//! Descriptor pool for blit.
		VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

		//! Descriptor set layout for blit.
		VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};

		//! Descriptor set for blit.
		VkDescriptorSet descriptor_set{VK_NULL_HANDLE};

		//! The pipelines for blitting/copying app textures into runtime swapchain images.
		std::array<VkPipeline, RENDER_BLIT_RESOLVE_COLOR_MODE_COUNT> pipelines{};
	} blit;
#endif

	std::optional<RenderStateCache> render_state_cache{std::nullopt};

	std::shared_ptr<Events> events{};

public: // Fields
	std::optional<timepoint_ns> last_predicted_display_time{std::nullopt};
	std::optional<time_duration_ns> last_predicted_display_period{std::nullopt};

	std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount> last_render_poses{};
	std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount> last_game_poses{};

	vr::ETrackingUniverseOrigin current_tracking_universe{vr::ETrackingUniverseOrigin::TrackingUniverseSeated};

private: // Methods
#ifdef XRT_HAVE_VULKAN
	xrt_result_t
	setupBlitPipelines(openvr_logger &logger);

	//! Sets up the Vulkan compositor as the active compositor, and begins the session.
	xrt_result_t
	setupVulkanCompositor(openvr_logger &logger, vr::VRVulkanTextureData_t &vulkan_data);

	void
	destroyVulkanResources();

	xrt_result_t
	transferAppImageToSwapchainImage(openvr_logger &logger,
	                                 xrt_swapchain *xsc,
	                                 uint32_t dst_index,
	                                 vr::VRVulkanTextureData_t &texture_data,
	                                 uint32_t src_array_index,
	                                 VkFormat storage_format,
	                                 vr::EColorSpace color_space,
	                                 const xrt_rect &rect);

	//! Handles submission of a Vulkan texture, which involves copying it into a runtime-owned swapchain image.
	vr::EVRCompositorError
	submitVulkan(openvr_logger &logger,
	             vr::EVREye eye,
	             vr::VRVulkanTextureData_t &texture_data,
	             uint32_t array_index,
	             vr::EColorSpace color_space,
	             const vr::VRTextureBounds_t &bounds);

	void
	getVulkanOutputDevice(openvr_logger &logger, uint64_t *out_device, VkInstance pInstance);
#endif

	//! Gets the projection layer data from the passed eye state
	xrt_layer_projection_view_data
	getProjectionLayerDataForEye(vr::EVREye eye, const EyeState &eye_state);

	//! Completes a frame and submits textures to the runtime.
	vr::EVRCompositorError
	completeFrame(openvr_logger &logger);

public: // Methods
	Compositor(xrt_system_devices *xsysd,
	           xrt_compositor_native *xcn,
	           xrt_system_compositor *xsysc,
	           std::shared_ptr<Events> &events);

	~Compositor();

	/*!
	 * Waits for the next frame and begins it, cancelling/releasing any previously active frame and it's resources,
	 * if it exists.
	 */
	vr::EVRCompositorError
	waitBeginFrame(openvr_logger &logger);

	//! Submits a texture for an eye, completing the frame if both eyes have been submit.
	vr::EVRCompositorError
	submit(openvr_logger &logger,
	       vr::EVREye eye,
	       const vr::Texture_t &texture,
	       uint32_t array_index,
	       vr::VRTextureBounds_t bounds,
	       vr::EVRSubmitFlags nSubmitFlags);

	void
	getOutputDevice(openvr_logger &logger,
	                uint64_t *out_device,
	                vr::ETextureType texture_type,
	                VkInstance pInstance);

	timepoint_ns
	getTimeForPredictions();

	time_duration_ns
	getFramePeriod();

	xrt_result_t
	getFrameRenderState(std::array<xrt_fov, 2> &fovs,
	                    std::array<xrt_pose, 2> &T_head_eyes,
	                    xrt_pose &head_relation);
};

}; // namespace xrt::state_trackers::openvr
