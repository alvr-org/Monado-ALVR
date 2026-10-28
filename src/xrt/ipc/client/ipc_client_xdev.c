// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Shared functions for IPC client @ref xrt_device.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
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

#include "client/ipc_client.h"
#include "client/ipc_client_connection.h"
#include "client/ipc_client_xdev.h"
#include "ipc_client_generated.h"


/*
 *
 * Functions from xrt_device.
 *
 */

static xrt_result_t
ipc_client_xdev_update_inputs(struct xrt_device *xdev)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);
	struct ipc_connection *ipc_c = icx->ipc_c;
	xrt_result_t xret;

	// Lock connection for varlen IPC
	ipc_client_connection_lock(ipc_c);

	// Send the request
	xret = ipc_send_device_update_input_locked(ipc_c, icx->device_id);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_send_device_update_input_locked", out_unlock);

	// Receive the reply (standard reply struct)
	struct ipc_result_reply reply = {0};
	xret = ipc_receive(&ipc_c->imc, &reply, sizeof(reply));
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive(reply)", out_unlock);

	// Check reply result
	xret = reply.result;
	IPC_CHK_WITH_GOTO(ipc_c, xret, "reply.result", out_unlock);

	// Receive inputs and outputs as varlen data directly into our allocated array
	const size_t input_size = xdev->input_count * sizeof(struct xrt_input);
	const size_t output_size = xdev->output_count * sizeof(struct xrt_output);

	if (input_size > 0) {
		xret = ipc_receive(&ipc_c->imc, xdev->inputs, input_size);
		IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive(inputs)", out_unlock);
	}
	if (output_size > 0) {
		xret = ipc_receive(&ipc_c->imc, xdev->outputs, output_size);
		IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive(outputs)", out_unlock);
	}

out_unlock:
	ipc_client_connection_unlock(ipc_c);

	return xret;
}

static xrt_result_t
ipc_client_xdev_get_tracked_pose(struct xrt_device *xdev,
                                 enum xrt_input_name name,
                                 int64_t at_timestamp_ns,
                                 struct xrt_space_relation *out_relation)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_get_tracked_pose( //
	    icx->ipc_c,                                       //
	    icx->device_id,                                   //
	    name,                                             //
	    at_timestamp_ns,                                  //
	    out_relation);                                    //
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_get_tracked_pose");
}

static xrt_result_t
ipc_client_xdev_get_hand_tracking(struct xrt_device *xdev,
                                  enum xrt_input_name name,
                                  int64_t at_timestamp_ns,
                                  struct xrt_hand_joint_set *out_value,
                                  int64_t *out_timestamp_ns)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_get_hand_tracking( //
	    icx->ipc_c,                                        //
	    icx->device_id,                                    //
	    name,                                              //
	    at_timestamp_ns,                                   //
	    out_value,                                         //
	    out_timestamp_ns);                                 //
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_get_hand_tracking");
}

static xrt_result_t
ipc_client_xdev_get_face_tracking(struct xrt_device *xdev,
                                  enum xrt_input_name facial_expression_type,
                                  int64_t at_timestamp_ns,
                                  struct xrt_facial_expression_set *out_value)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_get_face_tracking( //
	    icx->ipc_c,                                        //
	    icx->device_id,                                    //
	    facial_expression_type,                            //
	    at_timestamp_ns,                                   //
	    out_value);                                        //
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_get_face_tracking");
}

static xrt_result_t
ipc_client_xdev_get_face_calibration_state_android(struct xrt_device *xdev, bool *out_face_is_calibrated)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret =
	    ipc_call_device_get_face_calibration_state_android(icx->ipc_c, icx->device_id, out_face_is_calibrated);
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_get_face_calibration_state_android");
}

static xrt_result_t
ipc_client_xdev_get_body_skeleton(struct xrt_device *xdev,
                                  enum xrt_input_name body_tracking_type,
                                  struct xrt_body_skeleton *out_value)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_get_body_skeleton( //
	    icx->ipc_c,                                        //
	    icx->device_id,                                    //
	    body_tracking_type,                                //
	    out_value);                                        //
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_get_body_skeleton");
}

static xrt_result_t
ipc_client_xdev_get_body_joints(struct xrt_device *xdev,
                                enum xrt_input_name body_tracking_type,
                                int64_t desired_timestamp_ns,
                                struct xrt_body_joint_set *out_value)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_get_body_joints( //
	    icx->ipc_c,                                      //
	    icx->device_id,                                  //
	    body_tracking_type,                              //
	    desired_timestamp_ns,                            //
	    out_value);                                      //
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_get_body_joints");
}

static xrt_result_t
ipc_client_xdev_reset_body_tracking_calibration_meta(struct xrt_device *xdev)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_reset_body_tracking_calibration_meta( //
	    icx->ipc_c,                                                           //
	    icx->device_id);                                                      //
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_reset_body_tracking_calibration_meta");
}

static xrt_result_t
ipc_client_xdev_set_body_tracking_calibration_override_meta(struct xrt_device *xdev, float new_body_height)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_set_body_tracking_calibration_override_meta( //
	    icx->ipc_c,                                                                  //
	    icx->device_id,                                                              //
	    new_body_height);                                                            //
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_set_body_tracking_calibration_override_meta");
}

static xrt_result_t
ipc_client_xdev_set_body_tracking_fidelity_meta(struct xrt_device *xdev,
                                                enum xrt_body_tracking_fidelity_meta new_fidelity)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_set_body_tracking_fidelity_meta( //
	    icx->ipc_c,                                                      //
	    icx->device_id,                                                  //
	    new_fidelity);                                                   //
	IPC_CHK_ALWAYS_RET(icx->ipc_c, xret, "ipc_call_device_set_body_tracking_fidelity_meta");
}

static xrt_result_t
ipc_client_xdev_set_output(struct xrt_device *xdev, enum xrt_output_name name, const struct xrt_output_value *value)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);
	struct ipc_connection *ipc_c = icx->ipc_c;

	xrt_result_t xret;
	if (value->type == XRT_OUTPUT_VALUE_TYPE_PCM_VIBRATION) {
		uint32_t samples_sent = MIN(value->pcm_vibration.sample_rate, 4000);

		struct ipc_pcm_haptic_buffer samples = {
		    .append = value->pcm_vibration.append,
		    .num_samples = samples_sent,
		    .sample_rate = value->pcm_vibration.sample_rate,
		};

		ipc_client_connection_lock(ipc_c);

		xret = ipc_send_device_set_haptic_output_locked(ipc_c, icx->device_id, name, &samples);
		IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_send_device_set_haptic_output_locked", send_haptic_output_end);

		xrt_result_t alloc_xret;
		xret = ipc_receive(&ipc_c->imc, &alloc_xret, sizeof alloc_xret);
		if (xret != XRT_SUCCESS || alloc_xret != XRT_SUCCESS) {
			goto send_haptic_output_end;
		}

		xret = ipc_send(&ipc_c->imc, value->pcm_vibration.buffer, sizeof(float) * samples_sent);
		if (xret != XRT_SUCCESS) {
			goto send_haptic_output_end;
		}

		xret = ipc_receive(&ipc_c->imc, value->pcm_vibration.samples_consumed,
		                   sizeof(*value->pcm_vibration.samples_consumed));
		if (xret != XRT_SUCCESS) {
			goto send_haptic_output_end;
		}

	send_haptic_output_end:
		ipc_client_connection_unlock(ipc_c);
	} else {
		xret = ipc_call_device_set_output(ipc_c, icx->device_id, name, value);
		IPC_CHK_ONLY_PRINT(ipc_c, xret, "ipc_call_device_set_output");
	}

	return xret;
}

static xrt_result_t
ipc_client_xdev_get_output_limits(struct xrt_device *xdev, struct xrt_output_limits *limits)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_get_output_limits(icx->ipc_c, icx->device_id, limits);
	IPC_CHK_ONLY_PRINT(icx->ipc_c, xret, "ipc_call_device_get_output_limits");

	return xret;
}

static xrt_result_t
ipc_client_xdev_get_compositor_info(struct xrt_device *xdev,
                                    const struct xrt_device_compositor_mode *mode,
                                    struct xrt_device_compositor_info *out_info)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t xret = ipc_call_device_get_compositor_info(icx->ipc_c, icx->device_id, mode, out_info);
	IPC_CHK_ONLY_PRINT(icx->ipc_c, xret, "ipc_call_device_get_compositor_info");

	return xret;
}


/*
 *
 * Plane detection functions.
 *
 */

static xrt_result_t
ipc_client_xdev_begin_plane_detection_ext(struct xrt_device *xdev,
                                          const struct xrt_plane_detector_begin_info_ext *begin_info,
                                          uint64_t plane_detection_id,
                                          uint64_t *out_plane_detection_id)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	icx->ipc_c->ism->plane_begin_info_ext = *begin_info;

	xrt_result_t r = ipc_call_device_begin_plane_detection_ext(icx->ipc_c, icx->device_id, plane_detection_id,
	                                                           out_plane_detection_id);
	if (r != XRT_SUCCESS) {
		IPC_ERROR(icx->ipc_c, "Error sending hmd_begin_plane_detection_ext!");
		return r;
	}

	return XRT_SUCCESS;
}

static xrt_result_t
ipc_client_xdev_destroy_plane_detection_ext(struct xrt_device *xdev, uint64_t plane_detection_id)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t r = ipc_call_device_destroy_plane_detection_ext(icx->ipc_c, icx->device_id, plane_detection_id);
	if (r != XRT_SUCCESS) {
		IPC_ERROR(icx->ipc_c, "Error sending destroy_plane_detection_ext!");
		return r;
	}

	return XRT_SUCCESS;
}

/*!
 * Helper function for @ref xrt_device::get_plane_detection_state_ext.
 *
 * @public @memberof xrt_device
 */
static xrt_result_t
ipc_client_xdev_get_plane_detection_state_ext(struct xrt_device *xdev,
                                              uint64_t plane_detection_id,
                                              enum xrt_plane_detector_state_ext *out_state)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	xrt_result_t r =
	    ipc_call_device_get_plane_detection_state_ext(icx->ipc_c, icx->device_id, plane_detection_id, out_state);
	if (r != XRT_SUCCESS) {
		IPC_ERROR(icx->ipc_c, "Error sending get_plane_detection_state_ext!");
		return r;
	}

	return XRT_SUCCESS;
}

/*!
 * Helper function for @ref xrt_device::get_plane_detections_ext.
 *
 * @public @memberof xrt_device
 */
static xrt_result_t
ipc_client_xdev_get_plane_detections_ext(struct xrt_device *xdev,
                                         uint64_t plane_detection_id,
                                         struct xrt_plane_detections_ext *out_detections)
{
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);
	struct ipc_connection *ipc_c = icx->ipc_c;

	ipc_client_connection_lock(ipc_c);

	xrt_result_t xret = ipc_send_device_get_plane_detections_ext_locked(ipc_c, icx->device_id, plane_detection_id);
	IPC_CHK_WITH_GOTO(icx->ipc_c, xret, "ipc_send_device_get_plane_detections_ext_locked", out);

	// in this case, size == count
	uint32_t location_size = 0;
	uint32_t polygon_size = 0;
	uint32_t vertex_size = 0;

	xret = ipc_receive_device_get_plane_detections_ext_locked(ipc_c, &location_size, &polygon_size, &vertex_size);
	IPC_CHK_WITH_GOTO(icx->ipc_c, xret, "ipc_receive_device_get_plane_detections_ext_locked", out);


	// With no locations, the service won't send anything else
	if (location_size < 1) {
		out_detections->location_count = 0;
		goto out;
	}

	// realloc arrays in out_detections if necessary, then receive contents

	out_detections->location_count = location_size;
	if (out_detections->location_size < location_size) {
		U_ARRAY_REALLOC_OR_FREE(out_detections->locations, struct xrt_plane_detector_location_ext,
		                        location_size);
		U_ARRAY_REALLOC_OR_FREE(out_detections->polygon_info_start_index, uint32_t, location_size);
		out_detections->location_size = location_size;
	}

	if (out_detections->polygon_info_size < polygon_size) {
		U_ARRAY_REALLOC_OR_FREE(out_detections->polygon_infos, struct xrt_plane_polygon_info_ext, polygon_size);
		out_detections->polygon_info_size = polygon_size;
	}

	if (out_detections->vertex_size < vertex_size) {
		U_ARRAY_REALLOC_OR_FREE(out_detections->vertices, struct xrt_vec2, vertex_size);
		out_detections->vertex_size = vertex_size;
	}

	if ((location_size > 0 &&
	     (out_detections->locations == NULL || out_detections->polygon_info_start_index == NULL)) ||
	    (polygon_size > 0 && out_detections->polygon_infos == NULL) ||
	    (vertex_size > 0 && out_detections->vertices == NULL)) {
		IPC_ERROR(icx->ipc_c, "Error allocating memory for plane detections!");
		out_detections->location_size = 0;
		out_detections->polygon_info_size = 0;
		out_detections->vertex_size = 0;
		xret = XRT_ERROR_IPC_FAILURE;
		goto out;
	}

	if (location_size > 0) {
		// receive location_count * locations
		xret = ipc_receive(&ipc_c->imc, out_detections->locations,
		                   sizeof(struct xrt_plane_detector_location_ext) * location_size);
		IPC_CHK_WITH_GOTO(icx->ipc_c, xret, "ipc_receive(1)", out);

		// receive location_count * polygon_info_start_index
		xret = ipc_receive(&ipc_c->imc, out_detections->polygon_info_start_index,
		                   sizeof(uint32_t) * location_size);
		IPC_CHK_WITH_GOTO(icx->ipc_c, xret, "ipc_receive(2)", out);
	}


	if (polygon_size > 0) {
		// receive polygon_count * polygon_infos
		xret = ipc_receive(&ipc_c->imc, out_detections->polygon_infos,
		                   sizeof(struct xrt_plane_polygon_info_ext) * polygon_size);
		IPC_CHK_WITH_GOTO(icx->ipc_c, xret, "ipc_receive(3)", out);
	}

	if (vertex_size > 0) {
		// receive vertex_count * vertices
		xret = ipc_receive(&ipc_c->imc, out_detections->vertices, sizeof(struct xrt_vec2) * vertex_size);
		IPC_CHK_WITH_GOTO(icx->ipc_c, xret, "ipc_receive(4)", out);
	}

out:
	ipc_client_connection_unlock(ipc_c);
	return xret;
}


/*
 *
 * 'Exported' functions.
 *
 */

xrt_result_t
ipc_client_xdev_init(struct ipc_client_xdev *icx,
                     struct ipc_connection *ipc_c,
                     struct ipc_client_tracking_origin_manager *ictom,
                     uint32_t device_id,
                     u_device_destroy_function_t destroy_fn)
{
	// Helpers.
	xrt_result_t xret = XRT_SUCCESS;

	// Important fields.
	icx->ipc_c = ipc_c;

	// Slot index for IPC server lookups; not the same as xrt_device::id.
	icx->device_id = device_id;

	/*
	 * Fill in not implemented or noop versions first,
	 * destroy gets filled in by either device or HMD.
	 */
	u_device_populate_function_pointers(&icx->base, ipc_client_xdev_get_tracked_pose, destroy_fn);

	// Shared implemented functions.
	icx->base.update_inputs = ipc_client_xdev_update_inputs;
	icx->base.get_hand_tracking = ipc_client_xdev_get_hand_tracking;
	icx->base.get_face_tracking = ipc_client_xdev_get_face_tracking;
	icx->base.get_face_calibration_state_android = ipc_client_xdev_get_face_calibration_state_android;
	icx->base.get_body_skeleton = ipc_client_xdev_get_body_skeleton;
	icx->base.get_body_joints = ipc_client_xdev_get_body_joints;
	icx->base.reset_body_tracking_calibration_meta = ipc_client_xdev_reset_body_tracking_calibration_meta;
	icx->base.set_body_tracking_calibration_override_meta =
	    ipc_client_xdev_set_body_tracking_calibration_override_meta;
	icx->base.set_body_tracking_fidelity_meta = ipc_client_xdev_set_body_tracking_fidelity_meta;
	icx->base.set_output = ipc_client_xdev_set_output;
	icx->base.get_output_limits = ipc_client_xdev_get_output_limits;
	icx->base.get_compositor_info = ipc_client_xdev_get_compositor_info;

	// Plane detection EXT.
	icx->base.begin_plane_detection_ext = ipc_client_xdev_begin_plane_detection_ext;
	icx->base.destroy_plane_detection_ext = ipc_client_xdev_destroy_plane_detection_ext;
	icx->base.get_plane_detection_state_ext = ipc_client_xdev_get_plane_detection_state_ext;
	icx->base.get_plane_detections_ext = ipc_client_xdev_get_plane_detections_ext;

	// Lock the connection so we can do varlen IPC calls.
	ipc_client_connection_lock(ipc_c);

	// Call IPC to get device info with varlen data
	xret = ipc_send_device_get_info_locked(ipc_c, device_id);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_send_device_get_info_locked", out_free_and_unlock);

	struct ipc_device_info info = {0};
	xret = ipc_receive_device_get_info_locked(ipc_c, &info);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive_device_get_info_locked", out_free_and_unlock);

	// Copying the information from the info.
	icx->base.id.val = info.xrt_device_id_val;
	icx->base.device_type = info.device_type;
	icx->base.supported = info.supported;
	icx->base.name = info.name;

	// Print name.
	snprintf(icx->base.str, XRT_DEVICE_NAME_LEN, "%s", info.str);
	snprintf(icx->base.serial, XRT_DEVICE_NAME_LEN, "%s", info.serial);

	/*
	 * Allocate inputs array on client side. We receive the input names
	 * from the server, so we can use them to fill the inputs array.
	 */
	icx->base.input_count = info.input_count;
	if (info.input_count > 0) {
		// Allocate inputs array.
		icx->base.inputs = U_TYPED_ARRAY_CALLOC(struct xrt_input, info.input_count);
		// Allocate input names array (temporary, freed on all paths).
		enum xrt_input_name *input_names = U_TYPED_ARRAY_CALLOC(enum xrt_input_name, info.input_count);

		// Receive input names from server.
		xret = ipc_receive(&ipc_c->imc, input_names, sizeof(enum xrt_input_name) * info.input_count);
		if (xret == XRT_SUCCESS) {
			// On success, fill the inputs array with the input names.
			for (size_t i = 0; i < info.input_count; i++) {
				icx->base.inputs[i].name = input_names[i];
			}
		}
		free(input_names);
		IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive(input names)", out_free_and_unlock);
	} else {
		icx->base.inputs = NULL;
	}

	/*
	 * Allocate outputs array on client side. We receive the output names
	 * from the server, so we can use them to fill the outputs array.
	 */
	icx->base.output_count = info.output_count;
	if (info.output_count > 0) {
		// Allocate outputs array.
		icx->base.outputs = U_TYPED_ARRAY_CALLOC(struct xrt_output, info.output_count);
		// Allocate output names array (temporary, freed on all paths).
		enum xrt_output_name *output_names = U_TYPED_ARRAY_CALLOC(enum xrt_output_name, info.output_count);

		// Receive output names from server.
		xret = ipc_receive(&ipc_c->imc, output_names, sizeof(enum xrt_output_name) * info.output_count);
		if (xret == XRT_SUCCESS) {
			// On success, fill the outputs array with the output names.
			for (size_t i = 0; i < info.output_count; i++) {
				icx->base.outputs[i].name = output_names[i];
			}
		}
		free(output_names);
		IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive(output names)", out_free_and_unlock);
	} else {
		icx->base.outputs = NULL;
	}

out_free_and_unlock:
	// Out of the critical section.
	ipc_client_connection_unlock(ipc_c);

	// Check if we failed during the critical section.
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_client_xdev_init(failed during critical section)", err_fini);

	// Get the tracking origin, can't do this with the critical section, so do it after.
	struct xrt_tracking_origin *xtrack = NULL;
	xret = ipc_client_tracking_origin_manager_get(ictom, info.tracking_origin_id, &xtrack);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_client_tracking_origin_manager_get", err_fini);

	icx->base.tracking_origin = xtrack;

	// Return success.
	return XRT_SUCCESS;

err_fini:
	// Cleans up any allocations.
	ipc_client_xdev_fini(icx);

	return xret;
}

void
ipc_client_xdev_fini(struct ipc_client_xdev *icx)
{
	// Free inputs array
	if (icx->base.inputs != NULL) {
		free(icx->base.inputs);
		icx->base.inputs = NULL;
	}

	// Free outputs array
	if (icx->base.outputs != NULL) {
		free(icx->base.outputs);
		icx->base.outputs = NULL;
	}
}
