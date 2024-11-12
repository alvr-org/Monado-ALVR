Fixes issue ticket #411, fixes the following client-layer issues when `KHR_vulkan_swapchain_format_list` is enabled & used:
- Fixes `(oxr_)xrGetVulkanDeviceExtensionsKHR` not outputting `VK_KHR_image_format_list` to the list of extensions
  to enable for apps using `XR_KHR_vulkan_enable`.
- Adds piping of the enabled state of `VK_KHR_image_format_list` to `vk_bundle` and fixes `vk_bundle::has_KHR_image_format_list` not being set to enabled in the client layer.
- Fixes client-layer not using & setting format-lists for VK_KHR_image_format_list on client vulkan image creation when provided
by vulkan based apps using mutable formats.
