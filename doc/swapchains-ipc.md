# Swapchain Allocation and IPC {#swapchains-ipc}

<!--
Copyright 2024, Collabora, Ltd. and the Monado contributors
SPDX-License-Identifier: BSL-1.0
-->

The control flow in Monado for allocating the images/buffers used for a given
`XrSwapchain` can vary widely based on a number of factors.

## Simple in-process

In the minimal case, which is not common for widespread deployment but which is
useful for debugging, there is only a single process, with the entire runtime
loaded into the "client" application. Despite the absence of IPC in this
scenario, the same basic flow and interaction applies: images are conveyed using
system ("native") handles.

@mermaid{swapchain_allocation_inproc}

The control flow makes its way to the main compositor, which, in default
implementations, uses Vulkan to allocate exportable images.

## Simple out-of-process

The usual model on desktop is that the images for a given `XrSwapchain` are
allocated in the service, as shown below.

@mermaid{swapchain_allocation_ipc}

However, there are two additional paths possible.

## Client-side Allocation with XINA (Out-of-process)

In some builds, a "XINA" (like the warrior princess) is used: @ref
xrt_image_native_allocator. This allocates the "native" (system handle) images
within the client process, in a central location. Currently only Android builds
using AHardwareBuffer use this model, but ideally we would probably move toward
this model for all systems. The main advantage is that the swapchain images are
allocated in the application process and thus counted against application
quotas, rather than the service/runtime quotas, avoiding security, privacy, and
denial-of-service risks.

@mermaid{swapchain_allocation_ipc_xina}

Generally a XINA used in this way is intended for client-process allocation.
However, there is an implementation (disabled by default) of a "loopback XINA"
in the IPC client, which implements the XINA API using the IPC interface. So
using a XINA in that case would not result in client-process image allocation.

## Allocation for D3D client apps (Out-of-process)

Direct3D cannot reliably and portably import images allocated in Vulkan. So, if
an application uses Direct3D, the D3D client compositor does the allocation, on
the client side, as shown below. The example shown is out-of-process.

@mermaid{swapchain_allocation_ipc_d3d}

Note that this pattern is used even when running in-process with D3D, the result
is a hybrid of this diagram and the first (in-process) diagram, dropping the
client process native compositor and server IPC handler from the diagram.
