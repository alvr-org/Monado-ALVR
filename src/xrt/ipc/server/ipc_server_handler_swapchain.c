// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Swapchain handling functions called from generated dispatch function.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup ipc_server
 */

#include "util/u_misc.h"
#include "util/u_trace_marker.h"

#include "server/ipc_server.h"
#include "ipc_server_generated.h"
#include "xrt/xrt_results.h"


/*
 *
 * Helper functions.
 *
 */

static xrt_result_t
validate_swapchain_state(volatile struct ipc_client_state *ics, uint32_t *out_index)
{
	// Our handle is just the index for now.
	uint32_t index = 0;
	for (; index < IPC_MAX_CLIENT_SWAPCHAINS; index++) {
		if (!ics->swapchain_data[index].active) {
			break;
		}
	}

	if (index >= IPC_MAX_CLIENT_SWAPCHAINS) {
		IPC_ERROR(ics->server, "Too many swapchains!");
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_index = index;

	return XRT_SUCCESS;
}

static void
set_swapchain_info(volatile struct ipc_client_state *ics,
                   uint32_t index,
                   const struct xrt_swapchain_create_info *info,
                   struct xrt_swapchain *xsc)
{
	ics->xscs[index] = xsc;
	ics->swapchain_data[index].active = true;
	ics->swapchain_data[index].width = info->width;
	ics->swapchain_data[index].height = info->height;
	ics->swapchain_data[index].format = info->format;
	ics->swapchain_data[index].image_count = xsc->image_count;
}


/*
 *
 * Handle functions.
 *
 */

xrt_result_t
ipc_handle_swapchain_get_properties(volatile struct ipc_client_state *ics,
                                    const struct xrt_swapchain_create_info *info,
                                    struct xrt_swapchain_create_properties *xsccp)
{
	IPC_TRACE_MARKER();

	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	return xrt_comp_get_swapchain_create_properties(ics->xc, info, xsccp);
}

xrt_result_t
ipc_handle_swapchain_create(volatile struct ipc_client_state *ics,
                            const struct xrt_swapchain_create_info *info,
                            uint32_t *out_id,
                            uint32_t *out_image_count,
                            uint64_t *out_size,
                            bool *out_use_dedicated_allocation,
                            uint32_t max_handle_capacity,
                            xrt_graphics_buffer_handle_t *out_handles,
                            uint32_t *out_handle_count)
{
	IPC_TRACE_MARKER();

	xrt_result_t xret = XRT_SUCCESS;
	uint32_t index = 0;

	xret = validate_swapchain_state(ics, &index);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	// Create the swapchain
	struct xrt_swapchain *xsc = NULL; // Has to be NULL.
	xret = xrt_comp_create_swapchain(ics->xc, info, &xsc);
	if (xret != XRT_SUCCESS) {
		if (xret == XRT_ERROR_SWAPCHAIN_FLAG_VALID_BUT_UNSUPPORTED) {
			IPC_WARN(ics->server,
			         "xrt_comp_create_swapchain: Attempted to create valid, but unsupported swapchain");
		} else {
			IPC_ERROR(ics->server, "Error xrt_comp_create_swapchain failed!");
		}
		return xret;
	}

	// It's now safe to increment the number of swapchains.
	ics->swapchain_count++;

	IPC_TRACE(ics->server, "Created swapchain %d.", index);

	set_swapchain_info(ics, index, info, xsc);

	// return our result to the caller.
	struct xrt_swapchain_native *xscn = (struct xrt_swapchain_native *)xsc;

	// Limit checking
	assert(xsc->image_count <= XRT_MAX_SWAPCHAIN_IMAGES);
	assert(xsc->image_count <= max_handle_capacity);

	for (size_t i = 1; i < xsc->image_count; i++) {
		assert(xscn->images[0].size == xscn->images[i].size);
		assert(xscn->images[0].use_dedicated_allocation == xscn->images[i].use_dedicated_allocation);
	}

	// Assuming all images allocated in the same swapchain have the same allocation requirements.
	*out_size = xscn->images[0].size;
	*out_use_dedicated_allocation = xscn->images[0].use_dedicated_allocation;
	*out_id = index;
	*out_image_count = xsc->image_count;

	// Setup the fds.
	*out_handle_count = xsc->image_count;
	for (size_t i = 0; i < xsc->image_count; i++) {
		out_handles[i] = xscn->images[i].handle;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_swapchain_import(volatile struct ipc_client_state *ics,
                            const struct xrt_swapchain_create_info *info,
                            const struct ipc_arg_swapchain_from_native *args,
                            uint32_t *out_id,
                            const xrt_graphics_buffer_handle_t *handles,
                            uint32_t handle_count)
{
	IPC_TRACE_MARKER();

	xrt_result_t xret = XRT_SUCCESS;
	uint32_t index = 0;

	xret = validate_swapchain_state(ics, &index);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	struct xrt_image_native xins[XRT_MAX_SWAPCHAIN_IMAGES] = XRT_STRUCT_INIT;
	for (uint32_t i = 0; i < handle_count; i++) {
		xins[i].handle = handles[i];
		xins[i].size = args->sizes[i];
#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_WIN32_HANDLE)
		// DXGI handles need to be dealt with differently, they are identified
		// by having their lower bit set to 1 during transfer
		if ((size_t)xins[i].handle & 1) {
			xins[i].handle = (HANDLE)((size_t)xins[i].handle - 1);
			xins[i].is_dxgi_handle = true;
		}
#endif
	}

	// create the swapchain
	struct xrt_swapchain *xsc = NULL;
	xret = xrt_comp_import_swapchain(ics->xc, info, xins, handle_count, &xsc);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	// It's now safe to increment the number of swapchains.
	ics->swapchain_count++;

	IPC_TRACE(ics->server, "Created swapchain %d.", index);

	set_swapchain_info(ics, index, info, xsc);
	*out_id = index;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_swapchain_wait_image(volatile struct ipc_client_state *ics, uint32_t id, int64_t timeout_ns, uint32_t index)
{
	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	//! @todo Look up the index.
	uint32_t sc_index = id;
	struct xrt_swapchain *xsc = ics->xscs[sc_index];

	return xrt_swapchain_wait_image(xsc, timeout_ns, index);
}

xrt_result_t
ipc_handle_swapchain_acquire_image(volatile struct ipc_client_state *ics, uint32_t id, uint32_t *out_index)
{
	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	//! @todo Look up the index.
	uint32_t sc_index = id;
	struct xrt_swapchain *xsc = ics->xscs[sc_index];

	xrt_swapchain_acquire_image(xsc, out_index);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_swapchain_release_image(volatile struct ipc_client_state *ics, uint32_t id, uint32_t index)
{
	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	//! @todo Look up the index.
	uint32_t sc_index = id;
	struct xrt_swapchain *xsc = ics->xscs[sc_index];

	xrt_swapchain_release_image(xsc, index);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_swapchain_destroy(volatile struct ipc_client_state *ics, uint32_t id)
{
	if (ics->xc == NULL) {
		return XRT_ERROR_IPC_SESSION_NOT_CREATED;
	}

	ics->swapchain_count--;

	// Drop our reference, does NULL checking. Cast away volatile.
	xrt_swapchain_reference((struct xrt_swapchain **)&ics->xscs[id], NULL);
	ics->swapchain_data[id].active = false;

	return XRT_SUCCESS;
}
