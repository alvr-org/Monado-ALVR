#!/usr/bin/env python3
# Copyright 2019-2023, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""Simple script to generate vk_*.h.inc and vk_*.c.inc files."""

import argparse
from typing import Callable, Iterable, List, Optional


def get_device_cmds():
    # NOTE: Be sure to use the Vulkan 1.0 name of functions in here!
    # (so, the decorated extension version)
    # If you want to alias it in the generated file, use the member_name
    # keyword argument.
    return [
        Cmd("vkDestroyDevice"),
        Cmd("vkDeviceWaitIdle"),
        Cmd("vkAllocateMemory"),
        Cmd("vkFreeMemory"),
        Cmd("vkMapMemory"),
        Cmd("vkUnmapMemory"),
        None,
        Cmd("vkCreateBuffer"),
        Cmd("vkDestroyBuffer"),
        Cmd("vkBindBufferMemory"),
        None,
        Cmd("vkCreateImage"),
        Cmd("vkDestroyImage"),
        Cmd("vkBindImageMemory"),
        None,
        Cmd("vkGetBufferMemoryRequirements"),
        Cmd("vkFlushMappedMemoryRanges"),
        Cmd("vkGetImageMemoryRequirements"),
        Cmd(
            "vkGetImageMemoryRequirements2KHR",
            member_name="vkGetImageMemoryRequirements2",
        ),
        Cmd("vkGetImageSubresourceLayout"),
        None,
        Cmd("vkCreateImageView"),
        Cmd("vkDestroyImageView"),
        None,
        Cmd("vkCreateSampler"),
        Cmd("vkDestroySampler"),
        None,
        Cmd("vkCreateShaderModule"),
        Cmd("vkDestroyShaderModule"),
        None,
        Cmd("vkCreateQueryPool"),
        Cmd("vkDestroyQueryPool"),
        Cmd("vkGetQueryPoolResults"),
        None,
        Cmd("vkCreateCommandPool"),
        Cmd("vkDestroyCommandPool"),
        Cmd("vkResetCommandPool"),
        None,
        Cmd("vkAllocateCommandBuffers"),
        Cmd("vkBeginCommandBuffer"),
        Cmd("vkCmdBeginQuery"),
        Cmd("vkCmdCopyQueryPoolResults"),
        Cmd("vkCmdEndQuery"),
        Cmd("vkCmdResetQueryPool"),
        Cmd("vkCmdWriteTimestamp"),
        Cmd("vkCmdPipelineBarrier"),
        Cmd("vkCmdBeginRenderPass"),
        Cmd("vkCmdSetScissor"),
        Cmd("vkCmdSetViewport"),
        Cmd("vkCmdClearColorImage"),
        Cmd("vkCmdEndRenderPass"),
        Cmd("vkCmdBindDescriptorSets"),
        Cmd("vkCmdBindPipeline"),
        Cmd("vkCmdBindVertexBuffers"),
        Cmd("vkCmdBindIndexBuffer"),
        Cmd("vkCmdDraw"),
        Cmd("vkCmdDrawIndexed"),
        Cmd("vkCmdDispatch"),
        Cmd("vkCmdCopyBuffer"),
        Cmd("vkCmdCopyBufferToImage"),
        Cmd("vkCmdCopyImage"),
        Cmd("vkCmdCopyImageToBuffer"),
        Cmd("vkCmdBlitImage"),
        Cmd("vkCmdResolveImage"),
        Cmd("vkCmdPushConstants"),
        Cmd("vkCmdClearAttachments"),
        Cmd("vkEndCommandBuffer"),
        Cmd("vkFreeCommandBuffers"),
        None,
        Cmd("vkCreateRenderPass"),
        Cmd("vkDestroyRenderPass"),
        None,
        Cmd("vkCreateFramebuffer"),
        Cmd("vkDestroyFramebuffer"),
        None,
        Cmd("vkCreatePipelineCache"),
        Cmd("vkDestroyPipelineCache"),
        None,
        Cmd("vkResetDescriptorPool"),
        Cmd("vkCreateDescriptorPool"),
        Cmd("vkDestroyDescriptorPool"),
        None,
        Cmd("vkAllocateDescriptorSets"),
        Cmd("vkFreeDescriptorSets"),
        None,
        Cmd("vkCreateComputePipelines"),
        Cmd("vkCreateGraphicsPipelines"),
        Cmd("vkDestroyPipeline"),
        None,
        Cmd("vkCreatePipelineLayout"),
        Cmd("vkDestroyPipelineLayout"),
        None,
        Cmd("vkCreateDescriptorSetLayout"),
        Cmd("vkUpdateDescriptorSets"),
        Cmd("vkDestroyDescriptorSetLayout"),
        None,
        Cmd("vkGetDeviceQueue"),
        Cmd("vkQueueSubmit"),
        Cmd("vkQueueWaitIdle"),
        None,
        Cmd("vkCreateSemaphore"),
        Cmd(
            "vkSignalSemaphoreKHR",
            member_name="vkSignalSemaphore",
            requires=("VK_KHR_timeline_semaphore",),
        ),
        Cmd(
            "vkWaitSemaphoresKHR",
            member_name="vkWaitSemaphores",
            requires=("VK_KHR_timeline_semaphore",),
        ),
        Cmd(
            "vkGetSemaphoreCounterValueKHR",
            member_name="vkGetSemaphoreCounterValue",
            requires=("VK_KHR_timeline_semaphore",),
        ),
        Cmd("vkDestroySemaphore"),
        None,
        Cmd("vkCreateFence"),
        Cmd("vkWaitForFences"),
        Cmd("vkGetFenceStatus"),
        Cmd("vkDestroyFence"),
        Cmd("vkResetFences"),
        None,
        Cmd("vkCreateSwapchainKHR"),
        Cmd("vkDestroySwapchainKHR"),
        Cmd("vkGetSwapchainImagesKHR"),
        Cmd("vkAcquireNextImageKHR"),
        Cmd("vkQueuePresentKHR"),
        None,
        Cmd("vkGetMemoryWin32HandleKHR", requires=("VK_USE_PLATFORM_WIN32_KHR",)),
        Cmd("vkGetFenceWin32HandleKHR", requires=("VK_USE_PLATFORM_WIN32_KHR",)),
        Cmd("vkGetSemaphoreWin32HandleKHR", requires=("VK_USE_PLATFORM_WIN32_KHR",)),
        Cmd("vkImportFenceWin32HandleKHR", requires=("VK_USE_PLATFORM_WIN32_KHR",)),
        Cmd("vkImportSemaphoreWin32HandleKHR", requires=("VK_USE_PLATFORM_WIN32_KHR",)),
        None,
        Cmd("vkGetMemoryFdKHR", requires=("!defined(VK_USE_PLATFORM_WIN32_KHR)",)),
        Cmd("vkGetFenceFdKHR", requires=("!defined(VK_USE_PLATFORM_WIN32_KHR)",)),
        Cmd("vkGetSemaphoreFdKHR", requires=("!defined(VK_USE_PLATFORM_WIN32_KHR)",)),
        Cmd("vkImportFenceFdKHR", requires=("!defined(VK_USE_PLATFORM_WIN32_KHR)",)),
        Cmd("vkImportSemaphoreFdKHR", requires=("!defined(VK_USE_PLATFORM_WIN32_KHR)",)),
        None,
        Cmd("vkExportMetalObjectsEXT", requires=("VK_USE_PLATFORM_METAL_EXT",)),
        None,
        Cmd(
            "vkGetMemoryAndroidHardwareBufferANDROID",
            requires=("VK_USE_PLATFORM_ANDROID_KHR",),
        ),
        Cmd(
            "vkGetAndroidHardwareBufferPropertiesANDROID",
            requires=("VK_USE_PLATFORM_ANDROID_KHR",),
        ),
        None,
        Cmd('vkGetMemoryHostPointerPropertiesEXT', requires=("VK_EXT_external_memory_host",)),
        None,
        Cmd("vkGetCalibratedTimestampsEXT", requires=("VK_EXT_calibrated_timestamps",)),
        None,
        Cmd("vkGetCalibratedTimestampsKHR", requires=("VK_KHR_calibrated_timestamps",)),
        None,
        Cmd("vkGetPastPresentationTimingGOOGLE"),
        None,
        Cmd("vkGetSwapchainCounterEXT", requires=("VK_EXT_display_control",)),
        Cmd("vkRegisterDeviceEventEXT", requires=("VK_EXT_display_control",)),
        Cmd("vkRegisterDisplayEventEXT", requires=("VK_EXT_display_control",)),
        None,
        Cmd("vkGetImageDrmFormatModifierPropertiesEXT", requires=("VK_EXT_image_drm_format_modifier",)),
        None,
        Cmd("vkCmdBeginDebugUtilsLabelEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkCmdEndDebugUtilsLabelEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkCmdInsertDebugUtilsLabelEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkQueueBeginDebugUtilsLabelEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkQueueEndDebugUtilsLabelEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkQueueInsertDebugUtilsLabelEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkSetDebugUtilsObjectNameEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkSetDebugUtilsObjectTagEXT", requires=("VK_EXT_debug_utils",)),
        None,
        Cmd("vkWaitForPresentKHR", requires=("VK_KHR_present_wait",)),
        None,
        Cmd("vkWaitForPresent2KHR", requires=("VK_KHR_present_wait2",)),
        None,
        Cmd("vkGetSwapchainStatusKHR", requires=("VK_KHR_shared_presentable_image",)),
    ]


def get_instance_cmds():
    return [
        Cmd("vkDestroyInstance"),
        Cmd("vkGetDeviceProcAddr"),
        Cmd("vkCreateDevice"),
        Cmd("vkDestroySurfaceKHR"),
        None,
        Cmd("vkCreateDebugReportCallbackEXT"),
        Cmd("vkDestroyDebugReportCallbackEXT"),
        None,
        Cmd("vkEnumeratePhysicalDevices"),
        Cmd("vkGetPhysicalDeviceProperties"),
        Cmd(
            "vkGetPhysicalDeviceProperties2KHR",
            member_name="vkGetPhysicalDeviceProperties2",
        ),
        Cmd(
            "vkGetPhysicalDeviceFeatures2KHR",
            member_name="vkGetPhysicalDeviceFeatures2",
        ),
        Cmd("vkGetPhysicalDeviceMemoryProperties"),
        Cmd("vkGetPhysicalDeviceQueueFamilyProperties"),
        Cmd("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"),
        Cmd("vkGetPhysicalDeviceSurfaceFormatsKHR"),
        Cmd("vkGetPhysicalDeviceSurfacePresentModesKHR"),
        Cmd("vkGetPhysicalDeviceSurfaceSupportKHR"),
        Cmd("vkGetPhysicalDeviceFormatProperties"),
        Cmd(
            "vkGetPhysicalDeviceFormatProperties2KHR",
            member_name="vkGetPhysicalDeviceFormatProperties2",
        ),
        Cmd(
            "vkGetPhysicalDeviceImageFormatProperties2KHR",
            member_name="vkGetPhysicalDeviceImageFormatProperties2",
        ),
        Cmd("vkGetPhysicalDeviceExternalBufferPropertiesKHR"),
        Cmd("vkGetPhysicalDeviceExternalFencePropertiesKHR"),
        Cmd("vkGetPhysicalDeviceExternalSemaphorePropertiesKHR"),
        Cmd("vkEnumerateDeviceExtensionProperties"),
        Cmd("vkEnumerateDeviceLayerProperties"),
        None,
        Cmd(
            "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT",
            requires=("VK_EXT_calibrated_timestamps",),
        ),
        None,
        Cmd(
            "vkCreateDisplayPlaneSurfaceKHR", requires=("VK_USE_PLATFORM_DISPLAY_KHR",)
        ),
        Cmd(
            "vkGetDisplayPlaneCapabilitiesKHR",
            requires=("VK_USE_PLATFORM_DISPLAY_KHR",),
        ),
        Cmd(
            "vkGetPhysicalDeviceDisplayPropertiesKHR",
            requires=("VK_USE_PLATFORM_DISPLAY_KHR",),
        ),
        Cmd(
            "vkGetPhysicalDeviceDisplayPlanePropertiesKHR",
            requires=("VK_USE_PLATFORM_DISPLAY_KHR",),
        ),
        Cmd("vkGetDisplayModePropertiesKHR", requires=("VK_USE_PLATFORM_DISPLAY_KHR",)),
        Cmd("vkReleaseDisplayEXT", requires=("VK_USE_PLATFORM_DISPLAY_KHR",)),
        None,
        Cmd("vkCreateXcbSurfaceKHR", requires=("VK_USE_PLATFORM_XCB_KHR",)),
        None,
        Cmd("vkCreateWaylandSurfaceKHR", requires=("VK_USE_PLATFORM_WAYLAND_KHR",)),
        None,
        Cmd(
            "vkAcquireDrmDisplayEXT",
            requires=("VK_USE_PLATFORM_WAYLAND_KHR", "VK_EXT_acquire_drm_display"),
        ),
        Cmd(
            "vkGetDrmDisplayEXT",
            requires=("VK_USE_PLATFORM_WAYLAND_KHR", "VK_EXT_acquire_drm_display"),
        ),
        None,
        Cmd(
            "vkGetRandROutputDisplayEXT", requires=("VK_USE_PLATFORM_XLIB_XRANDR_EXT",)
        ),
        Cmd("vkAcquireXlibDisplayEXT", requires=("VK_USE_PLATFORM_XLIB_XRANDR_EXT",)),
        None,
        Cmd("vkCreateAndroidSurfaceKHR", requires=("VK_USE_PLATFORM_ANDROID_KHR",)),
        None,
        Cmd("vkCreateWin32SurfaceKHR", requires=("VK_USE_PLATFORM_WIN32_KHR",)),
        None,
        Cmd("vkCreateMetalSurfaceEXT", requires=("VK_USE_PLATFORM_METAL_EXT",)),
        None,
        Cmd("vkGetPhysicalDeviceSurfaceCapabilities2EXT", requires=("VK_EXT_display_surface_counter",)),
        None,
        Cmd("vkCreateDebugUtilsMessengerEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkSubmitDebugUtilsMessageEXT", requires=("VK_EXT_debug_utils",)),
        Cmd("vkDestroyDebugUtilsMessengerEXT", requires=("VK_EXT_debug_utils",)),
        None,
        Cmd("vkGetPhysicalDeviceSurfaceCapabilities2KHR", requires=("VK_KHR_get_surface_capabilities2",)),
        Cmd("vkGetPhysicalDeviceSurfaceFormats2KHR", requires=("VK_KHR_get_surface_capabilities2",)),
    ]


# Sorted KHR, EXT, Vendor, internally alphabetically
INSTANCE_EXTENSIONS_TO_CHECK = [
    "VK_KHR_external_memory_capabilities",
    "VK_KHR_get_surface_capabilities2",
    "VK_KHR_portability_enumeration",
    "VK_EXT_display_surface_counter",
    "VK_EXT_swapchain_colorspace",
    "VK_EXT_debug_utils",
]
# Sorted KHR, EXT, Vendor, internally alphabetically
DEVICE_EXTENSIONS_TO_CHECK = [
    "VK_KHR_8bit_storage",
    "VK_KHR_calibrated_timestamps",
    "VK_KHR_external_fence_fd",
    "VK_KHR_external_memory",
    "VK_KHR_external_semaphore_fd",
    "VK_KHR_format_feature_flags2",
    "VK_KHR_global_priority",
    "VK_KHR_image_format_list",
    "VK_KHR_maintenance1",
    "VK_KHR_maintenance2",
    "VK_KHR_maintenance3",
    "VK_KHR_maintenance4",
    "VK_KHR_present_id",
    "VK_KHR_present_id2",
    "VK_KHR_present_wait",
    "VK_KHR_present_wait2",
    "VK_KHR_portability_subset",
    "VK_KHR_shared_presentable_image",
    "VK_KHR_synchronization2",
    "VK_KHR_timeline_semaphore",
    "VK_KHR_video_maintenance1",
    "VK_EXT_calibrated_timestamps",
    "VK_EXT_display_control",
    "VK_EXT_external_memory_dma_buf",
    "VK_EXT_external_memory_host",
    "VK_EXT_global_priority",
    "VK_EXT_image_drm_format_modifier",
    "VK_EXT_image_tiling_control",
    "VK_EXT_metal_objects",
    "VK_EXT_robustness2",
    "VK_ANDROID_external_format_resolve",
    "VK_GOOGLE_display_timing",
]


class Cmd:
    def __init__(
        self,
        name: str,
        member_name: Optional[str] = None,
        *,
        requires: Optional[Iterable[str]] = None,
    ):
        self.name = name
        if not member_name:
            member_name = name
        self.member_name = member_name
        if not requires:
            # normalize empty lists to None
            requires = None
        self.requires = requires

    def __repr__(self) -> str:
        args = [repr(self.name)]
        if self.member_name != self.name:
            args.append(repr(self.member_name))
        if self.requires:
            args.append(f"requires={repr(self.requires)}")
        return "Function({})".format(", ".join(args))


def wrap_condition(condition):
    if "defined" in condition:
        return condition
    return "defined({})".format(condition)


def compute_condition(pp_conditions):
    if not pp_conditions:
        return None
    return " && ".join(wrap_condition(x) for x in pp_conditions)


class ConditionalGenerator:
    """Keep track of conditions to avoid unneeded repetition of ifdefs."""

    def __init__(self):
        self.current_condition = None

    def process_condition(self, new_condition: Optional[str], finish: bool = False) -> Optional[str]:
        """Return a line (or lines) to yield if required based on the new condition state."""
        lines = []
        if self.current_condition and new_condition != self.current_condition:
            # Close current condition if required.
            lines.append("#endif // {}".format(self.current_condition))
            if not finish:
                # empty line
                lines.append("")
            self.current_condition = None

        if not finish and new_condition != self.current_condition:
            # Open new condition if required
            lines.append("#if {}".format(new_condition))
            self.current_condition = new_condition

        if lines:
            return "\n".join(lines)

    def finish(self) -> Optional[str]:
        """Return a line (or lines) to yield if required at the end of the loop."""
        return self.process_condition(None, finish=True)


def generate_per_command(
    commands: List[Cmd], per_command_handler: Callable[[Cmd], str]
):
    conditional = ConditionalGenerator()
    for cmd in commands:
        if not cmd:
            # empty line
            yield ""
            continue
        condition = compute_condition(cmd.requires)
        condition_line = conditional.process_condition(condition)
        if condition_line:
            yield condition_line

        yield per_command_handler(cmd)

    # close any trailing conditions
    condition_line = conditional.finish()
    if condition_line:
        yield condition_line


def generate_structure_members(commands: List[Cmd]):
    def per_command(cmd: Cmd):
        return "\tPFN_{} {};".format(cmd.name, cmd.member_name)

    yield from generate_per_command(commands, per_command)
    yield ''


def generate_proc_macro(macro: str, commands: List[Cmd]):
    name_width = max([len(cmd.member_name) for cmd in commands if cmd])

    def per_command(cmd: Cmd) -> str:
        return "\tvk->{} = {}(vk, {});".format(
            cmd.member_name.ljust(name_width), macro, cmd.name
        )

    return generate_per_command(
        commands,
        per_command,
    )


def make_ext_member_name(ext: str):
    return "has_{}".format(ext[3:])


def make_ext_name_define(ext: str):
    str = ext.upper()
    str = str.replace("1", "_1")
    str = str.replace("2", "_2")
    str = str.replace("3", "_3")
    str = str.replace("4", "_4")

    return "{}_EXTENSION_NAME".format(str)


def generate_ext_members(exts):
    for ext in exts:
        yield "\tbool {};".format(make_ext_member_name(ext))


def generate_ext_check(exts):
    conditional = ConditionalGenerator()
    for ext in exts:
        condition_line = conditional.process_condition(compute_condition((ext,)))
        if condition_line:
            yield condition_line
        yield "\tvk->{} = u_extension_list_contains(ext_list, {});".format(
            make_ext_member_name(ext), make_ext_name_define(ext))
    # close any trailing conditions
    condition_line = conditional.finish()
    if condition_line:
        yield condition_line


def write_generated_file(output_path: str, lines: List[str]):
    """Write a generated file with auto-generated header."""
    with open(output_path, "w", encoding="utf-8") as fp:
        fp.write("// DO NOT EDIT - Auto-generated by vk_generate_inc_files.py\n")
        fp.write("\n") # Line break after header.
        fp.write("\n".join(lines))
        fp.write("\n") # Line break at the end of the file.


def generate_helpers_h_funcs(output: str):
    """Generate function pointer struct members for vk_helpers.h"""
    lines = []
    lines.append("\t// Instance functions")
    lines.extend(generate_structure_members(get_instance_cmds()))
    lines.append("")
    lines.append("\t// Device functions")
    lines.extend(generate_structure_members(get_device_cmds()))
    write_generated_file(output, lines)


def generate_helpers_h_ext(output: str):
    """Generate extension bool members for vk_helpers.h"""
    lines = []
    lines.append("\t// Instance extensions")
    lines.extend(generate_ext_members(INSTANCE_EXTENSIONS_TO_CHECK))
    lines.append("")
    lines.append("\t// Device extensions")
    lines.extend(generate_ext_members(DEVICE_EXTENSIONS_TO_CHECK))
    write_generated_file(output, lines)


def generate_bundle_init_instance_ext(output: str):
    """Generate instance extension check code for vk_bundle_init.c"""
    lines = list(generate_ext_check(INSTANCE_EXTENSIONS_TO_CHECK))
    write_generated_file(output, lines)


def generate_bundle_init_device_ext(output: str):
    """Generate device extension check code for vk_bundle_init.c"""
    lines = list(generate_ext_check(DEVICE_EXTENSIONS_TO_CHECK))
    write_generated_file(output, lines)


def generate_function_loaders_instance(output: str):
    """Generate instance loader code for vk_function_loaders.c"""
    lines = list(generate_proc_macro("GET_INS_PROC", get_instance_cmds()))
    write_generated_file(output, lines)


def generate_function_loaders_device(output: str):
    """Generate device loader code for vk_function_loaders.c"""
    lines = list(generate_proc_macro("GET_DEV_PROC", get_device_cmds()))
    write_generated_file(output, lines)


def main():
    """Handle command line and generate file(s)."""
    parser = argparse.ArgumentParser(description='Vulkan helpers generator.')
    parser.add_argument(
        'output', type=str, nargs='+',
        help='Output file, uses the name to choose output type')
    args = parser.parse_args()

    for output in args.output:
        if output.endswith("vk_helpers_h_funcs.h.inc"):
            generate_helpers_h_funcs(output)
        elif output.endswith("vk_helpers_h_ext.h.inc"):
            generate_helpers_h_ext(output)
        elif output.endswith("vk_bundle_init_instance_ext.c.inc"):
            generate_bundle_init_instance_ext(output)
        elif output.endswith("vk_bundle_init_device_ext.c.inc"):
            generate_bundle_init_device_ext(output)
        elif output.endswith("vk_function_loaders_instance.c.inc"):
            generate_function_loaders_instance(output)
        elif output.endswith("vk_function_loaders_device.c.inc"):
            generate_function_loaders_device(output)
        else:
            raise ValueError(f"Unknown output file: {output}")


if __name__ == "__main__":
    main()
