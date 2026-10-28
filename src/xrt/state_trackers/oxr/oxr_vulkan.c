// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds Vulkan related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_extension_list.h"

#include "vk/vk_helpers.h"

#include "xrt/xrt_gfx_vk.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_two_call.h"


/*
 *
 * Helpers
 *
 */

#define GET_PROC(INST, NAME) PFN_vk##NAME loaded_##NAME = (PFN_vk##NAME)vulkanGetInstanceProcAddr(INST, "vk" #NAME)

#define UUID_STR_SIZE (XRT_UUID_SIZE * 3 + 1)

static void
snprint_uuid(char *str, size_t size, const xrt_uuid_t *uuid)
{
	for (size_t i = 0, offset = 0; i < ARRAY_SIZE(uuid->data) && offset < size; i++, offset += 3) {
		snprintf(str + offset, size - offset, "%02x ", uuid->data[i]);
	}
}

static void
snprint_luid(char *str, size_t size, xrt_luid_t *luid)
{
	for (size_t i = 0, offset = 0; i < ARRAY_SIZE(luid->data) && offset < size; i++, offset += 3) {
		snprintf(str + offset, size - offset, "%02x ", luid->data[i]);
	}
}


/*
 *
 * Misc functions (to be organized).
 *
 */

XrResult
oxr_vk_get_instance_exts(struct oxr_logger *log,
                         struct oxr_system *sys,
                         uint32_t namesCapacityInput,
                         uint32_t *namesCountOutput,
                         char *namesString)
{
	size_t length = strlen(xrt_gfx_vk_instance_extensions) + 1;

	OXR_TWO_CALL_HELPER(log, namesCapacityInput, namesCountOutput, namesString, length,
	                    xrt_gfx_vk_instance_extensions, XR_SUCCESS);
}

XrResult
oxr_vk_get_device_exts(struct oxr_logger *log,
                       struct oxr_system *sys,
                       uint32_t namesCapacityInput,
                       uint32_t *namesCountOutput,
                       char *namesString)
{
	size_t length = strlen(xrt_gfx_vk_device_extensions) + 1;

	OXR_TWO_CALL_HELPER(log, namesCapacityInput, namesCountOutput, namesString, length,
	                    xrt_gfx_vk_device_extensions, XR_SUCCESS);
}

XrResult
oxr_vk_get_requirements(struct oxr_logger *log,
                        struct oxr_system *sys,
                        XrGraphicsRequirementsVulkanKHR *graphicsRequirements)
{
	struct xrt_api_requirements ver;

	xrt_gfx_vk_get_versions(&ver);
	graphicsRequirements->minApiVersionSupported = XR_MAKE_VERSION(ver.min_major, ver.min_minor, ver.min_patch);
	graphicsRequirements->maxApiVersionSupported = XR_MAKE_VERSION(ver.max_major, ver.max_minor, ver.max_patch);

	sys->gotten_requirements = true;

	return XR_SUCCESS;
}

DEBUG_GET_ONCE_LOG_OPTION(compositor_log, "XRT_COMPOSITOR_LOG", U_LOGGING_WARN)

//! @todo extension lists are duplicated as long strings in comp_vk_glue.c
static const char *required_vk_instance_extensions[] = {
    VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,      //
    VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,     //
    VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,  //
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, //
};

static const char *optional_vk_instance_extensions[] = {
#if defined(VK_EXT_debug_utils)
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

// The device extensions do vary by platform, but in a very regular way.
// This should match the list in comp_compositor, except it shouldn't include
// VK_KHR_SWAPCHAIN_EXTENSION_NAME
static const char *required_vk_device_extensions[] = {
    VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,      //
    VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,            //
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,           //
    VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,        //
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, //

// Platform version of "external_memory"
#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_FD)
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,

#elif defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER)
    VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME,
    VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_1_EXTENSION_NAME,
    VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
    VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME,

#elif defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_WIN32_HANDLE)
    VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
#else
#error "Need port!"
#endif

// Platform version of "external_fence" and "external_semaphore"
#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD) // Optional

#elif defined(XRT_GRAPHICS_SYNC_HANDLE_IS_WIN32_HANDLE)
    VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
    VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME,

#else
#error "Need port!"
#endif
};

static const char *optional_device_extensions[] = {
#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)
    VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME,

#elif defined(XRT_GRAPHICS_SYNC_HANDLE_IS_WIN32_HANDLE) // Not optional

#else
#error "Need port!"
#endif

#ifdef VK_KHR_image_format_list
    VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
#endif
#ifdef VK_KHR_timeline_semaphore
    VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
#endif

#ifdef VK_EXT_image_tiling_control
    VK_EXT_IMAGE_TILING_CONTROL_EXTENSION_NAME,
#else
    NULL, // avoid zero sized array with UB
#endif
};

static bool
vk_check_extension(VkExtensionProperties *props, uint32_t prop_count, const char *ext)
{
	for (uint32_t i = 0; i < prop_count; i++) {
		if (strcmp(props[i].extensionName, ext) == 0) {
			return true;
		}
	}

	return false;
}

static XrResult
vk_get_instance_ext_props(struct oxr_logger *log,
                          PFN_vkGetInstanceProcAddr vulkanGetInstanceProcAddr,
                          VkExtensionProperties **out_props,
                          uint32_t *out_prop_count)
{
	GET_PROC(VK_NULL_HANDLE, EnumerateInstanceExtensionProperties);

	if (!loaded_EnumerateInstanceExtensionProperties) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Failed to get vkEnumerateInstanceExtensionProperties function pointer.");
	}

	uint32_t prop_count = 0;
	VkResult res = loaded_EnumerateInstanceExtensionProperties(NULL, &prop_count, NULL);
	if (res != VK_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Failed to enumerate instance extension properties count (%d)", res);
	}


	VkExtensionProperties *props = U_TYPED_ARRAY_CALLOC(VkExtensionProperties, prop_count);

	res = loaded_EnumerateInstanceExtensionProperties(NULL, &prop_count, props);
	if (res != VK_SUCCESS) {
		free(props);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Failed to enumerate instance extension properties (%d)", res);
	}

	*out_props = props;
	*out_prop_count = prop_count;

	return XR_SUCCESS;
}

XrResult
oxr_vk_create_vulkan_instance(struct oxr_logger *log,
                              struct oxr_system *sys,
                              const XrVulkanInstanceCreateInfoKHR *createInfo,
                              VkInstance *vulkanInstance,
                              VkResult *vulkanResult)
{

	PFN_vkGetInstanceProcAddr vulkanGetInstanceProcAddr = createInfo->pfnGetInstanceProcAddr;
	GET_PROC(NULL, CreateInstance);

	if (!loaded_CreateInstance) {
		//! @todo: clarify in spec
		*vulkanResult = VK_ERROR_INITIALIZATION_FAILED;
		return XR_SUCCESS;
	}

	VkExtensionProperties *props = NULL;
	uint32_t prop_count = 0;
	XrResult res = vk_get_instance_ext_props(log, createInfo->pfnGetInstanceProcAddr, &props, &prop_count);
	if (res != XR_SUCCESS) {
		return res;
	}

	struct u_extension_list_builder *instance_ext_builder = u_extension_list_builder_create_from_array(
	    required_vk_instance_extensions, ARRAY_SIZE(required_vk_instance_extensions));

#if defined(VK_EXT_debug_utils)
	bool debug_utils_enabled = false;
#endif

	for (uint32_t i = 0; i < ARRAY_SIZE(optional_vk_instance_extensions); i++) {

		if (optional_vk_instance_extensions[i] == NULL ||
		    !vk_check_extension(props, prop_count, optional_vk_instance_extensions[i])) {
			continue;
		}

		u_extension_list_builder_append_unique(instance_ext_builder, optional_vk_instance_extensions[i]);

#if defined(VK_EXT_debug_utils)
		if (strcmp(optional_vk_instance_extensions[i], VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
			debug_utils_enabled = true;
		}
#endif
	}

	for (uint32_t i = 0; i < createInfo->vulkanCreateInfo->enabledExtensionCount; i++) {
		u_extension_list_builder_append_unique(instance_ext_builder,
		                                       createInfo->vulkanCreateInfo->ppEnabledExtensionNames[i]);
	}

	struct u_extension_list *instance_ext_list = u_extension_list_builder_build(&instance_ext_builder);

	VkInstanceCreateInfo modified_info = *createInfo->vulkanCreateInfo;
	modified_info.ppEnabledExtensionNames = u_extension_list_get_data(instance_ext_list);
	modified_info.enabledExtensionCount = u_extension_list_get_size(instance_ext_list);

	*vulkanResult = loaded_CreateInstance(&modified_info, createInfo->vulkanAllocator, vulkanInstance);


	// Logging
	{
		struct oxr_sink_logger slog = {0};

		oxr_slog(&slog, "Creation of VkInstance:");
		oxr_slog(&slog, "\n\tresult: %s", vk_result_string(*vulkanResult));
		oxr_slog(&slog, "\n\tvulkanInstance: 0x%" PRIx64, (uint64_t)(intptr_t)*vulkanInstance);
		oxr_slog(&slog, "\n\textensions:");
		for (uint32_t i = 0; i < modified_info.enabledExtensionCount; i++) {
			oxr_slog(&slog, "\n\t\t%s", modified_info.ppEnabledExtensionNames[i]);
		}

		oxr_log_slog(log, &slog);
	}

#if defined(VK_EXT_debug_utils)
	if (*vulkanResult == VK_SUCCESS) {
		sys->vk.debug_utils_enabled = debug_utils_enabled;
	}
#endif

	u_extension_list_destroy(&instance_ext_list);

	return XR_SUCCESS;
}

static XrResult
vk_get_device_ext_props(struct oxr_logger *log,
                        VkInstance vulkanInstance,
                        PFN_vkGetInstanceProcAddr vulkanGetInstanceProcAddr,
                        VkPhysicalDevice physical_device,
                        VkExtensionProperties **out_props,
                        uint32_t *out_prop_count)
{
	GET_PROC(vulkanInstance, EnumerateDeviceExtensionProperties);

	if (!loaded_EnumerateDeviceExtensionProperties) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Failed to get vkEnumerateDeviceExtensionProperties function pointer");
	}

	uint32_t prop_count = 0;
	VkResult res = loaded_EnumerateDeviceExtensionProperties(physical_device, NULL, &prop_count, NULL);
	if (res != VK_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Failed to enumerate device extension properties count (%d)", res);
	}


	VkExtensionProperties *props = U_TYPED_ARRAY_CALLOC(VkExtensionProperties, prop_count);

	res = loaded_EnumerateDeviceExtensionProperties(physical_device, NULL, &prop_count, props);
	if (res != VK_SUCCESS) {
		free(props);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to enumerate device extension properties (%d)",
		                 res);
	}

	*out_props = props;
	*out_prop_count = prop_count;

	return XR_SUCCESS;
}

static XrResult
vk_get_device_features(struct oxr_logger *log,
                       VkInstance vulkanInstance,
                       PFN_vkGetInstanceProcAddr vulkanGetInstanceProcAddr,
                       VkPhysicalDevice physical_device,
                       VkPhysicalDeviceFeatures2KHR *physical_device_features)
{
	GET_PROC(vulkanInstance, GetPhysicalDeviceFeatures2KHR);

	if (!loaded_GetPhysicalDeviceFeatures2KHR) {
		oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to get vkGetPhysicalDeviceFeatures2KHR fp");
	}

	loaded_GetPhysicalDeviceFeatures2KHR( //
	    physical_device,                  // physicalDevice
	    physical_device_features);        // pFeatures

	return XR_SUCCESS;
}

static inline VkBaseInStructure const *
vk_find_struct_in_chain(const VkBaseInStructure *base, VkStructureType type)
{
	while (base != NULL) {
		if (base->sType == type) {
			return base;
		}
		base = base->pNext;
	}
	return NULL;
}

#ifdef VK_KHR_timeline_semaphore
/*!
 * Finds the app's timelineSemaphore value in whichever feature struct carries
 * it. A valid chain holds at most one of the two (VUID-VkDeviceCreateInfo-pNext-02830).
 */
static bool
find_app_timeline_semaphore_feature(const VkDeviceCreateInfo *info, VkBool32 *out_enabled)
{
	const VkBaseInStructure *base = (const VkBaseInStructure *)info;

	const VkBaseInStructure *found =
	    vk_find_struct_in_chain(base, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES);
	if (found != NULL) {
		*out_enabled = ((const VkPhysicalDeviceTimelineSemaphoreFeatures *)found)->timelineSemaphore;
		return true;
	}

#ifdef VK_VERSION_1_2
	found = vk_find_struct_in_chain(base, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
	if (found != NULL) {
		*out_enabled = ((const VkPhysicalDeviceVulkan12Features *)found)->timelineSemaphore;
		return true;
	}
#endif

	return false;
}
#endif

XrResult
oxr_vk_create_vulkan_device(struct oxr_logger *log,
                            struct oxr_system *sys,
                            const XrVulkanDeviceCreateInfoKHR *createInfo,
                            VkDevice *vulkanDevice,
                            VkResult *vulkanResult)
{
	XrResult res;

	PFN_vkGetInstanceProcAddr vulkanGetInstanceProcAddr = createInfo->pfnGetInstanceProcAddr;
	GET_PROC(sys->vulkan_enable2_instance, CreateDevice);

	if (!loaded_CreateDevice) {
		//! @todo: clarify in spec
		*vulkanResult = VK_ERROR_INITIALIZATION_FAILED;
		return XR_SUCCESS;
	}

	VkPhysicalDevice physical_device = createInfo->vulkanPhysicalDevice;

	struct u_extension_list_builder *device_extension_builder = u_extension_list_builder_create_from_array(
	    required_vk_device_extensions, ARRAY_SIZE(required_vk_device_extensions));

	for (uint32_t i = 0; i < createInfo->vulkanCreateInfo->enabledExtensionCount; i++) {
		u_extension_list_builder_append_unique(device_extension_builder,
		                                       createInfo->vulkanCreateInfo->ppEnabledExtensionNames[i]);
	}



	VkExtensionProperties *props = NULL;
	uint32_t prop_count = 0;
	res = vk_get_device_ext_props(log, sys->vulkan_enable2_instance, createInfo->pfnGetInstanceProcAddr,
	                              physical_device, &props, &prop_count);
	if (res != XR_SUCCESS) {
		return res;
	}

#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)
	bool external_fence_fd_enabled = false;
	bool external_semaphore_fd_enabled = false;
#endif
	bool image_format_list_enabled = false;

	for (uint32_t i = 0; i < ARRAY_SIZE(optional_device_extensions); i++) {
		// Empty list or a not supported extension.
		if (optional_device_extensions[i] == NULL ||
		    !vk_check_extension(props, prop_count, optional_device_extensions[i])) {
			continue;
		}

		u_extension_list_builder_append_unique(device_extension_builder, optional_device_extensions[i]);

#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)
		if (strcmp(optional_device_extensions[i], VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME) == 0) {
			external_fence_fd_enabled = true;
		}
		if (strcmp(optional_device_extensions[i], VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME) == 0) {
			external_semaphore_fd_enabled = true;
		}
#endif

		if (strcmp(optional_device_extensions[i], VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0) {
			image_format_list_enabled = true;
		}
	}

	free(props);

	struct u_extension_list *device_extension_list = u_extension_list_builder_build(&device_extension_builder);

	VkPhysicalDeviceFeatures2KHR physical_device_features = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,
	    .pNext = NULL,
	};

#ifdef VK_KHR_timeline_semaphore
	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timeline_semaphore_info = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR,
	    .pNext = NULL,
	    .timelineSemaphore = VK_FALSE,
	};

	if (u_extension_list_contains(device_extension_list, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) {
		physical_device_features.pNext = &timeline_semaphore_info;
	}
#endif

	res = vk_get_device_features(log, sys->vulkan_enable2_instance, createInfo->pfnGetInstanceProcAddr,
	                             physical_device, &physical_device_features);
	if (res != XR_SUCCESS) {
		return res;
	}


	VkDeviceCreateInfo modified_info = *createInfo->vulkanCreateInfo;
	modified_info.ppEnabledExtensionNames = u_extension_list_get_data(device_extension_list);
	modified_info.enabledExtensionCount = u_extension_list_get_size(device_extension_list);

#ifdef VK_KHR_timeline_semaphore
	VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
	    .pNext = NULL,
	    .timelineSemaphore = timeline_semaphore_info.timelineSemaphore,
	};

	if (timeline_semaphore_info.timelineSemaphore) {
		VkBool32 app_enabled = VK_FALSE;
		if (!find_app_timeline_semaphore_feature(&modified_info, &app_enabled)) {
			// Insert struct at the front of the chain
			// Have to cast away const.
			timeline_semaphore.pNext = (void *)modified_info.pNext;
			modified_info.pNext = &timeline_semaphore;
		} else if (!app_enabled) {
			oxr_warn(log, "Application set timelineSemaphore to VK_FALSE, falling back to fences");
			timeline_semaphore_info.timelineSemaphore = VK_FALSE;
		}
		// Otherwise the application has already enabled them.
	}
#endif

	*vulkanResult = loaded_CreateDevice(physical_device, &modified_info, createInfo->vulkanAllocator, vulkanDevice);


	// Logging
	{
		struct oxr_sink_logger slog = {0};

		oxr_slog(&slog, "Creation of VkDevice:");
		oxr_slog(&slog, "\n\tresult: %s", vk_result_string(*vulkanResult));
		oxr_slog(&slog, "\n\tvulkanDevice: 0x%" PRIx64, (uint64_t)(intptr_t)*vulkanDevice);
		oxr_slog(&slog, "\n\tvulkanInstance: 0x%" PRIx64, (uint64_t)(intptr_t)sys->vulkan_enable2_instance);
#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)
		oxr_slog(&slog, "\n\texternal_fence_fd: %s", external_fence_fd_enabled ? "true" : "false");
		oxr_slog(&slog, "\n\texternal_semaphore_fd: %s", external_semaphore_fd_enabled ? "true" : "false");
#endif
#ifdef VK_KHR_timeline_semaphore
		oxr_slog(&slog, "\n\ttimelineSemaphore: %s",
		         timeline_semaphore_info.timelineSemaphore ? "true" : "false");
#endif
		oxr_slog(&slog, "\n\textensions:");
		for (uint32_t i = 0; i < modified_info.enabledExtensionCount; i++) {
			oxr_slog(&slog, "\n\t\t%s", modified_info.ppEnabledExtensionNames[i]);
		}

		oxr_log_slog(log, &slog);
	}

#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)
	if (*vulkanResult == VK_SUCCESS) {
		sys->vk.external_fence_fd_enabled = external_fence_fd_enabled;
		sys->vk.external_semaphore_fd_enabled = external_semaphore_fd_enabled;
	}
#endif

#ifdef VK_KHR_timeline_semaphore
	// Have timeline semaphores added and as such enabled.
	if (*vulkanResult == VK_SUCCESS) {
		sys->vk.timeline_semaphore_enabled = timeline_semaphore_info.timelineSemaphore;
		U_LOG_D("timeline semaphores enabled: %d", timeline_semaphore_info.timelineSemaphore);
	}
#endif

#ifdef VK_KHR_image_format_list
	if (*vulkanResult == VK_SUCCESS) {
		sys->vk.image_format_list_enabled = image_format_list_enabled;
	}
#endif

	u_extension_list_destroy(&device_extension_list);

	return XR_SUCCESS;
}


XrResult
oxr_vk_get_physical_device(struct oxr_logger *log,
                           struct oxr_instance *inst,
                           struct oxr_system *sys,
                           VkInstance vulkanInstance,
                           PFN_vkGetInstanceProcAddr vulkanGetInstanceProcAddr,
                           VkPhysicalDevice *vkPhysicalDevice)
{
	GET_PROC(vulkanInstance, EnumeratePhysicalDevices);
	GET_PROC(vulkanInstance, GetPhysicalDeviceProperties2KHR);

	VkResult vk_ret;
	uint32_t count;

	if (sys->xsysc == NULL) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, " sys->xsysc == NULL");
	}

	vk_ret = loaded_EnumeratePhysicalDevices(vulkanInstance, &count, NULL);
	if (vk_ret != VK_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Call to vkEnumeratePhysicalDevices returned %u",
		                 vk_ret);
	}
	if (count == 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Call to vkEnumeratePhysicalDevices returned zero VkPhysicalDevices");
	}

	VkPhysicalDevice *phys = U_TYPED_ARRAY_CALLOC(VkPhysicalDevice, count);
	vk_ret = loaded_EnumeratePhysicalDevices(vulkanInstance, &count, phys);
	if (vk_ret != VK_SUCCESS) {
		free(phys);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Call to vkEnumeratePhysicalDevices returned %u",
		                 vk_ret);
	}
	if (count == 0) {
		free(phys);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Call to vkEnumeratePhysicalDevices returned zero VkPhysicalDevices");
	}

	char suggested_uuid_str[UUID_STR_SIZE] = {0};
	snprint_uuid(suggested_uuid_str, ARRAY_SIZE(suggested_uuid_str), &sys->xsysc->info.client_vk_deviceUUID);

	enum u_logging_level log_level = debug_get_log_option_compositor_log();
	int gpu_index = -1;
	for (uint32_t i = 0; i < count; i++) {
		VkPhysicalDeviceIDProperties pdidp = {
		    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
		};

		VkPhysicalDeviceProperties2 pdp2 = {
		    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		    .pNext = &pdidp,
		};

		loaded_GetPhysicalDeviceProperties2KHR(phys[i], &pdp2);

		// These should always be true
		static_assert(VK_UUID_SIZE == XRT_UUID_SIZE, "uuid sizes mismatch");
		static_assert(ARRAY_SIZE(pdidp.deviceUUID) == XRT_UUID_SIZE, "array size mismatch");

		char buffer[UUID_STR_SIZE] = {0};
		if (log_level <= U_LOGGING_DEBUG) {
			snprint_uuid(buffer, ARRAY_SIZE(buffer), (xrt_uuid_t *)pdidp.deviceUUID);
			oxr_log(log, "GPU: #%d, uuid: %s", i, buffer);
			if (pdidp.deviceLUIDValid == VK_TRUE) {
				snprint_luid(buffer, ARRAY_SIZE(buffer), (xrt_luid_t *)pdidp.deviceLUID);
				oxr_log(log, "  LUID: %s", buffer);
			}
		}

		if (memcmp(pdidp.deviceUUID, sys->xsysc->info.client_vk_deviceUUID.data, XRT_UUID_SIZE) == 0) {
			gpu_index = i;
			if (log_level <= U_LOGGING_DEBUG) {
				oxr_log(log, "Using GPU #%d with uuid %s suggested by runtime", gpu_index, buffer);
			}
			break;
		}
	}

	if (gpu_index == -1) {
		oxr_warn(log, "Did not find runtime suggested GPU, fall back to GPU 0\n\tuuid: %s", suggested_uuid_str);
		gpu_index = 0;
	}

	*vkPhysicalDevice = phys[gpu_index];

	// vulkan_enable2 needs the physical device in xrCreateVulkanDeviceKHR
	if (inst->extensions.KHR_vulkan_enable2) {
		sys->vulkan_enable2_instance = vulkanInstance;
	}
	sys->suggested_vulkan_physical_device = *vkPhysicalDevice;
	if (log_level <= U_LOGGING_DEBUG) {
		oxr_log(log, "Suggesting vulkan physical device %p", (void *)*vkPhysicalDevice);
	}

	free(phys);

	return XR_SUCCESS;
}
