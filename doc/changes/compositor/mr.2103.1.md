render: Tweak alpha blending, before on the gfx path the written alpha was
always zero, this would pose a problem when we want to display the scratch
images in the debug UI as they would be completely black.
