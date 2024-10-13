- Fixes #381, fixes crashes or vulkan validation errors on android when using the graphics based compositor pipeline on android
  when apps submit multiple layers or fast-path is explicitly disabled.
- Reverts the default compositor pipeline on android from compute back to graphics based.
