// Copyright 2019-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Flags helpers for compositor swapchain images.
 *
 * These functions all concern only the compositor swapchain images that are
 * shared between the compositor and the application. That's why they are
 * grouped together and they are here because they need to be shared between
 * the @ref vk_image_collection and @ref comp_swapchain code so that they apply
 * the same flags everywhere.
 *
 * CSCI = Compositor SwapChain Images.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Benjamin Saunders <ben.e.saunders@gmail.com>
 * @ingroup aux_vk
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_vulkan_includes.h"

#include "vk/vk_helpers.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * String pretty printing
 *
 */

/*!
 * Returns xrt swapchain_usage flag if one valid bit is set,
 * if multiple bits are set, will return 'MULTIPLE BIT SET'.
 */
XRT_CHECK_RESULT const char *
xrt_swapchain_usage_flag_string(enum xrt_swapchain_usage_bits bits, bool null_on_unknown);


/*
 *
 * Compositor buffer and swapchain image flags helpers, in the vk_compositor_flags.c file.
 *
 */

/*!
 * Return the extern handle type that a buffer should be created with.
 *
 * cb = Compositor Buffer.
 */
VkExternalMemoryHandleTypeFlags
vk_cb_get_buffer_external_handle_type(struct vk_bundle *vk);

/*!
 * Helper for all of the supported formats to check support for.
 *
 * These are the available formats we will expose to our clients.
 *
 * In order of what we prefer. Start with a SRGB format that works on
 * both OpenGL and Vulkan. The two linear formats that works on both
 * OpenGL and Vulkan. A SRGB format that only works on Vulkan. The last
 * two formats should not be used as they are linear but doesn't have
 * enough bits to express it without resulting in banding.
 *
 * The format VK_FORMAT_A2B10G10R10_UNORM_PACK32 is not listed since
 * 10 bits are not considered enough to do linear colors without
 * banding. If there was a sRGB variant of it then we would have used it
 * instead but there isn't. Since it's not a popular format it's best
 * not to list it rather then listing it and people falling into the
 * trap. The absolute minimum is R11G11B10, but is a really weird format
 * so we are not exposing it.
 *
 * CSCI = Compositor SwapChain Images.
 *
 * @ingroup aux_vk
 */
#define VK_CSCI_FORMATS(THING_COLOR, THING_DS, THING_D, THING_S)                                                       \
	/* color formats */                                                                                            \
	THING_COLOR(R16G16B16A16_UNORM)  /* OGL VK */                                                                  \
	THING_COLOR(R16G16B16A16_SFLOAT) /* OGL VK */                                                                  \
	THING_COLOR(R16G16B16_UNORM)     /* OGL VK - Uncommon. */                                                      \
	THING_COLOR(R16G16B16_SFLOAT)    /* OGL VK - Uncommon. */                                                      \
	THING_COLOR(R8G8B8A8_SRGB)       /* OGL VK */                                                                  \
	THING_COLOR(B8G8R8A8_SRGB)       /* VK */                                                                      \
	THING_COLOR(R8G8B8_SRGB)         /* OGL VK - Uncommon. */                                                      \
	THING_COLOR(R8G8B8A8_UNORM)      /* OGL VK - Bad color precision. */                                           \
	THING_COLOR(B8G8R8A8_UNORM)      /* VK     - Bad color precision. */                                           \
	THING_COLOR(R8G8B8_UNORM)        /* OGL VK - Uncommon. Bad color precision. */                                 \
	THING_COLOR(B8G8R8_UNORM)        /* VK     - Uncommon. Bad color precision. */                                 \
	THING_COLOR(R5G6B5_UNORM_PACK16) /* OLG VK - Bad color precision. */                                           \
	THING_COLOR(R32_SFLOAT)          /* OGL VK */                                                                  \
	/* depth formats */                                                                                            \
	THING_D(D32_SFLOAT)          /* OGL VK */                                                                      \
	THING_D(D16_UNORM)           /* OGL VK */                                                                      \
	THING_D(X8_D24_UNORM_PACK32) /* OGL VK */                                                                      \
	/* depth stencil formats */                                                                                    \
	THING_DS(D24_UNORM_S8_UINT)  /* OGL VK */                                                                      \
	THING_DS(D32_SFLOAT_S8_UINT) /* OGL VK */                                                                      \
	/* stencil format */                                                                                           \
	THING_S(S8_UINT)

/*!
 * Returns the access flags for the compositor to app barriers.
 *
 * CSCI = Compositor SwapChain Images.
 */
VkAccessFlags
vk_csci_get_barrier_access_mask(enum xrt_swapchain_usage_bits bits);

/*!
 * Return the optimal layout for this format, this is the layout as given to the
 * app so is bound to the OpenXR spec.
 *
 * CSCI = Compositor SwapChain Images.
 */
VkImageLayout
vk_csci_get_barrier_optimal_layout(VkFormat format);

/*!
 * Return the barrier aspect mask for this format, this is intended for the
 * barriers that flush the data out before and after transfers between the
 * application and compositor.
 *
 * CSCI = Compositor SwapChain Images.
 */
VkImageAspectFlags
vk_csci_get_barrier_aspect_mask(VkFormat format);

/*!
 * Returns the usage bits for a given selected format and usage.
 *
 * For color formats always adds:
 * * `VK_IMAGE_USAGE_SAMPLED_BIT` for compositor reading in shaders.
 *
 * For depth & stencil formats always adds:
 * * `VK_IMAGE_USAGE_SAMPLED_BIT` for compositor reading in shaders.
 *
 * For depth formats always adds:
 * * `VK_IMAGE_USAGE_SAMPLED_BIT` for compositor reading in shaders.
 *
 * For stencil formats always adds:
 * * `VK_IMAGE_USAGE_SAMPLED_BIT` for compositor reading in shaders.
 *
 * CSCI = Compositor SwapChain Images.
 */
VkImageUsageFlags
vk_csci_get_image_usage_flags(struct vk_bundle *vk, VkFormat format, enum xrt_swapchain_usage_bits bits);

/*!
 * For images views created by the compositor to sample the images, what aspect
 * should be set. For color it's the color, for depth and stencil it's only
 * depth as both are disallowed by the Vulkan spec, for depth only depth, and
 * for stencil only it's stencil.
 *
 * CSCI = Compositor SwapChain Images.
 */
VkImageAspectFlags
vk_csci_get_image_view_aspect(VkFormat format, enum xrt_swapchain_usage_bits bits);

/*!
 * Return the extern handle type that a image should be created with.
 *
 * CSCI = Compositor SwapChain Images.
 */
VkExternalMemoryHandleTypeFlags
vk_csci_get_image_external_handle_type(struct vk_bundle *vk, struct xrt_image_native *xin);

/*!
 * Get whether a given image can be imported/exported for a handle type.
 *
 * CSCI = Compositor SwapChain Images.
 */
void
vk_csci_get_image_external_support(struct vk_bundle *vk,
                                   VkFormat image_format,
                                   enum xrt_swapchain_usage_bits bits,
                                   VkExternalMemoryHandleTypeFlags handle_type,
                                   bool *out_importable,
                                   bool *out_exportable);

/*!
 * Verify if a format is supported for a specific usage
 *
 * CSCI = Compositor SwapChain Images.
 */
bool
vk_csci_is_format_supported(struct vk_bundle *vk,
                            VkFormat format,
                            enum xrt_swapchain_create_flags create,
                            enum xrt_swapchain_usage_bits xbits);


#ifdef __cplusplus
}
#endif
