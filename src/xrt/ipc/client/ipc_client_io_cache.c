// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC client @ref xrt_io_cache proxy.
 * @ingroup ipc_client
 */

#include "client/ipc_client.h"
#include "client/ipc_client_connection.h"
#include "client/ipc_client_io_cache.h"

#include "ipc_client_generated.h"

#include "shared/ipc_message_channel.h"

#include "util/u_misc.h"


struct ipc_client_io_cache
{
	struct xrt_io_cache base;

	struct ipc_connection *ipc_c;

	uint32_t id;
};

static inline struct ipc_client_io_cache *
ipc_client_io_cache(struct xrt_io_cache *xic)
{
	return (struct ipc_client_io_cache *)xic;
}

static xrt_result_t
io_cache_get_info(struct xrt_io_cache *xic, struct xrt_io_cache_info *out_info)
{
	struct ipc_client_io_cache *icic = ipc_client_io_cache(xic);

	return ipc_call_io_cache_get_info(icic->ipc_c, icic->id, out_info);
}

static xrt_result_t
io_cache_read(struct xrt_io_cache *xic,
              uint64_t generation_id,
              uint32_t input_count,
              struct xrt_input *inputs,
              uint32_t output_count,
              struct xrt_output *outputs,
              bool *out_stale)
{
	struct ipc_client_io_cache *icic = ipc_client_io_cache(xic);
	struct ipc_connection *ipc_c = icic->ipc_c;

	xrt_result_t xret;
	bool stale = false;

	assert(out_stale != NULL);
	*out_stale = false;

	ipc_client_connection_lock(ipc_c);

	xret = ipc_send_io_cache_read_locked(ipc_c, icic->id, generation_id, input_count, output_count);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_send_io_cache_read_locked", io_cache_read_out);

	xret = ipc_receive_io_cache_read_locked(ipc_c, &stale);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive_io_cache_read_locked", io_cache_read_out);

	if (stale) {
		*out_stale = true;
		goto io_cache_read_out;
	}

	if (input_count > 0) {
		xret = ipc_receive(&ipc_c->imc, inputs, sizeof(struct xrt_input) * input_count);
		IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive: Receive io cache inputs", io_cache_read_out);
	}

	if (output_count > 0) {
		xret = ipc_receive(&ipc_c->imc, outputs, sizeof(struct xrt_output) * output_count);
		IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive: Receive io cache outputs", io_cache_read_out);
	}

io_cache_read_out:
	ipc_client_connection_unlock(ipc_c);
	return xret;
}

static xrt_result_t
io_cache_create_space(struct xrt_io_cache *xic,
                      struct xrt_device_id id_dev,
                      enum xrt_input_name name,
                      struct xrt_space **out_space)
{
	struct ipc_client_io_cache *icic = ipc_client_io_cache(xic);

	uint32_t space_id = 0;
	xrt_result_t xret = ipc_call_io_cache_create_space(icic->ipc_c, icic->id, &id_dev, name, &space_id);
	IPC_CHK_AND_RET(icic->ipc_c, xret, "ipc_call_io_cache_create_space");

	ipc_client_space_create(icic->ipc_c, space_id, out_space);

	return XRT_SUCCESS;
}

static xrt_result_t
io_cache_set_output(struct xrt_io_cache *xic,
                    struct xrt_device_id id_dev,
                    enum xrt_output_name name,
                    const struct xrt_output_value *value)
{
	struct ipc_client_io_cache *icic = ipc_client_io_cache(xic);

	return ipc_call_io_cache_set_output(icic->ipc_c, icic->id, &id_dev, name, value);
}

static xrt_result_t
io_cache_get_output_limits(struct xrt_io_cache *xic, struct xrt_device_id id_dev, struct xrt_output_limits *out_limits)
{
	struct ipc_client_io_cache *icic = ipc_client_io_cache(xic);

	return ipc_call_io_cache_get_output_limits(icic->ipc_c, icic->id, &id_dev, out_limits);
}

static void
io_cache_destroy(struct xrt_io_cache *xic)
{
	struct ipc_client_io_cache *icic = ipc_client_io_cache(xic);

	ipc_call_io_cache_destroy(icic->ipc_c, icic->id);

	free(icic);
}

xrt_result_t
ipc_client_io_cache_create(struct ipc_connection *ipc_c, uint32_t id, struct xrt_io_cache **out_xic)
{
	struct ipc_client_io_cache *icic = U_TYPED_CALLOC(struct ipc_client_io_cache);
	if (icic == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	icic->base.get_info = io_cache_get_info;
	icic->base.read = io_cache_read;
	icic->base.create_space = io_cache_create_space;
	icic->base.set_output = io_cache_set_output;
	icic->base.get_output_limits = io_cache_get_output_limits;
	icic->base.destroy = io_cache_destroy;
	icic->ipc_c = ipc_c;
	icic->id = id;

	*out_xic = &icic->base;

	return XRT_SUCCESS;
}
