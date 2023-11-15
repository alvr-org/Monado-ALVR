---
- mr.2005
- mr.2044
---

oxr: Enable VK_EXT_debug_utils extension for client side on the platform that
support it. Since it can not be reliably detected if the extension was enabled
by the application on `XR_KHR_vulkan_enable` instead use the the environmental
variable `OXR_DEBUG_FORCE_VK_DEBUG_UTILS` to force it on.
