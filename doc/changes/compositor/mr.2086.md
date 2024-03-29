---
- mr.2086
- mr.2175
- mr.2189
---

- xrt_layer_type: Renamed the `XRT_LAYER_STEREO_PROJECTION` to `XRT_LAYER_PROJECTION` and `XRT_LAYER_STEREO_PROJECTION_DEPTH` to `XRT_LAYER_PROJECTION_DEPTH` in the `xrt_layer_type` enumeration to support both mono and stereo projection layers. This change provides a more inclusive and versatile categorization of projection layers within the XRT framework, accommodating a wider range of use cases.

- multi_layer_entry: Updated the array length of xscs within multi_layer_entry from 4 to `2 * XRT_MAX_VIEWS` to accommodate a variable number of views.

- swapchain: Change `struct xrt_swapchain *l_xsc, struct xrt_swapchain *r_xsc` to `struct xrt_swapchain *xsc[XRT_MAX_VIEWS]`, in order to support multiple views' swapchains. When iterating, use `xrt_layer_data.view_count`.

- render: `render_resources` now has a `view_count` field, which is set to 1 for mono and 2 for stereo. This is used to iterate over the views in the render function.

- render: Use the `XRT_MAX_VIEWS` macro to calculate the length of a series of arrays.
