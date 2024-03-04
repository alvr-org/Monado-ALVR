// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Client side wrapper of compositor.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_compositor.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_config_os.h"


#include "os/os_time.h"

#include "util/u_misc.h"
#include "util/u_wait.h"
#include "util/u_handles.h"
#include "util/u_trace_marker.h"
#include "util/u_limited_unique_id.h"

#include "shared/ipc_protocol.h"
#include "client/ipc_client.h"
#include "ipc_client_generated.h"

#include <string.h>
#include <stdio.h>
#if !defined(XRT_OS_WINDOWS)
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <errno.h>
#include <assert.h>

#ifdef XRT_GRAPHICS_SYNC_HANDLE_IS_FD
#include <unistd.h>
#endif


/*
 *
 * Internal structs and helpers.
 *
 */

//! Define to test the loopback allocator.
#undef IPC_USE_LOOPBACK_IMAGE_ALLOCATOR

/*!
 * Client proxy for an xrt_compositor_native implementation over IPC.
 * @implements xrt_compositor_native
 */
struct ipc_client_compositor
{
	struct xrt_compositor_native base;

	//! Should be turned into its own object.
	struct xrt_system_compositor system;

	struct ipc_connection *ipc_c;

	//! Optional image allocator.
	struct xrt_image_native_allocator *xina;

	struct
	{
		//! Id that we are currently using for submitting layers.
		uint32_t slot_id;

		uint32_t layer_count;
	} layers;

	//! Has the native compositor been created, only supports one for now.
	bool compositor_created;

	//! To get better wake up in wait frame.
	struct os_precise_sleeper sleeper;

#ifdef IPC_USE_LOOPBACK_IMAGE_ALLOCATOR
	//! To test image allocator.
	struct xrt_image_native_allocator loopback_xina;
#endif
};

/*!
 * Client proxy for an xrt_swapchain_native implementation over IPC.
 * @implements xrt_swapchain_native
 */
struct ipc_client_swapchain
{
	struct xrt_swapchain_native base;

	struct ipc_client_compositor *icc;

	uint32_t id;
};

/*!
 * Client proxy for an xrt_compositor_semaphore implementation over IPC.
 * @implements xrt_compositor_semaphore
 */
struct ipc_client_compositor_semaphore
{
	struct xrt_compositor_semaphore base;

	struct ipc_client_compositor *icc;

	uint32_t id;
};


/*
 *
 * Helper functions.
 *
 */

static inline struct ipc_client_compositor *
ipc_client_compositor(struct xrt_compositor *xc)
{
	return (struct ipc_client_compositor *)xc;
}

static inline struct ipc_client_swapchain *
ipc_client_swapchain(struct xrt_swapchain *xs)
{
	return (struct ipc_client_swapchain *)xs;
}

static inline struct ipc_client_compositor_semaphore *
ipc_client_compositor_semaphore(struct xrt_compositor_semaphore *xcsem)
{
	return (struct ipc_client_compositor_semaphore *)xcsem;
}


/*
 *
 * Misc functions
 *
 */

static xrt_result_t
get_info(struct xrt_compositor *xc, struct xrt_compositor_info *out_info)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);

	xrt_result_t xret = ipc_call_compositor_get_info(icc->ipc_c, out_info);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_get_info");
}

static xrt_result_t
get_system_info(struct ipc_client_compositor *icc, struct xrt_system_compositor_info *out_info)
{
	xrt_result_t xret = ipc_call_system_compositor_get_info(icc->ipc_c, out_info);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_system_compositor_get_info");
}


/*
 *
 * Swapchain.
 *
 */

static void
ipc_compositor_swapchain_destroy(struct xrt_swapchain *xsc)
{
	struct ipc_client_swapchain *ics = ipc_client_swapchain(xsc);
	struct ipc_client_compositor *icc = ics->icc;
	xrt_result_t xret;

	xret = ipc_call_swapchain_destroy(icc->ipc_c, ics->id);

	// Can't return anything here, just continue.
	IPC_CHK_ONLY_PRINT(icc->ipc_c, xret, "ipc_call_compositor_semaphore_destroy");

	free(xsc);
}

static xrt_result_t
ipc_compositor_swapchain_wait_image(struct xrt_swapchain *xsc, uint64_t timeout_ns, uint32_t index)
{
	struct ipc_client_swapchain *ics = ipc_client_swapchain(xsc);
	struct ipc_client_compositor *icc = ics->icc;
	xrt_result_t xret;

	xret = ipc_call_swapchain_wait_image(icc->ipc_c, ics->id, timeout_ns, index);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_swapchain_wait_image");
}

static xrt_result_t
ipc_compositor_swapchain_acquire_image(struct xrt_swapchain *xsc, uint32_t *out_index)
{
	struct ipc_client_swapchain *ics = ipc_client_swapchain(xsc);
	struct ipc_client_compositor *icc = ics->icc;
	xrt_result_t xret;

	xret = ipc_call_swapchain_acquire_image(icc->ipc_c, ics->id, out_index);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_swapchain_acquire_image");
}

static xrt_result_t
ipc_compositor_swapchain_release_image(struct xrt_swapchain *xsc, uint32_t index)
{
	struct ipc_client_swapchain *ics = ipc_client_swapchain(xsc);
	struct ipc_client_compositor *icc = ics->icc;
	xrt_result_t xret;

	xret = ipc_call_swapchain_release_image(icc->ipc_c, ics->id, index);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_swapchain_release_image");
}


/*
 *
 * Compositor semaphore functions.
 *
 */

static xrt_result_t
ipc_client_compositor_semaphore_wait(struct xrt_compositor_semaphore *xcsem, uint64_t value, uint64_t timeout_ns)
{
	struct ipc_client_compositor_semaphore *iccs = ipc_client_compositor_semaphore(xcsem);
	struct ipc_client_compositor *icc = iccs->icc;

	IPC_ERROR(icc->ipc_c, "Cannot call wait on client side!");

	return XRT_ERROR_IPC_FAILURE;
}

static void
ipc_client_compositor_semaphore_destroy(struct xrt_compositor_semaphore *xcsem)
{
	struct ipc_client_compositor_semaphore *iccs = ipc_client_compositor_semaphore(xcsem);
	struct ipc_client_compositor *icc = iccs->icc;
	xrt_result_t xret;

	xret = ipc_call_compositor_semaphore_destroy(icc->ipc_c, iccs->id);

	// Can't return anything here, just continue.
	IPC_CHK_ONLY_PRINT(icc->ipc_c, xret, "ipc_call_compositor_semaphore_destroy");

	free(iccs);
}


/*
 *
 * Compositor functions.
 *
 */

static xrt_result_t
ipc_compositor_get_swapchain_create_properties(struct xrt_compositor *xc,
                                               const struct xrt_swapchain_create_info *info,
                                               struct xrt_swapchain_create_properties *xsccp)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_swapchain_get_properties(icc->ipc_c, info, xsccp);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_swapchain_get_properties");
}

static xrt_result_t
swapchain_server_create(struct ipc_client_compositor *icc,
                        const struct xrt_swapchain_create_info *info,
                        struct xrt_swapchain **out_xsc)
{
	xrt_graphics_buffer_handle_t remote_handles[XRT_MAX_SWAPCHAIN_IMAGES] = {0};
	xrt_result_t xret;
	uint32_t handle;
	uint32_t image_count;
	uint64_t size;
	bool use_dedicated_allocation;

	xret = ipc_call_swapchain_create( //
	    icc->ipc_c,                   // connection
	    info,                         // in
	    &handle,                      // out
	    &image_count,                 // out
	    &size,                        // out
	    &use_dedicated_allocation,    // out
	    remote_handles,               // handles
	    XRT_MAX_SWAPCHAIN_IMAGES);    // handles
	IPC_CHK_AND_RET(icc->ipc_c, xret, "ipc_call_swapchain_create");

	struct ipc_client_swapchain *ics = U_TYPED_CALLOC(struct ipc_client_swapchain);
	ics->base.base.image_count = image_count;
	ics->base.base.wait_image = ipc_compositor_swapchain_wait_image;
	ics->base.base.acquire_image = ipc_compositor_swapchain_acquire_image;
	ics->base.base.release_image = ipc_compositor_swapchain_release_image;
	ics->base.base.destroy = ipc_compositor_swapchain_destroy;
	ics->base.base.reference.count = 1;
	ics->base.limited_unique_id = u_limited_unique_id_get();
	ics->icc = icc;
	ics->id = handle;

	for (uint32_t i = 0; i < image_count; i++) {
		ics->base.images[i].handle = remote_handles[i];
		ics->base.images[i].size = size;
		ics->base.images[i].use_dedicated_allocation = use_dedicated_allocation;
	}

	*out_xsc = &ics->base.base;

	return XRT_SUCCESS;
}

static xrt_result_t
swapchain_server_import(struct ipc_client_compositor *icc,
                        const struct xrt_swapchain_create_info *info,
                        struct xrt_image_native *native_images,
                        uint32_t image_count,
                        struct xrt_swapchain **out_xsc)
{
	struct ipc_arg_swapchain_from_native args = {0};
	xrt_graphics_buffer_handle_t handles[XRT_MAX_SWAPCHAIN_IMAGES] = {0};
	xrt_result_t xret;
	uint32_t id = 0;

	for (uint32_t i = 0; i < image_count; i++) {
		handles[i] = native_images[i].handle;
		args.sizes[i] = native_images[i].size;

#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_WIN32_HANDLE)
		// DXGI handles need to be dealt with differently, they are identified
		// by having their lower bit set to 1 during transfer
		if (native_images[i].is_dxgi_handle) {
			handles[i] = (void *)((size_t)handles[i] | 1);
		}
#endif
	}

	// This does not consume the handles, it copies them.
	xret = ipc_call_swapchain_import( //
	    icc->ipc_c,                   // connection
	    info,                         // in
	    &args,                        // in
	    handles,                      // handles
	    image_count,                  // handles
	    &id);                         // out
	IPC_CHK_AND_RET(icc->ipc_c, xret, "ipc_call_swapchain_create");

	struct ipc_client_swapchain *ics = U_TYPED_CALLOC(struct ipc_client_swapchain);
	ics->base.base.image_count = image_count;
	ics->base.base.wait_image = ipc_compositor_swapchain_wait_image;
	ics->base.base.acquire_image = ipc_compositor_swapchain_acquire_image;
	ics->base.base.release_image = ipc_compositor_swapchain_release_image;
	ics->base.base.destroy = ipc_compositor_swapchain_destroy;
	ics->base.base.reference.count = 1;
	ics->base.limited_unique_id = u_limited_unique_id_get();
	ics->icc = icc;
	ics->id = id;

	// The handles were copied in the IPC call so we can reuse them here.
	for (uint32_t i = 0; i < image_count; i++) {
		ics->base.images[i] = native_images[i];
	}

	*out_xsc = &ics->base.base;

	return XRT_SUCCESS;
}

static xrt_result_t
swapchain_allocator_create(struct ipc_client_compositor *icc,
                           struct xrt_image_native_allocator *xina,
                           const struct xrt_swapchain_create_info *info,
                           struct xrt_swapchain **out_xsc)
{
	struct xrt_swapchain_create_properties xsccp = {0};
	struct xrt_image_native *images = NULL;
	xrt_result_t xret;

	// Get any needed properties.
	xret = ipc_compositor_get_swapchain_create_properties(&icc->base.base, info, &xsccp);
	IPC_CHK_AND_RET(icc->ipc_c, xret, "ipc_compositor_get_swapchain_create_properties");

	// Alloc the array of structs for the images.
	images = U_TYPED_ARRAY_CALLOC(struct xrt_image_native, xsccp.image_count);

	// Now allocate the images themselves
	xret = xrt_images_allocate(xina, info, xsccp.image_count, images);
	IPC_CHK_WITH_GOTO(icc->ipc_c, xret, "xrt_images_allocate", out_free);

	/*
	 * The import function takes ownership of the handles,
	 * we do not need free them if the call succeeds.
	 */
	xret = swapchain_server_import(icc, info, images, xsccp.image_count, out_xsc);
	IPC_CHK_ONLY_PRINT(icc->ipc_c, xret, "swapchain_server_import");
	if (xret != XRT_SUCCESS) {
		xrt_images_free(xina, xsccp.image_count, images);
	}

out_free:
	free(images);

	return xret;
}

static xrt_result_t
ipc_compositor_swapchain_create(struct xrt_compositor *xc,
                                const struct xrt_swapchain_create_info *info,
                                struct xrt_swapchain **out_xsc)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	struct xrt_image_native_allocator *xina = icc->xina;
	xrt_result_t xret;

	if (xina == NULL) {
		xret = swapchain_server_create(icc, info, out_xsc);
	} else {
		xret = swapchain_allocator_create(icc, xina, info, out_xsc);
	}

	// Errors already printed.
	return xret;
}

static xrt_result_t
ipc_compositor_create_passthrough(struct xrt_compositor *xc, const struct xrt_passthrough_create_info *info)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_compositor_create_passthrough(icc->ipc_c, info);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_create_passthrough");
}

static xrt_result_t
ipc_compositor_create_passthrough_layer(struct xrt_compositor *xc, const struct xrt_passthrough_layer_create_info *info)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_compositor_create_passthrough_layer(icc->ipc_c, info);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_create_passthrough_layer");
}

static xrt_result_t
ipc_compositor_destroy_passthrough(struct xrt_compositor *xc)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_compositor_destroy_passthrough(icc->ipc_c);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_destroy_passthrough");
}

static xrt_result_t
ipc_compositor_swapchain_import(struct xrt_compositor *xc,
                                const struct xrt_swapchain_create_info *info,
                                struct xrt_image_native *native_images,
                                uint32_t image_count,
                                struct xrt_swapchain **out_xsc)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);

	// Errors already printed.
	return swapchain_server_import(icc, info, native_images, image_count, out_xsc);
}

static xrt_result_t
ipc_compositor_semaphore_create(struct xrt_compositor *xc,
                                xrt_graphics_sync_handle_t *out_handle,
                                struct xrt_compositor_semaphore **out_xcsem)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_graphics_sync_handle_t handle = XRT_GRAPHICS_SYNC_HANDLE_INVALID;
	xrt_result_t xret;
	uint32_t id = 0;

	xret = ipc_call_compositor_semaphore_create(icc->ipc_c, &id, &handle, 1);
	IPC_CHK_AND_RET(icc->ipc_c, xret, "ipc_call_compositor_semaphore_create");

	struct ipc_client_compositor_semaphore *iccs = U_TYPED_CALLOC(struct ipc_client_compositor_semaphore);
	iccs->base.reference.count = 1;
	iccs->base.wait = ipc_client_compositor_semaphore_wait;
	iccs->base.destroy = ipc_client_compositor_semaphore_destroy;
	iccs->id = id;
	iccs->icc = icc;

	*out_handle = handle;
	*out_xcsem = &iccs->base;

	return XRT_SUCCESS;
}

static xrt_result_t
ipc_compositor_begin_session(struct xrt_compositor *xc, const struct xrt_begin_session_info *info)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	IPC_TRACE(icc->ipc_c, "Compositor begin session.");

	xret = ipc_call_session_begin(icc->ipc_c);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_session_begin");
}

static xrt_result_t
ipc_compositor_end_session(struct xrt_compositor *xc)
{
	IPC_TRACE_MARKER();

	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	IPC_TRACE(icc->ipc_c, "Compositor end session.");

	xret = ipc_call_session_end(icc->ipc_c);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_session_end");
}

static xrt_result_t
ipc_compositor_wait_frame(struct xrt_compositor *xc,
                          int64_t *out_frame_id,
                          uint64_t *out_predicted_display_time,
                          uint64_t *out_predicted_display_period)
{
	IPC_TRACE_MARKER();
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	int64_t frame_id = -1;
	uint64_t wake_up_time_ns = 0;
	uint64_t predicted_display_time = 0;
	uint64_t predicted_display_period = 0;

	xret = ipc_call_compositor_predict_frame( //
	    icc->ipc_c,                           // Connection
	    &frame_id,                            // Frame id
	    &wake_up_time_ns,                     // When we should wake up
	    &predicted_display_time,              // Display time
	    &predicted_display_period);           // Current period
	IPC_CHK_AND_RET(icc->ipc_c, xret, "ipc_call_compositor_predict_frame");

	// Wait until the given wake up time.
	u_wait_until(&icc->sleeper, wake_up_time_ns);

	// Signal that we woke up.
	xret = ipc_call_compositor_wait_woke(icc->ipc_c, frame_id);
	IPC_CHK_AND_RET(icc->ipc_c, xret, "ipc_call_compositor_wait_woke");

	// Only write arguments once we have fully waited.
	*out_frame_id = frame_id;
	*out_predicted_display_time = predicted_display_time;
	*out_predicted_display_period = predicted_display_period;

	return xret;
}

static xrt_result_t
ipc_compositor_begin_frame(struct xrt_compositor *xc, int64_t frame_id)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_compositor_begin_frame(icc->ipc_c, frame_id);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_begin_frame");
}

static xrt_result_t
ipc_compositor_layer_begin(struct xrt_compositor *xc, const struct xrt_layer_frame_data *data)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);

	struct ipc_shared_memory *ism = icc->ipc_c->ism;
	struct ipc_layer_slot *slot = &ism->slots[icc->layers.slot_id];

	slot->data = *data;

	return XRT_SUCCESS;
}

static xrt_result_t
ipc_compositor_layer_projection(struct xrt_compositor *xc,
                                struct xrt_device *xdev,
                                struct xrt_swapchain *xsc[XRT_MAX_VIEWS],
                                const struct xrt_layer_data *data)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);

	assert(data->type == XRT_LAYER_PROJECTION);

	struct ipc_shared_memory *ism = icc->ipc_c->ism;
	struct ipc_layer_slot *slot = &ism->slots[icc->layers.slot_id];
	struct ipc_layer_entry *layer = &slot->layers[icc->layers.layer_count];
	layer->xdev_id = 0; //! @todo Real id.
	layer->data = *data;
	for (uint32_t i = 0; i < data->view_count; ++i) {
		struct ipc_client_swapchain *ics = ipc_client_swapchain(xsc[i]);
		layer->swapchain_ids[i] = ics->id;
	}
	// Increment the number of layers.
	icc->layers.layer_count++;

	return XRT_SUCCESS;
}

static xrt_result_t
ipc_compositor_layer_projection_depth(struct xrt_compositor *xc,
                                      struct xrt_device *xdev,
                                      struct xrt_swapchain *xsc[XRT_MAX_VIEWS],
                                      struct xrt_swapchain *d_xsc[XRT_MAX_VIEWS],
                                      const struct xrt_layer_data *data)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);

	assert(data->type == XRT_LAYER_PROJECTION_DEPTH);

	struct ipc_shared_memory *ism = icc->ipc_c->ism;
	struct ipc_layer_slot *slot = &ism->slots[icc->layers.slot_id];
	struct ipc_layer_entry *layer = &slot->layers[icc->layers.layer_count];
	struct ipc_client_swapchain *xscn[XRT_MAX_VIEWS];
	struct ipc_client_swapchain *d_xscn[XRT_MAX_VIEWS];
	for (uint32_t i = 0; i < data->view_count; ++i) {
		xscn[i] = ipc_client_swapchain(xsc[i]);
		d_xscn[i] = ipc_client_swapchain(d_xsc[i]);

		layer->swapchain_ids[i] = xscn[i]->id;
		layer->swapchain_ids[i + data->view_count] = d_xscn[i]->id;
	}

	layer->xdev_id = 0; //! @todo Real id.

	layer->data = *data;

	// Increment the number of layers.
	icc->layers.layer_count++;

	return XRT_SUCCESS;
}

static xrt_result_t
handle_layer(struct xrt_compositor *xc,
             struct xrt_device *xdev,
             struct xrt_swapchain *xsc,
             const struct xrt_layer_data *data,
             enum xrt_layer_type type)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);

	assert(data->type == type);

	struct ipc_shared_memory *ism = icc->ipc_c->ism;
	struct ipc_layer_slot *slot = &ism->slots[icc->layers.slot_id];
	struct ipc_layer_entry *layer = &slot->layers[icc->layers.layer_count];
	struct ipc_client_swapchain *ics = ipc_client_swapchain(xsc);

	layer->xdev_id = 0; //! @todo Real id.
	layer->swapchain_ids[0] = ics->id;
	layer->swapchain_ids[1] = -1;
	layer->swapchain_ids[2] = -1;
	layer->swapchain_ids[3] = -1;
	layer->data = *data;

	// Increment the number of layers.
	icc->layers.layer_count++;

	return XRT_SUCCESS;
}

static xrt_result_t
ipc_compositor_layer_quad(struct xrt_compositor *xc,
                          struct xrt_device *xdev,
                          struct xrt_swapchain *xsc,
                          const struct xrt_layer_data *data)
{
	return handle_layer(xc, xdev, xsc, data, XRT_LAYER_QUAD);
}

static xrt_result_t
ipc_compositor_layer_cube(struct xrt_compositor *xc,
                          struct xrt_device *xdev,
                          struct xrt_swapchain *xsc,
                          const struct xrt_layer_data *data)
{
	return handle_layer(xc, xdev, xsc, data, XRT_LAYER_CUBE);
}

static xrt_result_t
ipc_compositor_layer_cylinder(struct xrt_compositor *xc,
                              struct xrt_device *xdev,
                              struct xrt_swapchain *xsc,
                              const struct xrt_layer_data *data)
{
	return handle_layer(xc, xdev, xsc, data, XRT_LAYER_CYLINDER);
}

static xrt_result_t
ipc_compositor_layer_equirect1(struct xrt_compositor *xc,
                               struct xrt_device *xdev,
                               struct xrt_swapchain *xsc,
                               const struct xrt_layer_data *data)
{
	return handle_layer(xc, xdev, xsc, data, XRT_LAYER_EQUIRECT1);
}

static xrt_result_t
ipc_compositor_layer_equirect2(struct xrt_compositor *xc,
                               struct xrt_device *xdev,
                               struct xrt_swapchain *xsc,
                               const struct xrt_layer_data *data)
{
	return handle_layer(xc, xdev, xsc, data, XRT_LAYER_EQUIRECT2);
}

static xrt_result_t
ipc_compositor_layer_passthrough(struct xrt_compositor *xc, struct xrt_device *xdev, const struct xrt_layer_data *data)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);

	assert(data->type == XRT_LAYER_PASSTHROUGH);

	struct ipc_shared_memory *ism = icc->ipc_c->ism;
	struct ipc_layer_slot *slot = &ism->slots[icc->layers.slot_id];
	struct ipc_layer_entry *layer = &slot->layers[icc->layers.layer_count];

	layer->xdev_id = 0; //! @todo Real id.
	layer->data = *data;

	// Increment the number of layers.
	icc->layers.layer_count++;

	return XRT_SUCCESS;
}

static xrt_result_t
ipc_compositor_layer_commit(struct xrt_compositor *xc, xrt_graphics_sync_handle_t sync_handle)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	bool valid_sync = xrt_graphics_sync_handle_is_valid(sync_handle);

	struct ipc_shared_memory *ism = icc->ipc_c->ism;
	struct ipc_layer_slot *slot = &ism->slots[icc->layers.slot_id];

	// Last bit of data to put in the shared memory area.
	slot->layer_count = icc->layers.layer_count;

	xret = ipc_call_compositor_layer_sync( //
	    icc->ipc_c,                        //
	    icc->layers.slot_id,               //
	    &sync_handle,                      //
	    valid_sync ? 1 : 0,                //
	    &icc->layers.slot_id);             //

	/*
	 * We are probably in a really bad state if we fail, at
	 * least print out the error and continue as best we can.
	 */
	IPC_CHK_ONLY_PRINT(icc->ipc_c, xret, "ipc_call_compositor_layer_sync_with_semaphore");

	// Reset.
	icc->layers.layer_count = 0;

	// Need to consume this handle.
	if (valid_sync) {
		u_graphics_sync_unref(&sync_handle);
	}

	return xret;
}

static xrt_result_t
ipc_compositor_layer_commit_with_semaphore(struct xrt_compositor *xc,
                                           struct xrt_compositor_semaphore *xcsem,
                                           uint64_t value)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	struct ipc_client_compositor_semaphore *iccs = ipc_client_compositor_semaphore(xcsem);
	xrt_result_t xret;

	struct ipc_shared_memory *ism = icc->ipc_c->ism;
	struct ipc_layer_slot *slot = &ism->slots[icc->layers.slot_id];

	// Last bit of data to put in the shared memory area.
	slot->layer_count = icc->layers.layer_count;

	xret = ipc_call_compositor_layer_sync_with_semaphore( //
	    icc->ipc_c,                                       //
	    icc->layers.slot_id,                              //
	    iccs->id,                                         //
	    value,                                            //
	    &icc->layers.slot_id);                            //

	/*
	 * We are probably in a really bad state if we fail, at
	 * least print out the error and continue as best we can.
	 */
	IPC_CHK_ONLY_PRINT(icc->ipc_c, xret, "ipc_call_compositor_layer_sync_with_semaphore");

	// Reset.
	icc->layers.layer_count = 0;

	return xret;
}

static xrt_result_t
ipc_compositor_discard_frame(struct xrt_compositor *xc, int64_t frame_id)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_compositor_discard_frame(icc->ipc_c, frame_id);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_discard_frame");
}

static xrt_result_t
ipc_compositor_set_performance_level(struct xrt_compositor *xc,
                                     enum xrt_perf_domain domain,
                                     enum xrt_perf_set_level level)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;
	xret = ipc_call_compositor_set_performance_level(icc->ipc_c, domain, level);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_set_performance_level");
}

static xrt_result_t
ipc_compositor_set_thread_hint(struct xrt_compositor *xc, enum xrt_thread_hint hint, uint32_t thread_id)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_compositor_set_thread_hint(icc->ipc_c, hint, thread_id);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_set_thread_hint");
}

static xrt_result_t
ipc_compositor_get_display_refresh_rate(struct xrt_compositor *xc, float *out_display_refresh_rate_hz)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_compositor_get_display_refresh_rate(icc->ipc_c, out_display_refresh_rate_hz);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_get_display_refresh_rate");
}

static xrt_result_t
ipc_compositor_request_display_refresh_rate(struct xrt_compositor *xc, float display_refresh_rate_hz)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);
	xrt_result_t xret;

	xret = ipc_call_compositor_request_display_refresh_rate(icc->ipc_c, display_refresh_rate_hz);
	IPC_CHK_ALWAYS_RET(icc->ipc_c, xret, "ipc_call_compositor_request_display_refresh_rate");
}

static void
ipc_compositor_destroy(struct xrt_compositor *xc)
{
	struct ipc_client_compositor *icc = ipc_client_compositor(xc);

	assert(icc->compositor_created);

	os_precise_sleeper_deinit(&icc->sleeper);

	icc->compositor_created = false;
}

static void
ipc_compositor_init(struct ipc_client_compositor *icc, struct xrt_compositor_native **out_xcn)
{
	icc->base.base.get_swapchain_create_properties = ipc_compositor_get_swapchain_create_properties;
	icc->base.base.create_swapchain = ipc_compositor_swapchain_create;
	icc->base.base.import_swapchain = ipc_compositor_swapchain_import;
	icc->base.base.create_semaphore = ipc_compositor_semaphore_create;
	icc->base.base.create_passthrough = ipc_compositor_create_passthrough;
	icc->base.base.create_passthrough_layer = ipc_compositor_create_passthrough_layer;
	icc->base.base.destroy_passthrough = ipc_compositor_destroy_passthrough;
	icc->base.base.begin_session = ipc_compositor_begin_session;
	icc->base.base.end_session = ipc_compositor_end_session;
	icc->base.base.wait_frame = ipc_compositor_wait_frame;
	icc->base.base.begin_frame = ipc_compositor_begin_frame;
	icc->base.base.discard_frame = ipc_compositor_discard_frame;
	icc->base.base.layer_begin = ipc_compositor_layer_begin;
	icc->base.base.layer_projection = ipc_compositor_layer_projection;
	icc->base.base.layer_projection_depth = ipc_compositor_layer_projection_depth;
	icc->base.base.layer_quad = ipc_compositor_layer_quad;
	icc->base.base.layer_cube = ipc_compositor_layer_cube;
	icc->base.base.layer_cylinder = ipc_compositor_layer_cylinder;
	icc->base.base.layer_equirect1 = ipc_compositor_layer_equirect1;
	icc->base.base.layer_equirect2 = ipc_compositor_layer_equirect2;
	icc->base.base.layer_passthrough = ipc_compositor_layer_passthrough;
	icc->base.base.layer_commit = ipc_compositor_layer_commit;
	icc->base.base.layer_commit_with_semaphore = ipc_compositor_layer_commit_with_semaphore;
	icc->base.base.destroy = ipc_compositor_destroy;
	icc->base.base.set_thread_hint = ipc_compositor_set_thread_hint;
	icc->base.base.get_display_refresh_rate = ipc_compositor_get_display_refresh_rate;
	icc->base.base.request_display_refresh_rate = ipc_compositor_request_display_refresh_rate;
	icc->base.base.set_performance_level = ipc_compositor_set_performance_level;

	// Using in wait frame.
	os_precise_sleeper_init(&icc->sleeper);

	// Fetch info from the compositor, among it the format format list.
	get_info(&(icc->base.base), &icc->base.base.info);

	*out_xcn = &icc->base;
}


/*
 *
 * Loopback image allocator.
 *
 */

#ifdef IPC_USE_LOOPBACK_IMAGE_ALLOCATOR
static inline xrt_result_t
ipc_compositor_images_allocate(struct xrt_image_native_allocator *xina,
                               const struct xrt_swapchain_create_info *xsci,
                               size_t in_image_count,
                               struct xrt_image_native *out_images)
{
	struct ipc_client_compositor *icc = container_of(xina, struct ipc_client_compositor, loopback_xina);

	int remote_fds[IPC_MAX_SWAPCHAIN_FDS] = {0};
	xrt_result_t xret;
	uint32_t image_count;
	uint32_t handle;
	uint64_t size;

	for (size_t i = 0; i < ARRAY_SIZE(remote_fds); i++) {
		remote_fds[i] = -1;
	}

	for (size_t i = 0; i < in_image_count; i++) {
		out_images[i].fd = -1;
		out_images[i].size = 0;
	}

	xret = ipc_call_swapchain_create( //
	    icc->ipc_c,                   // connection
	    xsci,                         // in
	    &handle,                      // out
	    &image_count,                 // out
	    &size,                        // out
	    remote_fds,                   // fds
	    IPC_MAX_SWAPCHAIN_FDS);       // fds
	IPC_CHK_AND_RET(icc->ipc_c, xret, "ipc_call_swapchain_create");

	/*
	 * It's okay to destroy it immediately, the native handles are
	 * now owned by us and we keep the buffers alive that way.
	 */
	xret = ipc_call_swapchain_destroy(icc->ipc_c, handle);
	assert(xret == XRT_SUCCESS);

	// Clumsy way of handling this.
	if (image_count < in_image_count) {
		for (uint32_t k = 0; k < image_count && k < in_image_count; k++) {
			/*
			 * Overly-broad condition: we know that any fd not touched by
			 * ipc_call_swapchain_create will be -1.
			 */
			if (remote_fds[k] >= 0) {
				close(remote_fds[k]);
				remote_fds[k] = -1;
			}
		}

		return XRT_ERROR_IPC_FAILURE;
	}

	// Copy up to in_image_count, or image_count what ever is lowest.
	uint32_t i = 0;
	for (; i < image_count && i < in_image_count; i++) {
		out_images[i].fd = remote_fds[i];
		out_images[i].size = size;
	}

	// Close any fds we are not interested in.
	for (; i < image_count; i++) {
		/*
		 * Overly-broad condition: we know that any fd not touched by
		 * ipc_call_swapchain_create will be -1.
		 */
		if (remote_fds[i] >= 0) {
			close(remote_fds[i]);
			remote_fds[i] = -1;
		}
	}

	return XRT_SUCCESS;
}

static inline xrt_result_t
ipc_compositor_images_free(struct xrt_image_native_allocator *xina,
                           size_t image_count,
                           struct xrt_image_native *out_images)
{
	for (uint32_t i = 0; i < image_count; i++) {
		close(out_images[i].fd);
		out_images[i].fd = -1;
		out_images[i].size = 0;
	}

	return XRT_SUCCESS;
}

static inline void
ipc_compositor_images_destroy(struct xrt_image_native_allocator *xina)
{
	// Noop
}
#endif


/*
 *
 * System compositor.
 *
 */

xrt_result_t
ipc_syscomp_create_native_compositor(struct xrt_system_compositor *xsc,
                                     const struct xrt_session_info *xsi,
                                     struct xrt_session_event_sink *xses,
                                     struct xrt_compositor_native **out_xcn)
{
	struct ipc_client_compositor *icc = container_of(xsc, struct ipc_client_compositor, system);

	IPC_ERROR(icc->ipc_c, "This function shouldn't be called!");

	return XRT_ERROR_IPC_FAILURE;
}

void
ipc_syscomp_destroy(struct xrt_system_compositor *xsc)
{
	struct ipc_client_compositor *icc = container_of(xsc, struct ipc_client_compositor, system);

	// Does null checking.
	xrt_images_destroy(&icc->xina);

	//! @todo Implement
	IPC_TRACE(icc->ipc_c, "NOT IMPLEMENTED compositor destroy.");

	free(icc);
}


/*
 *
 * 'Exported' functions.
 *
 */

xrt_result_t
ipc_client_create_native_compositor(struct xrt_system_compositor *xsysc,
                                    const struct xrt_session_info *xsi,
                                    struct xrt_compositor_native **out_xcn)
{
	struct ipc_client_compositor *icc = container_of(xsysc, struct ipc_client_compositor, system);
	xrt_result_t xret;

	if (icc->compositor_created) {
		return XRT_ERROR_MULTI_SESSION_NOT_IMPLEMENTED;
	}

	/*
	 * Needs to be done before init, we don't own the service side session
	 * the session does. But we create it here in case any extra arguments
	 * that only the compositor knows about needs to be sent.
	 */
	xret = ipc_call_session_create( //
	    icc->ipc_c,                 // ipc_c
	    xsi,                        // xsi
	    true);                      // create_native_compositor
	IPC_CHK_AND_RET(icc->ipc_c, xret, "ipc_call_session_create");

	// Needs to be done after session create call.
	ipc_compositor_init(icc, out_xcn);

	icc->compositor_created = true;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_client_create_system_compositor(struct ipc_connection *ipc_c,
                                    struct xrt_image_native_allocator *xina,
                                    struct xrt_device *xdev,
                                    struct xrt_system_compositor **out_xcs)
{
	struct ipc_client_compositor *c = U_TYPED_CALLOC(struct ipc_client_compositor);

	c->system.create_native_compositor = ipc_syscomp_create_native_compositor;
	c->system.destroy = ipc_syscomp_destroy;
	c->ipc_c = ipc_c;
	c->xina = xina;


#ifdef IPC_USE_LOOPBACK_IMAGE_ALLOCATOR
	c->loopback_xina.images_allocate = ipc_compositor_images_allocate;
	c->loopback_xina.images_free = ipc_compositor_images_free;
	c->loopback_xina.destroy = ipc_compositor_images_destroy;

	if (c->xina == NULL) {
		c->xina = &c->loopback_xina;
	}
#endif

	// Fetch info from the system compositor.
	get_system_info(c, &c->system.info);

	*out_xcs = &c->system;

	return XRT_SUCCESS;
}
