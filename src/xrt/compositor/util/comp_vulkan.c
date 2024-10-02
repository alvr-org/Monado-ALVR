// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Vulkan code for compositors.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup comp_util
 */

#include "os/os_time.h"

#include "util/u_handles.h"
#include "util/u_trace_marker.h"

#include "util/comp_vulkan.h"


/*
 *
 * Helper functions.
 *
 */

#define VK_ERROR_RET(VK, FUNC, MSG, RET) VK_ERROR(VK, FUNC ": %s\n\t" MSG, vk_result_string(RET))

#define UUID_STR_SIZE (XRT_UUID_SIZE * 3 + 1)

static void
snprint_luid(char *str, size_t size, xrt_luid_t *luid)
{
	for (size_t i = 0, offset = 0; i < ARRAY_SIZE(luid->data) && offset < size; i++, offset += 3) {
		snprintf(str + offset, size - offset, "%02x ", luid->data[i]);
	}
}

static void
snprint_uuid(char *str, size_t size, xrt_uuid_t *uuid)
{
	for (size_t i = 0, offset = 0; i < ARRAY_SIZE(uuid->data) && offset < size; i++, offset += 3) {
		snprintf(str + offset, size - offset, "%02x ", uuid->data[i]);
	}
}

static bool
get_device_id_props(struct vk_bundle *vk, int gpu_index, VkPhysicalDeviceIDProperties *out_id_props)
{
	VkPhysicalDeviceIDProperties pdidp = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
	};

	VkPhysicalDeviceProperties2 pdp2 = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
	    .pNext = &pdidp,
	};

	VkPhysicalDevice *phys = NULL;
	uint32_t gpu_count = 0;
	VkResult ret;

	ret = vk_enumerate_physical_devices( //
	    vk,                              // vk_bundle
	    &gpu_count,                      // out_physical_device_count
	    &phys);                          // out_physical_devices
	if (ret != VK_SUCCESS) {
		VK_ERROR_RET(vk, "vk_enumerate_physical_devices", "Failed to enumerate physical devices.", ret);
		return false;
	}
	if (gpu_count == 0) {
		VK_ERROR(vk, "vk_enumerate_physical_devices: Returned zero physical devices!");
		return false;
	}

	vk->vkGetPhysicalDeviceProperties2(phys[gpu_index], &pdp2);
	free(phys);

	*out_id_props = pdidp;

	return true;
}

static bool
get_device_uuid(struct vk_bundle *vk, int gpu_index, xrt_uuid_t *uuid)
{
	VkPhysicalDeviceIDProperties pdidp = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
	};

	if (!get_device_id_props(vk, gpu_index, &pdidp)) {
		VK_ERROR(vk, "get_device_id_props: false");
		return false;
	}

	memcpy(uuid->data, pdidp.deviceUUID, ARRAY_SIZE(uuid->data));

	return true;
}

static bool
get_device_luid(struct vk_bundle *vk, int gpu_index, xrt_luid_t *luid)
{
	VkPhysicalDeviceIDProperties pdidp = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
	};

	if (!get_device_id_props(vk, gpu_index, &pdidp)) {
		VK_ERROR(vk, "get_device_id_props: false");
		return false;
	}

	// Is the LUID even valid?
	if (pdidp.deviceLUIDValid != VK_TRUE) {
		return false;
	}

	memcpy(luid->data, pdidp.deviceLUID, ARRAY_SIZE(luid->data));

	return true;
}

VkResult
fill_in_results(struct vk_bundle *vk, const struct comp_vulkan_arguments *vk_args, struct comp_vulkan_results *vk_res)
{
	// Grab the device index from the vk_bundle
	vk_res->selected_gpu_index = vk->physical_device_index;

	// Grab the suggested device index for the client to use
	vk_res->client_gpu_index = vk_args->client_gpu_index;

	// Store physical device UUID for compositor in settings
	if (vk_res->selected_gpu_index >= 0) {
		if (get_device_uuid(vk, vk_res->selected_gpu_index, &vk_res->selected_gpu_deviceUUID)) {
			char uuid_str[UUID_STR_SIZE] = {0};
			snprint_uuid(uuid_str, ARRAY_SIZE(uuid_str), &vk_res->selected_gpu_deviceUUID);

			VK_DEBUG(vk, "Selected %d with uuid: %s", vk_res->selected_gpu_index, uuid_str);
		} else {
			VK_ERROR(vk, "Failed to get device %d uuid", vk_res->selected_gpu_index);
		}
	}

	// By default suggest GPU used by compositor to clients
	if (vk_res->client_gpu_index < 0) {
		vk_res->client_gpu_index = vk_res->selected_gpu_index;
	}

	// Store physical device UUID suggested to clients in settings
	if (vk_res->client_gpu_index >= 0) {
		if (get_device_uuid(vk, vk_res->client_gpu_index, &vk_res->client_gpu_deviceUUID)) {
			char buffer[UUID_STR_SIZE] = {0};
			snprint_uuid(buffer, ARRAY_SIZE(buffer), &vk_res->client_gpu_deviceUUID);

			// Trailing space from snprint_uuid, means 'to' should be right next to '%s'.
			VK_DEBUG(vk, "Suggest %d with uuid: %sto clients", vk_res->client_gpu_index, buffer);

			if (get_device_luid(vk, vk_res->client_gpu_index, &vk_res->client_gpu_deviceLUID)) {
				vk_res->client_gpu_deviceLUID_valid = true;
				snprint_luid(buffer, ARRAY_SIZE(buffer), &vk_res->client_gpu_deviceLUID);
				VK_DEBUG(vk, "\tDevice LUID: %s", buffer);
			}
		} else {
			VK_ERROR(vk, "Failed to get device %d uuid", vk_res->client_gpu_index);
		}
	}

	return VK_SUCCESS;
}

/*
 *
 * Creation functions.
 *
 */

static VkResult
create_instance(struct vk_bundle *vk, const struct comp_vulkan_arguments *vk_args)
{
	struct u_string_list *instance_ext_list = NULL;
	VkResult ret;

	assert(vk_args->required_instance_version != 0);


	/*
	 * Extension handling.
	 */

	// Check required extensions, results in clearer error message.
	ret = vk_check_required_instance_extensions(vk, vk_args->required_instance_extensions);
	if (ret == VK_ERROR_EXTENSION_NOT_PRESENT) {
		return ret; // Already printed.
	}
	if (ret != VK_SUCCESS) {
		VK_ERROR_RET(vk, "vk_check_required_instance_extensions", "Failed to check required extension(s)", ret);
		return ret;
	}

	// Build extension list.
	instance_ext_list = vk_build_instance_extensions( //
	    vk,                                           //
	    vk_args->required_instance_extensions,        //
	    vk_args->optional_instance_extensions);       //
	if (!instance_ext_list) {
		VK_ERROR(vk, "vk_build_instance_extensions: Failed to be list");
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}


	/*
	 * Direct arguments.
	 */

	VkApplicationInfo app_info = {
	    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	    .pApplicationName = "Monado Compositor",
	    .pEngineName = "Monado",
	    .apiVersion = vk_args->required_instance_version,
	};

	VkInstanceCreateInfo instance_info = {
	    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	    .pApplicationInfo = &app_info,
	    .enabledExtensionCount = u_string_list_get_size(instance_ext_list),
	    .ppEnabledExtensionNames = u_string_list_get_data(instance_ext_list),
	};

	ret = vk->vkCreateInstance(&instance_info, NULL, &vk->instance);
	if (ret != VK_SUCCESS) {
		VK_ERROR_RET(vk, "vkCreateInstance", "Failed to create Vulkan instance", ret);
		return ret;
	}

	VK_NAME_INSTANCE(vk, vk->instance, "monado vulkan instance");

	/*
	 * Post creation setup of Vulkan bundle.
	 */

	// Set information about instance after it has been created.
	vk->version = vk_args->required_instance_version;

	// Needs to be filled in before getting functions.
	vk_fill_in_has_instance_extensions(vk, instance_ext_list);

	u_string_list_destroy(&instance_ext_list);

	ret = vk_get_instance_functions(vk);
	if (ret != VK_SUCCESS) {
		VK_ERROR_RET(vk, "vk_get_instance_functions", "Failed to get Vulkan instance functions.", ret);
		return ret;
	}

	return ret;
}

static VkResult
create_device(struct vk_bundle *vk, const struct comp_vulkan_arguments *vk_args)
{
	VkResult ret;

	const char *prio_strs[3] = {
	    "QUEUE_GLOBAL_PRIORITY_REALTIME",
	    "QUEUE_GLOBAL_PRIORITY_HIGH",
	    "QUEUE_GLOBAL_PRIORITY_MEDIUM",
	};

	VkQueueGlobalPriorityEXT prios[3] = {
	    VK_QUEUE_GLOBAL_PRIORITY_REALTIME_EXT, // This is the one we really want.
	    VK_QUEUE_GLOBAL_PRIORITY_HIGH_EXT,     // Probably not as good but something.
	    VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT,   // Default fallback.
	};

	const bool only_compute_queue = vk_args->only_compute_queue;

	struct vk_device_features device_features = {
	    .shader_image_gather_extended = true,
	    .shader_storage_image_write_without_format = true,
	    .null_descriptor = only_compute_queue,
	    .timeline_semaphore = vk_args->timeline_semaphore,
	    .synchronization_2 = true,
	};

	ret = vk_init_mutex(vk);
	if (ret != VK_SUCCESS) {
		VK_ERROR_RET(vk, "vk_init_mutex", "Failed to init mutex.", ret);
		return ret;
	}

	// No other way then to try to see if realtime is available.
	for (size_t i = 0; i < ARRAY_SIZE(prios); i++) {
		ret = vk_create_device(                  //
		    vk,                                  //
		    vk_args->selected_gpu_index,         //
		    only_compute_queue,                  // compute_only
		    prios[i],                            // global_priority
		    vk_args->required_device_extensions, //
		    vk_args->optional_device_extensions, //
		    &device_features);                   // optional_device_features

		// All ok!
		if (ret == VK_SUCCESS) {
			VK_INFO(vk, "Created device and %s queue with %s.", only_compute_queue ? "COMPUTE" : "GRAPHICS",
			        prio_strs[i]);
			break;
		}

		// Try a lower priority.
		if (ret == VK_ERROR_NOT_PERMITTED_EXT) {
			continue;
		}

		// Some other error!
		VK_ERROR_RET(vk, "vk_create_device", "Failed to create Vulkan device.", ret);
		return ret;
	}

	// All tries failed, return error. Yes this code is clunky.
	if (ret != VK_SUCCESS) {
		VK_ERROR_RET(vk, "vk_create_device", "Failed to create Vulkan device.", ret);
		return ret;
	}

	// Print device information.
	vk_print_opened_device_info(vk, U_LOGGING_INFO);

	// Print features enabled.
	vk_print_features_info(vk, U_LOGGING_INFO);

	// Now that we are done debug some used external handles.
	vk_print_external_handles_info(vk, U_LOGGING_INFO);

	return VK_SUCCESS;
}


/*
 *
 * 'Exported' function.
 *
 */

bool
comp_vulkan_init_bundle(struct vk_bundle *vk,
                        const struct comp_vulkan_arguments *vk_args,
                        struct comp_vulkan_results *vk_res)
{
	VkResult ret;

	vk->log_level = vk_args->log_level;

	ret = vk_get_loader_functions(vk, vk_args->get_instance_proc_address);
	if (ret != VK_SUCCESS) {
		VK_ERROR_RET(vk, "vk_get_loader_functions", "Failed to get VkInstance get process address.", ret);
		return false;
	}

	ret = create_instance(vk, vk_args);
	if (ret != VK_SUCCESS) {
		// Error already reported.
		return false;
	}

	ret = create_device(vk, vk_args);
	if (ret != VK_SUCCESS) {
		// Error already reported.
		return false;
	}

	ret = fill_in_results(vk, vk_args, vk_res);
	if (ret != VK_SUCCESS) {
		// Error already reported.
		return false;
	}

	return true;
}

void
comp_vulkan_formats_check(struct vk_bundle *vk, struct comp_vulkan_formats *formats)
{
#define CHECK_COLOR(FORMAT)                                                                                            \
	formats->has_##FORMAT = vk_csci_is_format_supported(vk, VK_FORMAT_##FORMAT, XRT_SWAPCHAIN_USAGE_COLOR);
#define CHECK_DS(FORMAT)                                                                                               \
	formats->has_##FORMAT = vk_csci_is_format_supported(vk, VK_FORMAT_##FORMAT, XRT_SWAPCHAIN_USAGE_DEPTH_STENCIL);

	VK_CSCI_FORMATS(CHECK_COLOR, CHECK_DS, CHECK_DS, CHECK_DS)

#undef CHECK_COLOR
#undef CHECK_DS

#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER)
	/*
	 * Some Vulkan drivers will natively support importing and exporting
	 * SRGB formats (Qualcomm Adreno) even tho technically that's not intended
	 * by the AHardwareBuffer since they don't support sRGB formats.
	 * While others (arm Mali) does not support importing and exporting sRGB
	 * formats.
	 */
	if (!formats->has_R8G8B8A8_SRGB && formats->has_R8G8B8A8_UNORM) {
		formats->has_R8G8B8A8_SRGB = true;
		formats->emulated_R8G8B8A8_SRGB = true;
	}
#endif
}

void
comp_vulkan_formats_copy_to_info(const struct comp_vulkan_formats *formats, struct xrt_compositor_info *info)
{
	uint32_t format_count = 0;

#define ADD_IF_SUPPORTED(FORMAT)                                                                                       \
	if (formats->has_##FORMAT) {                                                                                   \
		info->formats[format_count++] = VK_FORMAT_##FORMAT;                                                    \
	}

	VK_CSCI_FORMATS(ADD_IF_SUPPORTED, ADD_IF_SUPPORTED, ADD_IF_SUPPORTED, ADD_IF_SUPPORTED)

#undef ADD_IF_SUPPORTED

	assert(format_count <= XRT_MAX_SWAPCHAIN_FORMATS);
	info->format_count = format_count;
}

void
comp_vulkan_formats_log(enum u_logging_level log_level, const struct comp_vulkan_formats *formats)
{
#define PRINT_NAME(FORMAT) "\n\tVK_FORMAT_" #FORMAT ": %s"
#define PRINT_BOOLEAN(FORMAT) , formats->has_##FORMAT ? "true" : "false"

	U_LOG_IFL_I(log_level, "Supported formats:"                                             //
	            VK_CSCI_FORMATS(PRINT_NAME, PRINT_NAME, PRINT_NAME, PRINT_NAME)             //
	            VK_CSCI_FORMATS(PRINT_BOOLEAN, PRINT_BOOLEAN, PRINT_BOOLEAN, PRINT_BOOLEAN) //
	);

#undef PRINT_NAME
#undef PRINT_BOOLEAN

#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER)
	U_LOG_IFL_I(log_level,
	            "Emulated formats:"
	            "\n\tVK_FORMAT_R8G8B8A8_SRGB: %s",
	            formats->emulated_R8G8B8A8_SRGB ? "emulated" : "native");
#endif
}
