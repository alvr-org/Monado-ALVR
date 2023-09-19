render: Refactor layer squasher code, the shader is now run once per view
instead of doing two views in one submission. Makes it easier to split up
targets and requires less samplers in one invocation.
