// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Common OpenGL code header.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_ogl
 */

#pragma once

#include "xrt/xrt_compositor.h"

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Results from a import, nicer then having to pass in multiple arrays.
 *
 * @ingroup aux_ogl
 */
struct ogl_import_results
{
	//! Imported textures.
	uint32_t textures[XRT_MAX_SWAPCHAIN_IMAGES];

	//! Memory objects for imported textures.
	uint32_t memories[XRT_MAX_SWAPCHAIN_IMAGES];

	//! The count of textures and memories.
	uint32_t image_count;

	//! Dimensions.
	uint32_t width, height;
};

/*!
 * Determine the texture target and the texture binding parameter to
 * save/restore for creation/use of an OpenGL texture from the given info.
 *
 * @ingroup aux_ogl
 */
void
ogl_texture_target_for_swapchain_info(const struct xrt_swapchain_create_info *info,
                                      uint32_t *out_tex_target,
                                      uint32_t *out_tex_param_name);

/*!
 * Converts a Vulkan format to corresponding OpenGL format,
 * will return 0 if no mapping exist for the given format.
 *
 * @ingroup aux_ogl
 */
XRT_CHECK_RESULT uint32_t
ogl_vk_format_to_gl(int64_t vk_format);

/*!
 * Import native images, a context needs to be current when called.
 *
 * @ingroup aux_ogl
 */
XRT_CHECK_RESULT bool
ogl_import_from_native(struct xrt_image_native *natives,
                       uint32_t native_count,
                       const struct xrt_swapchain_create_info *info,
                       struct ogl_import_results *results);


#ifdef __cplusplus
}
#endif
