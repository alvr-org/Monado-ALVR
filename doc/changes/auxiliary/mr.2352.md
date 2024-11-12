Fixes issue ticket #410, fixes the client layer not setting mutable usage flag on client vulkan image creation when requested
by vulkan based apps, causing vulkan validation errors on apps which create image views of swapchain images with different formats.
