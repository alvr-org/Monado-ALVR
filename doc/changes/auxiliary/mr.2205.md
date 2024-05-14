a/vk: Extend command buffer wait timeout to ~10 seconds

This is necessary because in some platforms (such as Windows 10, NVidia
RTX 3080ti) the OpenXR CTS will trigger an issue when the GPU memory
fills, where the system hangs for over one second during a paging queue
operation.
