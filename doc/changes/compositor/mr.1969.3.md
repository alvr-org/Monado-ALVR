---
- mr.1969
- mr.1970
---

main: Refactor the layer rendering code to use `render_gfx_render_pass`,
`render_gfx_target_resources` and an `VkCommandBuffer` that is passed in as an
argument to the draw call. This allows the layer renderer to share the scratch
images with the compute pipeline.
