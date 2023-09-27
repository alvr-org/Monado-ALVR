render: Refactor gfx path code to split out render pass vulkan objects from
the render target resources struct into the `render_gfx_render_pass` struct.
This allows the render pass to resused for more then one target.
