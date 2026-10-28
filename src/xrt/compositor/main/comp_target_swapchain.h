// Copyright 2019-2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Target Vulkan swapchain code header.
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_main
 */

#pragma once

#include "vk/vk_helpers.h"

#include "main/comp_target.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Structs.
 *
 */

struct u_pacing_compositor;

/*!
 * Wraps and manage VkSwapchainKHR and VkSurfaceKHR, used by @ref comp code.
 *
 * @ingroup comp_main
 */
struct comp_target_swapchain
{
	//! Base target.
	struct comp_target base;

	//! Compositor frame pacing helper
	struct u_pacing_compositor *upc;

	//! If we should use display timing.
	enum comp_target_display_timing_usage timing_usage;

	//! Also works as a frame index.
	int64_t current_frame_id;

	struct
	{
		/*!
		 * Should we ignore the compositor's preferred extents. Some
		 * targets, like the direct mode ones, requires a particular
		 * set of dimensions.
		 */
		bool compositor_extent;

		/*!
		 * The extents that a sub-class wants us to use,
		 * see @p ignore_compositor_extent above.
		 */
		VkExtent2D extent;
	} override;

	struct
	{
		VkSwapchainKHR handle;
	} swapchain;

	struct
	{
		VkSurfaceKHR handle;
		VkSurfaceFormatKHR format;
#ifdef VK_EXT_display_surface_counter
		VkSurfaceCounterFlagsEXT surface_counter_flags;
#endif

		//! Whether VK_KHR_present_id2 is supported for the surface.
		bool present_id2_supported;

		//! Whether VK_KHR_present_wait2 is supported for the surface.
		bool present_wait2_supported;
	} surface;

	struct
	{
		VkFormat color_format;
		VkColorSpaceKHR color_space;
	} preferred;

	//! Present mode that the system must support.
	VkPresentModeKHR present_mode;

	//! The current display used for direct mode, VK_NULL_HANDLE else.
	VkDisplayKHR display;

	struct
	{
		//! Must only be accessed from main compositor thread.
		bool thread_running;

		//! Protected by event_thread lock.
		bool should_wait;

		//! Protected by event_thread lock.
		uint64_t last_vblank_ns;

		//! Thread waiting on vblank_event_fence (first pixel out).
		struct os_thread_helper event_thread;

		//! Skipping the event on the first vblank avoids a spurious VK_ERROR_OUT_OF_HOST_MEMORY error.
		bool event_active;
	} vblank;

	/*!
	 * We print swapchain info as INFO the first time we create a
	 * VkSWapchain, this keeps track if we have done it.
	 */
	bool has_logged_info;

#ifdef VK_KHR_shared_presentable_image
	/*!
	 * Only relevant when using present modes from VK_KHR_shared_presentable_image,
	 *
	 * Shared presentable images for front buffer rendering we only acquire the image once,
	 * and then reuse it. This will be set the first time we acquire an image.
	 */
	bool shared_present_acquired;
#endif
};


/*
 *
 * Functions.
 *
 */

/*!
 * @brief Pre Vulkan initialisation, sets function pointers.
 *
 * Call from the creation function for your "subclass", after allocating.
 *
 * Initializes these function pointers, all other methods of @ref comp_target are the responsibility of the caller (the
 * "subclass"):
 *
 * - comp_target::check_ready
 * - comp_target::create_images
 * - comp_target::has_images
 * - comp_target::acquire
 * - comp_target::present
 * - comp_target::calc_frame_pacing
 * - comp_target::mark_timing_point
 * - comp_target::update_timings
 *
 * Also sets comp_target_swapchain::timing_usage to the provided value.
 *
 * @protected @memberof comp_target_swapchain
 *
 * @ingroup comp_main
 */
void
comp_target_swapchain_init_and_set_fnptrs(struct comp_target_swapchain *cts,
                                          enum comp_target_display_timing_usage timing_usage);

/*!
 * Set that any size from the compositor should be ignored and that given size
 * must be used for the @p VkSwapchain the helper code creates.
 *
 * @protected @memberof comp_target_swapchain
 *
 * @ingroup comp_main
 */
void
comp_target_swapchain_override_extents(struct comp_target_swapchain *cts, VkExtent2D extent);

/*!
 * Free all managed resources on the given @ref comp_target_swapchain,
 * does not free the struct itself.
 *
 * @protected @memberof comp_target_swapchain
 *
 * @ingroup comp_main
 */
void
comp_target_swapchain_cleanup(struct comp_target_swapchain *cts);


#ifdef __cplusplus
}
#endif
