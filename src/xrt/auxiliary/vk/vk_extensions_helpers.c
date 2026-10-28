// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper functions for Vulkan extension handling during initialization.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vk
 */

#include "util/u_pretty_print.h"
#include "vk_extensions_helpers.h"
#include <stdio.h>


/*
 *
 * Helpers.
 *
 */

static struct u_extension_list *
build_extension_list(struct vk_bundle *vk,
                     struct u_extension_list *available_ext_list,
                     struct u_extension_list *required_ext_list,
                     struct u_extension_list *optional_ext_list,
                     vk_should_skip_ext_func_t skip_func,
                     struct u_extension_list **out_skipped_ext_list)
{
	assert(available_ext_list != NULL);
	assert(required_ext_list != NULL);
	assert(optional_ext_list != NULL);
	assert(skip_func != NULL);
	assert(out_skipped_ext_list != NULL);

	// Start with required extensions (assumed to be supported).
	uint32_t required_ext_count = u_extension_list_get_size(required_ext_list);
	struct u_extension_list_builder *builder =
	    u_extension_list_builder_create_with_capacity(required_ext_count + 16);
	u_extension_list_builder_append_array(builder, u_extension_list_get_data(required_ext_list),
	                                      required_ext_count);

	// Create skipped list builder.
	struct u_extension_list_builder *skipped_builder = u_extension_list_builder_create();

	// Check any supported optional extensions.
	uint32_t optional_ext_count = u_extension_list_get_size(optional_ext_list);
	const char *const *optional_exts = u_extension_list_get_data(optional_ext_list);

	for (uint32_t i = 0; i < optional_ext_count; i++) {
		const char *optional_ext = optional_exts[i];

		// Check if we should skip this extension.
		bool skip = skip_func(vk, required_ext_list, optional_ext_list, optional_ext);
		if (skip) {
			// Track skipped extensions.
			u_extension_list_builder_append_unique(skipped_builder, optional_ext);
			continue;
		}

		// Check if the extension is available.
		if (!u_extension_list_contains(available_ext_list, optional_ext)) {
			continue;
		}

		int added = u_extension_list_builder_append_unique(builder, optional_ext);
		if (added != 1) {
			VK_WARN(vk, "Duplicate extension %s not added twice", optional_ext);
		}
	}

	// Build and return skipped list (sorted for consistent output).
	*out_skipped_ext_list = u_extension_list_builder_build_sorted_for_extensions(&skipped_builder);

	// Build and return the extension list (sorted for consistent output).
	return u_extension_list_builder_build_sorted_for_extensions(&builder);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct u_extension_list *
vk_convert_extension_properties_to_string_list(VkExtensionProperties *props, uint32_t prop_count)
{
	struct u_extension_list_builder *builder = u_extension_list_builder_create_with_capacity(prop_count);
	for (uint32_t i = 0; i < prop_count; i++) {
		u_extension_list_builder_append_unique(builder, props[i].extensionName);
	}
	return u_extension_list_builder_build_sorted_for_extensions(&builder);
}

void
vk_log_extension_list(struct vk_bundle *vk,
                      struct u_extension_list *ext_list,
                      struct u_extension_list *optional_ext_list,
                      struct u_extension_list *skipped_ext_list,
                      const char *ext_type_name,
                      enum u_logging_level log_level)
{
	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	u_pp_string_list_extensions(dg, ext_list, optional_ext_list, skipped_ext_list);

	U_LOG_IFL(log_level, vk->log_level, "Vulkan %s extensions list(s):%s", ext_type_name, sink.buffer);
}

VkResult
vk_check_required_extensions(struct vk_bundle *vk,
                             struct u_extension_list *available_ext_list,
                             struct u_extension_list *required_ext_list,
                             const char *ext_type_name)
{
	struct u_pp_sink_stack_only sink;
	bool have_missing = false;

	// Used to build a nice pretty list of missing extensions.
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	// Check if required extensions are supported.
	uint32_t required_ext_count = u_extension_list_get_size(required_ext_list);
	const char *const *required_exts = u_extension_list_get_data(required_ext_list);
	for (uint32_t i = 0; i < required_ext_count; i++) {
		const char *required_ext = required_exts[i];

		if (u_extension_list_contains(available_ext_list, required_ext)) {
			continue;
		}

		u_pp(dg, "\n\t%s", required_ext);
		have_missing = true;
	}

	if (!have_missing) {
		return VK_SUCCESS;
	}

	VK_ERROR(vk, "Missing required %s extensions:%s", ext_type_name, sink.buffer);
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}


/*
 *
 * 'Exported' instance functions.
 *
 */

VkResult
vk_build_instance_extensions_with_skip(struct vk_bundle *vk,
                                       struct u_extension_list *required_instance_ext_list,
                                       struct u_extension_list *optional_instance_ext_list,
                                       vk_should_skip_ext_func_t skip_func,
                                       struct u_extension_list **out_instance_ext_list)
{
	VkExtensionProperties *props = NULL;
	uint32_t prop_count = 0;
	VkResult ret;

	// Two call.
	ret = vk_enumerate_instance_extensions_properties( //
	    vk,                                            // vk_bundle
	    NULL,                                          // layer_name
	    &prop_count,                                   // out_prop_count
	    &props);                                       // out_props
	VK_CHK_AND_RET(ret, "vk_enumerate_instance_extensions_properties");

	// Convert to extension list for easier checking.
	struct u_extension_list *available_ext_list = vk_convert_extension_properties_to_string_list(props, prop_count);

	// Clean up props.
	free(props);

	// Check if all required extensions are available.
	ret = vk_check_required_extensions(vk, available_ext_list, required_instance_ext_list, "instance");
	if (ret != VK_SUCCESS) {
		u_extension_list_destroy(&available_ext_list);
		return ret;
	}

	// Build the extension list and track skipped extensions.
	struct u_extension_list *skipped_list = NULL;
	struct u_extension_list *list = build_extension_list( //
	    vk,                                               //
	    available_ext_list,                               //
	    required_instance_ext_list,                       //
	    optional_instance_ext_list,                       //
	    skip_func,                                        //
	    &skipped_list);                                   //

	// Clean up available list.
	u_extension_list_destroy(&available_ext_list);

	// Log the result.
	vk_log_extension_list(vk, list, optional_instance_ext_list, skipped_list, "instance", U_LOGGING_INFO);

	// Clean up skipped list.
	u_extension_list_destroy(&skipped_list);

	// Return the list.
	*out_instance_ext_list = list;

	return VK_SUCCESS;
}


/*
 *
 * 'Exported' device functions.
 *
 */

VkResult
vk_build_device_extensions_with_skip(struct vk_bundle *vk,
                                     VkPhysicalDevice physical_device,
                                     struct u_extension_list *required_device_ext_list,
                                     struct u_extension_list *optional_device_ext_list,
                                     vk_should_skip_ext_func_t skip_func,
                                     struct u_extension_list **out_device_ext_list)
{
	VkExtensionProperties *props = NULL;
	uint32_t prop_count = 0;
	VkResult ret;

	ret = vk_enumerate_physical_device_extension_properties( //
	    vk,                                                  // vk_bundle
	    physical_device,                                     // physical_device
	    NULL,                                                // layer_name
	    &prop_count,                                         // out_prop_count
	    &props);                                             // out_props
	VK_CHK_AND_RET(ret, "vk_enumerate_physical_device_extension_properties");

	// Convert to extension list for easier checking.
	struct u_extension_list *available_ext_list = vk_convert_extension_properties_to_string_list(props, prop_count);

	// Clean up props.
	free(props);

	// Check if all required extensions are available.
	ret = vk_check_required_extensions(vk, available_ext_list, required_device_ext_list, "device");
	if (ret != VK_SUCCESS) {
		u_extension_list_destroy(&available_ext_list);
		return ret;
	}

	// Build the extension list and track skipped extensions.
	struct u_extension_list *skipped_list = NULL;
	struct u_extension_list *list = build_extension_list( //
	    vk,                                               //
	    available_ext_list,                               //
	    required_device_ext_list,                         //
	    optional_device_ext_list,                         //
	    skip_func,                                        //
	    &skipped_list);                                   //

	// Clean up available list.
	u_extension_list_destroy(&available_ext_list);

	// Log the result.
	vk_log_extension_list(vk, list, optional_device_ext_list, skipped_list, "device", U_LOGGING_DEBUG);

	// Clean up skipped list.
	u_extension_list_destroy(&skipped_list);

	// Return the list.
	*out_device_ext_list = list;

	return VK_SUCCESS;
}
