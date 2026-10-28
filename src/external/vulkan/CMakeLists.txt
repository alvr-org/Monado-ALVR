# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

# Simple wrapper around Vulkan headers and loaders. Still uses the system
# installed Vulkan SDK, integrated more like the other external dependancies
# into the Monado build system.

# Vulkan headers target - uses system Vulkan headers
add_library(xrt-external-vulkan-headers INTERFACE)
target_include_directories(xrt-external-vulkan-headers SYSTEM INTERFACE ${Vulkan_INCLUDE_DIRS})

# Vulkan loader target - links to the Vulkan loader library
add_library(xrt-external-vulkan-loader INTERFACE)
target_link_libraries(
	xrt-external-vulkan-loader INTERFACE Vulkan::Vulkan xrt-external-vulkan-headers
	)
