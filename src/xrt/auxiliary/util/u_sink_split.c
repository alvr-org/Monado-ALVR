// Copyright 2019-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  An @ref xrt_frame_sink splitter.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "util/u_misc.h"
#include "util/u_sink.h"
#include "util/u_trace_marker.h"


/*!
 * An @ref xrt_frame_sink splitter.
 * @implements xrt_frame_sink
 * @implements xrt_frame_node
 */
struct u_sink_split
{
	struct xrt_frame_sink base;
	struct xrt_frame_node node;

	struct xrt_frame_sink *downstreams[U_SINK_MAX_SPLIT_DOWNSTREAMS];
	size_t downstream_count;
};

static void
split_frame(struct xrt_frame_sink *xfs, struct xrt_frame *xf)
{
	SINK_TRACE_MARKER();

	struct u_sink_split *s = (struct u_sink_split *)xfs;

	for (size_t i = 0; i < s->downstream_count; i++) {
		if (s->downstreams[i]) {
			xrt_sink_push_frame(s->downstreams[i], xf);
		}
	}
}

static void
split_break_apart(struct xrt_frame_node *node)
{
	// Noop
}

static void
split_destroy(struct xrt_frame_node *node)
{
	struct u_sink_split *s = container_of(node, struct u_sink_split, node);

	free(s);
}


/*
 *
 * Exported functions.
 *
 */

void
u_sink_split_multi_create(struct xrt_frame_context *xfctx,
                          struct xrt_frame_sink **downstreams,
                          size_t downstream_count,
                          struct xrt_frame_sink **out_xfs)
{
	assert(downstream_count <= U_SINK_MAX_SPLIT_DOWNSTREAMS);

	struct u_sink_split *s = U_TYPED_CALLOC(struct u_sink_split);

	s->base.push_frame = split_frame;
	s->node.break_apart = split_break_apart;
	s->node.destroy = split_destroy;

	memcpy(s->downstreams, downstreams, sizeof(s->downstreams[0]) * downstream_count);
	s->downstream_count = downstream_count;

	xrt_frame_context_add(xfctx, &s->node);

	*out_xfs = &s->base;
}

void
u_sink_split_create(struct xrt_frame_context *xfctx,
                    struct xrt_frame_sink *left,
                    struct xrt_frame_sink *right,
                    struct xrt_frame_sink **out_xfs)
{
	struct xrt_frame_sink *downstreams[2] = {left, right};

	u_sink_split_multi_create(xfctx, downstreams, 2, out_xfs);
}
