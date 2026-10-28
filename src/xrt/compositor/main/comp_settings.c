// Copyright 2019, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Settings struct for compositor.
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @ingroup comp_main
 */

#include "util/u_debug.h"
#include "comp_settings.h"

#ifdef XRT_OS_ANDROID
#define USE_COMPUTE_DEFAULT false
#else
#define USE_COMPUTE_DEFAULT true
#endif

// clang-format off
DEBUG_GET_ONCE_LOG_OPTION(log, "XRT_COMPOSITOR_LOG", U_LOGGING_INFO)
DEBUG_GET_ONCE_BOOL_OPTION(print_modes, "XRT_COMPOSITOR_PRINT_MODES", false)
DEBUG_GET_ONCE_BOOL_OPTION(force_randr, "XRT_COMPOSITOR_FORCE_RANDR", false)
DEBUG_GET_ONCE_BOOL_OPTION(force_wayland_direct, "XRT_COMPOSITOR_FORCE_WAYLAND_DIRECT", false)
DEBUG_GET_ONCE_BOOL_OPTION(force_nvidia, "XRT_COMPOSITOR_FORCE_NVIDIA", false)
DEBUG_GET_ONCE_OPTION(nvidia_display, "XRT_COMPOSITOR_FORCE_NVIDIA_DISPLAY", NULL)
DEBUG_GET_ONCE_NUM_OPTION(vk_display, "XRT_COMPOSITOR_FORCE_VK_DISPLAY", -1)
DEBUG_GET_ONCE_BOOL_OPTION(force_xcb, "XRT_COMPOSITOR_FORCE_XCB", false)
DEBUG_GET_ONCE_BOOL_OPTION(force_wayland, "XRT_COMPOSITOR_FORCE_WAYLAND", false)
DEBUG_GET_ONCE_NUM_OPTION(force_gpu_index, "XRT_COMPOSITOR_FORCE_GPU_INDEX", -1)
DEBUG_GET_ONCE_NUM_OPTION(force_client_gpu_index, "XRT_COMPOSITOR_FORCE_CLIENT_GPU_INDEX", -1)
DEBUG_GET_ONCE_NUM_OPTION(desired_mode, "XRT_COMPOSITOR_DESIRED_MODE", -1)
DEBUG_GET_ONCE_OPTION(present_mode, "XRT_COMPOSITOR_PRESENT_MODE", "fifo")
DEBUG_GET_ONCE_NUM_OPTION(scale_percentage, "XRT_COMPOSITOR_SCALE_PERCENTAGE", 140)
DEBUG_GET_ONCE_BOOL_OPTION(xcb_fullscreen, "XRT_COMPOSITOR_XCB_FULLSCREEN", false)
DEBUG_GET_ONCE_NUM_OPTION(xcb_display, "XRT_COMPOSITOR_XCB_DISPLAY", -1)
DEBUG_GET_ONCE_NUM_OPTION(default_framerate, "XRT_COMPOSITOR_DEFAULT_FRAMERATE", 60)
DEBUG_GET_ONCE_BOOL_OPTION(compute, "XRT_COMPOSITOR_COMPUTE", USE_COMPUTE_DEFAULT)
// clang-format on

static inline void
add_format(struct comp_settings *s, VkFormat format)
{
	uint32_t count = s->format_count;

	// Just in case, but should never happen.
	if (count >= ARRAY_SIZE(s->formats)) {
		U_LOG_E("Too many formats!");
		return;
	}

	s->formats[count++] = format;
	s->format_count = count;
}

static VkPresentModeKHR
get_desired_present_mode(void)
{
	const char *present_mode_str = debug_get_option_present_mode();

	if (present_mode_str == NULL || present_mode_str[0] == '\0' || strcmp(present_mode_str, "fifo") == 0 ||
	    strcmp(present_mode_str, "FIFO") == 0) {
		return VK_PRESENT_MODE_FIFO_KHR;
	} else if (strcmp(present_mode_str, "fifo_relaxed") == 0 || strcmp(present_mode_str, "FIFO_RELAXED") == 0) {
		return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	} else if (strcmp(present_mode_str, "immediate") == 0 || strcmp(present_mode_str, "IMMEDIATE") == 0) {
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	} else if (strcmp(present_mode_str, "mailbox") == 0 || strcmp(present_mode_str, "MAILBOX") == 0) {
		return VK_PRESENT_MODE_MAILBOX_KHR;
	}

#ifdef VK_KHR_shared_presentable_image
	if (strcmp(present_mode_str, "shared_continuous_refresh") == 0 ||
	    strcmp(present_mode_str, "SHARED_CONTINUOUS_REFRESH") == 0) {
		return VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR;
	}
	if (strcmp(present_mode_str, "shared_demand_refresh") == 0 ||
	    strcmp(present_mode_str, "SHARED_DEMAND_REFRESH") == 0) {
		return VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR;
	}
#endif

	U_LOG_E("Unrecognized present mode: %s", present_mode_str);
	return VK_PRESENT_MODE_FIFO_KHR;
}


/*
 *
 * 'Exported' functions.
 *
 */

void
comp_settings_init(struct comp_settings *s, struct xrt_device *xdev)
{
	int default_framerate = debug_get_num_option_default_framerate();

	uint64_t interval_ns = xdev->hmd->screens[0].nominal_frame_interval_ns;
	if (interval_ns == 0) {
		interval_ns = (1000 * 1000 * 1000) / default_framerate;
	}

	s->use_compute = debug_get_bool_option_compute();

	if (s->use_compute) {
		// Tested working with a PSVR2 and a patched Mesa. Native format of the PSVR2. 10-bit formats should be
		// preferred in all cases for lower colour banding.
		add_format(s, VK_FORMAT_A2B10G10R10_UNORM_PACK32);

		// Default 8-bit channel 32-bit pixel format, most commonly supported UNORM format across Windows and
		// Linux.
		add_format(s, VK_FORMAT_B8G8R8A8_UNORM);

		// Next most common UNORM format.
		add_format(s, VK_FORMAT_R8G8B8A8_UNORM);

		// Seen on some NVIDIA cards.
		add_format(s, VK_FORMAT_A8B8G8R8_UNORM_PACK32);

		// 32-bit formats should be preferred over a 64-bit format, for performance/memory reasons.
		add_format(s, VK_FORMAT_R16G16B16A16_SFLOAT);

		// Untested: Super constrained platforms. Absolute last resort.
		add_format(s, VK_FORMAT_A1R5G5B5_UNORM_PACK16);
	} else {
#if defined(XRT_OS_ANDROID)
		/*
		 * On Android the most ubiquitous sRGB format is R8G8B8A8_SRGB.
		 * https://vulkan.gpuinfo.org/listsurfaceformats.php?platform=android
		 */
		add_format(s, VK_FORMAT_R8G8B8A8_SRGB);

		// Fallback
		add_format(s, VK_FORMAT_B8G8R8A8_SRGB);
#elif defined(XRT_OS_LINUX) || defined(XRT_OS_WINDOWS)
		/*
		 * On Linux the most ubiquitous sRGB format is B8G8R8A8_SRGB.
		 * https://vulkan.gpuinfo.org/listsurfaceformats.php?platform=linux
		 *
		 * On Windows the most ubiquitous sRGB format is B8G8R8A8_SRGB.
		 * https://vulkan.gpuinfo.org/listsurfaceformats.php?platform=windows
		 */
		add_format(s, VK_FORMAT_B8G8R8A8_SRGB);

		// Fallback
		add_format(s, VK_FORMAT_R8G8B8A8_SRGB);
#elif defined(XRT_OS_OSX)
		/*
		 * On MacOS the most ubiquitous sRGB format is B8G8R8A8_SRGB,
		 * and it's the only sRGB (non-linear) format available.
		 * https://vulkan.gpuinfo.org/listsurfaceformats.php?platform=macos
		 */
		add_format(s, VK_FORMAT_B8G8R8A8_SRGB);
#else
#error "Need to pick default swapchain format for this platform!"
#endif

		// Seen as the only sRGB format on some NVIDIA cards.
		add_format(s, VK_FORMAT_A8B8G8R8_SRGB_PACK32);
	}

	s->display = debug_get_num_option_xcb_display();
	s->color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	s->present_mode = get_desired_present_mode();
	s->fullscreen = debug_get_bool_option_xcb_fullscreen();
	s->preferred.width = xdev->hmd->screens[0].w_pixels;
	s->preferred.height = xdev->hmd->screens[0].h_pixels;
	s->nominal_frame_interval_ns = interval_ns;
	s->log_level = debug_get_log_option_log();
	s->print_modes = debug_get_bool_option_print_modes();
	s->selected_gpu_index = debug_get_num_option_force_gpu_index();
	s->client_gpu_index = debug_get_num_option_force_client_gpu_index();
	s->desired_mode = debug_get_num_option_desired_mode();
	s->viewport_scale = debug_get_num_option_scale_percentage() / 100.0;


	s->nvidia_display = debug_get_option_nvidia_display();
	if (debug_get_bool_option_force_nvidia()) {
		s->target_identifier = "x11_direct_nvidia";
	}

	s->vk_display = debug_get_num_option_vk_display();
	if (s->vk_display >= 0) {
		s->target_identifier = "vk_display";
	}

	if (debug_get_bool_option_force_randr()) {
		s->target_identifier = "x11_direct";
	}

	if (debug_get_bool_option_force_wayland_direct()) {
		s->target_identifier = "direct_wayland";
	}

	if (debug_get_bool_option_force_xcb()) {
		s->target_identifier = "x11";

		// HMD screen tends to be much larger then monitors.
		s->preferred.width /= 2;
		s->preferred.height /= 2;
	}
	if (debug_get_bool_option_force_wayland()) {
		s->target_identifier = "wayland";

		// HMD screen tends to be much larger then monitors.
		s->preferred.width /= 2;
		s->preferred.height /= 2;
	}
}
