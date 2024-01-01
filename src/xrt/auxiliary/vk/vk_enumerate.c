// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vulkan enumeration helpers code.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vk
 */

#include "xrt/xrt_handles.h"

#include "util/u_misc.h"
#include "util/u_debug.h"

#include "vk/vk_helpers.h"


/*
 *
 * Helpers.
 *
 */

#define CHECK_FIRST_CALL(FUNC, RET, COUNT)                                                                             \
	do {                                                                                                           \
		VkResult _ret = RET;                                                                                   \
		if (_ret != VK_SUCCESS) {                                                                              \
			vk_print_result(vk, __FILE__, __LINE__, __func__, _ret, FUNC);                                 \
			return RET;                                                                                    \
		}                                                                                                      \
		if (COUNT == 0) {                                                                                      \
			goto out;                                                                                      \
		}                                                                                                      \
	} while (false)

#define CHECK_SECOND_CALL(FUNC, RET, TO_FREE)                                                                          \
	do {                                                                                                           \
		VkResult _ret = RET;                                                                                   \
		if (_ret != VK_SUCCESS) {                                                                              \
			vk_print_result(vk, __FILE__, __LINE__, __func__, _ret, FUNC);                                 \
			free(TO_FREE);                                                                                 \
			return RET;                                                                                    \
		}                                                                                                      \
	} while (false)


/*
 *
 * 'Exported' functions.
 *
 */

VkResult
vk_enumerate_instance_extensions_properties(struct vk_bundle *vk,
                                            const char *layer_name,
                                            uint32_t *out_prop_count,
                                            VkExtensionProperties **out_props)
{
	VkExtensionProperties *props = NULL;
	uint32_t prop_count = 0;
	VkResult ret;

	ret = vk->vkEnumerateInstanceExtensionProperties(layer_name, &prop_count, NULL);
	CHECK_FIRST_CALL("vkEnumerateInstanceExtensionProperties", ret, prop_count);

	props = U_TYPED_ARRAY_CALLOC(VkExtensionProperties, prop_count);
	ret = vk->vkEnumerateInstanceExtensionProperties(layer_name, &prop_count, props);
	CHECK_SECOND_CALL("vkEnumerateInstanceExtensionProperties", ret, props);

out:
	*out_prop_count = prop_count;
	*out_props = props;

	return VK_SUCCESS;
}

VkResult
vk_enumerate_physical_devices(struct vk_bundle *vk,
                              uint32_t *out_physical_device_count,
                              VkPhysicalDevice **out_physical_devices)
{
	VkPhysicalDevice *physical_devices = NULL;
	uint32_t physical_device_count = 0;
	VkResult ret;

	ret = vk->vkEnumeratePhysicalDevices(vk->instance, &physical_device_count, NULL);
	CHECK_FIRST_CALL("vkEnumeratePhysicalDevices", ret, physical_device_count);

	physical_devices = U_TYPED_ARRAY_CALLOC(VkPhysicalDevice, physical_device_count);
	ret = vk->vkEnumeratePhysicalDevices(vk->instance, &physical_device_count, physical_devices);
	CHECK_SECOND_CALL("vkEnumeratePhysicalDevices", ret, physical_devices);

out:
	*out_physical_device_count = physical_device_count;
	*out_physical_devices = physical_devices;

	return VK_SUCCESS;
}

VkResult
vk_enumerate_physical_device_extension_properties(struct vk_bundle *vk,
                                                  VkPhysicalDevice physical_device,
                                                  const char *layer_name,
                                                  uint32_t *out_prop_count,
                                                  VkExtensionProperties **out_props)
{
	VkExtensionProperties *props = NULL;
	uint32_t prop_count = 0;
	VkResult ret;

	ret = vk->vkEnumerateDeviceExtensionProperties(physical_device, layer_name, &prop_count, NULL);
	CHECK_FIRST_CALL("vkEnumerateDeviceExtensionProperties", ret, prop_count);

	props = U_TYPED_ARRAY_CALLOC(VkExtensionProperties, prop_count);
	ret = vk->vkEnumerateDeviceExtensionProperties(physical_device, layer_name, &prop_count, props);
	CHECK_SECOND_CALL("vkEnumerateDeviceExtensionProperties", ret, props);

out:
	*out_prop_count = prop_count;
	*out_props = props;

	return VK_SUCCESS;
}

#ifdef VK_KHR_surface
VkResult
vk_enumerate_surface_formats(struct vk_bundle *vk,
                             VkSurfaceKHR surface,
                             uint32_t *out_format_count,
                             VkSurfaceFormatKHR **out_formats)
{
	VkSurfaceFormatKHR *formats = NULL;
	uint32_t format_count = 0;
	VkResult ret;

	ret = vk->vkGetPhysicalDeviceSurfaceFormatsKHR(vk->physical_device, surface, &format_count, NULL);
	CHECK_FIRST_CALL("vkGetPhysicalDeviceSurfaceFormatsKHR", ret, format_count);

	formats = U_TYPED_ARRAY_CALLOC(VkSurfaceFormatKHR, format_count);
	ret = vk->vkGetPhysicalDeviceSurfaceFormatsKHR(vk->physical_device, surface, &format_count, formats);
	CHECK_SECOND_CALL("vkGetPhysicalDeviceSurfaceFormatsKHR", ret, formats);

out:
	*out_format_count = format_count;
	*out_formats = formats;

	return VK_SUCCESS;
}

VkResult
vk_enumerate_surface_present_modes(struct vk_bundle *vk,
                                   VkSurfaceKHR surface,
                                   uint32_t *out_present_mode_count,
                                   VkPresentModeKHR **out_present_modes)
{
	VkPresentModeKHR *present_modes = NULL;
	uint32_t present_mode_count = 0;
	VkResult ret;

	ret = vk->vkGetPhysicalDeviceSurfacePresentModesKHR(vk->physical_device, surface, &present_mode_count, NULL);
	CHECK_FIRST_CALL("vkGetPhysicalDeviceSurfacePresentModesKHR", ret, present_mode_count);

	present_modes = U_TYPED_ARRAY_CALLOC(VkPresentModeKHR, present_mode_count);
	ret = vk->vkGetPhysicalDeviceSurfacePresentModesKHR(vk->physical_device, surface, &present_mode_count,
	                                                    present_modes);
	CHECK_SECOND_CALL("vkGetPhysicalDeviceSurfacePresentModesKHR", ret, present_modes);

out:
	*out_present_mode_count = present_mode_count;
	*out_present_modes = present_modes;

	return VK_SUCCESS;
}
#endif

#ifdef VK_KHR_swapchain
VkResult
vk_enumerate_swapchain_images(struct vk_bundle *vk,
                              VkSwapchainKHR swapchain,
                              uint32_t *out_image_count,
                              VkImage **out_images)
{
	VkImage *images = NULL;
	uint32_t image_count = 0;
	VkResult ret;

	ret = vk->vkGetSwapchainImagesKHR(vk->device, swapchain, &image_count, NULL);
	CHECK_FIRST_CALL("vkGetSwapchainImagesKHR", ret, image_count);

	images = U_TYPED_ARRAY_CALLOC(VkImage, image_count);
	ret = vk->vkGetSwapchainImagesKHR(vk->device, swapchain, &image_count, images);
	CHECK_SECOND_CALL("vkGetSwapchainImagesKHR", ret, images);

out:
	*out_image_count = image_count;
	*out_images = images;

	return VK_SUCCESS;
}
#endif

#ifdef VK_USE_PLATFORM_DISPLAY_KHR
VkResult
vk_enumerate_physical_device_display_properties(struct vk_bundle *vk,
                                                VkPhysicalDevice physical_device,
                                                uint32_t *out_prop_count,
                                                VkDisplayPropertiesKHR **out_props)
{
	VkDisplayPropertiesKHR *props = NULL;
	uint32_t prop_count = 0;
	VkResult ret;

	ret = vk->vkGetPhysicalDeviceDisplayPropertiesKHR(physical_device, &prop_count, NULL);
	CHECK_FIRST_CALL("vkGetPhysicalDeviceDisplayPropertiesKHR", ret, prop_count);

	props = U_TYPED_ARRAY_CALLOC(VkDisplayPropertiesKHR, prop_count);
	ret = vk->vkGetPhysicalDeviceDisplayPropertiesKHR(physical_device, &prop_count, props);
	CHECK_SECOND_CALL("vkGetPhysicalDeviceDisplayPropertiesKHR", ret, props);

out:
	*out_props = props;
	*out_prop_count = prop_count;

	return VK_SUCCESS;
}

VkResult
vk_enumerate_physical_display_plane_properties(struct vk_bundle *vk,
                                               VkPhysicalDevice physical_device,
                                               uint32_t *out_prop_count,
                                               VkDisplayPlanePropertiesKHR **out_props)
{
	VkDisplayPlanePropertiesKHR *props = NULL;
	uint32_t prop_count = 0;
	VkResult ret;

	ret = vk->vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physical_device, &prop_count, NULL);
	CHECK_FIRST_CALL("vkGetPhysicalDeviceDisplayPlanePropertiesKHR", ret, prop_count);

	props = U_TYPED_ARRAY_CALLOC(VkDisplayPlanePropertiesKHR, prop_count);
	ret = vk->vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physical_device, &prop_count, props);
	CHECK_SECOND_CALL("vkGetPhysicalDeviceDisplayPlanePropertiesKHR", ret, props);

out:
	*out_props = props;
	*out_prop_count = prop_count;

	return VK_SUCCESS;
}

VkResult
vk_enumerate_display_mode_properties(struct vk_bundle *vk,
                                     VkPhysicalDevice physical_device,
                                     VkDisplayKHR display,
                                     uint32_t *out_prop_count,
                                     VkDisplayModePropertiesKHR **out_props)
{
	VkDisplayModePropertiesKHR *props = NULL;
	uint32_t prop_count = 0;
	VkResult ret;

	ret = vk->vkGetDisplayModePropertiesKHR(physical_device, display, &prop_count, NULL);
	CHECK_FIRST_CALL("vkGetDisplayModePropertiesKHR", ret, prop_count);

	props = U_TYPED_ARRAY_CALLOC(VkDisplayModePropertiesKHR, prop_count);
	ret = vk->vkGetDisplayModePropertiesKHR(physical_device, display, &prop_count, props);
	CHECK_SECOND_CALL("vkGetDisplayModePropertiesKHR", ret, props);

out:
	*out_props = props;
	*out_prop_count = prop_count;

	return VK_SUCCESS;
}
#endif
