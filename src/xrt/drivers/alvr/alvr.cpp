// Copyright 2020-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  ALVR uwu
 *
 *
 * Based largely on simulated_hmd.c
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup drv_alvr
 */

#include "alvr_binding.h"
#include "os/os_time.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_device.h"

#include "math/m_relation_history.h"
#include "math/m_api.h"
#include "math/m_mathinclude.h" // IWYU pragma: keep

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_distortion_mesh.h"
#include "util/u_logging.h"
#include "util/u_misc.h"
#include "util/u_time.h"
#include "util/u_var.h"
#include "util/u_visibility_mask.h"
#include "xrt/xrt_results.h"

#include <array>
#include <stdio.h>
#include <mutex>

#include <EventManager.hpp>
#include <Encoder.hpp>
#include <utils.hpp>

/*!
 * An alvr HMD device.
 *
 * @implements xrt_device
 */
struct alvr_hmd
{
	struct xrt_device base;

	enum u_logging_level log_level;

	// has built-in mutex so thread safe
	struct m_relation_history *relation_hist;

	std::mutex viewMutex;
	std::array<xrt_pose, 2> viewPoses;
};


/// Casting helper function
static inline struct alvr_hmd *
alvr_hmd(struct xrt_device *xdev)
{
	return (struct alvr_hmd *)xdev;
}

DEBUG_GET_ONCE_LOG_OPTION(alvr_log, "ALVR_LOG", U_LOGGING_DEBUG)

#define HMD_TRACE(hmd, ...) U_LOG_XDEV_IFL_T(&hmd->base, hmd->log_level, __VA_ARGS__)
#define HMD_DEBUG(hmd, ...) U_LOG_XDEV_IFL_D(&hmd->base, hmd->log_level, __VA_ARGS__)
#define HMD_INFO(hmd, ...) U_LOG_XDEV_IFL_I(&hmd->base, hmd->log_level, __VA_ARGS__)
#define HMD_ERROR(hmd, ...) U_LOG_XDEV_IFL_E(&hmd->base, hmd->log_level, __VA_ARGS__)

static void
alvr_hmd_destroy(struct xrt_device *xdev)
{
	struct alvr_hmd *hmd = alvr_hmd(xdev);

	// Remove the variable tracking.
	u_var_remove_root(hmd);


	m_relation_history_destroy(&hmd->relation_hist);

	u_device_free(&hmd->base);
}

static void
alvr_hmd_update_inputs(struct xrt_device *xdev)
{
	/*
	 * Empty for the alvr driver, if you need to you should
	 * put code to update the attached inputs fields. If not you can use
	 * the u_device_noop_update_inputs helper to make it a no-op.
	 */
}

static void
alvr_hmd_get_tracked_pose(struct xrt_device *xdev,
                          enum xrt_input_name name,
                          int64_t at_timestamp_ns,
                          struct xrt_space_relation *out_relation)
{
	struct alvr_hmd *hmd = alvr_hmd(xdev);

	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		HMD_ERROR(hmd, "unknown input name");
		return;
	}

	struct xrt_space_relation relation = XRT_SPACE_RELATION_ZERO;

	enum m_relation_history_result history_result =
	    m_relation_history_get(hmd->relation_hist, at_timestamp_ns, &relation);
	if (history_result == M_RELATION_HISTORY_RESULT_INVALID) {
		// If you get in here, it means you did not push any poses into the relation history.
		// You may want to handle this differently.
		HMD_ERROR(hmd, "Internal error: no poses pushed?");
	}

	if ((relation.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0) {
		// If we provide an orientation, make sure that it is normalized.
		math_quat_normalize(&relation.pose.orientation);
	}

	// HMD_ERROR(hmd, "%f, %f, %f, %u", relation.pose.position.x, relation.pose.orientation.x,
	//           relation.pose.orientation.y, m_relation_history_get_size(hmd->relation_hist));

	*out_relation = relation;
}

static void
alvr_hmd_get_view_poses(struct xrt_device *xdev,
                        const struct xrt_vec3 *default_eye_relation,
                        int64_t at_timestamp_ns,
                        uint32_t view_count,
                        struct xrt_space_relation *out_head_relation,
                        struct xrt_fov *out_fovs,
                        struct xrt_pose *out_poses)
{
	auto& hmd = *alvr_hmd(xdev);

	xrt_device_get_tracked_pose(xdev, XRT_INPUT_GENERIC_HEAD_POSE, at_timestamp_ns, out_head_relation);

	std::lock_guard mutexGuard_(hmd.viewMutex);

	for (uint32_t i = 0; i < view_count && i < ARRAY_SIZE(xdev->hmd->views); i++) {
		out_fovs[i] = xdev->hmd->distortion.fov[i];
	}
	out_poses[0] = hmd.viewPoses[0];
	out_poses[1] = hmd.viewPoses[1];
}

xrt_result_t
alvr_hmd_get_visibility_mask(struct xrt_device *xdev,
                             enum xrt_visibility_mask_type type,
                             uint32_t view_index,
                             struct xrt_visibility_mask **out_mask)
{
	struct xrt_fov fov = xdev->hmd->distortion.fov[view_index];
	u_visibility_mask_get_default(type, &fov, out_mask);
	return XRT_SUCCESS;
}

xrt_vec3
xrt_vec3_from_alvr_vec3(float *avec)
{
	return xrt_vec3{
	    .x = avec[0],
	    .y = avec[1],
	    .z = avec[2],
	};
}

xrt_pose
xrt_pose_from_alvr_pose(AlvrPose pose)
{
	return xrt_pose{
	    .orientation{
	        .x = pose.orientation.x,
	        .y = pose.orientation.y,
	        .z = pose.orientation.z,
	        .w = pose.orientation.w,
	    },
	    .position = xrt_vec3_from_alvr_vec3(pose.position),
	};
}

xrt_fov
xrt_fov_from_alvr_fov(AlvrFov fov)
{
	return xrt_fov{
	    .angle_left = fov.left,
	    .angle_right = fov.right,
	    .angle_up = fov.up,
	    .angle_down = fov.down,
	};
}

xrt_space_relation
xrt_rel_from_alvr_rel(AlvrSpaceRelation arel)
{
	return xrt_space_relation{
	    .relation_flags = XRT_SPACE_RELATION_BITMASK_ALL,
	    .pose = xrt_pose_from_alvr_pose(arel.pose),
	    .linear_velocity = xrt_vec3_from_alvr_vec3(arel.linear_velocity),
	    .angular_velocity = xrt_vec3_from_alvr_vec3(arel.angular_velocity),
	};
}

extern "C" struct xrt_device *
alvr_hmd_create(void)
{
	// This indicates you won't be using Monado's built-in tracking algorithms.
	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);

	struct alvr_hmd *hmd = U_DEVICE_ALLOCATE(struct alvr_hmd, flags, 1, 0);

	// This list should be ordered, most preferred first.
	size_t idx = 0;
	hmd->base.hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	hmd->base.hmd->blend_mode_count = idx;

	hmd->base.update_inputs = alvr_hmd_update_inputs;
	hmd->base.get_tracked_pose = alvr_hmd_get_tracked_pose;
	hmd->base.get_view_poses = alvr_hmd_get_view_poses;
	hmd->base.get_visibility_mask = alvr_hmd_get_visibility_mask;
	hmd->base.destroy = alvr_hmd_destroy;

	// TODO: Could use for foveated encoding
	u_distortion_mesh_set_none(&hmd->base);

	hmd->log_level = debug_get_log_option_alvr_log();

	snprintf(hmd->base.str, XRT_DEVICE_NAME_LEN, "Alvr HMD");
	snprintf(hmd->base.serial, XRT_DEVICE_NAME_LEN, "Alvr HMD S/N");

	m_relation_history_create(&hmd->relation_hist);

	hmd->base.name = XRT_DEVICE_GENERIC_HMD;
	hmd->base.device_type = XRT_DEVICE_TYPE_HMD;
	hmd->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	hmd->base.orientation_tracking_supported = true;
	hmd->base.position_tracking_supported = true;

	// TODO: Get dynamically / why do we even care about this?
	hmd->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 90.0f);

	// TODO: Shouldn't this mabye be called later?
	auto streamExtent = ensureInit();
	auto streamWidth = streamExtent.width / 2;

	hmd->base.hmd->screens[0].w_pixels = streamExtent.width;
	hmd->base.hmd->screens[0].h_pixels = streamExtent.height;

	for (uint8_t eye = 0; eye < 2; ++eye) {
		hmd->base.hmd->views[eye].display.w_pixels = streamWidth;
		hmd->base.hmd->views[eye].display.h_pixels = streamExtent.height;
		hmd->base.hmd->views[eye].viewport.y_pixels = 0;
		hmd->base.hmd->views[eye].viewport.w_pixels = streamWidth;
		hmd->base.hmd->views[eye].viewport.h_pixels = streamExtent.height;

		// if rotation is not identity, the dimensions can get more complex.
		hmd->base.hmd->views[eye].rot = u_device_rotation_ident;
	}
	hmd->base.hmd->views[0].viewport.x_pixels = 0;
	hmd->base.hmd->views[1].viewport.x_pixels = streamWidth;

	u_distortion_mesh_set_none(&hmd->base);

	// Just put an initial identity value in the tracker
	struct xrt_space_relation identity = XRT_SPACE_RELATION_ZERO;
	identity.relation_flags = (enum xrt_space_relation_flags)(XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
	                                                          XRT_SPACE_RELATION_ORIENTATION_VALID_BIT);
	// uint64_t now = os_monotonic_get_ns();
	m_relation_history_push(hmd->relation_hist, &identity, 0);

	// Setup variable tracker: Optional but useful for debugging
	u_var_add_root(hmd, "ALVR HMD", true);
	u_var_add_log_level(hmd, &hmd->log_level, "log_level");

	auto tracking_cb = [hmd](u64 ts_ns, AlvrSpaceRelation hmd_rel) {
		auto xrel = xrt_rel_from_alvr_rel(hmd_rel);

		// HMD_ERROR(hmd, "got a tracking callback woo %f, %f, %f, %lu", xrel.pose.position.x,
		//           xrel.pose.orientation.x, xrel.linear_velocity.x, ts_ns);

		m_relation_history_push(hmd->relation_hist, &xrel, ts_ns);
	};
	CallbackManager::get().registerCb<ALVR_EVENT_TRACKING_UPDATED>(std::move(tracking_cb));

	auto viewCb = [hmd](ViewsConfig_Body cfg) {
		HMD_ERROR(hmd, "Got views config");

		std::lock_guard mutexGuard_(hmd->viewMutex);

		hmd->viewPoses[0] = xrt_pose_from_alvr_pose(cfg.local_view_transform[0]);
		hmd->viewPoses[1] = xrt_pose_from_alvr_pose(cfg.local_view_transform[1]);

		// TODO: If monado internally access it then it's UB (shouldn't really matter tho)
		auto &fovs = hmd->base.hmd->distortion.fov;
		fovs[0] = xrt_fov_from_alvr_fov(cfg.fov[0]);
		fovs[1] = xrt_fov_from_alvr_fov(cfg.fov[1]);
	};
	CallbackManager::get().registerCb<ALVR_EVENT_VIEWS_CONFIG>(std::move(viewCb));

	return &hmd->base;
}
