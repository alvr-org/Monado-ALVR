---
- mr.1955
- mr.1967
- mr.1975
---

util: Add helpers to launch the compute layer squasher shaders and the compute
distortion shaders. They are in `comp_util` because it looks at a list of
`comp_layer` and `comp_swapchain` structs that `comp_base` manages.
