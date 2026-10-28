// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Handling functions called from generated dispatch function.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup ipc_server
 */

#include "util/u_misc.h"
#include "util/u_handles.h"
#include "util/u_pretty_print.h"
#include "util/u_visibility_mask.h"
#include "util/u_trace_marker.h"

#include "server/ipc_server.h"
#include "server/ipc_server_objects.h"
#include "ipc_server_generated.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_compositor.h"
#include "xrt/xrt_results.h"

#ifdef XRT_GRAPHICS_SYNC_HANDLE_IS_FD
#include <unistd.h>
#endif


/*
 *
 * Helper functions.
 *
 */

#define GET_XTRACK_OR_RETURN(ICS, ID, XTRACK)                                                                          \
	do {                                                                                                           \
		xrt_result_t xret = ipc_server_objects_get_xtrack_and_validate((ICS), ID, &(XTRACK));                  \
		IPC_CHK_AND_RET((ICS)->server, xret, "ipc_server_objects_get_xtrack_and_validate");                    \
	} while (0)

#define GET_XDEV_OR_RETURN(ICS, ID, XDEV)                                                                              \
	do {                                                                                                           \
		xrt_result_t xret = ipc_server_objects_get_xdev_and_validate((ICS), ID, &(XDEV));                      \
		IPC_CHK_AND_RET((ICS)->server, xret, "ipc_server_objects_get_xdev_and_validate");                      \
	} while (0)

#define GET_XSPC_OR_RETURN(ICS, ID, XSPC)                                                                              \
	do {                                                                                                           \
		xrt_result_t xret = ipc_server_objects_get_xspc_and_validate((ICS), ID, &(XSPC));                      \
		IPC_CHK_AND_RET((ICS)->server, xret, "ipc_server_objects_get_xspc_and_validate");                      \
	} while (0)


static xrt_result_t
validate_reference_space_type(volatile struct ipc_client_state *ics, enum xrt_reference_space_type type)
{
	if ((uint32_t)type >= XRT_SPACE_REFERENCE_TYPE_COUNT) {
		IPC_ERROR(ics->server, "Invalid reference space type %u", type);
		return XRT_ERROR_IPC_FAILURE;
	}

	return XRT_SUCCESS;
}

static xrt_result_t
validate_device_feature_type(volatile struct ipc_client_state *ics, enum xrt_device_feature_type type)
{
	if ((uint32_t)type >= XRT_DEVICE_FEATURE_MAX_ENUM) {
		IPC_ERROR(ics->server, "Invalid device feature type %u", type);
		return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	return XRT_SUCCESS;
}


static xrt_result_t
get_new_localspace_id(volatile struct ipc_client_state *ics, uint32_t *out_local_id, uint32_t *out_local_floor_id)
{
	// Our handle is just the index for now.
	uint32_t index = 0;
	for (; index < IPC_MAX_CLIENT_SPACES; index++) {
		if (ics->server->xso->localspace[index] == NULL) {
			break;
		}
	}

	if (index >= IPC_MAX_CLIENT_SPACES) {
		IPC_ERROR(ics->server, "Too many localspaces!");
		return XRT_ERROR_IPC_FAILURE;
	}

	ics->local_space_overseer_index = index;
	index = 0;
	for (; index < IPC_MAX_CLIENT_SPACES; index++) {
		if (ics->xspcs[index] == NULL) {
			break;
		}
	}

	if (index >= IPC_MAX_CLIENT_SPACES) {
		IPC_ERROR(ics->server, "Too many spaces!");
		return XRT_ERROR_IPC_FAILURE;
	}

	ics->local_space_index = index;
	*out_local_id = index;

	for (index = 0; index < IPC_MAX_CLIENT_SPACES; index++) {
		if (ics->server->xso->localfloorspace[index] == NULL) {
			break;
		}
	}

	if (index >= IPC_MAX_CLIENT_SPACES) {
		IPC_ERROR(ics->server, "Too many localfloorspaces!");
		return XRT_ERROR_IPC_FAILURE;
	}

	ics->local_floor_space_overseer_index = index;

	for (index = 0; index < IPC_MAX_CLIENT_SPACES; index++) {
		if (ics->xspcs[index] == NULL && index != ics->local_space_index) {
			break;
		}
	}

	if (index >= IPC_MAX_CLIENT_SPACES) {
		IPC_ERROR(ics->server, "Too many spaces!");
		return XRT_ERROR_IPC_FAILURE;
	}

	ics->local_floor_space_index = index;
	*out_local_floor_id = index;

	return XRT_SUCCESS;
}

static xrt_result_t
create_localspace(volatile struct ipc_client_state *ics, uint32_t *out_local_id, uint32_t *out_local_floor_id)
{
	uint32_t local_id = UINT32_MAX;
	uint32_t local_floor_id = UINT32_MAX;
	xrt_result_t xret = get_new_localspace_id(ics, &local_id, &local_floor_id);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	struct xrt_space_overseer *xso = ics->server->xso;
	struct xrt_space **xslocal_ptr = (struct xrt_space **)&ics->xspcs[local_id];
	struct xrt_space **xslocalfloor_ptr = (struct xrt_space **)&ics->xspcs[local_floor_id];

	xret = xrt_space_overseer_create_local_space(xso, &xso->localspace[ics->local_space_overseer_index],
	                                             &xso->localfloorspace[ics->local_floor_space_overseer_index]);
	if (xret != XRT_SUCCESS) {
		return xret;
	}
	xrt_space_reference(xslocal_ptr, xso->localspace[ics->local_space_overseer_index]);
	xrt_space_reference(xslocalfloor_ptr, xso->localfloorspace[ics->local_floor_space_overseer_index]);
	*out_local_id = local_id;
	*out_local_floor_id = local_floor_id;

	return XRT_SUCCESS;
}

XRT_MAYBE_UNUSED xrt_result_t
get_new_future_id(volatile struct ipc_client_state *ics, uint32_t *out_id)
{
	// Our handle is just the index for now.
	uint32_t index = 0;
	for (; index < IPC_MAX_CLIENT_FUTURES; ++index) {
		if (ics->xfts[index] == NULL) {
			break;
		}
	}

	if (index >= IPC_MAX_CLIENT_FUTURES) {
		IPC_ERROR(ics->server, "Too many futures!");
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_id = index;

	return XRT_SUCCESS;
}

static inline xrt_result_t
validate_future_id(volatile struct ipc_client_state *ics, uint32_t future_id, struct xrt_future **out_xft)
{
	if (future_id >= IPC_MAX_CLIENT_FUTURES) {
		return XRT_ERROR_IPC_FAILURE;
	}

	if (ics->xfts[future_id] == NULL) {
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xft = (struct xrt_future *)ics->xfts[future_id];
	return (*out_xft != NULL) ? XRT_SUCCESS : XRT_ERROR_ALLOCATION;
}

static inline xrt_result_t
release_future(volatile struct ipc_client_state *ics, uint32_t future_id)
{
	struct xrt_future *xft = NULL;
	xrt_result_t xret = validate_future_id(ics, future_id, &xft);
	if (xret != XRT_SUCCESS) {
		return xret;
	}
	xrt_future_reference(&xft, NULL);
	ics->xfts[future_id] = NULL;
	return XRT_SUCCESS;
}

static xrt_result_t
send_device_inputs(volatile struct ipc_client_state *ics, struct xrt_device *xdev)
{
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	xrt_result_t xret;

	const size_t input_size = xdev->input_count * sizeof(struct xrt_input);
	if (input_size == 0) {
		return XRT_SUCCESS;
	}

	volatile struct ipc_client_io_blocks *iob = &ics->client_state.io_blocks;
	// Send inputs as varlen data
	// We don't need to do any filtering, send the inputs as-is.
	if (!iob->block_poses && !iob->block_hand_tracking && !iob->block_inputs) {
		// Send the full input state
		xret = ipc_send(imc, xdev->inputs, input_size);
		IPC_CHK_ALWAYS_RET(ics->server, xret, "ipc_send(inputs)");
	}

	struct xrt_input filtered[1024] = {0};
	if (ARRAY_SIZE(filtered) < xdev->input_count) {
		return XRT_ERROR_IPC_FAILURE;
	}

	for (uint32_t i = 0; i < xdev->input_count; i++) {
		struct xrt_input *src = &xdev->inputs[i];
		struct xrt_input *dst = &filtered[i];

		bool blocked = false;
		switch (XRT_GET_INPUT_TYPE(src->name)) {
		case XRT_INPUT_TYPE_POSE: //
			blocked = iob->block_poses && src->name != XRT_INPUT_GENERIC_HEAD_POSE;
			break;
		case XRT_INPUT_TYPE_HAND_TRACKING: //
			blocked = iob->block_hand_tracking;
			break;
		case XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE:
		case XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE:
		case XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE:
		case XRT_INPUT_TYPE_VEC3_MINUS_ONE_TO_ONE:
		case XRT_INPUT_TYPE_BOOLEAN: //
			blocked = iob->block_inputs;
			break;
		// Don't block face or body tracking.
		case XRT_INPUT_TYPE_BODY_TRACKING:
		case XRT_INPUT_TYPE_FACE_TRACKING: break;
		}

		if (blocked) {
			// Make sure it's zeroed out, and we only zero out what we send.
			U_ZERO(dst);
			dst->name = src->name;
		} else {
			memcpy(dst, src, sizeof(*dst));
		}
	}

	xret = ipc_send(imc, filtered, input_size);
	IPC_CHK_ALWAYS_RET(ics->server, xret, "ipc_send(filtered inputs)");
}

static xrt_result_t
send_device_outputs(volatile struct ipc_client_state *ics, struct xrt_device *xdev)
{
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	xrt_result_t xret;

	const size_t output_size = xdev->output_count * sizeof(struct xrt_output);
	if (output_size == 0) {
		return XRT_SUCCESS;
	}

	volatile struct ipc_client_io_blocks *iob = &ics->client_state.io_blocks;
	// Send outputs as varlen data
	// We don't need to do any filtering, send the outputs as is.
	if (!iob->block_outputs) {
		// Send the full output state
		xret = ipc_send(imc, xdev->outputs, output_size);
		IPC_CHK_ALWAYS_RET(ics->server, xret, "ipc_send(outputs)");
	}

	struct xrt_output filtered[1024];
	if (ARRAY_SIZE(filtered) < xdev->output_count) {
		return XRT_ERROR_IPC_FAILURE;
	}

	for (uint32_t i = 0; i < xdev->output_count; i++) {
		struct xrt_output *dst = &filtered[i];
		// Make sure it's zeroed out, and we only zero out what we send.
		U_ZERO(dst);
		dst->name = xdev->outputs[i].name;
	}

	xret = ipc_send(imc, filtered, output_size);
	IPC_CHK_ALWAYS_RET(ics->server, xret, "ipc_send(filtered outputs)");
}

/*
 *
 * Handle functions.
 *
 */

xrt_result_t
ipc_handle_instance_get_shm_fd(volatile struct ipc_client_state *ics,
                               uint32_t max_handle_capacity,
                               xrt_shmem_handle_t *out_handles,
                               uint32_t *out_handle_count)
{
	IPC_TRACE_MARKER();

	assert(max_handle_capacity >= 1);

	out_handles[0] = get_ism_handle(ics);
	*out_handle_count = 1;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_instance_describe_client(volatile struct ipc_client_state *ics,
                                    const struct ipc_client_description *client_desc)
{
	ics->client_state.info = client_desc->info;
	ics->client_state.pid = client_desc->pid;

	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

#define P(...) u_pp(dg, __VA_ARGS__)
#define PNT(...) u_pp(dg, "\n\t" __VA_ARGS__)
#define PNTT(...) u_pp(dg, "\n\t\t" __VA_ARGS__)
#define EXT(NAME) PNTT(#NAME ": %s", client_desc->info.NAME ? "true" : "false")

	P("Client info:");
	PNT("id: %u", ics->client_state.id);
	PNT("application_name: '%s'", client_desc->info.application_name);
	PNT("pid: " PID_T_FMT, client_desc->pid);
	PNT("extensions:");

	EXT(ext_hand_tracking_enabled);
	EXT(ext_hand_tracking_data_source_enabled);
	EXT(ext_eye_gaze_interaction_enabled);
	EXT(ext_future_enabled);
	EXT(ext_hand_interaction_enabled);
	EXT(htc_facial_tracking_enabled);
	EXT(fb_body_tracking_enabled);
	EXT(meta_body_tracking_full_body_enabled);
	EXT(meta_body_tracking_calibration_enabled);
	EXT(meta_body_tracking_fidelity_enabled);
	EXT(fb_face_tracking2_enabled);
	EXT(android_face_tracking_enabled);

#undef EXT
#undef PTT
#undef PT
#undef P

	// Log the pretty message.
	IPC_INFO(ics->server, "%s", sink.buffer);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_instance_is_system_available(volatile struct ipc_client_state *ics, bool *out_available)
{
	IPC_TRACE_MARKER();

	xrt_result_t xret = XRT_SUCCESS;

	struct ipc_server *s = ics->server;

	// No client socket I/O while holding the global state lock.
	os_mutex_lock(&s->global_state.lock);

	xret = ipc_server_init_system_if_available_locked(s, ics, out_available);
	IPC_CHK_WITH_GOTO(s, xret, "ipc_server_init_system_if_available_locked", cleanup);

cleanup:
	os_mutex_unlock(&s->global_state.lock);
	return xret;
}

xrt_result_t
ipc_handle_system_compositor_get_info(volatile struct ipc_client_state *ics,
                                      struct xrt_system_compositor_info *out_info)
{
	IPC_TRACE_MARKER();

	*out_info = ics->server->xsysc->info;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_system_compositor_get_view_config(volatile struct ipc_client_state *ics,
                                             enum xrt_view_type view_type,
                                             struct xrt_view_config *out_view_config)
{
	IPC_TRACE_MARKER();

	return xrt_syscomp_get_view_config(ics->server->xsysc, view_type, out_view_config);
}

xrt_result_t
ipc_handle_session_create(volatile struct ipc_client_state *ics,
                          const struct xrt_session_info *xsi,
                          bool create_native_compositor)
{
	IPC_TRACE_MARKER();

	struct xrt_session *xs = NULL;
	struct xrt_compositor_native *xcn = NULL;
	struct xrt_compositor_native **xcn_ptr = NULL;

	if (ics->xs != NULL) {
		return XRT_ERROR_IPC_SESSION_ALREADY_CREATED;
	}

#ifndef XRT_FEATURE_NO_COMPOSITOR_FOR_HEADLESS_SESSIONS
	/*
	 * Currently if we don't create a compositor the session will not
	 * receive focused/visibility events since the IPC layer can not change
	 * tell the multi compositor about it.
	 *
	 * The default for XRT_FEATURE_NO_COMPOSITOR_FOR_HEADLESS_SESSIONS is
	 * off, meaning that the ifndef is true and as such this code is being
	 * run by default.
	 */
	if (!create_native_compositor) {
		IPC_INFO(ics->server, "App asked for headless session, creating native compositor anyways");
		create_native_compositor = true;
	}
#endif

	// This is false in headless sessions, don't create a native compositor.
	xcn_ptr = create_native_compositor ? &xcn : NULL;

	xrt_result_t xret = xrt_system_create_session(ics->server->xsys, xsi, &xs, xcn_ptr);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	ics->client_state.session_overlay = xsi->is_overlay;
	ics->client_state.z_order = xsi->z_order;

	// Either we didn't ask for a native compositor or we got one.
	assert(xcn_ptr == NULL || xcn != NULL);

	ics->xs = xs;
	ics->xc = xcn != NULL ? &xcn->base : NULL;

	if (ics->xc != NULL) {
		xrt_syscomp_set_state(                 //
		    ics->server->xsysc,                //
		    ics->xc,                           //
		    ics->client_state.session_visible, //
		    ics->client_state.session_focused, //
		    os_monotonic_get_ns());            //

		xrt_syscomp_set_z_order(        //
		    ics->server->xsysc,         //
		    ics->xc,                    //
		    ics->client_state.z_order); //
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_session_poll_events(volatile struct ipc_client_state *ics, union xrt_session_event *out_xse)
{
	// Have we created the session?
	if (ics->xs == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_session_poll_events(ics->xs, out_xse);
}

xrt_result_t
ipc_handle_session_request_exit(volatile struct ipc_client_state *ics)
{
	// Have we created the session?
	if (ics->xs == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_session_request_exit(ics->xs);
}

xrt_result_t
ipc_handle_session_begin(volatile struct ipc_client_state *ics)
{
	IPC_TRACE_MARKER();

	// Have we created the session?
	if (ics->xs == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	// Need to check both because begin session is handled by compositor.
	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_COMPOSITOR_NOT_CREATED;
	}

	//! @todo Pass the view type down.
	const struct xrt_begin_session_info begin_session_info = {
	    .view_type = XRT_VIEW_TYPE_STEREO,
	    .ext_hand_tracking_enabled = ics->client_state.info.ext_hand_tracking_enabled,
	    .ext_hand_tracking_data_source_enabled = ics->client_state.info.ext_hand_tracking_data_source_enabled,
	    .ext_eye_gaze_interaction_enabled = ics->client_state.info.ext_eye_gaze_interaction_enabled,
	    .ext_future_enabled = ics->client_state.info.ext_future_enabled,
	    .ext_hand_interaction_enabled = ics->client_state.info.ext_hand_interaction_enabled,
	    .htc_facial_tracking_enabled = ics->client_state.info.htc_facial_tracking_enabled,
	    .fb_body_tracking_enabled = ics->client_state.info.fb_body_tracking_enabled,
	    .fb_face_tracking2_enabled = ics->client_state.info.fb_face_tracking2_enabled,
	    .meta_body_tracking_full_body_enabled = ics->client_state.info.meta_body_tracking_full_body_enabled,
	    .meta_body_tracking_calibration_enabled = ics->client_state.info.meta_body_tracking_calibration_enabled,
	    .meta_body_tracking_fidelity_enabled = ics->client_state.info.meta_body_tracking_fidelity_enabled,
	    .android_face_tracking_enabled = ics->client_state.info.android_face_tracking_enabled,
	};

	return xrt_comp_begin_session(ics->xc, &begin_session_info);
}

xrt_result_t
ipc_handle_session_end(volatile struct ipc_client_state *ics)
{
	IPC_TRACE_MARKER();

	// Have we created the session?
	if (ics->xs == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	// Need to check both because end session is handled by compositor.
	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_COMPOSITOR_NOT_CREATED;
	}

	return xrt_comp_end_session(ics->xc);
}

xrt_result_t
ipc_handle_session_destroy(volatile struct ipc_client_state *ics)
{
	IPC_TRACE_MARKER();

	// Have we created the session?
	if (ics->xs == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	ipc_server_client_destroy_session_and_compositor(ics);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_space_create_semantic_ids(volatile struct ipc_client_state *ics,
                                     uint32_t *out_root_id,
                                     uint32_t *out_view_id,
                                     uint32_t *out_local_id,
                                     uint32_t *out_local_floor_id,
                                     uint32_t *out_stage_id,
                                     uint32_t *out_unbounded_id)
{
	IPC_TRACE_MARKER();

	struct xrt_space_overseer *xso = ics->server->xso;

#define CREATE(NAME)                                                                                                   \
	do {                                                                                                           \
		*out_##NAME##_id = UINT32_MAX;                                                                         \
		if (xso->semantic.NAME == NULL) {                                                                      \
			break;                                                                                         \
		}                                                                                                      \
		uint32_t id = 0;                                                                                       \
		xrt_result_t xret = ipc_server_objects_get_xspc_id_or_add(ics, xso->semantic.NAME, &id);               \
		if (xret != XRT_SUCCESS) {                                                                             \
			break;                                                                                         \
		}                                                                                                      \
		*out_##NAME##_id = id;                                                                                 \
	} while (false)

	CREATE(root);
	CREATE(view);
	CREATE(stage);
	CREATE(unbounded);

#undef CREATE
	return create_localspace(ics, out_local_id, out_local_floor_id);
}

xrt_result_t
ipc_handle_space_create_offset(volatile struct ipc_client_state *ics,
                               uint32_t parent_id,
                               const struct xrt_pose *offset,
                               uint32_t *out_space_id)
{
	IPC_TRACE_MARKER();

	struct xrt_space_overseer *xso = ics->server->xso;

	struct xrt_space *parent = NULL;
	GET_XSPC_OR_RETURN(ics, parent_id, parent);


	struct xrt_space *xs = NULL;
	xrt_result_t xret = xrt_space_overseer_create_offset_space(xso, parent, offset, &xs);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	uint32_t space_id = UINT32_MAX;
	xret = ipc_server_objects_get_xspc_id_or_add(ics, xs, &space_id);

	// Track space grabs a reference, or it errors and we don't want to keep it around.
	xrt_space_reference(&xs, NULL);

	if (xret != XRT_SUCCESS) {
		return xret;
	}

	*out_space_id = space_id;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_space_create_pose(volatile struct ipc_client_state *ics,
                             uint32_t xdev_id,
                             enum xrt_input_name name,
                             uint32_t *out_space_id)
{
	IPC_TRACE_MARKER();

	struct xrt_space_overseer *xso = ics->server->xso;

	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, xdev_id, xdev);

	struct xrt_space *xs = NULL;
	xrt_result_t xret = xrt_space_overseer_create_pose_space(xso, xdev, name, &xs);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	uint32_t space_id = UINT32_MAX;
	xret = ipc_server_objects_get_xspc_id_or_add(ics, xs, &space_id);

	// Track space grabs a reference, or it errors and we don't want to keep it around.
	xrt_space_reference(&xs, NULL);

	if (xret != XRT_SUCCESS) {
		return xret;
	}

	*out_space_id = space_id;

	return xret;
}

xrt_result_t
ipc_handle_space_locate_space(volatile struct ipc_client_state *ics,
                              uint32_t base_space_id,
                              const struct xrt_pose *base_offset,
                              int64_t at_timestamp,
                              uint32_t space_id,
                              const struct xrt_pose *offset,
                              struct xrt_space_relation *out_relation)
{
	IPC_TRACE_MARKER();

	struct xrt_space_overseer *xso = ics->server->xso;
	struct xrt_space *base_space = NULL;
	struct xrt_space *space = NULL;

	GET_XSPC_OR_RETURN(ics, base_space_id, base_space);

	GET_XSPC_OR_RETURN(ics, space_id, space);

	return xrt_space_overseer_locate_space( //
	    xso,                                //
	    base_space,                         //
	    base_offset,                        //
	    at_timestamp,                       //
	    space,                              //
	    offset,                             //
	    out_relation);                      //
}

/*
 * Exchanges several messages with the client, so do not hold the global state
 * lock anywhere in this function.
 */
xrt_result_t
ipc_handle_space_locate_spaces(volatile struct ipc_client_state *ics,
                               uint32_t base_space_id,
                               const struct xrt_pose *base_offset,
                               uint32_t space_count,
                               int64_t at_timestamp)
{
	IPC_TRACE_MARKER();
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;

	struct xrt_space_overseer *xso = ics->server->xso;
	struct xrt_space *base_space = NULL;

	struct xrt_space **xspaces = U_TYPED_ARRAY_CALLOC(struct xrt_space *, space_count);
	struct xrt_pose *offsets = U_TYPED_ARRAY_CALLOC(struct xrt_pose, space_count);
	struct xrt_space_relation *out_relations = U_TYPED_ARRAY_CALLOC(struct xrt_space_relation, space_count);

	xrt_result_t xret = XRT_SUCCESS;

	uint32_t *space_ids = U_TYPED_ARRAY_CALLOC(uint32_t, space_count);

	// we need to send back whether allocation succeeded so the client knows whether to send more data
	if (space_ids == NULL) {
		xret = XRT_ERROR_ALLOCATION;
	}

	xret = ipc_send(imc, &xret, sizeof(enum xrt_result));
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "Failed to send spaces allocate result");
		// Nothing else we can do
		goto out_locate_spaces;
	}

	// only after sending the allocation result can we skip to the end in the allocation error case
	if (space_ids == NULL) {
		IPC_ERROR(ics->server, "Failed to allocate space for receiving spaces ids");
		xret = XRT_ERROR_ALLOCATION;
		goto out_locate_spaces;
	}


	xret = ipc_receive(imc, space_ids, space_count * sizeof(uint32_t));
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "Failed to receive spaces ids");
		// assume early abort is possible, i.e. client will not send more data for this request
		goto out_locate_spaces;
	}

	xret = ipc_receive(imc, offsets, space_count * sizeof(struct xrt_pose));
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "Failed to receive spaces offsets");
		// assume early abort is possible, i.e. client will not send more data for this request
		goto out_locate_spaces;
	}

	xret = ipc_server_objects_get_xspc_and_validate(ics, base_space_id, &base_space);
	// Client is receiving out_relations now, it will get xret on this receive.
	IPC_CHK_WITH_GOTO(ics->server, xret, "ipc_server_objects_get_xspc_and_validate(base_space_id)",
	                  out_locate_spaces);

	for (uint32_t i = 0; i < space_count; i++) {
		if (space_ids[i] == UINT32_MAX) {
			xspaces[i] = NULL;
		} else {
			xret = ipc_server_objects_get_xspc_and_validate(ics, space_ids[i], &xspaces[i]);
			// Client is receiving xspaces[i] now, it will get xret on this receive.
			IPC_CHK_WITH_GOTO(ics->server, xret, "ipc_server_objects_get_xspc_and_validate(space_ids[i])",
			                  out_locate_spaces);
		}
	}
	xret = xrt_space_overseer_locate_spaces( //
	    xso,                                 //
	    base_space,                          //
	    base_offset,                         //
	    at_timestamp,                        //
	    xspaces,                             //
	    space_count,                         //
	    offsets,                             //
	    out_relations);                      //

	xret = ipc_send(imc, out_relations, sizeof(struct xrt_space_relation) * space_count);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "Failed to send spaces relations");
		// Nothing else we can do
		goto out_locate_spaces;
	}

out_locate_spaces:
	free(space_ids);
	free(xspaces);
	free(offsets);
	free(out_relations);
	return xret;
}

xrt_result_t
ipc_handle_space_locate_device(volatile struct ipc_client_state *ics,
                               uint32_t base_space_id,
                               const struct xrt_pose *base_offset,
                               int64_t at_timestamp,
                               uint32_t xdev_id,
                               struct xrt_space_relation *out_relation)
{
	IPC_TRACE_MARKER();

	struct xrt_space_overseer *xso = ics->server->xso;
	struct xrt_space *base_space = NULL;
	struct xrt_device *xdev = NULL;

	GET_XSPC_OR_RETURN(ics, base_space_id, base_space);

	GET_XDEV_OR_RETURN(ics, xdev_id, xdev);

	return xrt_space_overseer_locate_device( //
	    xso,                                 //
	    base_space,                          //
	    base_offset,                         //
	    at_timestamp,                        //
	    xdev,                                //
	    out_relation);                       //
}

xrt_result_t
ipc_handle_space_destroy(volatile struct ipc_client_state *ics, uint32_t space_id)
{
	xrt_result_t xret = ipc_server_objects_destroy_xspc(ics, space_id);
	IPC_CHK_AND_RET(ics->server, xret, "ipc_server_objects_destroy_xspc");

	if (space_id == ics->local_space_index) {
		struct xrt_space **xslocal_ptr =
		    (struct xrt_space **)&ics->server->xso->localspace[ics->local_space_overseer_index];
		xrt_space_reference(xslocal_ptr, NULL);
	}

	if (space_id == ics->local_floor_space_index) {
		struct xrt_space **xslocalfloor_ptr =
		    (struct xrt_space **)&ics->server->xso->localfloorspace[ics->local_floor_space_overseer_index];
		xrt_space_reference(xslocalfloor_ptr, NULL);
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_space_mark_ref_space_in_use(volatile struct ipc_client_state *ics, enum xrt_reference_space_type type)
{
	struct xrt_space_overseer *xso = ics->server->xso;
	xrt_result_t xret;

	xret = validate_reference_space_type(ics, type);
	if (xret != XRT_SUCCESS) {
		return XRT_ERROR_IPC_FAILURE;
	}

	// Is this space already used?
	if (ics->ref_space_used[type]) {
		IPC_ERROR(ics->server, "Space '%u' already used!", type);
		return XRT_ERROR_IPC_FAILURE;
	}

	xret = xrt_space_overseer_ref_space_inc(xso, type);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "xrt_space_overseer_ref_space_inc failed");
		return xret;
	}

	// Can now mark it as used.
	ics->ref_space_used[type] = true;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_space_unmark_ref_space_in_use(volatile struct ipc_client_state *ics, enum xrt_reference_space_type type)
{
	struct xrt_space_overseer *xso = ics->server->xso;
	xrt_result_t xret;

	xret = validate_reference_space_type(ics, type);
	if (xret != XRT_SUCCESS) {
		return XRT_ERROR_IPC_FAILURE;
	}

	if (!ics->ref_space_used[type]) {
		IPC_ERROR(ics->server, "Space '%u' not used!", type);
		return XRT_ERROR_IPC_FAILURE;
	}

	xret = xrt_space_overseer_ref_space_dec(xso, type);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "xrt_space_overseer_ref_space_dec failed");
		return xret;
	}

	// Now we can mark it as not used.
	ics->ref_space_used[type] = false;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_space_recenter_local_spaces(volatile struct ipc_client_state *ics)
{
	struct xrt_space_overseer *xso = ics->server->xso;

	return xrt_space_overseer_recenter_local_spaces(xso);
}

xrt_result_t
ipc_handle_space_get_tracking_origin_offset(volatile struct ipc_client_state *ics,
                                            uint32_t origin_id,
                                            struct xrt_pose *out_offset)
{
	struct xrt_space_overseer *xso = ics->server->xso;
	struct xrt_tracking_origin *xto = NULL;
	GET_XTRACK_OR_RETURN(ics, origin_id, xto);

	return xrt_space_overseer_get_tracking_origin_offset(xso, xto, out_offset);
}

xrt_result_t
ipc_handle_space_set_tracking_origin_offset(volatile struct ipc_client_state *ics,
                                            uint32_t origin_id,
                                            const struct xrt_pose *offset)
{
	struct xrt_space_overseer *xso = ics->server->xso;
	struct xrt_tracking_origin *xto = NULL;
	GET_XTRACK_OR_RETURN(ics, origin_id, xto);

	return xrt_space_overseer_set_tracking_origin_offset(xso, xto, offset);
}

xrt_result_t
ipc_handle_space_get_reference_space_offset(volatile struct ipc_client_state *ics,
                                            enum xrt_reference_space_type type,
                                            struct xrt_pose *out_offset)
{
	struct xrt_space_overseer *xso = ics->server->xso;
	return xrt_space_overseer_get_reference_space_offset(xso, type, out_offset);
}

xrt_result_t
ipc_handle_space_set_reference_space_offset(volatile struct ipc_client_state *ics,
                                            enum xrt_reference_space_type type,
                                            const struct xrt_pose *offset)
{
	struct xrt_space_overseer *xso = ics->server->xso;
	return xrt_space_overseer_set_reference_space_offset(xso, type, offset);
}

xrt_result_t
ipc_handle_compositor_get_info(volatile struct ipc_client_state *ics, struct xrt_compositor_info *out_info)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	*out_info = ics->xc->info;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_compositor_predict_frame(volatile struct ipc_client_state *ics,
                                    int64_t *out_frame_id,
                                    int64_t *out_wake_up_time_ns,
                                    int64_t *out_predicted_display_time_ns,
                                    int64_t *out_predicted_display_period_ns)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	/*
	 * We use this to signal that the session has started, this is needed
	 * to make this client/session active/visible/focused.
	 */
	ipc_server_activate_session(ics);

	int64_t gpu_time_ns = 0;
	return xrt_comp_predict_frame(        //
	    ics->xc,                          //
	    out_frame_id,                     //
	    out_wake_up_time_ns,              //
	    &gpu_time_ns,                     //
	    out_predicted_display_time_ns,    //
	    out_predicted_display_period_ns); //
}

xrt_result_t
ipc_handle_compositor_wait_woke(volatile struct ipc_client_state *ics, int64_t frame_id)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_mark_frame(ics->xc, frame_id, XRT_COMPOSITOR_FRAME_POINT_WOKE, os_monotonic_get_ns());
}

xrt_result_t
ipc_handle_compositor_begin_frame(volatile struct ipc_client_state *ics, int64_t frame_id)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_begin_frame(ics->xc, frame_id);
}

xrt_result_t
ipc_handle_compositor_discard_frame(volatile struct ipc_client_state *ics, int64_t frame_id)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_discard_frame(ics->xc, frame_id);
}

xrt_result_t
ipc_handle_compositor_get_display_refresh_rate(volatile struct ipc_client_state *ics,
                                               float *out_display_refresh_rate_hz)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_get_display_refresh_rate(ics->xc, out_display_refresh_rate_hz);
}

xrt_result_t
ipc_handle_compositor_request_display_refresh_rate(volatile struct ipc_client_state *ics, float display_refresh_rate_hz)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_request_display_refresh_rate(ics->xc, display_refresh_rate_hz);
}

xrt_result_t
ipc_handle_compositor_set_performance_level(volatile struct ipc_client_state *ics,
                                            enum xrt_perf_domain domain,
                                            enum xrt_perf_set_level level)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_COMPOSITOR_NOT_CREATED;
	}

	if (ics->xc->set_performance_level == NULL) {
		return XRT_ERROR_IPC_FAILURE;
	}

	return xrt_comp_set_performance_level(ics->xc, domain, level);
}

static xrt_result_t
_update_projection_layer(struct xrt_compositor *xc,
                         volatile struct ipc_client_state *ics,
                         volatile struct ipc_layer_entry *layer,
                         uint32_t i)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, layer->xdev_id, xdev);

	if (xdev == NULL) {
		U_LOG_E("Invalid xdev for projection layer!");
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_layer_data *data = (struct xrt_layer_data *)&layer->data;

	struct xrt_swapchain *xcs[XRT_MAX_VIEWS] = {0};

	for (uint32_t k = 0; k < data->view_count; k++) {
		const uint32_t xsci = layer->swapchain_ids[k];
		xcs[k] = ics->xscs[xsci];
		if (xcs[k] == NULL) {
			U_LOG_E("Invalid swap chain for projection layer!");
			return XRT_ERROR_IPC_FAILURE;
		}
	}


	return xrt_comp_layer_projection(xc, xdev, xcs, data);
}

static xrt_result_t
_update_projection_layer_depth(struct xrt_compositor *xc,
                               volatile struct ipc_client_state *ics,
                               volatile struct ipc_layer_entry *layer,
                               uint32_t i)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, layer->xdev_id, xdev);

	if (xdev == NULL) {
		U_LOG_E("Invalid xdev for projection layer!");
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_layer_data *data = (struct xrt_layer_data *)&layer->data;

	struct xrt_swapchain *xcs[XRT_MAX_VIEWS] = {0};
	struct xrt_swapchain *d_xcs[XRT_MAX_VIEWS] = {0};

	for (uint32_t j = 0; j < data->view_count; j++) {
		const uint32_t xsci = layer->swapchain_ids[j];
		const uint32_t d_xsci = layer->swapchain_ids[j + data->view_count];

		xcs[j] = ics->xscs[xsci];
		d_xcs[j] = ics->xscs[d_xsci];
		if (xcs[j] == NULL || d_xcs[j] == NULL) {
			U_LOG_E("Invalid swap chain for projection layer #%u!", i);
			return XRT_ERROR_IPC_FAILURE;
		}
	}

	return xrt_comp_layer_projection_depth(xc, xdev, xcs, d_xcs, data);
}

static xrt_result_t
do_single(struct xrt_compositor *xc,
          volatile struct ipc_client_state *ics,
          volatile struct ipc_layer_entry *layer,
          uint32_t i,
          const char *name,
          struct xrt_device **out_xdev,
          struct xrt_swapchain **out_xcs)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, layer->xdev_id, xdev);

	if (xdev == NULL) {
		U_LOG_E("Invalid xdev for layer #%u, '%s'!", i, name);
		return XRT_ERROR_IPC_FAILURE;
	}

	const uint32_t sci = layer->swapchain_ids[0];
	struct xrt_swapchain *xcs = ics->xscs[sci];

	if (xcs == NULL) {
		U_LOG_E("Invalid swapchain for layer #%u, '%s'!", i, name);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xdev = xdev;
	*out_xcs = xcs;

	return XRT_SUCCESS;
}

static bool
_update_quad_layer(struct xrt_compositor *xc,
                   volatile struct ipc_client_state *ics,
                   volatile struct ipc_layer_entry *layer,
                   uint32_t i)
{
	struct xrt_device *xdev = NULL;
	struct xrt_swapchain *xcs = NULL;

	xrt_result_t xret = do_single(xc, ics, layer, i, "quad", &xdev, &xcs);
	IPC_CHK_AND_RET(ics->server, xret, "_update_quad_layer");

	struct xrt_layer_data *data = (struct xrt_layer_data *)&layer->data;
	return xrt_comp_layer_quad(xc, xdev, xcs, data);
}

static bool
_update_cube_layer(struct xrt_compositor *xc,
                   volatile struct ipc_client_state *ics,
                   volatile struct ipc_layer_entry *layer,
                   uint32_t i)
{
	struct xrt_device *xdev = NULL;
	struct xrt_swapchain *xcs = NULL;

	xrt_result_t xret = do_single(xc, ics, layer, i, "cube", &xdev, &xcs);
	IPC_CHK_AND_RET(ics->server, xret, "_update_cube_layer");

	struct xrt_layer_data *data = (struct xrt_layer_data *)&layer->data;
	return xrt_comp_layer_cube(xc, xdev, xcs, data);
}

static bool
_update_cylinder_layer(struct xrt_compositor *xc,
                       volatile struct ipc_client_state *ics,
                       volatile struct ipc_layer_entry *layer,
                       uint32_t i)
{
	struct xrt_device *xdev = NULL;
	struct xrt_swapchain *xcs = NULL;

	xrt_result_t xret = do_single(xc, ics, layer, i, "cylinder", &xdev, &xcs);
	IPC_CHK_AND_RET(ics->server, xret, "_update_cylinder_layer");

	struct xrt_layer_data *data = (struct xrt_layer_data *)&layer->data;
	return xrt_comp_layer_cylinder(xc, xdev, xcs, data);
}

static bool
_update_equirect1_layer(struct xrt_compositor *xc,
                        volatile struct ipc_client_state *ics,
                        volatile struct ipc_layer_entry *layer,
                        uint32_t i)
{
	struct xrt_device *xdev = NULL;
	struct xrt_swapchain *xcs = NULL;

	xrt_result_t xret = do_single(xc, ics, layer, i, "equirect1", &xdev, &xcs);
	IPC_CHK_AND_RET(ics->server, xret, "_update_equirect1_layer");

	struct xrt_layer_data *data = (struct xrt_layer_data *)&layer->data;
	return xrt_comp_layer_equirect1(xc, xdev, xcs, data);
}

static bool
_update_equirect2_layer(struct xrt_compositor *xc,
                        volatile struct ipc_client_state *ics,
                        volatile struct ipc_layer_entry *layer,
                        uint32_t i)
{
	struct xrt_device *xdev = NULL;
	struct xrt_swapchain *xcs = NULL;

	xrt_result_t xret = do_single(xc, ics, layer, i, "equirect2", &xdev, &xcs);
	IPC_CHK_AND_RET(ics->server, xret, "_update_equirect2_layer");

	struct xrt_layer_data *data = (struct xrt_layer_data *)&layer->data;
	return xrt_comp_layer_equirect2(xc, xdev, xcs, data);
}

static xrt_result_t
_update_passthrough_layer(struct xrt_compositor *xc,
                          volatile struct ipc_client_state *ics,
                          volatile struct ipc_layer_entry *layer,
                          uint32_t i)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, layer->xdev_id, xdev);

	if (xdev == NULL) {
		U_LOG_E("Invalid swapchain for layer #%u, 'passthrough'!", i);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_layer_data *data = (struct xrt_layer_data *)&layer->data;
	return xrt_comp_layer_passthrough(xc, xdev, data);
}

static bool
_update_layers(volatile struct ipc_client_state *ics, struct ipc_layer_slot *slot)
{
	IPC_TRACE_MARKER();

	struct xrt_compositor *xc = ics->xc;

	xrt_result_t xret = XRT_SUCCESS;

	for (uint32_t i = 0; i < slot->layer_count; i++) {
		volatile struct ipc_layer_entry *layer = &slot->layers[i];

		switch (layer->data.type) {
		case XRT_LAYER_PROJECTION:
			xret = _update_projection_layer(xc, ics, layer, i);
			IPC_CHK_AND_RET(ics->server, xret, "_update_projection_layer");
			break;
		case XRT_LAYER_PROJECTION_DEPTH:
			xret = _update_projection_layer_depth(xc, ics, layer, i);
			IPC_CHK_AND_RET(ics->server, xret, "_update_projection_layer_depth");
			break;
		case XRT_LAYER_QUAD:
			xret = _update_quad_layer(xc, ics, layer, i);
			IPC_CHK_AND_RET(ics->server, xret, "_update_quad_layer");
			break;
		case XRT_LAYER_CUBE:
			xret = _update_cube_layer(xc, ics, layer, i);
			IPC_CHK_AND_RET(ics->server, xret, "_update_cube_layer");
			break;
		case XRT_LAYER_CYLINDER:
			xret = _update_cylinder_layer(xc, ics, layer, i);
			IPC_CHK_AND_RET(ics->server, xret, "_update_cylinder_layer");
			break;
		case XRT_LAYER_EQUIRECT1:
			xret = _update_equirect1_layer(xc, ics, layer, i);
			IPC_CHK_AND_RET(ics->server, xret, "_update_equirect1_layer");
			break;
		case XRT_LAYER_EQUIRECT2:
			xret = _update_equirect2_layer(xc, ics, layer, i);
			IPC_CHK_AND_RET(ics->server, xret, "_update_equirect2_layer");
			break;
		case XRT_LAYER_PASSTHROUGH:
			xret = _update_passthrough_layer(xc, ics, layer, i);
			IPC_CHK_AND_RET(ics->server, xret, "_update_passthrough_layer");
			break;
		default: U_LOG_E("Unhandled layer type '%i'!", layer->data.type); return XRT_ERROR_IPC_FAILURE;
		}
	}

	return xret;
}


xrt_result_t
ipc_handle_compositor_layer_sync(volatile struct ipc_client_state *ics,
                                 uint32_t slot_id,
                                 uint32_t *out_free_slot_id,
                                 const xrt_graphics_sync_handle_t *handles,
                                 const uint32_t handle_count)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	xrt_graphics_sync_handle_t sync_handle = XRT_GRAPHICS_SYNC_HANDLE_INVALID;

	// If we have one or more save the first handle.
	if (handle_count >= 1) {
		sync_handle = handles[0];
	}

	// Free all sync handles after the first one.
	for (uint32_t i = 1; i < handle_count; i++) {
		// Checks for valid handle.
		xrt_graphics_sync_handle_t tmp = handles[i];
		u_graphics_sync_unref(&tmp);
	}

	struct ipc_shared_memory *ism = get_ism(ics);

	// Copy the layer slot in case the shared memory gets overwritten during update
	struct ipc_layer_slot slot = ism->slots[slot_id];

	/*
	 * Transfer data to underlying compositor.
	 */

	xrt_comp_layer_begin(ics->xc, &slot.data);

	xrt_result_t xret = _update_layers(ics, &slot);
	IPC_CHK_AND_RET(ics->server, xret, "_update_layers");

	xrt_comp_layer_commit(ics->xc, sync_handle);


	/*
	 * Manage shared state.
	 */

	// No client socket I/O while holding the global state lock.
	os_mutex_lock(&ics->server->global_state.lock);

	*out_free_slot_id = (ics->server->current_slot_index + 1) % IPC_MAX_SLOTS;
	ics->server->current_slot_index = *out_free_slot_id;

	os_mutex_unlock(&ics->server->global_state.lock);

	return xret;
}

xrt_result_t
ipc_handle_compositor_layer_sync_with_semaphore(volatile struct ipc_client_state *ics,
                                                uint32_t slot_id,
                                                uint32_t semaphore_id,
                                                uint64_t semaphore_value,
                                                uint32_t *out_free_slot_id)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}
	if (semaphore_id >= IPC_MAX_CLIENT_SEMAPHORES) {
		IPC_ERROR(ics->server, "Invalid semaphore_id");
		return XRT_ERROR_IPC_FAILURE;
	}
	if (ics->xcsems[semaphore_id] == NULL) {
		IPC_ERROR(ics->server, "Semaphore of id %u not created!", semaphore_id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_compositor_semaphore *xcsem = ics->xcsems[semaphore_id];

	struct ipc_shared_memory *ism = get_ism(ics);

	// Copy the layer slot in case the shared memory gets overwritten during update
	struct ipc_layer_slot slot = ism->slots[slot_id];

	/*
	 * Transfer data to underlying compositor.
	 */

	xrt_comp_layer_begin(ics->xc, &slot.data);

	xrt_result_t xret = _update_layers(ics, &slot);
	IPC_CHK_AND_RET(ics->server, xret, "_update_layers");

	xrt_comp_layer_commit_with_semaphore(ics->xc, xcsem, semaphore_value);


	/*
	 * Manage shared state.
	 */

	// No client socket I/O while holding the global state lock.
	os_mutex_lock(&ics->server->global_state.lock);

	*out_free_slot_id = (ics->server->current_slot_index + 1) % IPC_MAX_SLOTS;
	ics->server->current_slot_index = *out_free_slot_id;

	os_mutex_unlock(&ics->server->global_state.lock);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_compositor_create_passthrough(volatile struct ipc_client_state *ics,
                                         const struct xrt_passthrough_create_info *info)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_create_passthrough(ics->xc, info);
}

xrt_result_t
ipc_handle_compositor_create_passthrough_layer(volatile struct ipc_client_state *ics,
                                               const struct xrt_passthrough_layer_create_info *info)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_create_passthrough_layer(ics->xc, info);
}

xrt_result_t
ipc_handle_compositor_destroy_passthrough(volatile struct ipc_client_state *ics)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	xrt_comp_destroy_passthrough(ics->xc);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_compositor_set_thread_hint(volatile struct ipc_client_state *ics,
                                      enum xrt_thread_hint hint,
                                      uint32_t thread_id)

{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_set_thread_hint(ics->xc, hint, thread_id);
}

xrt_result_t
ipc_handle_compositor_get_reference_bounds_rect(volatile struct ipc_client_state *ics,
                                                enum xrt_reference_space_type reference_space_type,
                                                struct xrt_vec2 *bounds)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_get_reference_bounds_rect(ics->xc, reference_space_type, bounds);
}

xrt_result_t
ipc_handle_system_get_clients(volatile struct ipc_client_state *_ics, struct ipc_client_list *list)
{
	struct ipc_server *s = _ics->server;

	/*
	 * Look client list.
	 * No client socket I/O while holding the global state lock.
	 */
	os_mutex_lock(&s->global_state.lock);

	uint32_t count = 0;
	for (uint32_t i = 0; i < IPC_MAX_CLIENTS; i++) {

		volatile struct ipc_client_state *ics = &s->threads[i].ics;

		// Is this thread running?
		if (ics->server_thread_index < 0) {
			continue;
		}

		list->ids[count++] = ics->client_state.id;
	}

	list->id_count = count;

	// Unlock now.
	os_mutex_unlock(&s->global_state.lock);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_system_get_properties(volatile struct ipc_client_state *_ics, struct xrt_system_properties *out_properties)
{
	struct ipc_server *s = _ics->server;

	return ipc_server_get_system_properties(s, out_properties);
}

xrt_result_t
ipc_handle_system_get_client_info(volatile struct ipc_client_state *_ics,
                                  uint32_t client_id,
                                  struct ipc_app_state *out_ias)
{
	struct ipc_server *s = _ics->server;

	return ipc_server_get_client_app_state(s, client_id, out_ias);
}

xrt_result_t
ipc_handle_system_set_primary_client(volatile struct ipc_client_state *_ics, uint32_t client_id)
{
	struct ipc_server *s = _ics->server;

	IPC_INFO(s, "System setting active client to %d.", client_id);

	return ipc_server_set_active_client(s, client_id);
}

xrt_result_t
ipc_handle_system_set_focused_client(volatile struct ipc_client_state *ics, uint32_t client_id)
{
	IPC_INFO(ics->server, "UNIMPLEMENTED: system setting focused client to %d.", client_id);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_system_toggle_io_client(volatile struct ipc_client_state *_ics, uint32_t client_id)
{
	struct ipc_server *s = _ics->server;

	IPC_DEBUG(s, "System toggling io for client %u.", client_id);

	return ipc_server_toggle_io_client(s, client_id);
}

xrt_result_t
ipc_handle_system_set_client_io_blocks(volatile struct ipc_client_state *_ics,
                                       uint32_t client_id,
                                       const struct ipc_client_io_blocks *blocks)
{
	struct ipc_server *s = _ics->server;

	IPC_DEBUG(s,
	          "System setting io blocks for client %u. (block_poses=%s block_hand_tracking=%s block_inputs=%s "
	          "block_outputs=%s)",
	          client_id,                                      //
	          blocks->block_poses ? "true" : "false",         //
	          blocks->block_hand_tracking ? "true" : "false", //
	          blocks->block_inputs ? "true" : "false",        //
	          blocks->block_outputs ? "true" : "false");

	return ipc_server_set_client_io_blocks(s, client_id, blocks);
}

xrt_result_t
ipc_handle_system_get_client_session_running_state(volatile struct ipc_client_state *_ics,
                                                   uint32_t client_id,
                                                   struct xrt_compositor_session_running_state *out_running_state)
{
	struct ipc_server *s = _ics->server;

	return ipc_server_get_client_session_running_state(s, client_id, out_running_state);
}

xrt_result_t
ipc_handle_system_get_client_view_config(volatile struct ipc_client_state *_ics,
                                         uint32_t client_id,
                                         enum xrt_view_type view_type,
                                         struct xrt_view_config *out_default_view_config,
                                         struct xrt_recommended_view_config *out_recommended_view_config)
{
	struct ipc_server *s = _ics->server;

	return ipc_server_get_client_view_config(s, client_id, view_type, out_default_view_config,
	                                         out_recommended_view_config);
}

xrt_result_t
ipc_handle_system_set_client_recommended_view_config(volatile struct ipc_client_state *_ics,
                                                     uint32_t client_id,
                                                     enum xrt_view_type view_type,
                                                     const struct xrt_recommended_view_config *recommended_view_config)
{
	struct ipc_server *s = _ics->server;

	return ipc_server_set_client_recommended_view_config(s, client_id, view_type, recommended_view_config);
}


/*
 *
 * Compositor semaphore function..
 *
 */

xrt_result_t
ipc_handle_compositor_semaphore_create(volatile struct ipc_client_state *ics,
                                       uint32_t *out_id,
                                       uint32_t max_handle_count,
                                       xrt_graphics_sync_handle_t *out_handles,
                                       uint32_t *out_handle_count)
{
	xrt_result_t xret;

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	int id = 0;
	for (; id < IPC_MAX_CLIENT_SEMAPHORES; id++) {
		if (ics->xcsems[id] == NULL) {
			break;
		}
	}

	if (id == IPC_MAX_CLIENT_SEMAPHORES) {
		IPC_ERROR(ics->server, "Too many compositor semaphores alive!");
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_compositor_semaphore *xcsem = NULL;
	xrt_graphics_sync_handle_t handle = XRT_GRAPHICS_SYNC_HANDLE_INVALID;

	xret = xrt_comp_create_semaphore(ics->xc, &handle, &xcsem);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "Failed to create compositor semaphore!");
		return xret;
	}

	// Set it directly, no need to use reference here.
	ics->xcsems[id] = xcsem;

	// Set out parameters.
	*out_id = id;
	out_handles[0] = handle;
	*out_handle_count = 1;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_compositor_semaphore_destroy(volatile struct ipc_client_state *ics, uint32_t id)
{
	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	if (ics->xcsems[id] == NULL) {
		IPC_ERROR(ics->server, "Client tried to delete non-existent compositor semaphore!");
		return XRT_ERROR_IPC_FAILURE;
	}

	ics->compositor_semaphore_count--;

	// Drop our reference, does NULL checking. Cast away volatile.
	xrt_compositor_semaphore_reference((struct xrt_compositor_semaphore **)&ics->xcsems[id], NULL);

	return XRT_SUCCESS;
}


/*
 *
 * Tracking origin functions.
 *
 */

xrt_result_t
ipc_handle_tracking_origin_get_list(volatile struct ipc_client_state *ics, struct ipc_tracking_origin_list *out_list)
{
	// Count and collect origin IDs.
	uint32_t count = 0;
	for (uint32_t i = 0; i < XRT_SYSTEM_MAX_DEVICES; i++) {
		struct xrt_tracking_origin *xtrack = ics->objects.xtracks[i];
		if (xtrack != NULL) {
			out_list->origins[count].id = i;
			count++;
		}
	}

	out_list->origin_count = count;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_tracking_origin_get_info(volatile struct ipc_client_state *ics,
                                    uint32_t tracking_origin_id,
                                    struct ipc_tracking_origin_info *out_info)
{
	// Validate the tracking origin ID and get the tracking origin
	struct xrt_tracking_origin *xtrack = NULL;
	GET_XTRACK_OR_RETURN(ics, tracking_origin_id, xtrack);

	// Copy tracking origin info to output
	memcpy(out_info->name, xtrack->name, sizeof(out_info->name));
	out_info->type = xtrack->type;
	out_info->offset = xtrack->initial_offset;

	return XRT_SUCCESS;
}


/*
 *
 * Device functions.
 *
 */

static void
fill_device_info(struct xrt_device *xdev, uint32_t tracking_origin_id, struct ipc_device_info *out_info)
{
	// Synchronize the stable device ID to IPC clients.
	out_info->xrt_device_id_val = xdev->id.val;

	// Fill in basic device info
	out_info->name = xdev->name;
	out_info->device_type = xdev->device_type;
	out_info->tracking_origin_id = tracking_origin_id;
	memcpy(out_info->str, xdev->str, sizeof(out_info->str));
	memcpy(out_info->serial, xdev->serial, sizeof(out_info->serial));
	out_info->supported = xdev->supported;

	// Fill in input/output counts
	out_info->input_count = xdev->input_count;
	out_info->output_count = xdev->output_count;
}

xrt_result_t
ipc_handle_device_get_info(volatile struct ipc_client_state *ics, uint32_t device_id)
{
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	xrt_result_t xret = XRT_SUCCESS;

	// Validate the device ID and get the device
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	// Get tracking origin ID
	uint32_t tracking_origin_id = UINT32_MAX;
	xret = ipc_server_objects_get_xtrack_id_or_add(ics, xdev->tracking_origin, &tracking_origin_id);
	IPC_CHK_AND_RET(ics->server, xret, "ipc_server_objects_get_xtrack_id_or_add");

	// Reply structure.
	struct ipc_device_get_info_reply reply = XRT_STRUCT_INIT;

	// Use helper to fill device info
	fill_device_info(xdev, tracking_origin_id, &reply.info);

	// Send the device info.
	xret = ipc_send(imc, &reply, sizeof(reply));
	IPC_CHK_AND_RET(ics->server, xret, "ipc_send(device info)");

	// Send input names as varlen data
	if (xdev->input_count > 0) {
		// Temporary, freed on all paths.
		enum xrt_input_name *input_names = U_TYPED_ARRAY_CALLOC(enum xrt_input_name, xdev->input_count);
		for (size_t i = 0; i < xdev->input_count; i++) {
			input_names[i] = xdev->inputs[i].name;
		}
		xret = ipc_send(imc, input_names, sizeof(enum xrt_input_name) * xdev->input_count);
		free(input_names);
		IPC_CHK_AND_RET(ics->server, xret, "ipc_send(input names)");
	}

	// Send output names as varlen data
	if (xdev->output_count > 0) {
		// Temporary, freed on all paths.
		enum xrt_output_name *output_names = U_TYPED_ARRAY_CALLOC(enum xrt_output_name, xdev->output_count);
		for (size_t i = 0; i < xdev->output_count; i++) {
			output_names[i] = xdev->outputs[i].name;
		}
		xret = ipc_send(imc, output_names, sizeof(enum xrt_output_name) * xdev->output_count);
		free(output_names);
		IPC_CHK_AND_RET(ics->server, xret, "ipc_send(output names)");
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_device_get_info_no_arrays(volatile struct ipc_client_state *ics,
                                     uint32_t device_id,
                                     struct ipc_device_info *out_info)
{
	xrt_result_t xret = XRT_SUCCESS;
	struct xrt_device *xdev = NULL;

	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	// Get tracking origin ID
	uint32_t tracking_origin_id = UINT32_MAX;
	xret = ipc_server_objects_get_xtrack_id_or_add(ics, xdev->tracking_origin, &tracking_origin_id);
	IPC_CHK_AND_RET(ics->server, xret, "ipc_server_objects_get_xtrack_id_or_add");

	// Use helper to fill device info
	fill_device_info(xdev, tracking_origin_id, out_info);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_device_update_input(volatile struct ipc_client_state *ics, uint32_t id)
{
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	xrt_result_t xret;

	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	// Update inputs.
	xret = xrt_device_update_inputs(xdev);

	/*
	 * Reply structure, make sure to send the result. We need to tell the
	 * client if the update was successful or not. We can not return the
	 * result from the function call because we are sending varlen data.
	 *
	 * And if we return not success we disconnect the client, so we should
	 * only do that if there is a problem with the connection.
	 */
	struct ipc_result_reply reply = {
	    .result = xret,
	};

	xret = ipc_send(imc, &reply, sizeof(reply));
	IPC_CHK_AND_RET(ics->server, xret, "ipc_send(result reply)");

	xret = send_device_inputs(ics, xdev);
	IPC_CHK_AND_RET(ics->server, xret, "send_device_inputs");

	xret = send_device_outputs(ics, xdev);
	IPC_CHK_AND_RET(ics->server, xret, "send_device_outputs");

	return XRT_SUCCESS;
}

static struct xrt_input *
find_input(struct xrt_device *xdev, enum xrt_input_name name)
{
	for (uint32_t i = 0; i < xdev->input_count; i++) {
		if (xdev->inputs[i].name == name) {
			return &xdev->inputs[i];
		}
	}

	return NULL;
}

xrt_result_t
ipc_handle_device_get_tracked_pose(volatile struct ipc_client_state *ics,
                                   uint32_t id,
                                   enum xrt_input_name name,
                                   int64_t at_timestamp,
                                   struct xrt_space_relation *out_relation)
{
	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;

	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	// Find the input
	struct xrt_input *input = find_input(xdev, name);
	if (input == NULL) {
		return XRT_ERROR_IPC_FAILURE;
	}

	// Special case the headpose.
	bool disabled = ics->client_state.io_blocks.block_poses && name != XRT_INPUT_GENERIC_HEAD_POSE;
	bool active_on_client = input->active;

	// We have been disabled but the client hasn't called update.
	if (disabled && active_on_client) {
		U_ZERO(out_relation);
		return XRT_SUCCESS;
	}

	if (disabled || !active_on_client) {
		return XRT_ERROR_POSE_NOT_ACTIVE;
	}

	// Get the pose.
	return xrt_device_get_tracked_pose(xdev, name, at_timestamp, out_relation);
}

xrt_result_t
ipc_handle_device_get_hand_tracking(volatile struct ipc_client_state *ics,
                                    uint32_t id,
                                    enum xrt_input_name name,
                                    int64_t at_timestamp,
                                    struct xrt_hand_joint_set *out_value,
                                    int64_t *out_timestamp)
{
	if (ics->client_state.io_blocks.block_hand_tracking) {
		out_value->is_active = false;
		return XRT_SUCCESS;
	}

	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	// Get the pose.
	return xrt_device_get_hand_tracking(xdev, name, at_timestamp, out_value, out_timestamp);
}

xrt_result_t
ipc_handle_device_get_view_poses(volatile struct ipc_client_state *ics,
                                 uint32_t id,
                                 const struct xrt_vec3 *fallback_eye_relation,
                                 int64_t at_timestamp_ns,
                                 enum xrt_view_type view_type,
                                 uint32_t view_count)
{
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	struct ipc_device_get_view_poses_reply reply = XRT_STRUCT_INIT;
	struct ipc_server *s = ics->server;
	xrt_result_t xret;

	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);


	if (view_count == 0 || view_count > IPC_MAX_RAW_VIEWS) {
		IPC_ERROR(s, "Client asked for zero or too many views! (%u)", view_count);

		reply.result = XRT_ERROR_IPC_FAILURE;
		// Send the full reply, the client expects it.
		return ipc_send(imc, &reply, sizeof(reply));
	}

	// Data to get.
	struct xrt_fov fovs[IPC_MAX_RAW_VIEWS];
	struct xrt_pose poses[IPC_MAX_RAW_VIEWS];

	reply.result = xrt_device_get_view_poses( //
	    xdev,                                 //
	    fallback_eye_relation,                //
	    at_timestamp_ns,                      //
	    view_type,                            //
	    view_count,                           //
	    &reply.head_relation,                 //
	    fovs,                                 //
	    poses);                               //

	/*
	 * This isn't really needed, but demonstrates the server sending the
	 * length back in the reply, a common pattern for other functions.
	 */
	reply.view_count = view_count;

	/*
	 * Send the reply first isn't required for functions in general, but it
	 * will need to match what the client expects. This demonstrates the
	 * server sending the length back in the reply, a common pattern for
	 * other functions.
	 */
	xret = ipc_send(imc, &reply, sizeof(reply));
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(s, "Failed to send reply!");
		return xret;
	}

	// Send the fovs that we got.
	xret = ipc_send(imc, fovs, sizeof(struct xrt_fov) * view_count);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(s, "Failed to send fovs!");
		return xret;
	}

	// And finally the poses.
	xret = ipc_send(imc, poses, sizeof(struct xrt_pose) * view_count);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(s, "Failed to send poses!");
		return xret;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_device_get_view_poses_2(volatile struct ipc_client_state *ics,
                                   uint32_t id,
                                   const struct xrt_vec3 *default_eye_relation,
                                   int64_t at_timestamp_ns,
                                   enum xrt_view_type view_type,
                                   uint32_t view_count,
                                   struct ipc_info_get_view_poses_2 *out_info)
{
	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	return xrt_device_get_view_poses( //
	    xdev,                         //
	    default_eye_relation,         //
	    at_timestamp_ns,              //
	    view_type,                    //
	    view_count,                   //
	    &out_info->head_relation,     //
	    out_info->fovs,               //
	    out_info->poses);             //
}

xrt_result_t
ipc_handle_device_compute_distortion(volatile struct ipc_client_state *ics,
                                     uint32_t id,
                                     uint32_t view,
                                     float u,
                                     float v,
                                     struct xrt_uv_triplet *out_triplet)
{
	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	return xrt_device_compute_distortion(xdev, view, u, v, out_triplet);
}

xrt_result_t
ipc_handle_device_begin_plane_detection_ext(volatile struct ipc_client_state *ics,
                                            uint32_t id,
                                            uint64_t plane_detection_id,
                                            uint64_t *out_plane_detection_id)
{
	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	uint64_t new_count = ics->plane_detection_count + 1;

	if (new_count > ics->plane_detection_size) {
		IPC_TRACE(ics->server, "Plane detections tracking size: %u -> %u", (uint32_t)ics->plane_detection_count,
		          (uint32_t)new_count);

		U_ARRAY_REALLOC_OR_FREE(ics->plane_detection_ids, uint64_t, new_count);
		U_ARRAY_REALLOC_OR_FREE(ics->plane_detection_xdev, struct xrt_device *, new_count);
		ics->plane_detection_size = new_count;
	}

	struct xrt_plane_detector_begin_info_ext *begin_info = &get_ism(ics)->plane_begin_info_ext;

	enum xrt_result xret =
	    xrt_device_begin_plane_detection_ext(xdev, begin_info, plane_detection_id, out_plane_detection_id);
	if (xret != XRT_SUCCESS) {
		IPC_TRACE(ics->server, "xrt_device_begin_plane_detection_ext error: %d", xret);
		return xret;
	}

	if (*out_plane_detection_id != 0) {
		uint64_t index = ics->plane_detection_count;
		ics->plane_detection_ids[index] = *out_plane_detection_id;
		ics->plane_detection_xdev[index] = xdev;
		ics->plane_detection_count = new_count;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_device_destroy_plane_detection_ext(volatile struct ipc_client_state *ics,
                                              uint32_t id,
                                              uint64_t plane_detection_id)
{
	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	enum xrt_result xret = xrt_device_destroy_plane_detection_ext(xdev, plane_detection_id);

	// Iterate through plane detection ids. Once found, move every item one slot to the left.
	bool compact_right = false;
	for (uint32_t i = 0; i < ics->plane_detection_count; i++) {
		if (ics->plane_detection_ids[i] == plane_detection_id) {
			compact_right = true;
		}
		if (compact_right && (i + 1) < ics->plane_detection_count) {
			ics->plane_detection_ids[i] = ics->plane_detection_ids[i + 1];
			ics->plane_detection_xdev[i] = ics->plane_detection_xdev[i + 1];
		}
	}
	// if the plane detection was correctly tracked compact_right should always be true
	if (compact_right) {
		ics->plane_detection_count -= 1;
	} else {
		IPC_ERROR(ics->server, "Destroyed plane detection that was not tracked");
	}

	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "xrt_device_destroy_plane_detection_ext error: %d", xret);
		return xret;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_device_get_plane_detection_state_ext(volatile struct ipc_client_state *ics,
                                                uint32_t id,
                                                uint64_t plane_detection_id,
                                                enum xrt_plane_detector_state_ext *out_state)
{
	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	xrt_result_t xret = xrt_device_get_plane_detection_state_ext(xdev, plane_detection_id, out_state);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "xrt_device_get_plane_detection_state_ext error: %d", xret);
		return xret;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_device_get_plane_detections_ext(volatile struct ipc_client_state *ics,
                                           uint32_t id,
                                           uint64_t plane_detection_id)

{
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	struct ipc_device_get_plane_detections_ext_reply reply = XRT_STRUCT_INIT;
	struct ipc_server *s = ics->server;

	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	struct xrt_plane_detections_ext out = {0};

	xrt_result_t xret = xrt_device_get_plane_detections_ext(xdev, plane_detection_id, &out);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "xrt_device_get_plane_detections_ext error: %d", xret);
		// probably nothing allocated on error, but make sure
		xrt_plane_detections_ext_clear(&out);
		return xret;
	}

	reply.result = XRT_SUCCESS;
	reply.location_size = out.location_count; // because we initialized to 0, now size == count
	reply.polygon_size = out.polygon_info_size;
	reply.vertex_size = out.vertex_size;

	xret = ipc_send(imc, &reply, sizeof(reply));
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(s, "Failed to send reply!");
		goto out;
	}

	// send expected contents

	if (out.location_count > 0) {
		xret =
		    ipc_send(imc, out.locations, sizeof(struct xrt_plane_detector_location_ext) * out.location_count);
		if (xret != XRT_SUCCESS) {
			IPC_ERROR(s, "Failed to send locations!");
			goto out;
		}

		xret = ipc_send(imc, out.polygon_info_start_index, sizeof(uint32_t) * out.location_count);
		if (xret != XRT_SUCCESS) {
			IPC_ERROR(s, "Failed to send locations!");
			goto out;
		}
	}

	if (out.polygon_info_size > 0) {
		xret =
		    ipc_send(imc, out.polygon_infos, sizeof(struct xrt_plane_polygon_info_ext) * out.polygon_info_size);
		if (xret != XRT_SUCCESS) {
			IPC_ERROR(s, "Failed to send polygon_infos!");
			goto out;
		}
	}

	if (out.vertex_size > 0) {
		xret = ipc_send(imc, out.vertices, sizeof(struct xrt_vec2) * out.vertex_size);
		if (xret != XRT_SUCCESS) {
			IPC_ERROR(s, "Failed to send vertices!");
			goto out;
		}
	}

out:
	xrt_plane_detections_ext_clear(&out);
	return xret;
}

xrt_result_t
ipc_handle_device_set_output(volatile struct ipc_client_state *ics,
                             uint32_t id,
                             enum xrt_output_name name,
                             const struct xrt_output_value *value)
{
	if (ics->client_state.io_blocks.block_outputs) {
		return XRT_SUCCESS;
	}

	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	// Set the output.
	return xrt_device_set_output(xdev, name, value);
}

/*
 * Exchanges several messages with the client, so only hold the global state
 * lock around the device call, never across the client socket I/O.
 */
xrt_result_t
ipc_handle_device_set_haptic_output(volatile struct ipc_client_state *ics,
                                    uint32_t id,
                                    enum xrt_output_name name,
                                    const struct ipc_pcm_haptic_buffer *buffer)
{
	IPC_TRACE_MARKER();

	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	struct ipc_server *s = ics->server;

	xrt_result_t xret;

	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	float *samples = U_TYPED_ARRAY_CALLOC(float, buffer->num_samples);

	// send the allocation result
	xret = samples ? XRT_SUCCESS : XRT_ERROR_ALLOCATION;
	xret = ipc_send(imc, &xret, sizeof xret);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "Failed to send samples allocate result");
		goto set_haptic_output_end;
	}

	if (!samples) {
		IPC_ERROR(s, "Failed to allocate samples for haptic output");
		xret = XRT_ERROR_ALLOCATION;
		goto set_haptic_output_end;
	}

	xret = ipc_receive(imc, samples, sizeof(float) * buffer->num_samples);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(s, "Failed to receive samples");
		goto set_haptic_output_end;
	}

	uint32_t samples_consumed = 0;

	// Serializes PCM output across clients. No client socket I/O while holding the global state lock.
	os_mutex_lock(&ics->server->global_state.lock);

	// Blocked outputs still take the PCM. The device is not asked to play it.
	if (!ics->client_state.io_blocks.block_outputs) {
		struct xrt_output_value value = {
		    .type = XRT_OUTPUT_VALUE_TYPE_PCM_VIBRATION,
		    .pcm_vibration =
		        {
		            .append = buffer->append,
		            .buffer_size = buffer->num_samples,
		            .sample_rate = buffer->sample_rate,
		            .samples_consumed = &samples_consumed,
		            .buffer = samples,
		        },
		};

		// Set the output.
		xrt_device_set_output(xdev, name, &value);
	}

	os_mutex_unlock(&ics->server->global_state.lock);

	xret = ipc_send(imc, &samples_consumed, sizeof samples_consumed);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "Failed to send samples consumed");
		goto set_haptic_output_end;
	}

	xret = XRT_SUCCESS;

set_haptic_output_end:
	free(samples);

	return xret;
}

xrt_result_t
ipc_handle_device_get_output_limits(volatile struct ipc_client_state *ics,
                                    uint32_t id,
                                    struct xrt_output_limits *limits)
{
	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	// Set the output.
	return xrt_device_get_output_limits(xdev, limits);
}

xrt_result_t
ipc_handle_device_get_visibility_mask(volatile struct ipc_client_state *ics,
                                      uint32_t device_id,
                                      enum xrt_visibility_mask_type type,
                                      uint32_t view_index)
{
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	struct ipc_device_get_visibility_mask_reply reply = XRT_STRUCT_INIT;
	struct ipc_server *s = ics->server;
	xrt_result_t xret;

	// @todo verify
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	struct xrt_visibility_mask *mask = NULL;
	if (xdev->get_visibility_mask) {
		xret = xrt_device_get_visibility_mask(xdev, type, view_index, &mask);
		if (xret != XRT_SUCCESS) {
			IPC_ERROR(s, "Failed to get visibility mask");
			return xret;
		}
	} else {
		struct xrt_fov fov = xdev->hmd->distortion.fov[view_index];
		u_visibility_mask_get_default(type, &fov, &mask);
	}

	if (mask == NULL) {
		IPC_ERROR(s, "Failed to get visibility mask");
		reply.mask_size = 0;
	} else {
		reply.mask_size = xrt_visibility_mask_get_size(mask);
	}

	xret = ipc_send(imc, &reply, sizeof(reply));
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(s, "Failed to send reply");
		goto out_free;
	}

	// A zero-length Windows WriteFile is a message. Send the mask only when it has bytes.
	if (reply.mask_size != 0) {
		xret = ipc_send(imc, mask, reply.mask_size);
		if (xret != XRT_SUCCESS) {
			IPC_ERROR(s, "Failed to send mask");
			goto out_free;
		}
	}

out_free:
	free(mask);
	return xret;
}

xrt_result_t
ipc_handle_device_is_form_factor_available(volatile struct ipc_client_state *ics,
                                           uint32_t id,
                                           enum xrt_form_factor form_factor,
                                           bool *out_available)
{
	// To make the code a bit more readable.
	uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);

	return xrt_device_is_form_factor_available(xdev, form_factor, out_available);
}

xrt_result_t
ipc_handle_system_devices_get_list(volatile struct ipc_client_state *ics, struct ipc_device_list *out_list)
{
	// Count and collect device types
	uint32_t count = 0;
	for (uint32_t i = 0; i < XRT_SYSTEM_MAX_DEVICES; i++) {
		struct xrt_device *xdev = ics->objects.xdevs[i];
		if (xdev != NULL) {
			out_list->devices[count].id = i;
			out_list->devices[count].device_type = xdev->device_type;
			count++;
		}
	}

	out_list->device_count = count;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_system_devices_get_roles(volatile struct ipc_client_state *ics, struct xrt_system_roles *out_roles)
{
	return xrt_system_devices_get_roles(ics->server->xsysd, out_roles);
}

xrt_result_t
ipc_handle_system_devices_begin_feature(volatile struct ipc_client_state *ics, enum xrt_device_feature_type type)
{
	struct xrt_system_devices *xsysd = ics->server->xsysd;
	xrt_result_t xret;

	xret = validate_device_feature_type(ics, type);
	if (xret != XRT_SUCCESS) {
		return XRT_ERROR_IPC_FAILURE;
	}

	// Is this feature already used?
	if (ics->device_feature_used[type]) {
		IPC_ERROR(ics->server, "feature '%u' already used!", type);
		return XRT_ERROR_IPC_FAILURE;
	}

	xret = xrt_system_devices_feature_inc(xsysd, type);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "xrt_system_devices_feature_inc failed");
		return xret;
	}

	// Can now mark it as used.
	ics->device_feature_used[type] = true;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_system_devices_end_feature(volatile struct ipc_client_state *ics, enum xrt_device_feature_type type)
{
	struct xrt_system_devices *xsysd = ics->server->xsysd;
	xrt_result_t xret;

	xret = validate_device_feature_type(ics, type);
	if (xret != XRT_SUCCESS) {
		return XRT_ERROR_IPC_FAILURE;
	}

	if (!ics->device_feature_used[type]) {
		IPC_ERROR(ics->server, "feature '%u' not used!", type);
		return XRT_ERROR_IPC_FAILURE;
	}

	xret = xrt_system_devices_feature_dec(xsysd, type);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR(ics->server, "xrt_system_devices_feature_dec failed");
		return xret;
	}

	// Now we can mark it as not used.
	ics->device_feature_used[type] = false;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_device_get_face_tracking(volatile struct ipc_client_state *ics,
                                    uint32_t id,
                                    enum xrt_input_name facial_expression_type,
                                    int64_t at_timestamp_ns,
                                    struct xrt_facial_expression_set *out_value)
{
	const uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);
	// Get facial expression data.
	return xrt_device_get_face_tracking(xdev, facial_expression_type, at_timestamp_ns, out_value);
}

xrt_result_t
ipc_handle_device_get_face_calibration_state_android(volatile struct ipc_client_state *ics,
                                                     uint32_t id,
                                                     bool *out_face_is_calibrated)
{
	const uint32_t device_id = id;
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, device_id, xdev);
	return xrt_device_get_face_calibration_state_android(xdev, out_face_is_calibrated);
}

xrt_result_t
ipc_handle_device_get_body_skeleton(volatile struct ipc_client_state *ics,
                                    uint32_t id,
                                    enum xrt_input_name body_tracking_type,
                                    struct xrt_body_skeleton *out_value)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);
	return xrt_device_get_body_skeleton(xdev, body_tracking_type, out_value);
}

xrt_result_t
ipc_handle_device_get_body_joints(volatile struct ipc_client_state *ics,
                                  uint32_t id,
                                  enum xrt_input_name body_tracking_type,
                                  int64_t desired_timestamp_ns,
                                  struct xrt_body_joint_set *out_value)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);
	return xrt_device_get_body_joints(xdev, body_tracking_type, desired_timestamp_ns, out_value);
}

xrt_result_t
ipc_handle_device_reset_body_tracking_calibration_meta(volatile struct ipc_client_state *ics, uint32_t id)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);
	return xrt_device_reset_body_tracking_calibration_meta(xdev);
}

xrt_result_t
ipc_handle_device_set_body_tracking_calibration_override_meta(volatile struct ipc_client_state *ics,
                                                              uint32_t id,
                                                              float new_body_height)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);
	return xrt_device_set_body_tracking_calibration_override_meta(xdev, new_body_height);
}

xrt_result_t
ipc_handle_device_set_body_tracking_fidelity_meta(volatile struct ipc_client_state *ics,
                                                  uint32_t id,
                                                  enum xrt_body_tracking_fidelity_meta new_fidelity)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);
	return xrt_device_set_body_tracking_fidelity_meta(xdev, new_fidelity);
}

xrt_result_t
ipc_handle_device_get_battery_status(
    volatile struct ipc_client_state *ics, uint32_t id, bool *out_present, bool *out_charging, float *out_charge)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);
	return xrt_device_get_battery_status(xdev, out_present, out_charging, out_charge);
}

xrt_result_t
ipc_handle_device_get_brightness(volatile struct ipc_client_state *ics, uint32_t id, float *out_brightness)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);

	if (!xdev->supported.brightness_control) {
		return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	return xrt_device_get_brightness(xdev, out_brightness);
}

xrt_result_t
ipc_handle_device_set_brightness(volatile struct ipc_client_state *ics, uint32_t id, float brightness, bool relative)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);

	if (!xdev->supported.brightness_control) {
		return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	return xrt_device_set_brightness(xdev, brightness, relative);
}

xrt_result_t
ipc_handle_compositor_set_chroma_key_params(volatile struct ipc_client_state *ics,
                                            const struct xrt_colour_hsv_f32 *hsv_min,
                                            const struct xrt_colour_hsv_f32 *hsv_max,
                                            float curve,
                                            float despill)
{
	struct xrt_system_compositor *sysc = ics->server->xsysc;
	return (sysc->xmcc->set_base_chroma_key_params)(sysc, *hsv_min, *hsv_max, curve, despill);
}

xrt_result_t
ipc_handle_device_get_compositor_info(volatile struct ipc_client_state *ics,
                                      uint32_t id,
                                      const struct xrt_device_compositor_mode *mode,
                                      struct xrt_device_compositor_info *out_info)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);

	return xrt_device_get_compositor_info(xdev, mode, out_info);
}

xrt_result_t
ipc_handle_future_get_state(volatile struct ipc_client_state *ics, uint32_t future_id, enum xrt_future_state *out_state)
{
	struct xrt_future *xft = NULL;
	xrt_result_t xret = validate_future_id(ics, future_id, &xft);
	if (xret != XRT_SUCCESS) {
		return xret;
	}
	return xrt_future_get_state(xft, out_state);
}

xrt_result_t
ipc_handle_future_get_result(volatile struct ipc_client_state *ics,
                             uint32_t future_id,
                             struct xrt_future_result *out_ft_result)
{
	struct xrt_future *xft = NULL;
	xrt_result_t xret = validate_future_id(ics, future_id, &xft);
	if (xret != XRT_SUCCESS) {
		return xret;
	}
	return xrt_future_get_result(xft, out_ft_result);
}

xrt_result_t
ipc_handle_future_cancel(volatile struct ipc_client_state *ics, uint32_t future_id)
{
	struct xrt_future *xft = NULL;
	xrt_result_t xret = validate_future_id(ics, future_id, &xft);
	if (xret != XRT_SUCCESS) {
		return xret;
	}
	return xrt_future_cancel(xft);
}

xrt_result_t
ipc_handle_future_destroy(volatile struct ipc_client_state *ics, uint32_t future_id)
{
	return release_future(ics, future_id);
}
