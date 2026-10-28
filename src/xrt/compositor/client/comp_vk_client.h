// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vulkan client side glue to compositor header.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup comp_client
 */

#pragma once

#include "vk/vk_helpers.h"
#include "vk/vk_cmd_pool.h"
#include "xrt/xrt_gfx_vk.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Structs
 *
 */

struct client_vk_compositor;

/*!
 * Wraps the real compositor swapchain providing a Vulkan based interface.
 *
 * Almost a one to one mapping to a OpenXR swapchain.
 *
 * @ingroup comp_client
 * @implements xrt_swapchain_vk
 */
struct client_vk_swapchain
{
	struct xrt_swapchain_vk base;

	//! Owning reference to the backing native swapchain.
	struct xrt_swapchain_native *xscn;

	//! Non-owning reference to our parent compositor.
	struct client_vk_compositor *c;

	// Memory
	VkDeviceMemory mems[XRT_MAX_SWAPCHAIN_IMAGES];

	// Prerecorded swapchain image ownership/layout transition barriers
	VkCommandBuffer acquire[XRT_MAX_SWAPCHAIN_IMAGES];
	VkCommandBuffer release[XRT_MAX_SWAPCHAIN_IMAGES];
};

/*!
 * @class client_vk_compositor
 *
 * Wraps the real compositor providing a Vulkan based interface.
 *
 * @ingroup comp_client
 * @implements xrt_compositor
 */
struct client_vk_compositor
{
	struct xrt_compositor base;

	//! Owning reference to the backing native compositor
	struct xrt_compositor_native *xcn;

	struct
	{
		VkSemaphore semaphore;
		struct xrt_compositor_semaphore *xcsem;
		uint64_t value;
	} sync;

	struct vk_bundle vk;

	struct vk_cmd_pool pool;

	bool renderdoc_enabled;
	VkCommandBuffer dcb;
};


/*
 *
 * Functions and helpers.
 *
 */


/*!
 * Create a new client_vk_compositor.
 *
 * Takes ownership of provided xcn.
 *
 * @public @memberof client_vk_compositor
 * @see xrt_compositor_native
 */
struct client_vk_compositor *
client_vk_compositor_create(struct xrt_compositor_native *xcn,
                            VkInstance instance,
                            PFN_vkGetInstanceProcAddr getProc,
                            VkPhysicalDevice physicalDevice,
                            VkDevice device,
                            bool external_fence_fd_enabled,
                            bool external_semaphore_fd_enabled,
                            bool timeline_semaphore_enabled,
                            bool image_format_list_enabled,
                            bool debug_utils_enabled,
                            bool renderdoc_enabled,
                            uint32_t queueFamilyIndex,
                            uint32_t queueIndex);

/*!
 * Blit a Vulkan image to a swapchain image.
 *
 * @public @memberof client_vk_compositor
 * @param c Client compositor to use for blit.
 * @param xsc Swapchain to blit to, must be a client_vk_swapchain.
 * @param src_image Source image to blit from, must be compatible with the swapchain format.
 * @param src_params Blit parameters for the source image.
 * @param dst_index Index of the swapchain image to blit to.
 * @param dst_params Blit parameters for the destination image.
 * @return XRT_SUCCESS on success, XRT_ERROR_VULKAN if a Vulkan error occurred.
 */
xrt_result_t
client_vk_compositor_blit_to_swapchain(struct client_vk_compositor *c,
                                       struct xrt_swapchain *xsc,
                                       const struct vk_cmd_first_mip_image *src_image,
                                       const struct vk_cmd_blit_image_params *src_params,
                                       uint32_t dst_index,
                                       const struct vk_cmd_blit_image_params *dst_params);


#ifdef __cplusplus
}
#endif
