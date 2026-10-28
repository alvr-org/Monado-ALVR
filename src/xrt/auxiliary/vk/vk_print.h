// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Printing helper code.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_vk
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_vulkan_includes.h"

#include "util/u_logging.h"


#ifdef __cplusplus
extern "C" {
#endif

struct vk_bundle;

/*
 *
 * Printing helpers.
 *
 */

/*!
 * Print the result of a function, info level if ret == `VK_SUCCESS` and error
 * level otherwise. Also prints file and line.
 *
 * @ingroup aux_vk
 */
void
vk_print_result(
    struct vk_bundle *vk, const char *file, int line, const char *calling_func, VkResult ret, const char *called_func);

/*!
 * Print device information to the logger at the given logging level,
 * if the vk_bundle has that level enabled.
 *
 * @ingroup aux_vk
 */
void
vk_print_device_info(struct vk_bundle *vk,
                     enum u_logging_level log_level,
                     const VkPhysicalDeviceProperties *pdp,
                     uint32_t gpu_index,
                     const char *title);

/*!
 * Print device information about the device that bundle manages at the given
 * logging level if the vk_bundle has that level enabled.
 *
 * @ingroup aux_vk
 */
void
vk_print_opened_device_info(struct vk_bundle *vk, enum u_logging_level log_level);

/*!
 * Print device features to the logger at the given logging level, if the
 * vk_bundle has that level enabled.
 */
void
vk_print_features_info(struct vk_bundle *vk, enum u_logging_level log_level);

/*!
 * Print external handle features to the logger at the given logging level,
 * if the vk_bundle has that level enabled.
 */
void
vk_print_external_handles_info(struct vk_bundle *vk, enum u_logging_level log_level);

/*!
 * Print a @p VkSwapchainCreateInfoKHR, used to log during creation.
 */
void
vk_print_swapchain_create_info(struct vk_bundle *vk, VkSwapchainCreateInfoKHR *i, enum u_logging_level log_level);

#ifdef VK_KHR_display
/*!
 * Print a @p VkDisplaySurfaceCreateInfoKHR, used to log during creation.
 */
void
vk_print_display_surface_create_info(struct vk_bundle *vk,
                                     VkDisplaySurfaceCreateInfoKHR *i,
                                     enum u_logging_level log_level);
#endif

/*!
 * Print queue info to the logger at the given logging level,
 * if the vk_bundle has that level enabled.
 */
void
vk_print_queues_info(const struct vk_bundle *vk, enum u_logging_level log_level);


/*
 *
 * String helper functions.
 *
 */

XRT_CHECK_RESULT const char *
vk_result_string(VkResult code);

XRT_CHECK_RESULT const char *
vk_object_type_string(VkObjectType type);

XRT_CHECK_RESULT const char *
vk_physical_device_type_string(VkPhysicalDeviceType device_type);

XRT_CHECK_RESULT const char *
vk_format_string(VkFormat code);

XRT_CHECK_RESULT const char *
vk_sharing_mode_string(VkSharingMode code);

XRT_CHECK_RESULT const char *
vk_present_mode_string(VkPresentModeKHR code);

XRT_CHECK_RESULT const char *
vk_color_space_string(VkColorSpaceKHR code);

XRT_CHECK_RESULT const char *
vk_power_state_string(VkDisplayPowerStateEXT code);

XRT_CHECK_RESULT const char *
vk_format_string(VkFormat code);


#ifdef __cplusplus
}
#endif
