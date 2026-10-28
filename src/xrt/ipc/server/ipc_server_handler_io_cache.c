// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC server handlers for @ref xrt_io_cache.
 * @ingroup ipc_server
 */

#include "xrt/xrt_io_cache.h"
#include "xrt/xrt_space.h"
#include "xrt/xrt_system.h"

#include "util/u_logging.h"
#include "util/u_misc.h"

#include "shared/ipc_message_channel.h"
#include "shared/ipc_protocol.h"
#include "server/ipc_server.h"
#include "server/ipc_server_objects.h"

#include "ipc_server_generated.h"



/*
 *
 * Helpers.
 *
 */

#define MAX_INPUTS_AND_OUTPUTS_COUNT 1024

#define FREE_AND_ZERO_MEMORY(PTR)                                                                                      \
	do {                                                                                                           \
		if (PTR != NULL) {                                                                                     \
			free(PTR);                                                                                     \
			PTR = NULL;                                                                                    \
		}                                                                                                      \
	} while (0)

#define GET_XIC_OR_RETURN(ICS, ID, XIC)                                                                                \
	do {                                                                                                           \
		xrt_result_t xret = ipc_server_objects_get_xic_and_validate((ICS), ID, &(XIC));                        \
		IPC_CHK_AND_RET((ICS)->server, xret, "ipc_server_objects_get_xic_and_validate");                       \
	} while (0)


/*
 *
 * Handle functions.
 *
 */

xrt_result_t
ipc_handle_io_cache_create(volatile struct ipc_client_state *ics, uint32_t *out_id)
{
	struct xrt_io_cache *xic = NULL;
	xrt_result_t xret = xrt_system_devices_create_io_cache( //
	    ics->server->xsysd,                                 //
	    ics->server->xso,                                   //
	    &xic);                                              //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	xret = ipc_server_objects_get_xic_id_or_add(ics, xic, out_id);
	if (xret != XRT_SUCCESS) {
		xrt_io_cache_destroy(&xic);
		return xret;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_io_cache_get_info(volatile struct ipc_client_state *ics, uint32_t id, struct xrt_io_cache_info *out_info)
{
	struct xrt_io_cache *xic = NULL;
	GET_XIC_OR_RETURN(ics, id, xic);

	return xrt_io_cache_get_info(xic, out_info);
}

xrt_result_t
ipc_handle_io_cache_read(volatile struct ipc_client_state *ics,
                         uint32_t id,
                         uint64_t generation_id,
                         uint32_t input_count,
                         uint32_t output_count)
{
	struct ipc_message_channel *imc = (struct ipc_message_channel *)&ics->imc;
	struct ipc_io_cache_read_reply reply = XRT_STRUCT_INIT;
	xrt_result_t xret = XRT_SUCCESS;
	struct xrt_input *inputs = NULL;
	struct xrt_output *outputs = NULL;

	// Basic validation, failure will disconnect the client.
	struct xrt_io_cache *xic = NULL;
	GET_XIC_OR_RETURN(ics, id, xic);

	if (input_count > MAX_INPUTS_AND_OUTPUTS_COUNT) {
		IPC_ERROR(ics->server, "Input count exceeds maximum allowed count (%u > %u)", input_count,
		          MAX_INPUTS_AND_OUTPUTS_COUNT);
		// Client validation error, disconnect the client.
		return XRT_ERROR_INVALID_ARGUMENT;
	}
	if (output_count > MAX_INPUTS_AND_OUTPUTS_COUNT) {
		IPC_ERROR(ics->server, "Output count exceeds maximum allowed count (%u > %u)", output_count,
		          MAX_INPUTS_AND_OUTPUTS_COUNT);
		// Client validation error, disconnect the client.
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	if (input_count > 0) {
		inputs = U_TYPED_ARRAY_CALLOC(struct xrt_input, input_count);
		if (inputs == NULL) {
			IPC_ERROR(ics->server, "Failed to allocate inputs");
			xret = XRT_ERROR_ALLOCATION;
		}
	}
	if (output_count > 0) {
		outputs = U_TYPED_ARRAY_CALLOC(struct xrt_output, output_count);
		if (outputs == NULL) {
			IPC_ERROR(ics->server, "Failed to allocate outputs");
			xret = XRT_ERROR_ALLOCATION;
		}
	}
	// Server error, inform the client about it.
	IPC_CHK_WITH_GOTO(ics->server, xret, "U_TYPED_ARRAY_CALLOC", error_send_reply);

	// This function validates the inputs and outputs, so we don't need to do it here.
	xret = xrt_io_cache_read( //
	    xic,                  //
	    generation_id,        //
	    input_count,          //
	    inputs,               //
	    output_count,         //
	    outputs,              //
	    &reply.stale);        //
	// Server error, inform the client about it.
	IPC_CHK_WITH_GOTO(ics->server, xret, "xrt_io_cache_read", error_send_reply);

	reply.result = XRT_SUCCESS;
	xret = ipc_send(imc, &reply, sizeof(reply));
	// Connection error, disconnect the client.
	IPC_CHK_WITH_GOTO(ics->server, xret, "ipc_send(reply)", error_disconnect_client);

	// The client doesn't expect any arrays for a stale read.
	if (reply.stale) {
		FREE_AND_ZERO_MEMORY(inputs);
		FREE_AND_ZERO_MEMORY(outputs);
		return XRT_SUCCESS;
	}

	if (input_count > 0) {
		xret = ipc_send(imc, inputs, sizeof(struct xrt_input) * input_count);
		// Connection error, disconnect the client.
		IPC_CHK_WITH_GOTO(ics->server, xret, "ipc_send(inputs)", error_disconnect_client);
	}
	if (output_count > 0) {
		xret = ipc_send(imc, outputs, sizeof(struct xrt_output) * output_count);
		// Connection error, disconnect the client.
		IPC_CHK_WITH_GOTO(ics->server, xret, "ipc_send(outputs)", error_disconnect_client);
	}

	FREE_AND_ZERO_MEMORY(inputs);
	FREE_AND_ZERO_MEMORY(outputs);

	return XRT_SUCCESS;

error_send_reply:
	// We got an error and want to tell the client about it.
	FREE_AND_ZERO_MEMORY(inputs);
	FREE_AND_ZERO_MEMORY(outputs);

	reply.result = xret;
	reply.stale = false;
	xret = ipc_send(imc, &reply, sizeof(reply));
	IPC_CHK_ALWAYS_RET(ics->server, xret, "ipc_send(reply)");

error_disconnect_client:
	// We got an error and want to disconnect the client by returning the error.
	FREE_AND_ZERO_MEMORY(inputs);
	FREE_AND_ZERO_MEMORY(outputs);
	return xret;
}

xrt_result_t
ipc_handle_io_cache_create_space(volatile struct ipc_client_state *ics,
                                 uint32_t id,
                                 const struct xrt_device_id *device_id,
                                 enum xrt_input_name name,
                                 uint32_t *out_space_id)
{
	struct xrt_io_cache *xic = NULL;
	GET_XIC_OR_RETURN(ics, id, xic);

	struct xrt_space *xs = NULL;
	xrt_result_t xret = xrt_io_cache_create_space(xic, *device_id, name, &xs);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	return ipc_server_objects_get_xspc_id_or_add(ics, xs, out_space_id);
}

xrt_result_t
ipc_handle_io_cache_set_output(volatile struct ipc_client_state *ics,
                               uint32_t id,
                               const struct xrt_device_id *device_id,
                               enum xrt_output_name name,
                               const struct xrt_output_value *value)
{
	struct xrt_io_cache *xic = NULL;
	GET_XIC_OR_RETURN(ics, id, xic);

	return xrt_io_cache_set_output(xic, *device_id, name, value);
}

xrt_result_t
ipc_handle_io_cache_get_output_limits(volatile struct ipc_client_state *ics,
                                      uint32_t id,
                                      const struct xrt_device_id *device_id,
                                      struct xrt_output_limits *out_limits)
{
	struct xrt_io_cache *xic = NULL;
	GET_XIC_OR_RETURN(ics, id, xic);

	return xrt_io_cache_get_output_limits(xic, *device_id, out_limits);
}

xrt_result_t
ipc_handle_io_cache_destroy(volatile struct ipc_client_state *ics, uint32_t id)
{
	return ipc_server_objects_destroy_xic(ics, id);
}

#undef GET_XIC_OR_RETURN
