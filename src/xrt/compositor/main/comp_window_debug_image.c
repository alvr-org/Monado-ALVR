// Copyright 2019-2026, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Simple debug image based target.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup comp_main
 */

#include "util/u_misc.h"
#include "util/u_pacing.h"
#include "util/u_pretty_print.h"

#include "main/comp_window.h"


/*
 *
 * Structs and defines.
 *
 */

struct debug_image_target
{
	//! Base "class", so that we are a target the compositor can use.
	struct comp_target base;

	//! For error checking.
	int64_t index;

	//! Used to create the Vulkan resources, also manages index.
	struct comp_scratch_single_images target;

	/*!
	 * Storage for 'exported' images, these are pointed at by
	 * comt_target::images pointer in the @p base struct.
	 */
	struct comp_target_image images[COMP_SCRATCH_NUM_IMAGES];

	//! Compositor frame pacing helper.
	struct u_pacing_compositor *upc;

	// So we know we can free Vulkan resources safely.
	bool has_init_vulkan;
};


/*
 *
 * Target members.
 *
 */

static bool
target_init_pre_vulkan(struct comp_target *ct)
{
	return true; // No-op
}

static bool
target_init_post_vulkan(struct comp_target *ct, uint32_t preferred_width, uint32_t preferred_height)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;

	// We now know Vulkan is running and we can use it.
	dit->has_init_vulkan = true;

	return true;
}

static bool
target_check_ready(struct comp_target *ct)
{
	return true; // Always ready.
}

static void
target_create_images(struct comp_target *ct,
                     const struct comp_target_create_images_info *create_info,
                     struct vk_bundle_queue *present_queue)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;
	struct vk_bundle *vk = &dit->base.c->base.vk;
	bool use_unorm = false, use_srgb = false, maybe_convert = false;

	// Debug image target doesn't use the present_queue parameter, but it must not be NULL
	assert(present_queue != NULL);
	(void)present_queue;

	// Paranoia.
	assert(dit->has_init_vulkan);

	/*
	 * Find the format we should use, since we are using the scratch images
	 * to allocate the images we only support the two formats it uses
	 * (listed below). We search for those breaking as soon as we find those
	 * and setting if the compositor wanted SRGB or UNORM. But we also look
	 * for two other commonly used formats, but continue searching for the
	 * other true formats.
	 *
	 * The format used by the scratch image is:
	 * - VK_FORMAT_R8G8B8A8_SRGB
	 * - VK_FORMAT_R8G8B8A8_UNORM
	 *
	 * The other formats used to determine SRGB vs UNORM:
	 * - VK_FORMAT_B8G8R8A8_SRGB
	 * - VK_FORMAT_B8G8R8A8_UNORM
	 */
	for (uint32_t i = 0; i < create_info->format_count; i++) {
		VkFormat format = create_info->formats[i];

		// Used to figure out if we want SRGB or UNORM only.
		if (!maybe_convert && format == VK_FORMAT_B8G8R8A8_UNORM) {
			use_unorm = true;
			maybe_convert = true;
			continue; // Keep going, we might get better formats.
		}
		if (!maybe_convert && format == VK_FORMAT_B8G8R8A8_SRGB) {
			use_srgb = true;
			maybe_convert = true;
			continue; // Keep going, we might get better formats.
		}

		// These two are what the scratch image allocates.
		if (format == VK_FORMAT_R8G8B8A8_UNORM) {
			use_unorm = true;
			maybe_convert = false;
			break; // Best match, stop searching.
		}
		if (format == VK_FORMAT_R8G8B8A8_SRGB) {
			use_srgb = true;
			maybe_convert = false;
			break; // Best match, stop searching.
		}
	}

	// Check
	assert(use_unorm || use_srgb);
	if (maybe_convert) {
		COMP_WARN(ct->c, "Ignoring the format and picking something we use.");
	}

	// Do the allocation.
	comp_scratch_single_images_ensure_mutable(&dit->target, vk, create_info->extent);

	// Share the Vulkan handles of images and image views.
	for (uint32_t i = 0; i < COMP_SCRATCH_NUM_IMAGES; i++) {
		dit->images[i].handle = dit->target.images[i].image;
		if (use_unorm) {
			dit->images[i].view = dit->target.images[i].unorm_view;
		}
		if (use_srgb) {
			dit->images[i].view = dit->target.images[i].srgb_view;
		}
	}

	// Fill in exported data.
	dit->base.image_count = COMP_SCRATCH_NUM_IMAGES;
	dit->base.images = &dit->images[0];
	dit->base.width = create_info->extent.width;
	dit->base.height = create_info->extent.height;
	if (use_unorm) {
		dit->base.format = VK_FORMAT_R8G8B8A8_UNORM;
	}
	if (use_srgb) {
		dit->base.format = VK_FORMAT_R8G8B8A8_SRGB;
	}
	dit->base.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

static bool
target_has_images(struct comp_target *ct)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;

	// Simple check.
	return dit->base.images != NULL;
}

static VkResult
target_acquire(struct comp_target *ct, uint32_t *out_index)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;

	// Error checking.
	assert(dit->index == -1);

	uint32_t index = 0;
	comp_scratch_single_images_get(&dit->target, &index);

	// For error checking.
	dit->index = index;

	// Return the variable.
	*out_index = index;

	return VK_SUCCESS;
}

static VkResult
target_present(struct comp_target *ct,
               struct vk_bundle_queue *present_queue,
               uint32_t index,
               uint64_t timeline_semaphore_value,
               int64_t desired_present_time_ns,
               int64_t present_slop_ns)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;

	// Debug image target doesn't use the present_queue parameter, but it must not be NULL
	assert(present_queue != NULL);
	(void)present_queue;

	assert(index == dit->index);

	comp_scratch_single_images_done(&dit->target);

	// For error checking.
	dit->index = -1;

	return VK_SUCCESS;
}

static VkResult
target_wait_for_present(struct comp_target *ct, time_duration_ns timeout_ns)
{
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void
target_flush(struct comp_target *ct)
{
	// No-op
}

static void
target_calc_frame_pacing(struct comp_target *ct,
                         int64_t *out_frame_id,
                         int64_t *out_wake_up_time_ns,
                         int64_t *out_desired_present_time_ns,
                         int64_t *out_present_slop_ns,
                         int64_t *out_predicted_display_time_ns)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;

	int64_t frame_id = -1;
	int64_t wake_up_time_ns = 0;
	int64_t desired_present_time_ns = 0;
	int64_t present_slop_ns = 0;
	int64_t predicted_display_time_ns = 0;
	int64_t predicted_display_period_ns = 0;
	int64_t min_display_period_ns = 0;
	int64_t now_ns = os_monotonic_get_ns();

	u_pc_predict(dit->upc,                     //
	             now_ns,                       //
	             &frame_id,                    //
	             &wake_up_time_ns,             //
	             &desired_present_time_ns,     //
	             &present_slop_ns,             //
	             &predicted_display_time_ns,   //
	             &predicted_display_period_ns, //
	             &min_display_period_ns);      //

	*out_frame_id = frame_id;
	*out_wake_up_time_ns = wake_up_time_ns;
	*out_desired_present_time_ns = desired_present_time_ns;
	*out_predicted_display_time_ns = predicted_display_time_ns;
	*out_present_slop_ns = present_slop_ns;
}

static void
target_mark_timing_point(struct comp_target *ct, enum comp_target_timing_point point, int64_t frame_id, int64_t when_ns)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;

	switch (point) {
	case COMP_TARGET_TIMING_POINT_WAKE_UP:
		u_pc_mark_point(dit->upc, U_TIMING_POINT_WAKE_UP, frame_id, when_ns);
		break;
	case COMP_TARGET_TIMING_POINT_BEGIN: //
		u_pc_mark_point(dit->upc, U_TIMING_POINT_BEGIN, frame_id, when_ns);
		break;
	case COMP_TARGET_TIMING_POINT_SUBMIT_BEGIN:
		u_pc_mark_point(dit->upc, U_TIMING_POINT_SUBMIT_BEGIN, frame_id, when_ns);
		break;
	case COMP_TARGET_TIMING_POINT_SUBMIT_END:
		u_pc_mark_point(dit->upc, U_TIMING_POINT_SUBMIT_END, frame_id, when_ns);
		break;
	default: assert(false);
	}
}

static VkResult
target_update_timings(struct comp_target *ct)
{
	return VK_SUCCESS; // No-op
}

static void
target_info_gpu(struct comp_target *ct, int64_t frame_id, int64_t gpu_start_ns, int64_t gpu_end_ns, int64_t when_ns)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;

	u_pc_info_gpu(dit->upc, frame_id, gpu_start_ns, gpu_end_ns, when_ns);
}

static VkResult
target_queue_supports_present(struct comp_target *ct, struct vk_bundle_queue *queue, VkBool32 *out_supported)
{
	// Debug image target doesn't use real presentation, so all queues are "supported"
	(void)queue;
	*out_supported = VK_TRUE;
	return VK_SUCCESS;
}

static void
target_set_title(struct comp_target *ct, const char *title)
{
	// No-op
}

static bool
target_is_shared_presentable_image(struct comp_target *ct)
{
	return false;
}

static void
target_destroy(struct comp_target *ct)
{
	struct debug_image_target *dit = (struct debug_image_target *)ct;
	struct vk_bundle *vk = &dit->base.c->base.vk;

	// Do this first.
	u_var_remove_root(dit);

	// Can only allocate if we have Vulkan.
	if (dit->has_init_vulkan) {
		comp_scratch_single_images_free(&dit->target, vk);
		dit->has_init_vulkan = false;
		dit->base.image_count = 0;
		dit->base.images = NULL;
		dit->base.width = 0;
		dit->base.height = 0;
		dit->base.format = VK_FORMAT_UNDEFINED;
		dit->base.final_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	// Always free non-Vulkan resources.
	comp_scratch_single_images_destroy(&dit->target);

	// Pacing is always created.
	u_pc_destroy(&dit->upc);

	// Free memory.
	free(dit);
}

struct comp_target *
target_create(struct comp_compositor *c)
{
	struct debug_image_target *dit = U_TYPED_CALLOC(struct debug_image_target);

	dit->base.name = "debug_image";
	dit->base.init_pre_vulkan = target_init_pre_vulkan;
	dit->base.init_post_vulkan = target_init_post_vulkan;
	dit->base.check_ready = target_check_ready;
	dit->base.create_images = target_create_images;
	dit->base.has_images = target_has_images;
	dit->base.acquire = target_acquire;
	dit->base.present = target_present;
	dit->base.wait_for_present = target_wait_for_present;
	dit->base.flush = target_flush;
	dit->base.calc_frame_pacing = target_calc_frame_pacing;
	dit->base.mark_timing_point = target_mark_timing_point;
	dit->base.update_timings = target_update_timings;
	dit->base.info_gpu = target_info_gpu;
	dit->base.set_title = target_set_title;
	dit->base.queue_supports_present = target_queue_supports_present;
	dit->base.is_shared_presentable_image = target_is_shared_presentable_image;
	dit->base.destroy = target_destroy;
	dit->base.c = c;

	dit->base.wait_for_present_supported = false;

	// Create the pacer.
	uint64_t now_ns = os_monotonic_get_ns();
	u_pc_fake_create(c->settings.nominal_frame_interval_ns, now_ns, &dit->upc);

	// Only inits locking, Vulkan resources inited later.
	comp_scratch_single_images_init(&dit->target);

	// For error checking.
	dit->index = -1;

	// Variable tracking.
	u_var_add_root(dit, "Compositor output", true);
	u_var_add_native_images_debug(dit, &dit->target.unid, "Image");

	return &dit->base;
}


/*
 *
 * Factory
 *
 */

static bool
factory_detect(const struct comp_target_factory *ctf, struct comp_compositor *c)
{
	return false;
}

static bool
factory_create_target(const struct comp_target_factory *ctf, struct comp_compositor *c, struct comp_target **out_ct)
{
	struct comp_target *ct = target_create(c);
	if (ct == NULL) {
		return false;
	}

	COMP_INFO(c,
	          "\n################################################################################\n"
	          "#    Debug image target used, if you wanted to see something in your headset   #\n"
	          "#             something is probably wrong with your setup, sorry.              #\n"
	          "################################################################################");

	*out_ct = ct;

	return true;
}

const struct comp_target_factory comp_target_factory_debug_image = {
    .name = "Debug Image",
    .identifier = "debug_image",
    .requires_vulkan_for_create = false,
    .is_deferred = false,
    .required_instance_version = 0,
    .required_instance_extensions = NULL,
    .required_instance_extension_count = 0,
    .optional_device_extensions = NULL,
    .optional_device_extension_count = 0,
    .detect = factory_detect,
    .create_target = factory_create_target,
};
