render: Remove unused render_gfx_view and other fields on render_gfx,
the limiting factor to how many views the graphics path can do now is the sizes
of descriptor pools and UBO buffer.
