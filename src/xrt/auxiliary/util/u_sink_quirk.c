// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  An @ref xrt_frame_sink that quirks frames.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "util/u_misc.h"
#include "util/u_sink.h"

/*!
 * An @ref xrt_frame_sink that quirks frames.
 * @implements xrt_frame_sink
 * @implements xrt_frame_node
 */
struct u_sink_quirk
{
	struct xrt_frame_sink base;
	struct xrt_frame_node node;

	struct xrt_frame_sink *downstream;
	struct xrt_frame_sink *right;

	struct u_sink_quirk_params params;
};

static void
quirk_frame(struct xrt_frame_sink *xfs, struct xrt_frame *xf)
{
	struct u_sink_quirk *q = (struct u_sink_quirk *)xfs;

	//! @todo this is not thread safe, but right no other thing has access
	//! to the frame (or should).

	if (q->params.stereo_sbs) {
		xf->stereo_format = XRT_STEREO_FORMAT_SBS;
	}

	if (q->params.leap_motion) {
		xf->stereo_format = XRT_STEREO_FORMAT_INTERLEAVED;
		xf->format = XRT_FORMAT_L8;
		xf->width *= 2;
	}

	if (q->params.ps4_cam) {
		// Stereo format.
		xf->stereo_format = XRT_STEREO_FORMAT_SBS;

		// Apply a offset.
		xf->data = &xf->data[32 + 64];

		switch (xf->width) {
		case 3448:
			xf->width = 1280 * 2;
			xf->height = 800;
			break;
		case 1748:
			xf->width = 640 * 2;
			xf->height = 400;
			break;
		case 898:
			xf->width = 320 * 2;
			xf->height = 192;
			break;
		default: break;
		}
	}

	if (q->params.bayer_as_l8 && xf->format == XRT_FORMAT_BAYER_GR8) {
		xf->format = XRT_FORMAT_L8;
	}

	q->downstream->push_frame(q->downstream, xf);
}

static void
break_apart(struct xrt_frame_node *node)
{}

static void
destroy(struct xrt_frame_node *node)
{
	struct u_sink_quirk *q = container_of(node, struct u_sink_quirk, node);

	free(q);
}


/*
 *
 * Exported functions.
 *
 */

void
u_sink_quirk_create(struct xrt_frame_context *xfctx,
                    struct xrt_frame_sink *downstream,
                    struct u_sink_quirk_params *params,
                    struct xrt_frame_sink **out_xfs)
{
	struct u_sink_quirk *q = U_TYPED_CALLOC(struct u_sink_quirk);

	q->base.push_frame = quirk_frame;
	q->node.break_apart = break_apart;
	q->node.destroy = destroy;
	q->downstream = downstream;

	q->params = *params;

	xrt_frame_context_add(xfctx, &q->node);

	*out_xfs = &q->base;
}
