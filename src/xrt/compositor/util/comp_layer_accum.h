// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Re-assemble a collection of composition layers submitted for a frame.
 *
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @ingroup comp_util
 */

#pragma once

#include "xrt/xrt_compositor.h"
#include "xrt/xrt_limits.h"
#include "xrt/xrt_results.h"
#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_swapchain;

/*!
 * A single layer in a @ref comp_layer_accum.
 *
 * Independent of graphics API, swapchain implementation, etc.
 *
 * @ingroup comp_util
 * @see comp_layer_accum
 */
struct comp_layer
{
	/*!
	 * Up to two compositor swapchains referenced per view (color and depth) for a layer.
	 *
	 * Unused elements should be set to null.
	 */
	struct xrt_swapchain *sc_array[XRT_MAX_VIEWS * 2];

	/*!
	 * All basic (trivially-serializable) data associated with a layer.
	 */
	struct xrt_layer_data data;
};


/*!
 * Get a (color) swapchain associated with a layer.
 *
 * @param cla self
 * @param swapchain_index index of swapchain - typically this is 0 for most layers, the view index for projection.

 * @public @memberof comp_layer
 */
struct xrt_swapchain *
comp_layer_get_swapchain(const struct comp_layer *cl, uint32_t swapchain_index);


/*!
 * Get a depth swapchain associated with a (projection with depth) layer
 *
 * @param cla self
 * @param swapchain_index index of **color** swapchain - typically this is the view index.
 *
 * @public @memberof comp_layer
 */
struct xrt_swapchain *
comp_layer_get_depth_swapchain(const struct comp_layer *cl, uint32_t swapchain_index);



/*!
 * Collect a stack of layers - one frame's worth.
 *
 * Independent of graphics API, swapchain implementation, etc.
 *
 * Used to turn the step by step "one call per layer" compositor API back
 * into a single structure per frame.
 *
 * @ingroup comp_util
 * @see comp_layer
 */
struct comp_layer_accum
{
	//! The per frame data, supplied by `begin`.
	struct xrt_layer_frame_data data;

	//! All of the layers.
	struct comp_layer layers[XRT_MAX_LAYERS];

	//! Number of submitted layers.
	uint32_t layer_count;
};

/*!
 * Reset all layer data and reset count to 0.
 *
 * Call at the beginning of a frame.
 *
 * @public @memberof comp_layer_accum
 */
xrt_result_t
comp_layer_accum_begin(struct comp_layer_accum *cla, const struct xrt_layer_frame_data *data);

/*!
 * Accumulate swapchains and data for a projection layer for a frame.
 *
 * @public @memberof comp_layer_accum
 */
xrt_result_t
comp_layer_accum_projection(struct comp_layer_accum *cla,
                            struct xrt_swapchain *xsc[XRT_MAX_VIEWS],
                            const struct xrt_layer_data *data);

/*!
 * Accumulate swapchains and data for a projection layer (with depth image) for a frame.
 *
 * @public @memberof comp_layer_accum
 */
xrt_result_t
comp_layer_accum_projection_depth(struct comp_layer_accum *cla,
                                  struct xrt_swapchain *xsc[XRT_MAX_VIEWS],
                                  struct xrt_swapchain *d_xsc[XRT_MAX_VIEWS],
                                  const struct xrt_layer_data *data);
/*!
 * Accumulate swapchain and data for a quad layer for a frame.
 *
 * @public @memberof comp_layer_accum
 */
xrt_result_t
comp_layer_accum_quad(struct comp_layer_accum *cla, struct xrt_swapchain *xsc, const struct xrt_layer_data *data);


/*!
 * Accumulate swapchain and data for a cube layer for a frame.
 *
 * @public @memberof comp_layer_accum
 */
xrt_result_t
comp_layer_accum_cube(struct comp_layer_accum *cla, struct xrt_swapchain *xsc, const struct xrt_layer_data *data);


/*!
 * Accumulate swapchain and data for a cylinder layer for a frame.
 *
 * @public @memberof comp_layer_accum
 */
xrt_result_t
comp_layer_accum_cylinder(struct comp_layer_accum *cla, struct xrt_swapchain *xsc, const struct xrt_layer_data *data);


/*!
 * Accumulate swapchain and data for an equirect(1) layer for a frame.
 *
 * @public @memberof comp_layer_accum
 */
xrt_result_t
comp_layer_accum_equirect1(struct comp_layer_accum *cla, struct xrt_swapchain *xsc, const struct xrt_layer_data *data);


/*!
 * Accumulate swapchain and data for an equirect2 layer for a frame.
 *
 * @public @memberof comp_layer_accum
 */
xrt_result_t
comp_layer_accum_equirect2(struct comp_layer_accum *cla, struct xrt_swapchain *xsc, const struct xrt_layer_data *data);


/*!
 * Get a (color) swapchain associated with a layer.
 *
 * @param cla self
 * @param layer_index index of layer
 * @param swapchain_index index of swapchain - typically this is 0 for most layers, the view index for projection.

 * @public @memberof comp_layer_accum
 */
inline struct xrt_swapchain *
comp_layer_accum_get_swapchain(const struct comp_layer_accum *cla, uint32_t layer_index, uint32_t swapchain_index)
{
	assert(layer_index < cla->layer_count);
	return cla->layers[layer_index].sc_array[swapchain_index];
}

/*!
 * Get a depth swapchain associated with a (projection with depth) layer
 *
 * @param cla self
 * @param layer_index index of layer
 * @param swapchain_index index of **color** swapchain - typically this is the view index.
 *
 * @public @memberof comp_layer_accum
 */
inline struct xrt_swapchain *
comp_layer_accum_get_depth_swapchain(const struct comp_layer_accum *cla, uint32_t layer_index, uint32_t swapchain_index)
{
	assert(layer_index < cla->layer_count);
	assert(cla->layers[layer_index].data.type == XRT_LAYER_PROJECTION_DEPTH);
	return cla->layers[layer_index].sc_array[XRT_MAX_VIEWS + swapchain_index];
}

#ifdef __cplusplus
}
#endif
