// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Vulkan helpers for formats.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_vk
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_vulkan_includes.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Given a UNORM format, return the corresponding sRGB format.
 *
 * @param unorm The UNORM format to convert.
 * @return      The corresponding sRGB format, or VK_FORMAT_UNDEFINED if the input format is not recognized.
 *
 * @ingroup aux_vk
 */
VkFormat
vk_format_convert_unorm_to_srgb(VkFormat unorm);

/*!
 * Given an sRGB format, return the corresponding UNORM format.
 *
 * @param srgb The sRGB format to convert.
 * @return     The corresponding UNORM format, or VK_FORMAT_UNDEFINED if the input format is not recognized.
 *
 * @ingroup aux_vk
 */
VkFormat
vk_format_convert_srgb_to_unorm(VkFormat srgb);


#ifdef __cplusplus
}
#endif
