#include <thread>

extern "C" {
#include "main/comp_compositor.h"
#include "main/comp_target.h"
}

#include <Encoder.hpp>
#include <monado_interface.h>

// TODO: Deal with the exceptions?

struct comp_target_alvr
{
	comp_target base;

	comp_target_image imgs[3];

	Optional<alvr::Encoder> enc;

	uint32_t curimg;
};

// TODO: Make connection async? (aka let it continue to composit even if connection lost?)
bool
alvr_target_init_pre_vulkan(comp_target *ct)
{
	ensureInit();

	ct->semaphores.render_complete_is_timeline = true;

	return true;
}

vk_bundle *
get_vk(comp_target *ct)
{
	return &ct->c->base.vk;
}

bool
alvr_target_init_post_vulkan(comp_target *ct, uint32_t pref_w, uint32_t pref_h)
{
	auto &base = *(comp_target_alvr *)ct;

	// preferred dimensions will depend on driver which is controlled by alvr too, so they will just be ignored
	// here, esp cuz limiting to the max the encoder supports is done in comp

	// TODO: Get actual dimensions from encoder (hardware limitations)
	ct->width = pref_w;
	ct->height = pref_h;

	vk_bundle *vk = get_vk(ct);

	auto lock_mutex = [](MutexProxy *m) { os_mutex_lock(reinterpret_cast<os_mutex *>(m)); };

	auto unlock_mutex = [](MutexProxy *m) { os_mutex_unlock(reinterpret_cast<os_mutex *>(m)); };

	AlvrVkInfo info = {.instance = vk->instance,
	                   .version = vk->version,
	                   .physDev = vk->physical_device,
	                   .phyDevIdx = static_cast<u32>(vk->physical_device_index),
	                   .device = vk->device,
	                   .queueFamIdx = vk->queue_family_index,
	                   .queueIdx = vk->queue_index,
	                   .queue = vk->queue,
	                   .mutex{.lock = lock_mutex,
	                          .unlock = unlock_mutex,
	                          .mutex = MutexProxy{
	                              .mutex = reinterpret_cast<void *>(&vk->queue_mutex),
	                          }}};

	base.enc.emplace(info);

	std::this_thread::sleep_for(std::chrono::seconds(2));

	// TODO: Encoder init here?
	return true;
}

bool
alvr_target_check_ready(comp_target *ct)
{
	// TODO: At least act like we're checking anything?

	// auto& base = *(comp_target_alvr *)ct;
	// return base.enc.hasValue();
	return true;
}


void
alvr_target_create_images(comp_target *ct, const comp_target_create_images_info *create_info)
{
	vk_bundle *vk = get_vk(ct);
	auto &base = *(comp_target_alvr *)ct;

	ImageRequirements imgReqs = {
	    .image_usage = create_info->image_usage,
	    .format_count = create_info->format_count,
	    .extent = create_info->extent,
	};
	for (uint32_t i = 0; i < create_info->format_count; ++i) {
		imgReqs.formats[i] = create_info->formats[i];
	}

	auto expt = base.enc.get().createImages(imgReqs);
	base.enc.get().initEncoding();

	ct->semaphores.render_complete_is_timeline = true;
	ct->semaphores.render_complete = expt.sem;


	// TODO: This
	ct->format = VK_FORMAT_R8G8B8A8_UNORM;

	for (int i = 0; i < 3; ++i) {
		base.imgs[i].handle = expt.imgs[i].img;
		base.imgs[i].view = expt.imgs[i].view;
	}

	ct->images = base.imgs;
	ct->image_count = ALVR_SWAPCHAIN_IMGS;

	base.curimg = 0;
}

bool
alvr_target_has_images(comp_target *ct)
{
	// fine because of exceptions
	return true;
}

VkResult
alvr_target_acquire(comp_target *ct, uint32_t *out_index)
{
	auto &base = *(comp_target_alvr *)ct;
	// TODO: Unfuck

	*out_index = base.curimg++;
	base.curimg %= ALVR_SWAPCHAIN_IMGS;

	return VK_SUCCESS;
}

VkResult
alvr_target_present(comp_target *ct,
                    VkQueue queue,
                    uint32_t img_idx,
                    uint64_t timeline_semaphore_value,
                    uint64_t desired_present_time_ns,
                    uint64_t preset_slop_ns)
{
	auto &base = *(comp_target_alvr *)ct;

	printf("present\n");

	base.enc.get().present(img_idx, timeline_semaphore_value);

	// TODO: Figure out whether we need a frame count
	return VK_SUCCESS;
}

void
alvr_target_set_title(comp_target *ct, const char *title)
{}

VkResult
alvr_target_update_timings(comp_target *ct)
{
	return VK_SUCCESS;
}


void
alvr_target_calc_frame_pacing(comp_target *ct,
                              int64_t *out_frame_id,
                              uint64_t *out_wake_up,
                              uint64_t *out_desired_present,
                              uint64_t *out_present_slop,
                              uint64_t *out_predicted_display)
{
	// TODO: Obvs lmao
	static int64_t frame = 0;

	*out_frame_id = ++frame;
	*out_wake_up = os_monotonic_get_ns();
	*out_desired_present = *out_wake_up + 5;
	*out_present_slop = 0;
	*out_predicted_display = *out_wake_up + 5;
}

void
alvr_target_mark_timing_point(comp_target *ct, enum comp_target_timing_point tp, int64_t t, uint64_t t2)
{}

void
alvr_target_flush_wsi(comp_target *ct)
{}

void
alvr_target_info_gpu(comp_target *ct, int64_t frame_id, uint64_t gpu_start_ns, uint64_t gpu_end_ns, uint64_t when_ns)
{}

bool
create_target_lel(const comp_target_factory *factory, struct comp_compositor *compositor, comp_target **target)
{
	auto t = new comp_target_alvr{.base = {
	                                  .c = compositor,
	                                  .name = "Alvr",
	                                  .init_pre_vulkan = alvr_target_init_pre_vulkan,
	                                  .init_post_vulkan = alvr_target_init_post_vulkan,
	                                  .check_ready = alvr_target_check_ready,
	                                  .create_images = alvr_target_create_images,
	                                  .has_images = alvr_target_has_images,
	                                  .acquire = alvr_target_acquire,
	                                  .present = alvr_target_present,
	                                  .flush = alvr_target_flush_wsi,
	                                  .calc_frame_pacing = alvr_target_calc_frame_pacing,
	                                  .mark_timing_point = alvr_target_mark_timing_point,
	                                  .update_timings = alvr_target_update_timings,
	                                  .info_gpu = alvr_target_info_gpu,
	                                  .set_title = alvr_target_set_title,
	                              }};


	*target = &t->base;

	return true;
}

extern "C" comp_target_factory
alvr_create_target_factory()
{
	const char *device_extensions[] = {
	    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,      VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
	    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME, VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
	    VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,   VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME,
	    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,         VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
	    VK_EXT_PHYSICAL_DEVICE_DRM_EXTENSION_NAME,     VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
	};

	return comp_target_factory{
	    .name = "Alvr",
	    .is_deferred = false,
	    .required_instance_version = VK_MAKE_VERSION(1, 3, 0),
	    .optional_device_extensions = device_extensions,
	    .optional_device_extension_count = 10,
	    .create_target = create_target_lel,
	};
}
