// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client HMD device.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */


#include "xrt/xrt_device.h"

#include "os/os_time.h"

#include "math/m_api.h"

#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_distortion_mesh.h"

#include "client/ipc_client.h"
#include "ipc_client_generated.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*
 *
 * Structs and defines.
 *
 */

/*!
 * An IPC client proxy for an HMD @ref xrt_device and @ref ipc_client_xdev.
 * Using a typedef reduce impact of refactor change.
 *
 * @implements ipc_client_xdev
 * @ingroup ipc_client
 */
typedef struct ipc_client_xdev ipc_client_hmd_t;


/*
 *
 * Functions
 *
 */

static inline ipc_client_hmd_t *
ipc_client_hmd(struct xrt_device *xdev)
{
	return (ipc_client_hmd_t *)xdev;
}

static void
ipc_client_hmd_destroy(struct xrt_device *xdev)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);

	// Remove the variable tracking.
	u_var_remove_root(ich);

	// We do not own these, so don't free them.
	ich->base.inputs = NULL;
	ich->base.outputs = NULL;

	// Free this device with the helper.
	u_device_free(&ich->base);
}

static void
ipc_client_hmd_update_inputs(struct xrt_device *xdev)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);

	xrt_result_t r = ipc_call_device_update_input(ich->ipc_c, ich->device_id);
	if (r != XRT_SUCCESS) {
		IPC_ERROR(ich->ipc_c, "Error calling input update!");
	}
}

static void
ipc_client_hmd_get_tracked_pose(struct xrt_device *xdev,
                                enum xrt_input_name name,
                                uint64_t at_timestamp_ns,
                                struct xrt_space_relation *out_relation)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);

	xrt_result_t r =
	    ipc_call_device_get_tracked_pose(ich->ipc_c, ich->device_id, name, at_timestamp_ns, out_relation);
	if (r != XRT_SUCCESS) {
		IPC_ERROR(ich->ipc_c, "Error calling tracked pose!");
	}
}

static void
ipc_client_hmd_get_view_poses(struct xrt_device *xdev,
                              const struct xrt_vec3 *default_eye_relation,
                              uint64_t at_timestamp_ns,
                              uint32_t view_count,
                              struct xrt_space_relation *out_head_relation,
                              struct xrt_fov *out_fovs,
                              struct xrt_pose *out_poses)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);

	struct ipc_info_get_view_poses_2 info = {0};

	if (view_count == 2) {
		xrt_result_t r = ipc_call_device_get_view_poses_2( //
		    ich->ipc_c,                                    //
		    ich->device_id,                                //
		    default_eye_relation,                          //
		    at_timestamp_ns,                               //
		    &info);                                        //
		if (r != XRT_SUCCESS) {
			IPC_ERROR(ich->ipc_c, "Error calling view poses!");
		}

		*out_head_relation = info.head_relation;
		for (int i = 0; i < 2; i++) {
			out_fovs[i] = info.fovs[i];
			out_poses[i] = info.poses[i];
		}
	} else {
		IPC_ERROR(ich->ipc_c, "Cannot handle %u view_count, only 2 supported.", view_count);
		assert(false && !"Can only handle view_count of 2.");
	}
}

static bool
ipc_client_hmd_compute_distortion(
    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);

	bool ret;
	xrt_result_t xret = ipc_call_device_compute_distortion( //
	    ich->ipc_c,                                         //
	    ich->device_id,                                     //
	    view,                                               //
	    u,                                                  //
	    v,                                                  //
	    &ret,                                               //
	    out_result);                                        //
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ich->ipc_c, "Error calling compute distortion!");
		return false;
	}

	return ret;
}

static bool
ipc_client_hmd_is_form_factor_available(struct xrt_device *xdev, enum xrt_form_factor form_factor)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);
	bool available = false;
	xrt_result_t r = ipc_call_device_is_form_factor_available(ich->ipc_c, ich->device_id, form_factor, &available);
	if (r != XRT_SUCCESS) {
		IPC_ERROR(ich->ipc_c, "Error calling is available!");
	}
	return available;
}

/*!
 * @public @memberof ipc_client_hmd
 */
struct xrt_device *
ipc_client_hmd_create(struct ipc_connection *ipc_c, struct xrt_tracking_origin *xtrack, uint32_t device_id)
{
	struct ipc_shared_memory *ism = ipc_c->ism;
	struct ipc_shared_device *isdev = &ism->isdevs[device_id];



	enum u_device_alloc_flags flags = (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD);
	ipc_client_hmd_t *ich = U_DEVICE_ALLOCATE(ipc_client_hmd_t, flags, 0, 0);
	ich->ipc_c = ipc_c;
	ich->device_id = device_id;
	ich->base.update_inputs = ipc_client_hmd_update_inputs;
	ich->base.get_tracked_pose = ipc_client_hmd_get_tracked_pose;
	ich->base.get_view_poses = ipc_client_hmd_get_view_poses;
	ich->base.compute_distortion = ipc_client_hmd_compute_distortion;
	ich->base.destroy = ipc_client_hmd_destroy;
	ich->base.is_form_factor_available = ipc_client_hmd_is_form_factor_available;

	// Start copying the information from the isdev.
	ich->base.tracking_origin = xtrack;
	ich->base.name = isdev->name;
	ich->device_id = device_id;

	// Print name.
	snprintf(ich->base.str, XRT_DEVICE_NAME_LEN, "%s", isdev->str);
	snprintf(ich->base.serial, XRT_DEVICE_NAME_LEN, "%s", isdev->serial);

	// Setup inputs, by pointing directly to the shared memory.
	assert(isdev->input_count > 0);
	ich->base.inputs = &ism->inputs[isdev->first_input_index];
	ich->base.input_count = isdev->input_count;

#if 0
	// Setup info.
	struct u_device_simple_info info;
	info.display.w_pixels = 1920;
	info.display.h_pixels = 1080;
	info.display.w_meters = 0.13f;
	info.display.h_meters = 0.07f;
	info.lens_horizontal_separation_meters = 0.13f / 2.0f;
	info.lens_vertical_position_meters = 0.07f / 2.0f;
	info.views[0].fov = 85.0f * (M_PI / 180.0f);
	info.views[1].fov = 85.0f * (M_PI / 180.0f);

	if (!u_device_setup_split_side_by_side(&ich->base, &info)) {
		IPC_ERROR(ich->ipc_c, "Failed to setup basic device info");
		ipc_client_hmd_destroy(&ich->base);
		return NULL;
	}
#endif
	for (int i = 0; i < XRT_MAX_DEVICE_BLEND_MODES; i++) {
		ich->base.hmd->blend_modes[i] = ipc_c->ism->hmd.blend_modes[i];
	}
	ich->base.hmd->blend_mode_count = ipc_c->ism->hmd.blend_mode_count;

	ich->base.hmd->views[0].display.w_pixels = ipc_c->ism->hmd.views[0].display.w_pixels;
	ich->base.hmd->views[0].display.h_pixels = ipc_c->ism->hmd.views[0].display.h_pixels;
	ich->base.hmd->views[1].display.w_pixels = ipc_c->ism->hmd.views[1].display.w_pixels;
	ich->base.hmd->views[1].display.h_pixels = ipc_c->ism->hmd.views[1].display.h_pixels;

	// Distortion information, fills in xdev->compute_distortion().
	u_distortion_mesh_set_none(&ich->base);

	// Setup variable tracker.
	u_var_add_root(ich, ich->base.str, true);
	u_var_add_ro_u32(ich, &ich->device_id, "device_id");

	ich->base.orientation_tracking_supported = isdev->orientation_tracking_supported;
	ich->base.position_tracking_supported = isdev->position_tracking_supported;
	ich->base.device_type = isdev->device_type;
	ich->base.hand_tracking_supported = isdev->hand_tracking_supported;
	ich->base.eye_gaze_supported = isdev->eye_gaze_supported;
	ich->base.force_feedback_supported = isdev->force_feedback_supported;
	ich->base.form_factor_check_supported = isdev->form_factor_check_supported;

	return &ich->base;
}
