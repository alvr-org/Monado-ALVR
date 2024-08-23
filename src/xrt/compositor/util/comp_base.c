// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper implementation for native compositors.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup comp_util
 */

#include "util/comp_layer_accum.h"
#include "util/comp_sync.h"
#include "util/u_wait.h"
#include "util/u_trace_marker.h"

#include "util/comp_base.h"
#include "util/comp_semaphore.h"
#include "xrt/xrt_compositor.h"


/*
 *
 * Helper function.
 *
 */


/*
 *
 * xrt_compositor functions.
 *
 */

//! Delegates to code in `comp_swapchain`
static xrt_result_t
base_get_swapchain_create_properties(struct xrt_compositor *xc,
                                     const struct xrt_swapchain_create_info *info,
                                     struct xrt_swapchain_create_properties *xsccp)
{
	return comp_swapchain_get_create_properties(info, xsccp);
}

//! Delegates to code in `comp_swapchain`
static xrt_result_t
base_create_swapchain(struct xrt_compositor *xc,
                      const struct xrt_swapchain_create_info *info,
                      struct xrt_swapchain **out_xsc)
{
	struct comp_base *cb = comp_base(xc);

	/*
	 * In case the default get properties function have been overridden
	 * make sure to correctly dispatch the call to get the properties.
	 */
	struct xrt_swapchain_create_properties xsccp = {0};
	xrt_comp_get_swapchain_create_properties(xc, info, &xsccp);

	return comp_swapchain_create(&cb->vk, &cb->cscs, info, &xsccp, out_xsc);
}

//! Delegates to code in `comp_swapchain`
static xrt_result_t
base_import_swapchain(struct xrt_compositor *xc,
                      const struct xrt_swapchain_create_info *info,
                      struct xrt_image_native *native_images,
                      uint32_t image_count,
                      struct xrt_swapchain **out_xsc)
{
	struct comp_base *cb = comp_base(xc);

	return comp_swapchain_import(&cb->vk, &cb->cscs, info, native_images, image_count, out_xsc);
}

//! Delegates to code in `comp_sync`
static xrt_result_t
base_import_fence(struct xrt_compositor *xc, xrt_graphics_sync_handle_t handle, struct xrt_compositor_fence **out_xcf)
{
	struct comp_base *cb = comp_base(xc);

	return comp_fence_import(&cb->vk, handle, out_xcf);
}

//! Delegates to code in `comp_semaphore`
static xrt_result_t
base_create_semaphore(struct xrt_compositor *xc,
                      xrt_graphics_sync_handle_t *out_handle,
                      struct xrt_compositor_semaphore **out_xcsem)
{
	struct comp_base *cb = comp_base(xc);

	return comp_semaphore_create(&cb->vk, out_handle, out_xcsem);
}

static xrt_result_t
base_layer_begin(struct xrt_compositor *xc, const struct xrt_layer_frame_data *data)
{
	struct comp_base *cb = comp_base(xc);
	return comp_layer_accum_begin(&cb->layer_accum, data);
}

static xrt_result_t
base_layer_projection(struct xrt_compositor *xc,
                      struct xrt_device *xdev,
                      struct xrt_swapchain *xsc[XRT_MAX_VIEWS],
                      const struct xrt_layer_data *data)
{
	struct comp_base *cb = comp_base(xc);
	return comp_layer_accum_projection(&cb->layer_accum, xsc, data);
}

static xrt_result_t
base_layer_projection_depth(struct xrt_compositor *xc,
                            struct xrt_device *xdev,
                            struct xrt_swapchain *xsc[XRT_MAX_VIEWS],
                            struct xrt_swapchain *d_xsc[XRT_MAX_VIEWS],
                            const struct xrt_layer_data *data)
{
	struct comp_base *cb = comp_base(xc);
	return comp_layer_accum_projection_depth(&cb->layer_accum, xsc, d_xsc, data);
}

static xrt_result_t
base_layer_quad(struct xrt_compositor *xc,
                struct xrt_device *xdev,
                struct xrt_swapchain *xsc,
                const struct xrt_layer_data *data)
{
	struct comp_base *cb = comp_base(xc);
	return comp_layer_accum_quad(&cb->layer_accum, xsc, data);
}

static xrt_result_t
base_layer_cube(struct xrt_compositor *xc,
                struct xrt_device *xdev,
                struct xrt_swapchain *xsc,
                const struct xrt_layer_data *data)
{
	struct comp_base *cb = comp_base(xc);
	return comp_layer_accum_cube(&cb->layer_accum, xsc, data);
}

static xrt_result_t
base_layer_cylinder(struct xrt_compositor *xc,
                    struct xrt_device *xdev,
                    struct xrt_swapchain *xsc,
                    const struct xrt_layer_data *data)
{
	struct comp_base *cb = comp_base(xc);
	return comp_layer_accum_cylinder(&cb->layer_accum, xsc, data);
}

static xrt_result_t
base_layer_equirect1(struct xrt_compositor *xc,
                     struct xrt_device *xdev,
                     struct xrt_swapchain *xsc,
                     const struct xrt_layer_data *data)
{
	struct comp_base *cb = comp_base(xc);
	return comp_layer_accum_equirect1(&cb->layer_accum, xsc, data);
}

static xrt_result_t
base_layer_equirect2(struct xrt_compositor *xc,
                     struct xrt_device *xdev,
                     struct xrt_swapchain *xsc,
                     const struct xrt_layer_data *data)
{
	struct comp_base *cb = comp_base(xc);
	return comp_layer_accum_equirect2(&cb->layer_accum, xsc, data);
}

static xrt_result_t
base_wait_frame(struct xrt_compositor *xc,
                int64_t *out_frame_id,
                int64_t *out_predicted_display_time_ns,
                int64_t *out_predicted_display_period_ns)
{
	COMP_TRACE_MARKER();

	struct comp_base *cb = comp_base(xc);

	int64_t frame_id = -1;
	int64_t wake_up_time_ns = 0;
	int64_t predicted_gpu_time_ns = 0;

	xrt_comp_predict_frame(               //
	    xc,                               //
	    &frame_id,                        //
	    &wake_up_time_ns,                 //
	    &predicted_gpu_time_ns,           //
	    out_predicted_display_time_ns,    //
	    out_predicted_display_period_ns); //

	// Wait until the given wake up time.
	u_wait_until(&cb->sleeper, wake_up_time_ns);

	int64_t now_ns = os_monotonic_get_ns();

	// Signal that we woke up.
	xrt_comp_mark_frame(xc, frame_id, XRT_COMPOSITOR_FRAME_POINT_WOKE, now_ns);

	*out_frame_id = frame_id;

	return XRT_SUCCESS;
}


/*
 *
 * 'Exported' functions.
 *
 */

void
comp_base_init(struct comp_base *cb)
{
	struct xrt_compositor *iface = &cb->base.base;
	iface->get_swapchain_create_properties = base_get_swapchain_create_properties;
	iface->create_swapchain = base_create_swapchain;
	iface->import_swapchain = base_import_swapchain;
	iface->create_semaphore = base_create_semaphore;
	iface->import_fence = base_import_fence;
	iface->layer_begin = base_layer_begin;
	iface->layer_projection = base_layer_projection;
	iface->layer_projection_depth = base_layer_projection_depth;
	iface->layer_quad = base_layer_quad;
	iface->layer_cube = base_layer_cube;
	iface->layer_cylinder = base_layer_cylinder;
	iface->layer_equirect1 = base_layer_equirect1;
	iface->layer_equirect2 = base_layer_equirect2;
	iface->wait_frame = base_wait_frame;

	u_threading_stack_init(&cb->cscs.destroy_swapchains);

	os_precise_sleeper_init(&cb->sleeper);
}

void
comp_base_fini(struct comp_base *cb)
{
	os_precise_sleeper_deinit(&cb->sleeper);

	u_threading_stack_fini(&cb->cscs.destroy_swapchains);
}
