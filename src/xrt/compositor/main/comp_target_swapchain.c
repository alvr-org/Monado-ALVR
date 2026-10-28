// Copyright 2019-2026, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Target Vulkan swapchain code.
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_main
 */

#include "os/os_threading.h"

#include "util/u_misc.h"
#include "util/u_pacing.h"
#include "util/u_debug.h"
#include "util/u_pretty_print.h"
#include "util/u_thread_priority.h"

#include "vk/vk_surface_info.h"

#include "main/comp_compositor.h"
#include "main/comp_target_swapchain.h"

#include <assert.h>
#include <stdlib.h>


/*
 *
 * Vulkan functions.
 *
 */

/*
 * For all direct mode outputs 2 is what we want since we want to run
 * lockstep with the display. Most direct mode swapchains only supports
 * FIFO mode, and since there is no commonly available Vulkan API to
 * wait for a specific VBLANK event, even just the latest, we can set
 * the number of images to two and then acquire immediately after
 * present. Since the old images are being displayed and the new can't
 * be flipped this will block until the flip has gone through. Crude but
 * works well enough on both AMD(Mesa) and Nvidia(Blob).
 *
 * When not in direct mode and display to a composited window we
 * probably want 3, but most compositors on Linux sets the minImageCount
 * to 3 anyways so we get what we want.
 */
DEBUG_GET_ONCE_NUM_OPTION(preferred_at_least_image_count, "XRT_COMPOSITOR_PREFERRED_IMAGE_COUNT", 2)
DEBUG_GET_ONCE_BOOL_OPTION(use_present_wait, "XRT_COMPOSITOR_USE_PRESENT_WAIT", false)
DEBUG_GET_ONCE_BOOL_OPTION(use_display_timing, "XRT_COMPOSITOR_USE_DISPLAY_TIMING", true)

static inline struct vk_bundle *
get_vk(struct comp_target_swapchain *cts)
{
	return &cts->base.c->base.vk;
}

static inline bool
is_shared_present_mode(struct vk_bundle *vk, VkPresentModeKHR present_mode)
{
#ifdef VK_KHR_shared_presentable_image
	return vk->has_KHR_shared_presentable_image &&                           //
	       (present_mode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR || //
	        present_mode == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR);      //
#else
	(void)present_mode;
	return false;
#endif
}

static inline bool
is_shared_presentable_image(struct comp_target_swapchain *cts)
{
	struct vk_bundle *vk = get_vk(cts);
	return is_shared_present_mode(vk, cts->present_mode);
}

static void
destroy_old(struct comp_target_swapchain *cts, VkSwapchainKHR old)
{
	struct vk_bundle *vk = get_vk(cts);

	if (old != VK_NULL_HANDLE) {
		vk->vkDestroySwapchainKHR(vk->device, old, NULL);
	}
}

static void
destroy_image_views(struct comp_target_swapchain *cts)
{
	if (cts->base.images == NULL) {
		return;
	}

	struct vk_bundle *vk = get_vk(cts);

	for (uint32_t i = 0; i < cts->base.image_count; i++) {
		if (cts->base.images[i].view == VK_NULL_HANDLE) {
			continue;
		}

		vk->vkDestroyImageView(vk->device, cts->base.images[i].view, NULL);
		cts->base.images[i].view = VK_NULL_HANDLE;
	}

	free(cts->base.images);
	cts->base.images = NULL;
}

static void
create_image_views(struct comp_target_swapchain *cts)
{
	struct vk_bundle *vk = get_vk(cts);
	uint32_t image_count = 0;
	VkImage *images = NULL;
	VkResult ret;

	// Destroy old images first.
	destroy_image_views(cts);

	ret = vk_enumerate_swapchain_images( //
	    vk,                              // vk_bundle
	    cts->swapchain.handle,           // swapchain
	    &image_count,                    // out_image_count
	    &images);                        // out_images
	VK_CHK_WITH_GOTO(ret, "vk_enumerate_swapchain_images", err);


	/*
	 * Create image views.
	 */

	cts->base.image_count = image_count;
	cts->base.images = U_TYPED_ARRAY_CALLOC(struct comp_target_image, cts->base.image_count);

	VkImageSubresourceRange subresource_range = {
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = 1,
	    .baseArrayLayer = 0,
	    .layerCount = 1,
	};

	for (uint32_t i = 0; i < cts->base.image_count; i++) {
		cts->base.images[i].handle = images[i];
		vk_create_view(                 //
		    vk,                         // vk_bundle
		    cts->base.images[i].handle, // image
		    VK_IMAGE_VIEW_TYPE_2D,      // type
		    cts->surface.format.format, // format
		    subresource_range,          // subresource_range
		    &cts->base.images[i].view); // out_view


		VK_NAME_IMAGE_VIEW(vk, cts->base.images[i].view, "comp_target_swapchain image view");
	}

	free(images);

	return;

err:
	cts->base.image_count = 0;
}

static VkExtent2D
select_extent(struct comp_target_swapchain *cts, VkSurfaceCapabilitiesKHR caps, VkExtent2D preferred)
{
	/*
	 * A sub-class wants us to use these extents over the ones the
	 * compositor preferred, this is probably due to the target only
	 * supporting this size so we better respect those wishes.
	 */
	if (cts->override.compositor_extent) {
		preferred.width = cts->override.extent.width;
		preferred.height = cts->override.extent.height;
	}

	// If width (and height) equals the special value 0xFFFFFFFF,
	// the size of the surface will be set by the swapchain
	if (caps.currentExtent.width == (uint32_t)-1) {
		assert(preferred.width > 0 && preferred.height > 0);

		VkExtent2D extent = {
		    .width = preferred.width,
		    .height = preferred.height,
		};
		return extent;
	}

	if (caps.currentExtent.width != preferred.width || //
	    caps.currentExtent.height != preferred.height) {
		COMP_DEBUG(cts->base.c, "Using swap chain extent dimensions %dx%d instead of requested %dx%d.",
		           caps.currentExtent.width,  //
		           caps.currentExtent.height, //
		           preferred.width,           //
		           preferred.height);         //
	}

	return caps.currentExtent;
}

static uint32_t
select_image_count(struct comp_target_swapchain *cts,
                   VkSurfaceCapabilitiesKHR caps,
                   uint32_t preferred_at_least_image_count)
{
	if (is_shared_presentable_image(cts)) {
		/*!
		 * Ignore caps.minImageCount as it was observed one android device
		 * it's value was still > 1 for non-shared present modes.
		 */
		return 1;
	}

	// Min is equals to or greater to what we prefer, pick min then.
	if (caps.minImageCount >= preferred_at_least_image_count) {
		return caps.minImageCount;
	}

	// Any max is good, so pick the one we want.
	if (caps.maxImageCount == 0) {
		return preferred_at_least_image_count;
	}

	// Clamp to max.
	if (caps.maxImageCount < preferred_at_least_image_count) {
		return caps.maxImageCount;
	}

	// More then min less the max, pick what we want.
	return preferred_at_least_image_count;
}

static bool
check_surface_present_mode(struct comp_target_swapchain *cts,
                           const struct vk_surface_info *info,
                           VkImageUsageFlags image_usage,
                           VkPresentModeKHR present_mode)
{
	struct vk_bundle *vk = get_vk(cts);

	bool image_usage_supported = true;
	for (uint32_t i = 0; i < info->present_mode_count; i++) {
		if (info->present_modes[i] != present_mode) {
			continue;
		}

#ifdef VK_KHR_shared_presentable_image
		if (!is_shared_present_mode(vk, present_mode)) {
			return true;
		}

		const VkImageUsageFlags supported_image_usage =
		    info->shared_present_caps.sharedPresentSupportedUsageFlags;
		if ((supported_image_usage & image_usage) != image_usage) {
			image_usage_supported = false;
			break;
		}
#endif
		// Everything checked out!
		return true;
	}

	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	u_pp(dg, "Present mode %s not supported, available:", vk_present_mode_string(present_mode));

	if (!image_usage_supported) {
		u_pp(dg, "Requested image_usage 0x%x not supported for present mode %s.", image_usage,
		     vk_present_mode_string(present_mode));
	}

	for (uint32_t i = 0; i < info->present_mode_count; i++) {
		u_pp(dg, "\n\t%s", vk_present_mode_string(info->present_modes[i]));
	}

	COMP_ERROR(cts->base.c, "%s", sink.buffer);

	return false;
}

static bool
pick_first_matching_surface_format(const struct comp_target_create_images_info *create_info,
                                   VkSurfaceFormatKHR *surface_formats,
                                   uint32_t surface_format_count,
                                   VkSurfaceFormatKHR *out_surface_format)
{
	for (uint32_t i = 0; i < create_info->format_count; i++) {
		// Format to check against.
		VkFormat format = create_info->formats[i];

		for (uint32_t k = 0; k < surface_format_count; k++) {
			if (surface_formats[k].format == format) {
				// Perfect match.
				*out_surface_format = surface_formats[k];
				return true;
			}
		}
	}

	return false;
}

static bool
find_surface_format(struct comp_target_swapchain *cts,
                    const struct comp_target_create_images_info *create_info,
                    const struct vk_surface_info *info,
                    VkSurfaceFormatKHR *out_surface_format)
{
	VkSurfaceFormatKHR *colorspace_matches = NULL;
	uint32_t colorspace_match_count = 0;
	bool bret = false;


	/*
	 * Gather surface formats that match our color space,
	 * we will select from these in preference to others.
	 */
	colorspace_matches = U_TYPED_ARRAY_CALLOC(VkSurfaceFormatKHR, info->format_count);
	for (uint32_t i = 0; i < info->format_count; i++) {
		if (info->formats[i].colorSpace == cts->preferred.color_space) {
			colorspace_matches[colorspace_match_count++] = info->formats[i];
		}
	}

	/*
	 * We first try the list of surface formats with
	 * a matching color space, if any, to the one we want.
	 */
	bret = pick_first_matching_surface_format( //
	    create_info,                           //
	    colorspace_matches,                    //
	    colorspace_match_count,                //
	    out_surface_format);                   //

	// Always free these formats.
	free(colorspace_matches);

	// Check result.
	if (bret) {
		// Done now.
		goto done;
	}

	/*
	 * Try to find any surface format that at least has a matching
	 * Vulkan format to one of the formats the compositor can use.
	 */
	bret = pick_first_matching_surface_format( //
	    create_info,                           //
	    info->formats,                         //
	    info->format_count,                    //
	    out_surface_format);                   //
	if (!bret) {
		COMP_ERROR(cts->base.c, "Could not find any matching surface formats!");
		return false;
	}

	COMP_WARN(cts->base.c,
	          "Returning VkSurfaceFormat with known-wrong color space!\n"
	          "\tVkSurfaceFormat::format matched one we wanted.\n"
	          "\tBut color shift will probably be happening.");

done:
	COMP_DEBUG(cts->base.c,
	           "VkSurfaceFormatKHR"
	           "\n\tpicked: [format = %s, colorSpace = %s]"
	           "\n\tpreferred: [format = %s, colorSpace = %s]",
	           vk_format_string(out_surface_format->format),          //
	           vk_color_space_string(out_surface_format->colorSpace), //
	           vk_format_string(create_info->formats[0]),             //
	           vk_color_space_string(cts->preferred.color_space));    //

	return true;
}

static void
do_update_timings_google_display_timing(struct comp_target_swapchain *cts)
{
	struct vk_bundle *vk = get_vk(cts);

	if (!vk->has_GOOGLE_display_timing) {
		return;
	}

	if (cts->swapchain.handle == VK_NULL_HANDLE) {
		return;
	}

	uint32_t count = 0;
	vk->vkGetPastPresentationTimingGOOGLE( //
	    vk->device,                        //
	    cts->swapchain.handle,             //
	    &count,                            //
	    NULL);                             //
	if (count <= 0) {
		return;
	}

	VkPastPresentationTimingGOOGLE *timings = U_TYPED_ARRAY_CALLOC(VkPastPresentationTimingGOOGLE, count);
	vk->vkGetPastPresentationTimingGOOGLE( //
	    vk->device,                        //
	    cts->swapchain.handle,             //
	    &count,                            //
	    timings);                          //
	int64_t now_ns = os_monotonic_get_ns();
	for (uint32_t i = 0; i < count; i++) {
		u_pc_info(cts->upc,                       //
		          timings[i].presentID,           //
		          timings[i].desiredPresentTime,  //
		          timings[i].actualPresentTime,   //
		          timings[i].earliestPresentTime, //
		          timings[i].presentMargin,       //
		          now_ns);                        //
	}

	free(timings);
}

static void
do_update_timings_vblank_thread(struct comp_target_swapchain *cts)
{
	if (!cts->vblank.thread_running) {
		return;
	}

	int64_t last_vblank_ns;

	os_thread_helper_lock(&cts->vblank.event_thread);
	last_vblank_ns = cts->vblank.last_vblank_ns;
	cts->vblank.last_vblank_ns = 0;
	os_thread_helper_unlock(&cts->vblank.event_thread);

	if (last_vblank_ns) {
		u_pc_update_vblank_from_display_control(cts->upc, last_vblank_ns);
	}
}

#if defined(VK_EXT_display_surface_counter) && defined(VK_EXT_display_control)
static bool
check_surface_counter_caps(struct comp_target *ct, struct vk_bundle *vk, struct comp_target_swapchain *cts)
{
	if (!vk->has_EXT_display_surface_counter) {
		return true;
	}

	VkSurfaceCapabilities2EXT caps = {
	    .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES2_EXT,
	};

	VkResult ret = vk->vkGetPhysicalDeviceSurfaceCapabilities2EXT(vk->physical_device, cts->surface.handle, &caps);
	if (ret != VK_SUCCESS) {
		COMP_ERROR(ct->c, "vkGetPhysicalDeviceSurfaceCapabilities2EXT: %s", vk_result_string(ret));
		return false;
	}

	cts->surface.surface_counter_flags = caps.supportedSurfaceCounters;
	COMP_DEBUG(ct->c, "Supported surface counter flags: %d", caps.supportedSurfaceCounters);

	return true;
}

static uint64_t
get_surface_counter_val(struct comp_target *ct)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	struct vk_bundle *vk = get_vk(cts);
	VkResult ret;

	if ((cts->surface.surface_counter_flags & VK_SURFACE_COUNTER_VBLANK_EXT) == 0) {
		return 0;
	}

	uint64_t counter_val = 0;
	ret = vk->vkGetSwapchainCounterEXT( //
	    vk->device,                     // device
	    cts->swapchain.handle,          // swapchain
	    VK_SURFACE_COUNTER_VBLANK_EXT,  // counter
	    &counter_val);                  // pCounterValue

	if (ret == VK_SUCCESS) {
		COMP_SPEW(cts->base.c, "vkGetSwapchainCounterEXT: %" PRIu64, counter_val);
	} else if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
		COMP_ERROR(cts->base.c, "vkGetSwapchainCounterEXT: Swapchain out of date!");
	} else {
		COMP_ERROR(cts->base.c, "vkGetSwapchainCounterEXT: %s", vk_result_string(ret));
	}

	return counter_val;
}

static bool
vblank_event_func(struct comp_target *ct, int64_t *out_timestamp_ns)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;

	struct vk_bundle *vk = get_vk(cts);
	VkResult ret;


	VkDisplayEventInfoEXT event_info = {
	    .sType = VK_STRUCTURE_TYPE_DISPLAY_EVENT_INFO_EXT,
	    .displayEvent = VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT,
	};

	VkFence vblank_event_fence = VK_NULL_HANDLE;
	ret = vk->vkRegisterDisplayEventEXT(vk->device, cts->display, &event_info, NULL, &vblank_event_fence);
	if (ret != VK_SUCCESS) {
		COMP_ERROR(ct->c, "vkRegisterDisplayEventEXT: %s", vk_result_string(ret));
		return false;
	}

	// Name for debugging.
	VK_NAME_FENCE(vk, vblank_event_fence, "Comp VBlank");

	// Not scoped to not effect timing.
	COMP_TRACE_IDENT(vblank);

	// Do the wait
	ret = vk->vkWaitForFences(vk->device, 1, &vblank_event_fence, true, time_s_to_ns(1));

	// As quickly as possible after the fence has fired.
	int64_t now_ns = os_monotonic_get_ns();

	bool valid = false;
	if (ret == VK_SUCCESS) {
		/*
		 * Causes a lot of multiple thread access validation warnings
		 * and is currently not used by the code so skip for now.
		 */
#if 0
		uint64_t counter_val = get_surface_counter_val(ct);

		static uint64_t last_ns = 0;
		uint64_t diff_ns = now_ns - last_ns;
		last_ns = now_ns;

		double now_ms_f = time_ns_to_ms_f(now_ns);
		double diff_ms_f = time_ns_to_ms_f(diff_ns);

		COMP_DEBUG(ct->c, "vblank event at os time %fms, diff: %fms, vblank: #%lu", now_ms_f, diff_ms_f,
		           counter_val);
#else
		(void)&get_surface_counter_val;
#endif

		*out_timestamp_ns = now_ns;
		valid = true;

	} else if (ret == VK_TIMEOUT) {
		COMP_WARN(ct->c, "vkWaitForFences: VK_TIMEOUT");
	} else {
		COMP_ERROR(ct->c, "vkWaitForFences: %s", vk_result_string(ret));
	}

	vk->vkDestroyFence(vk->device, vblank_event_fence, NULL);

	return valid;
}

static void *
run_vblank_event_thread(void *ptr)
{
	struct comp_target *ct = (struct comp_target *)ptr;
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;

	COMP_DEBUG(ct->c, "Surface thread starting");

	os_thread_name_self("VBlank Events");
	U_TRACE_SET_THREAD_NAME("VBlank Events");

	// Try to raise priority of this thread.
	u_try_to_set_realtime_priority_on_thread(U_LOGGING_INFO, "VBlank Events");

	os_thread_helper_lock(&cts->vblank.event_thread);

	while (os_thread_helper_is_running_locked(&cts->vblank.event_thread)) {

		if (!cts->vblank.should_wait) {
			// Wait to be woken up.
			os_thread_helper_wait_locked(&cts->vblank.event_thread);

			/*
			 * Loop back to the top to check if we should stop,
			 * also handles spurious wakeups by re-checking the
			 * condition in the if case. Essentially two loops.
			 */
			continue;
		}

		// We should wait for a vblank event.
		cts->vblank.should_wait = false;

		if (!cts->vblank.event_active) {
			cts->vblank.event_active = true;
			continue;
		}

		// Unlock while waiting.
		os_thread_helper_unlock(&cts->vblank.event_thread);

		int64_t when_ns = 0;
		bool valid = vblank_event_func(ct, &when_ns);

		// Just keep swimming.
		os_thread_helper_lock(&cts->vblank.event_thread);

		if (valid) {
			cts->vblank.last_vblank_ns = when_ns;
		}
	}

	os_thread_helper_unlock(&cts->vblank.event_thread);

	return NULL;
}

static bool
create_vblank_event_thread(struct comp_target *ct)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	if (cts->display == VK_NULL_HANDLE) {
		return true;
	}

	int thread_ret = os_thread_helper_start(&cts->vblank.event_thread, run_vblank_event_thread, ct);
	if (thread_ret != 0) {
		COMP_ERROR(ct->c, "Failed to start vblank (first pixel out) event thread");
		return false;
	}

	COMP_DEBUG(ct->c, "Started vblank (first pixel out) event thread.");

	// Set this here.
	cts->vblank.thread_running = true;

	return true;
}
#endif

static void
target_fini_semaphores(struct comp_target_swapchain *cts)
{
	struct vk_bundle *vk = get_vk(cts);

	if (cts->base.semaphores.present_complete != VK_NULL_HANDLE) {
		vk->vkDestroySemaphore(vk->device, cts->base.semaphores.present_complete, NULL);
		cts->base.semaphores.present_complete = VK_NULL_HANDLE;
	}

	if (cts->base.semaphores.render_complete != VK_NULL_HANDLE) {
		vk->vkDestroySemaphore(vk->device, cts->base.semaphores.render_complete, NULL);
		cts->base.semaphores.render_complete = VK_NULL_HANDLE;
	}
}

static void
target_init_semaphores(struct comp_target_swapchain *cts)
{
	struct vk_bundle *vk = get_vk(cts);
	VkResult ret;

	target_fini_semaphores(cts);

	VkSemaphoreCreateInfo info = {
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	ret = vk->vkCreateSemaphore(vk->device, &info, NULL, &cts->base.semaphores.present_complete);
	if (ret != VK_SUCCESS) {
		COMP_ERROR(cts->base.c, "vkCreateSemaphore: %s", vk_result_string(ret));
	}

	VK_NAME_SEMAPHORE(vk, cts->base.semaphores.present_complete,
	                  "comp_target_swapchain semaphore present complete");

	cts->base.semaphores.render_complete_is_timeline = false;
	ret = vk->vkCreateSemaphore(vk->device, &info, NULL, &cts->base.semaphores.render_complete);
	if (ret != VK_SUCCESS) {
		COMP_ERROR(cts->base.c, "vkCreateSemaphore: %s", vk_result_string(ret));
	}

	VK_NAME_SEMAPHORE(vk, cts->base.semaphores.render_complete, "comp_target_swapchain semaphore render complete");
}


/*
 *
 * Member functions.
 *
 */

static void
comp_target_swapchain_create_images(struct comp_target *ct,
                                    const struct comp_target_create_images_info *create_info,
                                    struct vk_bundle_queue *present_queue)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	struct vk_bundle *vk = get_vk(cts);
	VkBool32 supported;
	VkResult ret;

	assert(present_queue != NULL);

	int64_t now_ns = os_monotonic_get_ns();
	// Some platforms really don't like the pacing_compositor code.
	bool use_display_timing_if_available =
	    debug_get_bool_option_use_display_timing() && cts->timing_usage == COMP_TARGET_USE_DISPLAY_IF_AVAILABLE;
	if (cts->upc == NULL && use_display_timing_if_available && vk->has_GOOGLE_display_timing) {
		struct u_pc_display_timing_config config = u_pc_display_timing_get_default_config();
		u_pc_display_timing_create(ct->c->frame_interval_ns, &config, &cts->upc);
	} else if (cts->upc == NULL) {
		u_pc_fake_create(ct->c->frame_interval_ns, now_ns, &cts->upc);
	}

	// Free old image views.
	destroy_image_views(cts);

	target_init_semaphores(cts);

	VkSwapchainKHR old_swapchain_handle = cts->swapchain.handle;

	cts->base.image_count = 0;
	cts->swapchain.handle = VK_NULL_HANDLE;
	cts->present_mode = create_info->present_mode;
	cts->preferred.color_space = create_info->color_space;


	/*
	 * Checking for support and selecting some properties.
	 */

	// Get information.
	struct vk_surface_info info = {0};
	ret = vk_surface_info_fill_in(vk, &info, cts->surface.handle);
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vk_surface_info_fill_in: %s", vk_result_string(ret));
		destroy_old(cts, old_swapchain_handle);
		return;
	}

#ifdef VK_KHR_present_id2
	cts->surface.present_id2_supported = vk->features.present_id2 && info.present_id2_caps.presentId2Supported;
#endif
#ifdef VK_KHR_present_wait2
	cts->surface.present_wait2_supported =
	    vk->features.present_wait2 && info.present_wait2_caps.presentWait2Supported;
#endif

	// If we have the present wait extension, mark it as supported now
	ct->wait_for_present_supported =
	    ((vk->features.present_id && vk->features.present_wait) ||
	     (cts->surface.present_id2_supported && cts->surface.present_wait2_supported)) &&
	    debug_get_bool_option_use_present_wait();

	// Can we create swapchains from the surface on this device and queue.
	ret = comp_target_queue_supports_present(ct, present_queue, &supported);
	VK_CHK_WITH_GOTO(ret, "comp_target_queue_supports_present", error_print_and_free);
	if (!supported) {
		/*
		 * As the code is written now this won't actually fail in this
		 * function, we can happily create the images. But when trying
		 * to present the images it will fail.
		 */
		COMP_ERROR(ct->c, "Queue family (index: %u) does not support presentation ops for %s surfaces.",
		           present_queue->family_index, ct->name);
		goto error_print_and_free;
	}

	// Check that the present mode is supported.
	bool bret = check_surface_present_mode( //
	    cts,                                //
	    &info,                              //
	    create_info->image_usage,           //
	    cts->present_mode);                 //
	if (!bret) {
		goto error_print_and_free;
	}

	// Find the correct format.
	if (!find_surface_format(cts, create_info, &info, &cts->surface.format)) {
		goto error_print_and_free;
	}

	// Always print the first one.
	enum u_logging_level print_log_level = U_LOGGING_DEBUG;
	if (!cts->has_logged_info) {
		print_log_level = U_LOGGING_INFO;
		cts->has_logged_info = true;
	}

	// Print info about the surface.
	vk_print_surface_info(vk, &info, print_log_level);

	// Get the caps first.
	VkSurfaceCapabilitiesKHR surface_caps = info.caps;

	// Now we can free the info.
	vk_surface_info_destroy(&info);


	/*
	 * Non-failable selections.
	 */

	// Get the extents of the swapchain.
	VkExtent2D extent = select_extent(cts, surface_caps, create_info->extent);

	if (surface_caps.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
	    surface_caps.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
		COMP_DEBUG(ct->c, "Swapping width and height, since we are going to pre rotate");
		uint32_t w2 = extent.width;
		uint32_t h2 = extent.height;
		extent.width = h2;
		extent.height = w2;
	}

	const uint32_t preferred_at_least_image_count = debug_get_num_option_preferred_at_least_image_count();

	// Get the image count.
	uint32_t image_count = select_image_count(cts, surface_caps, preferred_at_least_image_count);

	/*
	 * VUID-VkSwapchainCreateInfoKHR-compositeAlpha-01280
	 * compositeAlpha must be one of the bits present in the supportedCompositeAlpha member of the
	 * VkSurfaceCapabilitiesKHR structure returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR for the surface.
	 */
	VkCompositeAlphaFlagsKHR composite_alpha = 0;
	if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
		composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	} else if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
		composite_alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	} else {
		COMP_ERROR(ct->c, "Unsupported composite alpha");
		goto error_print_and_free;
	}

	/*
	 * Do the creation.
	 */

	// Create the swapchain now.
	VkSwapchainCreateInfoKHR swapchain_info = {
	    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .surface = cts->surface.handle,
	    .minImageCount = image_count,
	    .imageFormat = cts->surface.format.format,
	    .imageColorSpace = cts->surface.format.colorSpace,
	    .imageExtent =
	        {
	            .width = extent.width,
	            .height = extent.height,
	        },
	    .imageArrayLayers = 1,
	    .imageUsage = create_info->image_usage,
	    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .preTransform = surface_caps.currentTransform,
	    .compositeAlpha = composite_alpha,
	    .presentMode = cts->present_mode,
	    .clipped = VK_TRUE,
	    .oldSwapchain = old_swapchain_handle,
	};

#if defined(VK_KHR_present_id2) && defined(VK_KHR_present_wait2)
	if (cts->surface.present_id2_supported && cts->surface.present_wait2_supported) {
		swapchain_info.flags |= VK_SWAPCHAIN_CREATE_PRESENT_ID_2_BIT_KHR;
		swapchain_info.flags |= VK_SWAPCHAIN_CREATE_PRESENT_WAIT_2_BIT_KHR;
	}
#endif

	// Print what we are creating.
	vk_print_swapchain_create_info(vk, &swapchain_info, print_log_level);

	// Everything decided and logged, do the creation.
	ret = vk->vkCreateSwapchainKHR(vk->device, &swapchain_info, NULL, &cts->swapchain.handle);

	// Always destroy the old.
	destroy_old(cts, old_swapchain_handle);

	if (ret != VK_SUCCESS) {
		COMP_ERROR(ct->c, "vkCreateSwapchainKHR: %s", vk_result_string(ret));
		return;
	}

	VK_NAME_SWAPCHAIN(vk, cts->swapchain.handle, "comp_target_swapchain swapchain");


	/*
	 * Set target info.
	 */

	cts->base.width = extent.width;
	cts->base.height = extent.height;
	cts->base.format = cts->surface.format.format;
	cts->base.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	cts->base.surface_transform = surface_caps.currentTransform;
	cts->base.present_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;

#ifdef VK_KHR_shared_presentable_image
	if (is_shared_presentable_image(cts)) {
		cts->base.present_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		cts->base.final_layout = VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
	}
#endif

	create_image_views(cts);

#ifdef VK_EXT_display_control
	if (!check_surface_counter_caps(ct, vk, cts)) {
		COMP_ERROR(ct->c, "Failed to query surface counter capabilities");
	}

	if (vk->has_EXT_display_control && cts->display != VK_NULL_HANDLE) {
		if (cts->vblank.thread_running) {
			// Already running.
		} else if (create_vblank_event_thread(ct)) {
			COMP_INFO(ct->c, "Started vblank event thread!");
		} else {
			COMP_ERROR(ct->c, "Failed to start vblank event thread!");
		}
	} else {
		COMP_INFO(ct->c, "Not using vblank event thread!");
	}
#endif

	// Done now.
	return;

error_print_and_free:
	vk_print_surface_info(vk, &info, U_LOGGING_ERROR);
	destroy_old(cts, old_swapchain_handle);
	vk_surface_info_destroy(&info);
}

static bool
comp_target_swapchain_has_images(struct comp_target *ct)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	return cts->surface.handle != VK_NULL_HANDLE && cts->swapchain.handle != VK_NULL_HANDLE;
}

static VkResult
comp_target_swapchain_acquire_next_image(struct comp_target *ct, uint32_t *out_index)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	struct vk_bundle *vk = get_vk(cts);

	if (!comp_target_swapchain_has_images(ct)) {
		//! @todo what error to return here?
		return VK_ERROR_INITIALIZATION_FAILED;
	}

#ifdef VK_KHR_shared_presentable_image
	const bool is_shared_presentable = is_shared_presentable_image(cts);

	if (is_shared_presentable && cts->shared_present_acquired) {
		*out_index = 0;
		return vk->vkGetSwapchainStatusKHR(vk->device, cts->swapchain.handle);
	}
#endif

	VkResult ret = vk->vkAcquireNextImageKHR(  //
	    vk->device,                            // device
	    cts->swapchain.handle,                 // swapchain
	    UINT64_MAX,                            // timeout
	    cts->base.semaphores.present_complete, // semaphore
	    VK_NULL_HANDLE,                        // fence
	    out_index);                            // pImageIndex
	VK_CHK_AND_RET(ret, "vkAcquireNextImageKHR");

#ifdef VK_KHR_shared_presentable_image
	if (is_shared_presentable) {
		assert(*out_index == 0);
		cts->shared_present_acquired = true;
	}
#endif

	return ret;
}

static VkResult
comp_target_swapchain_present(struct comp_target *ct,
                              struct vk_bundle_queue *present_queue,
                              uint32_t index,
                              uint64_t timeline_semaphore_value,
                              int64_t desired_present_time_ns,
                              int64_t present_slop_ns)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	struct vk_bundle *vk = get_vk(cts);

	assert(present_queue != NULL);
	assert(cts->current_frame_id > 0);
	assert(cts->current_frame_id <= UINT32_MAX);

	VkPresentInfoKHR present_info = {
	    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	    .pNext = NULL,
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores = &cts->base.semaphores.render_complete,
	    .swapchainCount = 1,
	    .pSwapchains = &cts->swapchain.handle,
	    .pImageIndices = &index,
	};

#ifdef VK_GOOGLE_display_timing
	VkPresentTimeGOOGLE times = {
	    .presentID = (uint32_t)cts->current_frame_id,
	    .desiredPresentTime = desired_present_time_ns - present_slop_ns,
	};

	VkPresentTimesInfoGOOGLE timings = {
	    .sType = VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE,
	    .swapchainCount = 1,
	    .pTimes = &times,
	};

	if (vk->has_GOOGLE_display_timing) {
		vk_append_to_pnext_chain((VkBaseInStructure *)&present_info, (VkBaseInStructure *)&timings);
	}
#endif

#if defined(VK_KHR_present_id) || defined(VK_KHR_present_id2)
	uint64_t present_id = (uint64_t)cts->current_frame_id;

#ifdef VK_KHR_present_id2
	VkPresentId2KHR vk_present_id2 = {
	    .sType = VK_STRUCTURE_TYPE_PRESENT_ID_2_KHR,
	    .swapchainCount = 1,
	    .pPresentIds = &present_id,
	};
#endif

#ifdef VK_KHR_present_id
	VkPresentIdKHR vk_present_id = {
	    .sType = VK_STRUCTURE_TYPE_PRESENT_ID_KHR,
	    .swapchainCount = 1,
	    .pPresentIds = &present_id,
	};
#endif

#ifdef VK_KHR_present_id2
	if (cts->surface.present_id2_supported && cts->surface.present_wait2_supported) {
		vk_append_to_pnext_chain((VkBaseInStructure *)&present_info, (VkBaseInStructure *)&vk_present_id2);
	} else
#endif

	{
#ifdef VK_KHR_present_id
		if (vk->features.present_id && vk->features.present_wait) {
			vk_append_to_pnext_chain((VkBaseInStructure *)&present_info,
			                         (VkBaseInStructure *)&vk_present_id);
		}
#endif
	}
#endif


	// Need to take the queue lock for present.
	vk_queue_lock(present_queue);
	VkResult ret = vk->vkQueuePresentKHR(present_queue->queue, &present_info);
	vk_queue_unlock(present_queue);


#ifdef VK_EXT_display_control
	if (cts->vblank.thread_running) {
		os_thread_helper_lock(&cts->vblank.event_thread);
		if (!cts->vblank.should_wait) {
			cts->vblank.should_wait = true;
			os_thread_helper_signal_locked(&cts->vblank.event_thread);
		}
		os_thread_helper_unlock(&cts->vblank.event_thread);
	}
#endif

	return ret;
}

static VkResult
comp_target_swapchain_wait_for_present(struct comp_target *ct, time_duration_ns timeout_ns)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	struct vk_bundle *vk = get_vk(cts);

#ifdef VK_KHR_present_wait2
	if (cts->surface.present_wait2_supported) {
		VkPresentWait2InfoKHR info2 = {
		    .sType = VK_STRUCTURE_TYPE_PRESENT_WAIT_2_INFO_KHR,
		    .pNext = NULL,
		    .presentId = (uint64_t)cts->current_frame_id,
		    .timeout = timeout_ns,
		};

		return vk->vkWaitForPresent2KHR(vk->device, cts->swapchain.handle, &info2);
	}

#endif

#ifdef VK_KHR_present_wait
	if (!vk->features.present_wait) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	// @note current frame ID is incremented by 1 to match the ID given to Vulkan, see comp_target_swapchain_present
	return vk->vkWaitForPresentKHR(vk->device, cts->swapchain.handle, (uint64_t)cts->current_frame_id, timeout_ns);
#else
	return VK_ERROR_EXTENSION_NOT_PRESENT;
#endif
}

static bool
comp_target_swapchain_check_ready(struct comp_target *ct)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	return cts->surface.handle != VK_NULL_HANDLE;
}

static inline bool
comp_target_swapchain_is_shared_presentable_image(struct comp_target *ct)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	return is_shared_presentable_image(cts);
}


/*
 *
 * Timing member functions.
 *
 */

static void
comp_target_swapchain_calc_frame_pacing(struct comp_target *ct,
                                        int64_t *out_frame_id,
                                        int64_t *out_wake_up_time_ns,
                                        int64_t *out_desired_present_time_ns,
                                        int64_t *out_present_slop_ns,
                                        int64_t *out_predicted_display_time_ns)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;

	int64_t frame_id = -1;
	int64_t wake_up_time_ns = 0;
	int64_t desired_present_time_ns = 0;
	int64_t present_slop_ns = 0;
	int64_t predicted_display_time_ns = 0;
	int64_t predicted_display_period_ns = 0;
	int64_t min_display_period_ns = 0;
	int64_t now_ns = os_monotonic_get_ns();

	u_pc_predict(cts->upc,                     //
	             now_ns,                       //
	             &frame_id,                    //
	             &wake_up_time_ns,             //
	             &desired_present_time_ns,     //
	             &present_slop_ns,             //
	             &predicted_display_time_ns,   //
	             &predicted_display_period_ns, //
	             &min_display_period_ns);      //

	cts->current_frame_id = frame_id;

	*out_frame_id = frame_id;
	*out_wake_up_time_ns = wake_up_time_ns;
	*out_desired_present_time_ns = desired_present_time_ns;
	*out_predicted_display_time_ns = predicted_display_time_ns;
	*out_present_slop_ns = present_slop_ns;
}

static void
comp_target_swapchain_mark_timing_point(struct comp_target *ct,
                                        enum comp_target_timing_point point,
                                        int64_t frame_id,
                                        int64_t when_ns)
{
	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	assert(frame_id == cts->current_frame_id);

	switch (point) {
	case COMP_TARGET_TIMING_POINT_WAKE_UP:
		u_pc_mark_point(cts->upc, U_TIMING_POINT_WAKE_UP, cts->current_frame_id, when_ns);
		break;
	case COMP_TARGET_TIMING_POINT_BEGIN:
		u_pc_mark_point(cts->upc, U_TIMING_POINT_BEGIN, cts->current_frame_id, when_ns);
		break;
	case COMP_TARGET_TIMING_POINT_SUBMIT_BEGIN:
		u_pc_mark_point(cts->upc, U_TIMING_POINT_SUBMIT_BEGIN, cts->current_frame_id, when_ns);
		break;
	case COMP_TARGET_TIMING_POINT_SUBMIT_END:
		u_pc_mark_point(cts->upc, U_TIMING_POINT_SUBMIT_END, cts->current_frame_id, when_ns);
		break;
	default: assert(false);
	}
}

static VkResult
comp_target_swapchain_update_timings(struct comp_target *ct)
{
	COMP_TRACE_MARKER();

	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;

	do_update_timings_google_display_timing(cts);
	do_update_timings_vblank_thread(cts);

	return VK_SUCCESS;
}

static void
comp_target_swapchain_info_gpu(
    struct comp_target *ct, int64_t frame_id, int64_t gpu_start_ns, int64_t gpu_end_ns, int64_t when_ns)
{
	COMP_TRACE_MARKER();

	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;

	u_pc_info_gpu(cts->upc, frame_id, gpu_start_ns, gpu_end_ns, when_ns);
}


/*
 *
 * 'Exported' functions.
 *
 */

void
comp_target_swapchain_override_extents(struct comp_target_swapchain *cts, VkExtent2D extent)
{
	VkExtent2D old = cts->override.extent;

	COMP_INFO(                                                                  //
	    cts->base.c,                                                            //
	    "Target '%s' overrides compositor extents with (%ux%u) was (%ux%u %s)", //
	    cts->base.name,                                                         //
	    extent.width,                                                           //
	    extent.height,                                                          //
	    old.width,                                                              //
	    old.height,                                                             //
	    cts->override.compositor_extent ? "true" : "false");                    //

	cts->override.compositor_extent = true;
	cts->override.extent = extent;
}

void
comp_target_swapchain_cleanup(struct comp_target_swapchain *cts)
{
	struct vk_bundle *vk = get_vk(cts);

	// Thread if it has been started must be stopped first.
	if (cts->vblank.thread_running) {
		// Destroy also stops the thread.
		os_thread_helper_destroy(&cts->vblank.event_thread);
		cts->vblank.thread_running = false;
	}

	destroy_image_views(cts);

	if (cts->swapchain.handle != VK_NULL_HANDLE) {
		vk->vkDestroySwapchainKHR( //
		    vk->device,            // device
		    cts->swapchain.handle, // swapchain
		    NULL);                 //
		cts->swapchain.handle = VK_NULL_HANDLE;
	}

	if (cts->surface.handle != VK_NULL_HANDLE) {
		vk->vkDestroySurfaceKHR( //
		    vk->instance,        // instance
		    cts->surface.handle, // surface
		    NULL);               //
		cts->surface.handle = VK_NULL_HANDLE;
	}

	target_fini_semaphores(cts);

	u_pc_destroy(&cts->upc);
}

static VkResult
comp_target_swapchain_queue_supports_present(struct comp_target *ct,
                                             struct vk_bundle_queue *queue,
                                             VkBool32 *out_supported)
{
	COMP_TRACE_MARKER();

	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)ct;
	struct vk_bundle *vk = get_vk(cts);

	VkResult ret = vk->vkGetPhysicalDeviceSurfaceSupportKHR( //
	    vk->physical_device,                                 // physicalDevice
	    queue->family_index,                                 // queueFamilyIndex
	    cts->surface.handle,                                 // surface
	    out_supported);                                      // pSupported

	VK_CHK_AND_RET(ret, "vkGetPhysicalDeviceSurfaceSupportKHR");
	return VK_SUCCESS;
}

void
comp_target_swapchain_init_and_set_fnptrs(struct comp_target_swapchain *cts,
                                          enum comp_target_display_timing_usage timing_usage)
{
	cts->timing_usage = timing_usage;
	cts->base.check_ready = comp_target_swapchain_check_ready;
	cts->base.is_shared_presentable_image = comp_target_swapchain_is_shared_presentable_image;
	cts->base.create_images = comp_target_swapchain_create_images;
	cts->base.has_images = comp_target_swapchain_has_images;
	cts->base.acquire = comp_target_swapchain_acquire_next_image;
	cts->base.present = comp_target_swapchain_present;
	cts->base.wait_for_present = comp_target_swapchain_wait_for_present;
	cts->base.calc_frame_pacing = comp_target_swapchain_calc_frame_pacing;
	cts->base.mark_timing_point = comp_target_swapchain_mark_timing_point;
	cts->base.update_timings = comp_target_swapchain_update_timings;
	cts->base.info_gpu = comp_target_swapchain_info_gpu;
	cts->base.queue_supports_present = comp_target_swapchain_queue_supports_present;

	os_thread_helper_init(&cts->vblank.event_thread);
}
