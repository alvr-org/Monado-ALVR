---
- mr.2377
---
- Add: New `comp_layer_accum` helper, factored out from `comp_base`, that collects layers and swapchains for a frame.
- Change: Modify `comp_base` to use `comp_layer_accum` helper, instead of inlining that code. Users of `comp_base` will need to update their code accordingly.
