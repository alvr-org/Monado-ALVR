// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper functions for Vulkan extension handling during initialization.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vk
 */

#pragma once

#include "vk/vk_helpers.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Callback type for checking if an optional extension should be skipped.
 */
typedef bool (*vk_should_skip_ext_func_t)(struct vk_bundle *vk,
                                          struct u_extension_list *required_ext_list,
                                          struct u_extension_list *optional_ext_list,
                                          const char *ext);


/*!
 * Convert VkExtensionProperties array to u_extension_list.
 */
struct u_extension_list *
vk_convert_extension_properties_to_string_list(VkExtensionProperties *props, uint32_t prop_count);

/*!
 * Log an extension list using pretty printing.
 * The list will be sorted and logged at the specified log level. The argument
 * skipped_ext_list is used to distinguish between skipped and unsupported
 * extensions (must not be NULL).
 */
void
vk_log_extension_list(struct vk_bundle *vk,
                      struct u_extension_list *ext_list,
                      struct u_extension_list *optional_ext_list,
                      struct u_extension_list *skipped_ext_list,
                      const char *ext_type_name,
                      enum u_logging_level log_level);

/*!
 * Check if all required extensions are present in the available extensions
 * list. Prints a clear error message with all missing extensions if not all
 * required extensions are available. Returns VK_SUCCESS if all required
 * extensions are available, VK_ERROR_EXTENSION_NOT_PRESENT otherwise.
 */
VkResult
vk_check_required_extensions(struct vk_bundle *vk,
                             struct u_extension_list *available_ext_list,
                             struct u_extension_list *required_ext_list,
                             const char *ext_type_name);

/*!
 * Build instance extensions from required and optional instance extensions with
 * skip callback.
 *
 * This function enumerates available instance extensions, checks required ones,
 * and builds a final list. Returns VK_SUCCESS if successful.
 * Only requires @ref vk_get_loader_functions to have been called.
 *
 * This internal function, use @ref vk_build_instance_extensions() instead.
 *
 * @param skip_func Callback to determine if an optional extension should be skipped,
 *                  must not be NULL.
 */
VkResult
vk_build_instance_extensions_with_skip(struct vk_bundle *vk,
                                       struct u_extension_list *required_instance_ext_list,
                                       struct u_extension_list *optional_instance_ext_list,
                                       vk_should_skip_ext_func_t skip_func,
                                       struct u_extension_list **out_instance_ext_list);

/*!
 * Build device extensions from required and optional device extensions with
 * skip callback. This function enumerates available device extensions, checks
 * required ones, and builds a final list. Returns VK_SUCCESS if successful.
 *
 * This internal function, typically called from @ref vk_create_device().
 *
 * @param skip_func Callback to determine if an optional extension should be skipped,
 *                  must not be NULL.
 */
VkResult
vk_build_device_extensions_with_skip(struct vk_bundle *vk,
                                     VkPhysicalDevice physical_device,
                                     struct u_extension_list *required_device_ext_list,
                                     struct u_extension_list *optional_device_ext_list,
                                     vk_should_skip_ext_func_t skip_func,
                                     struct u_extension_list **out_device_ext_list);


#ifdef __cplusplus
}
#endif
