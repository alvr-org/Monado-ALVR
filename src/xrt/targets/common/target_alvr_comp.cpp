#include <thread>
#include <iostream>

extern "C" {
#include "main/comp_compositor.h"
#include "main/comp_target.h"
}

#include "EventManager.hpp"
#include <Encoder.hpp>
#include <monado_interface.h>
// TODO: We should probably create an api boundary here
#include <alvr_binding.h>

// TODO: Deal with the exceptions?

struct comp_target_alvr
{
	comp_target base;

	comp_target_image imgs[3];

	// TODO: Handling errors on construction becomes a lot harder with all the emplace shenanigans, solve that
	// somehow (could simply to a dynamic allocation fwiw)
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

comp_target_alvr &
get_acomp(comp_target *c)
{
	return *reinterpret_cast<comp_target_alvr *>(c);
}

vk_bundle &
get_vk(comp_target *ct)
{
	return ct->c->base.vk;
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

	vk_bundle &vk = get_vk(ct);

	auto lock_mutex = [](MutexProxy *m) { os_mutex_lock(reinterpret_cast<os_mutex *>(m->mutex)); };

	auto unlock_mutex = [](MutexProxy *m) { os_mutex_unlock(reinterpret_cast<os_mutex *>(m->mutex)); };

	AlvrVkInfo info = {
	    .instance = vk.instance,
	    .version = vk.version,

	    .physDev = vk.physical_device,
	    .phyDevIdx = static_cast<u32>(vk.physical_device_index),
	    .device = vk.device,

	    .queueFamIdx = vk.queue_family_index,
	    .queueIdx = vk.queue_index,
	    .queue = vk.queue,
	    .queueMutex{.lock = lock_mutex,
	                .unlock = unlock_mutex,
	                .mutex =
	                    MutexProxy{
	                        .mutex = reinterpret_cast<void *>(&vk.queue_mutex),
	                    }},

	    .encQueueFamily = vk.encode_queue_family_index,
	    .encQueue = vk.encode_queue,
	};

	base.enc.emplace(info);

	// TODO: This should obviously go
	std::this_thread::sleep_for(std::chrono::seconds(2));

	return true;
}

bool
alvr_target_check_ready(comp_target *ct)
{
	auto &acomp = get_acomp(ct);

	return acomp.enc.hasValue();
}


void
alvr_target_create_images(comp_target *ct, const comp_target_create_images_info *create_info)
{
	auto &acomp = get_acomp(ct);

	ImageRequirements imgReqs = {
	    .image_usage = create_info->image_usage,
	    .format_count = create_info->format_count,
	    .extent = create_info->extent,
	};
	for (uint32_t i = 0; i < create_info->format_count; ++i) {
		imgReqs.formats[i] = create_info->formats[i];
	}

	auto expt = acomp.enc.get().createImages(imgReqs);
	acomp.enc.get().initEncoding();

	ct->semaphores.render_complete_is_timeline = true;
	ct->semaphores.render_complete = expt.sem;

	// TODO: This
	ct->format = VK_FORMAT_R8G8B8A8_UNORM;

	for (int i = 0; i < 3; ++i) {
		acomp.imgs[i].handle = expt.imgs[i].img;
		acomp.imgs[i].view = expt.imgs[i].view;
	}

	ct->images = acomp.imgs;
	ct->image_count = ALVR_SWAPCHAIN_IMGS;

	acomp.curimg = 0;
}

bool
alvr_target_has_images(comp_target *ct)
{
	// TODO: Should be fine because of exceptions, but actually checking is a lot better
	return true;
}

VkResult
alvr_target_acquire(comp_target *ct, uint32_t *out_index)
{
	auto &acomp = get_acomp(ct);

	// TODO: Write actual pseudo-swapchain implementation (currently works because we ensure the job completed
	// before we start a new one, not given with sliced encoding)
	//
	// Also this logic is kinda weird, but it seems to work
	*out_index = acomp.curimg++;
	acomp.curimg %= ALVR_SWAPCHAIN_IMGS;

	return VK_SUCCESS;
}

// TODO: Extract all of these into some sorta helper and potentially c++-ify them
void
set_avec_from_xvec(float *avec, xrt_vec3 &xvec)
{
	avec[0] = xvec.x;
	avec[1] = xvec.y;
	avec[2] = xvec.z;
}

AlvrPose
apose_from_xpose(xrt_pose &xpose)
{
	auto &rot = xpose.orientation;

	AlvrPose apose{
	    .orientation{
	        .x = rot.x,
	        .y = rot.y,
	        .z = rot.z,
	        .w = rot.w,
	    },
	    .position = {},
	};
	set_avec_from_xvec(apose.position, xpose.position);

	return apose;
}

AlvrFov
afov_from_xfov(xrt_fov &xfov)
{
	return AlvrFov{
	    .left = xfov.angle_left,
	    .right = xfov.angle_right,
	    .up = xfov.angle_up,
	    .down = xfov.angle_down,
	};
}

VkResult
alvr_target_present(comp_target *ct,
                    VkQueue queue,
                    uint32_t img_idx,
                    uint64_t timeline_semaphore_value,
                    int64_t desired_present_time_ns,
                    int64_t preset_slop_ns)
{
	auto &base = *(comp_target_alvr *)ct;

	auto &frameParms = ct->c->base.frame_params;

	ViewsInfo viewInfo{
	    .left =
	        {
	            .pose = apose_from_xpose(frameParms.poses[0]),
	            .fov = afov_from_xfov(frameParms.fovs[0]),
	        },
	    .right =
	        {
	            .pose = apose_from_xpose(frameParms.poses[1]),
	            .fov = afov_from_xfov(frameParms.fovs[1]),
	        },
	};


	printf("present\n");

	base.enc.get().present(img_idx, timeline_semaphore_value, viewInfo);

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
                              int64_t *out_wake_up,
                              int64_t *out_desired_present,
                              int64_t *out_present_slop,
                              int64_t *out_predicted_display)
{
	// NOTE: I think it's deriving the point to query poses for from the display time
	// and how to schedule the rendering from the present time

	// TODO: Do we need the frame index for anything?
	static int64_t frame = 0;
	*out_frame_id = ++frame;

	// TODO: Improve handling
	u64 nextVsync = 0;
	// NOTE: That's the next vsync that will be performed, we want to aim for the one after it
	alvr_duration_until_next_vsync(&nextVsync);

	std::cout << "duration until next vsync: " << nextVsync << "\n";

	nextVsync += os_monotonic_get_ns();

	std::cout << "next vsync time: " << nextVsync << "\n";

	// timed for 120 fps
	*out_wake_up = nextVsync;
	*out_desired_present = nextVsync + 8;
	// graphics and encode are each allowed to take one full frametime since they are async
	*out_present_slop = 8;
	*out_predicted_display = nextVsync + 16;

	std::cout << "calculated wake up time: " << *out_wake_up << "\n";
}

void
alvr_target_mark_timing_point(comp_target *ct, enum comp_target_timing_point tp, int64_t t, int64_t t2)
{}

void
alvr_target_flush_wsi(comp_target *ct)
{}

void
alvr_target_info_gpu(comp_target *ct, int64_t frame_id, int64_t gpu_start_ns, int64_t gpu_end_ns, int64_t when_ns)
{}

bool
create_target_alvr(const comp_target_factory *factory, struct comp_compositor *compositor, comp_target **target)
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
	// TODO: Figure out how to handle missing ones
	//
	// Can't make this const because the factory api wants a mutable pointer
	static std::array device_extensions = {
	    // These are only needed for ffmpeg
	    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
	    VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
	    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
	    VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
	    VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
	    VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME,

	    // TODO: Clean these up
	    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
	    VK_EXT_PHYSICAL_DEVICE_DRM_EXTENSION_NAME,
	    VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,

	    // These are for vulkan encoding
	    VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
	    VK_KHR_VIDEO_MAINTENANCE_1_EXTENSION_NAME,
	    VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME,
	    VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME,
	    VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME,
	    VK_KHR_VIDEO_ENCODE_AV1_EXTENSION_NAME,

	};

	return comp_target_factory{
	    .name = "ALVR",
	    .identifier = "alvr",
	    .requires_vulkan_for_create = false,
	    .is_deferred = false,

	    .required_instance_version = VK_MAKE_VERSION(1, 3, 0),
	    .optional_device_extensions = device_extensions.data(),
	    .optional_device_extension_count = device_extensions.size(),

	    .create_target = create_target_alvr,
	};
}
