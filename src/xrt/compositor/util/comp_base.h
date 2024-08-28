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

#pragma once

#include "util/comp_swapchain.h"
#include "util/comp_layer_accum.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Additional per-frame parameters.
 *
 * Independent of graphics API, swapchain implementation, etc.
 *
 * @ingroup comp_util
 */
struct comp_frame_params
{
	//! Special case one layer projection/projection-depth fast-path.
	bool one_projection_layer_fast_path;

	//! fov as reported by device for the current submit.
	struct xrt_fov fovs[XRT_MAX_VIEWS];
	//! absolute pose as reported by device for the current submit.
	struct xrt_pose poses[XRT_MAX_VIEWS];
};

/*!
 * A simple compositor base that handles a lot of things for you.
 *
 * Things it handles for you:
 *
 * - App swapchains
 * - App fences
 * - Vulkan bundle (needed for swapchains and fences)
 * - Layer tracking, not @ref xrt_compositor::layer_commit
 * - Wait function, not @ref xrt_compositor::predict_frame
 *
 * Functions it does not implement:
 *
 * - @ref xrt_compositor::begin_session
 * - @ref xrt_compositor::end_session
 * - @ref xrt_compositor::predict_frame
 * - @ref xrt_compositor::mark_frame
 * - @ref xrt_compositor::begin_frame
 * - @ref xrt_compositor::discard_frame
 * - @ref xrt_compositor::layer_commit
 * - @ref xrt_compositor::destroy
 *
 * Partially implements @ref xrt_compositor_native, meant to serve as
 * the base of a main compositor implementation.
 *
 * @implements xrt_compositor_native
 * @ingroup comp_util
 */
struct comp_base
{
	//! Base native compositor.
	struct xrt_compositor_native base;

	//! Vulkan bundle of useful things, used by swapchain and fence.
	struct vk_bundle vk;

	//! For default @ref xrt_compositor::wait_frame.
	struct os_precise_sleeper sleeper;

	//! Swapchain garbage collector, used by swapchain, child class needs to call.
	struct comp_swapchain_shared cscs;

	//! Collect layers for a single frame.
	struct comp_layer_accum layer_accum;

	//! Parameters for a single frame.
	struct comp_frame_params frame_params;
};


/*
 *
 * Helper functions.
 *
 */

/*!
 * Convenience function to convert an xrt_compositor to a comp_base.
 *
 * @private @memberof comp_base
 */
static inline struct comp_base *
comp_base(struct xrt_compositor *xc)
{
	return (struct comp_base *)xc;
}


/*
 *
 * 'Exported' functions.
 *
 */

/*!
 * Inits all of the supported functions and structs, except @ref vk_bundle.
 *
 * The bundle needs to be initialised before any of the implemented functions
 * are call, but is not required to be initialised before this function is
 * called.
 *
 * @protected @memberof comp_base
 */
void
comp_base_init(struct comp_base *cb);

/*!
 * De-initialises all structs, except @ref vk_bundle.
 *
 * The bundle needs to be de-initialised by the sub-class.
 *
 * @private @memberof comp_base
 */
void
comp_base_fini(struct comp_base *cb);


#ifdef __cplusplus
}
#endif
