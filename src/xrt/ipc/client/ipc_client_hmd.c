// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client HMD device.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
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
#include "client/ipc_client_xdev.h"
#include "client/ipc_client_connection.h"
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
 * Helpers.
 *
 */

static inline ipc_client_hmd_t *
ipc_client_hmd(struct xrt_device *xdev)
{
	return (ipc_client_hmd_t *)xdev;
}

static xrt_result_t
call_get_view_poses_raw(ipc_client_hmd_t *ich,
                        const struct xrt_vec3 *default_eye_relation,
                        int64_t at_timestamp_ns,
                        enum xrt_view_type view_type,
                        uint32_t view_count,
                        struct xrt_space_relation *out_head_relation,
                        struct xrt_fov *out_fovs,
                        struct xrt_pose *out_poses)
{
	struct ipc_connection *ipc_c = ich->ipc_c;
	xrt_result_t xret;

	ipc_client_connection_lock(ipc_c);

	// Using the raw send helper is the only one that is required.
	xret = ipc_send_device_get_view_poses_locked( //
	    ipc_c,                                    //
	    ich->device_id,                           //
	    default_eye_relation,                     //
	    at_timestamp_ns,                          //
	    view_type,                                //
	    view_count);                              //
	IPC_CHK_WITH_GOTO(ich->ipc_c, xret, "ipc_send_device_get_view_poses_locked", out);

	// This is the data we get back in the provided reply.
	uint32_t returned_view_count = 0;
	struct xrt_space_relation head_relation = XRT_SPACE_RELATION_ZERO;

	// Get the reply, use the raw function helper.
	xret = ipc_receive_device_get_view_poses_locked( //
	    ipc_c,                                       //
	    &head_relation,                              //
	    &returned_view_count);                       //
	IPC_CHK_WITH_GOTO(ich->ipc_c, xret, "ipc_receive_device_get_view_poses_locked", out);

	if (view_count != returned_view_count) {
		IPC_ERROR(ich->ipc_c, "Wrong view counts (sent: %u != got: %u)", view_count, returned_view_count);
		assert(false);
	}

	// We can read directly to the output variables.
	xret = ipc_receive(&ipc_c->imc, out_fovs, sizeof(struct xrt_fov) * view_count);
	IPC_CHK_WITH_GOTO(ich->ipc_c, xret, "ipc_receive(1)", out);

	// We can read directly to the output variables.
	xret = ipc_receive(&ipc_c->imc, out_poses, sizeof(struct xrt_pose) * view_count);
	IPC_CHK_WITH_GOTO(ich->ipc_c, xret, "ipc_receive(2)", out);

	/*
	 * Finally set the head_relation that we got in the reply, mostly to
	 * demonstrate that you can use the reply struct in such a way.
	 */
	*out_head_relation = head_relation;

out:
	ipc_client_connection_unlock(ipc_c);
	return xret;
}


/*
 *
 * Member functions
 *
 */

static xrt_result_t
ipc_client_hmd_get_view_poses(struct xrt_device *xdev,
                              const struct xrt_vec3 *default_eye_relation,
                              int64_t at_timestamp_ns,
                              enum xrt_view_type view_type,
                              uint32_t view_count,
                              struct xrt_space_relation *out_head_relation,
                              struct xrt_fov *out_fovs,
                              struct xrt_pose *out_poses)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);

	// Artificial limit.
	if (view_count == 0 || view_count > IPC_MAX_RAW_VIEWS) {
		IPC_ERROR(ich->ipc_c, "Cannot handle %u view_count, %u or less supported.", view_count,
		          (uint32_t)IPC_MAX_RAW_VIEWS);
		return XRT_ERROR_IPC_FAILURE;
	}

	// Fast path.
	if (view_count == 2) {
		struct ipc_info_get_view_poses_2 info = {0};
		xrt_result_t xret = ipc_call_device_get_view_poses_2( //
		    ich->ipc_c,                                       //
		    ich->device_id,                                   //
		    default_eye_relation,                             //
		    at_timestamp_ns,                                  //
		    view_type,                                        //
		    view_count,                                       //
		    &info);                                           //
		IPC_CHK_AND_RET(ich->ipc_c, xret, "ipc_call_device_get_view_poses_2");

		*out_head_relation = info.head_relation;
		for (int i = 0; i < 2; i++) {
			out_fovs[i] = info.fovs[i];
			out_poses[i] = info.poses[i];
		}

		return xret;
	}

	return call_get_view_poses_raw( //
	    ich,                        //
	    default_eye_relation,       //
	    at_timestamp_ns,            //
	    view_type,                  //
	    view_count,                 //
	    out_head_relation,          //
	    out_fovs,                   //
	    out_poses);                 //
}

static xrt_result_t
ipc_client_hmd_compute_distortion(
    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);
	xrt_result_t xret;

	xret = ipc_call_device_compute_distortion( //
	    ich->ipc_c,                            //
	    ich->device_id,                        //
	    view,                                  //
	    u,                                     //
	    v,                                     //
	    out_result);                           //

	IPC_CHK_ALWAYS_RET(ich->ipc_c, xret, "ipc_call_device_compute_distortion");
}

static xrt_result_t
ipc_client_hmd_is_form_factor_available(struct xrt_device *xdev, enum xrt_form_factor form_factor, bool *out_available)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);
	xrt_result_t xret;

	xret = ipc_call_device_is_form_factor_available( //
	    ich->ipc_c,                                  //
	    ich->device_id,                              //
	    form_factor,                                 //
	    out_available);                              //

	IPC_CHK_ALWAYS_RET(ich->ipc_c, xret, "ipc_call_device_is_form_factor_available");
}

static xrt_result_t
ipc_client_hmd_get_visibility_mask(struct xrt_device *xdev,
                                   enum xrt_visibility_mask_type type,
                                   uint32_t view_index,
                                   struct xrt_visibility_mask **out_mask)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);
	struct ipc_connection *ipc_c = ich->ipc_c;
	struct xrt_visibility_mask *mask = NULL;
	xrt_result_t xret;

	ipc_client_connection_lock(ipc_c);

	xret = ipc_send_device_get_visibility_mask_locked(ipc_c, ich->device_id, type, view_index);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_send_device_get_visibility_mask_locked", err_mask_unlock);

	uint32_t mask_size;
	xret = ipc_receive_device_get_visibility_mask_locked(ipc_c, &mask_size);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive_device_get_visibility_mask_locked", err_mask_unlock);

	if (mask_size < sizeof(struct xrt_visibility_mask)) {
		if (mask_size != 0) {
			uint8_t discard[sizeof(struct xrt_visibility_mask)];

			// The server sent these bytes. Read them before returning.
			xret = ipc_receive(&ipc_c->imc, discard, mask_size);
			IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive(discard visibility mask)", err_mask_unlock);
		}

		IPC_ERROR(ich->ipc_c, "visibility mask size %u is too small (need at least %zu)", mask_size,
		          sizeof(struct xrt_visibility_mask));
		// Error path. Something has clearly gone wrong if we end up here.
		// This function should not be called when the device does not support visibility masks.
		xret = XRT_ERROR_ALLOCATION;
		goto err_mask_unlock;
	}

	mask = U_CALLOC_WITH_CAST(struct xrt_visibility_mask, mask_size);
	if (mask == NULL) {
		// Okay to return without reading the payload. A failure to allocate should just crash the client.
		IPC_ERROR(ich->ipc_c, "failed to allocate xrt_visibility_mask");
		xret = XRT_ERROR_ALLOCATION;
		goto err_mask_unlock;
	}

	xret = ipc_receive(&ipc_c->imc, mask, mask_size);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive", err_mask_free);

	*out_mask = mask;
	ipc_client_connection_unlock(ipc_c);

	return XRT_SUCCESS;

err_mask_free:
	free(mask);
err_mask_unlock:
	ipc_client_connection_unlock(ipc_c);
	return xret;
}

static void
ipc_client_hmd_destroy(struct xrt_device *xdev)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);

	// Remove the variable tracking.
	u_var_remove_root(ich);

	// Free and de-init the shared things.
	ipc_client_xdev_fini(ich);

	// Free this device with the helper.
	u_device_free(&ich->base);
}

static xrt_result_t
ipc_client_hmd_get_brightness(struct xrt_device *xdev, float *out_brightness)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);
	struct ipc_connection *ipc_c = ich->ipc_c;
	xrt_result_t xret;

	ipc_client_connection_lock(ipc_c);

	xret = ipc_call_device_get_brightness(ipc_c, ich->device_id, out_brightness);
	IPC_CHK_ONLY_PRINT(ipc_c, xret, "ipc_call_device_get_brightness");

	ipc_client_connection_unlock(ipc_c);

	return xret;
}

static xrt_result_t
ipc_client_hmd_set_brightness(struct xrt_device *xdev, float brightness, bool relative)
{
	ipc_client_hmd_t *ich = ipc_client_hmd(xdev);
	struct ipc_connection *ipc_c = ich->ipc_c;
	xrt_result_t xret;

	ipc_client_connection_lock(ipc_c);

	xret = ipc_call_device_set_brightness(ipc_c, ich->device_id, brightness, relative);
	IPC_CHK_ONLY_PRINT(ipc_c, xret, "ipc_call_device_set_brightness");

	ipc_client_connection_unlock(ipc_c);

	return xret;
}

/*!
 * @public @memberof ipc_client_hmd
 */
struct xrt_device *
ipc_client_hmd_create(struct ipc_connection *ipc_c,
                      struct ipc_client_tracking_origin_manager *ictom,
                      uint32_t device_id)
{
	// Convenience helper.
	struct ipc_shared_memory *ism = ipc_c->ism;

	// Allocate a HMD device.
	enum u_device_alloc_flags flags = (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD);
	ipc_client_hmd_t *ich = U_DEVICE_ALLOCATE(ipc_client_hmd_t, flags, 0, 0);

	// Fills in almost everything a regular device needs.
	xrt_result_t xret = ipc_client_xdev_init(ich, ipc_c, ictom, device_id, ipc_client_hmd_destroy);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ipc_c, "Failed to initialize IPC client HMD: %d", xret);
		u_device_free(&ich->base);
		return NULL;
	}

	// Fill in needed HMD functions.
	ich->base.get_view_poses = ipc_client_hmd_get_view_poses;
	ich->base.compute_distortion = ipc_client_hmd_compute_distortion;
	ich->base.is_form_factor_available = ipc_client_hmd_is_form_factor_available;
	ich->base.get_visibility_mask = ipc_client_hmd_get_visibility_mask;
	ich->base.get_brightness = ipc_client_hmd_get_brightness;
	ich->base.set_brightness = ipc_client_hmd_set_brightness;

	// Setup blend-modes.
	ich->base.hmd->blend_mode_count = ipc_c->ism->hmd.blend_mode_count;
	for (int i = 0; i < XRT_MAX_DEVICE_BLEND_MODES; i++) {
		ich->base.hmd->blend_modes[i] = ipc_c->ism->hmd.blend_modes[i];
	}

	// Setup the views.
	ich->base.hmd->view_count = ism->hmd.view_count;
	for (uint32_t i = 0; i < ich->base.hmd->view_count; ++i) {
		ich->base.hmd->views[i].display.w_pixels = ipc_c->ism->hmd.views[i].display.w_pixels;
		ich->base.hmd->views[i].display.h_pixels = ipc_c->ism->hmd.views[i].display.h_pixels;
	}

	// Distortion information, fills in xdev->compute_distortion().
	u_distortion_mesh_set_none(&ich->base);

	// Setup variable tracker.
	u_var_add_root(ich, ich->base.str, true);
	u_var_add_ro_u32(ich, &ich->device_id, "device_id");

	return &ich->base;
}
