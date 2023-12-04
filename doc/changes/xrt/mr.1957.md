---
- mr.1957
- mr.2066
---

Add `xrt_limited_unique_id` and `xrt_limited_unique_id_t` types to donate a
special id that is unique to the current process. Use that to decorate
`xrt_swapchain_native` with a limited unique id, useful for caching of the
`xrt_image_native` imports of swapchains and other objects.
