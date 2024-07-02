// Copyright 2020-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Sample HMD device, use as a starting point to make your own device driver.
 *
 *
 * Based largely on simulated_hmd.c
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup drv_sample
 */

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

#include <stdio.h>


/*
 *
 * Structs and defines.
 *
 */

/*!
 * A sample HMD device.
 *
 * @implements xrt_device
 */
struct sample_hmd
{
	struct xrt_device base;

	struct xrt_pose pose;

	enum u_logging_level log_level;

	// has built-in mutex so thread safe
	struct m_relation_history *relation_hist;
};


/// Casting helper function
static inline struct sample_hmd *
sample_hmd(struct xrt_device *xdev)
{
	return (struct sample_hmd *)xdev;
}

DEBUG_GET_ONCE_LOG_OPTION(sample_log, "SAMPLE_LOG", U_LOGGING_WARN)

#define HMD_TRACE(hmd, ...) U_LOG_XDEV_IFL_T(&hmd->base, hmd->log_level, __VA_ARGS__)
#define HMD_DEBUG(hmd, ...) U_LOG_XDEV_IFL_D(&hmd->base, hmd->log_level, __VA_ARGS__)
#define HMD_INFO(hmd, ...) U_LOG_XDEV_IFL_I(&hmd->base, hmd->log_level, __VA_ARGS__)
#define HMD_ERROR(hmd, ...) U_LOG_XDEV_IFL_E(&hmd->base, hmd->log_level, __VA_ARGS__)

static void
sample_hmd_destroy(struct xrt_device *xdev)
{
	struct sample_hmd *hmd = sample_hmd(xdev);

	// Remove the variable tracking.
	u_var_remove_root(hmd);


	m_relation_history_destroy(&hmd->relation_hist);

	u_device_free(&hmd->base);
}

static xrt_result_t
sample_hmd_update_inputs(struct xrt_device *xdev)
{
	/*
	 * Empty for the sample driver, if you need to you should
	 * put code to update the attached inputs fields. If not you can use
	 * the u_device_noop_update_inputs helper to make it a no-op.
	 */
	return XRT_SUCCESS;
}

static void
sample_hmd_get_tracked_pose(struct xrt_device *xdev,
                            enum xrt_input_name name,
                            int64_t at_timestamp_ns,
                            struct xrt_space_relation *out_relation)
{
	struct sample_hmd *hmd = sample_hmd(xdev);

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

	*out_relation = relation;
}

static void
sample_hmd_get_view_poses(struct xrt_device *xdev,
                          const struct xrt_vec3 *default_eye_relation,
                          int64_t at_timestamp_ns,
                          uint32_t view_count,
                          struct xrt_space_relation *out_head_relation,
                          struct xrt_fov *out_fovs,
                          struct xrt_pose *out_poses)
{
	/*
	 * For HMDs you can call this function or directly set
	 * the `get_view_poses` function on the device to it.
	 */
	u_device_get_view_poses(  //
	    xdev,                 //
	    default_eye_relation, //
	    at_timestamp_ns,      //
	    view_count,           //
	    out_head_relation,    //
	    out_fovs,             //
	    out_poses);           //
}

xrt_result_t
sample_hmd_get_visibility_mask(struct xrt_device *xdev,
                               enum xrt_visibility_mask_type type,
                               uint32_t view_index,
                               struct xrt_visibility_mask **out_mask)
{
	struct xrt_fov fov = xdev->hmd->distortion.fov[view_index];
	u_visibility_mask_get_default(type, &fov, out_mask);
	return XRT_SUCCESS;
}

struct xrt_device *
sample_hmd_create(void)
{
	// This indicates you won't be using Monado's built-in tracking algorithms.
	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);

	struct sample_hmd *hmd = U_DEVICE_ALLOCATE(struct sample_hmd, flags, 1, 0);

	// This list should be ordered, most preferred first.
	size_t idx = 0;
	hmd->base.hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	hmd->base.hmd->blend_mode_count = idx;

	hmd->base.update_inputs = sample_hmd_update_inputs;
	hmd->base.get_tracked_pose = sample_hmd_get_tracked_pose;
	hmd->base.get_view_poses = sample_hmd_get_view_poses;
	hmd->base.get_visibility_mask = sample_hmd_get_visibility_mask;
	hmd->base.destroy = sample_hmd_destroy;

	// Distortion information, fills in xdev->compute_distortion().
	u_distortion_mesh_set_none(&hmd->base);

	// populate this with something more complex if required
	// hmd->base.compute_distortion = sample_hmd_compute_distortion;

	hmd->pose = (struct xrt_pose)XRT_POSE_IDENTITY;
	hmd->log_level = debug_get_log_option_sample_log();

	// Print name.
	snprintf(hmd->base.str, XRT_DEVICE_NAME_LEN, "Sample HMD");
	snprintf(hmd->base.serial, XRT_DEVICE_NAME_LEN, "Sample HMD S/N");

	m_relation_history_create(&hmd->relation_hist);

	// Setup input.
	hmd->base.name = XRT_DEVICE_GENERIC_HMD;
	hmd->base.device_type = XRT_DEVICE_TYPE_HMD;
	hmd->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	hmd->base.orientation_tracking_supported = true;
	hmd->base.position_tracking_supported = true;

	// Set up display details
	// refresh rate
	hmd->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 90.0f);

	const double hFOV = 90 * (M_PI / 180.0);
	const double vFOV = 96.73 * (M_PI / 180.0);
	// center of projection
	const double hCOP = 0.529;
	const double vCOP = 0.5;
	if (
	    /* right eye */
	    !math_compute_fovs(1, hCOP, hFOV, 1, vCOP, vFOV, &hmd->base.hmd->distortion.fov[1]) ||
	    /*
	     * left eye - same as right eye, except the horizontal center of projection is moved in the opposite
	     * direction now
	     */
	    !math_compute_fovs(1, 1.0 - hCOP, hFOV, 1, vCOP, vFOV, &hmd->base.hmd->distortion.fov[0])) {
		// If those failed, it means our math was impossible.
		HMD_ERROR(hmd, "Failed to setup basic device info");
		sample_hmd_destroy(&hmd->base);
		return NULL;
	}
	const int panel_w = 1080;
	const int panel_h = 1200;

	// Single "screen" (always the case)
	hmd->base.hmd->screens[0].w_pixels = panel_w * 2;
	hmd->base.hmd->screens[0].h_pixels = panel_h;

	// Left, Right
	for (uint8_t eye = 0; eye < 2; ++eye) {
		hmd->base.hmd->views[eye].display.w_pixels = panel_w;
		hmd->base.hmd->views[eye].display.h_pixels = panel_h;
		hmd->base.hmd->views[eye].viewport.y_pixels = 0;
		hmd->base.hmd->views[eye].viewport.w_pixels = panel_w;
		hmd->base.hmd->views[eye].viewport.h_pixels = panel_h;
		// if rotation is not identity, the dimensions can get more complex.
		hmd->base.hmd->views[eye].rot = u_device_rotation_ident;
	}
	// left eye starts at x=0, right eye starts at x=panel_width
	hmd->base.hmd->views[0].viewport.x_pixels = 0;
	hmd->base.hmd->views[1].viewport.x_pixels = panel_w;

	// Distortion information, fills in xdev->compute_distortion().
	u_distortion_mesh_set_none(&hmd->base);

	// Just put an initial identity value in the tracker
	struct xrt_space_relation identity = XRT_SPACE_RELATION_ZERO;
	identity.relation_flags = (enum xrt_space_relation_flags)(XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
	                                                          XRT_SPACE_RELATION_ORIENTATION_VALID_BIT);
	uint64_t now = os_monotonic_get_ns();
	m_relation_history_push(hmd->relation_hist, &identity, now);

	// Setup variable tracker: Optional but useful for debugging
	u_var_add_root(hmd, "Sample HMD", true);
	u_var_add_log_level(hmd, &hmd->log_level, "log_level");


	return &hmd->base;
}
