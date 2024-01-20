// Copyright 2023-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Visual-Intertial Tracking consumer helper.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @ingroup aux_tracking
 */

#include "xrt/xrt_config_os.h"
#include "tracking/t_vit_loader.h"
#include "util/u_logging.h"
#include "vit/vit_interface.h"

#include <stdlib.h>

#if defined(XRT_OS_LINUX) || defined(XRT_OS_ANDROID)
#include <dlfcn.h>
#endif

static inline void
vit_get_proc(void *handle, const char *name, void *proc_ptr)
{
#if defined(XRT_OS_LINUX) || defined(XRT_OS_ANDROID)
	void *proc = dlsym(handle, name);
	char *err = dlerror();
	if (err != NULL) {
		U_LOG_E("Failed to load symbol %s", err);
		return;
	}

	*(void **)proc_ptr = proc;
#else
#error "Unknown platform"
#endif
}

bool
t_vit_bundle_load(struct t_vit_bundle *vit, const char *path)
{
#if defined(XRT_OS_LINUX) || defined(XRT_OS_ANDROID)
	vit->handle = dlopen(path, RTLD_LAZY);
	if (vit->handle == NULL) {
		U_LOG_E("Failed to open VIT library: %s", dlerror());
		return false;
	}
#else
#error "Unknown platform"
#endif

#define GET_PROC(SYM)                                                                                                  \
	do {                                                                                                           \
		vit_get_proc(vit->handle, "vit_" #SYM, &vit->SYM);                                                     \
	} while (0)

	// Get the version first.
	GET_PROC(api_get_version);
	vit->api_get_version(&vit->version.major, &vit->version.minor, &vit->version.patch);

	// Check major version.
	if (vit->version.major != VIT_HEADER_VERSION_MAJOR) {
		U_LOG_E("Incompatible versions: expecting %u.%u.%u but got %u.%u.%u",                 //
		        VIT_HEADER_VERSION_MAJOR, VIT_HEADER_VERSION_MINOR, VIT_HEADER_VERSION_PATCH, //
		        vit->version.major, vit->version.minor, vit->version.patch);
		dlclose(vit->handle);
		return false;
	}

	GET_PROC(tracker_create);
	GET_PROC(tracker_destroy);
	GET_PROC(tracker_has_image_format);
	GET_PROC(tracker_get_capabilities);
	GET_PROC(tracker_get_pose_capabilities);
	GET_PROC(tracker_set_pose_capabilities);
	GET_PROC(tracker_start);
	GET_PROC(tracker_stop);
	GET_PROC(tracker_reset);
	GET_PROC(tracker_is_running);
	GET_PROC(tracker_push_imu_sample);
	GET_PROC(tracker_push_img_sample);
	GET_PROC(tracker_add_imu_calibration);
	GET_PROC(tracker_add_camera_calibration);
	GET_PROC(tracker_pop_pose);
	GET_PROC(tracker_get_timing_titles);
	GET_PROC(pose_destroy);
	GET_PROC(pose_get_data);
	GET_PROC(pose_get_timing);
	GET_PROC(pose_get_features);
#undef GET_PROC

	return true;
}

void
t_vit_bundle_unload(struct t_vit_bundle *vit)
{
#if defined(XRT_OS_LINUX) || defined(XRT_OS_ANDROID)
	dlclose(vit->handle);
#else
#error "Unknown platform"
#endif
}
