// Copyright 2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Base timing source implementation.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup base
 */

#include "util/u_misc.h"

#include "os/os_threading.h"

#include "b_timing_source.h"


#define B_TIMING_SOURCE_MAX_SINKS 4

struct b_timing_source
{
	struct t_timing_event_source source;
	struct t_timing_event_sink sink;
	struct xrt_frame_node node;

	struct os_mutex mutex;

	bool running;

	struct t_timing_event_sink *sinks[B_TIMING_SOURCE_MAX_SINKS];
};


/*!
 * @ref t_timing_event_source implementation
 */

static struct b_timing_source *
from_source(struct t_timing_event_source *ttes)
{
	return container_of(ttes, struct b_timing_source, source);
}

static int
b_timing_source_add_sink(struct t_timing_event_source *ttes, struct t_timing_event_sink *sink)
{
	struct b_timing_source *bts = from_source(ttes);

	int ret = -1;

	os_mutex_lock(&bts->mutex);
	for (size_t i = 0; i < ARRAY_SIZE(bts->sinks); i++) {
		if (bts->sinks[i] == NULL) {
			bts->sinks[i] = sink;

			// Success
			ret = 0;
			break;
		}
	}
	os_mutex_unlock(&bts->mutex);

	return ret;
}

static void
b_timing_source_remove_sink(struct t_timing_event_source *ttes, struct t_timing_event_sink *sink)
{
	struct b_timing_source *bts = from_source(ttes);

	os_mutex_lock(&bts->mutex);
	for (size_t i = 0; i < ARRAY_SIZE(bts->sinks); i++) {
		if (bts->sinks[i] == sink) {
			bts->sinks[i] = NULL;
		}
	}
	os_mutex_unlock(&bts->mutex);
}

/*!
 * @ref t_timing_event_sink implementation
 */

static struct b_timing_source *
from_sink(struct t_timing_event_sink *sink)
{
	return container_of(sink, struct b_timing_source, sink);
}

/*!
 * Fan out @p event to every registered downstream sink.
 *
 * The mutex must be held for the entire fan-out, including while calling each
 * downstream `push_timing_event`. Timing sinks unregister themselves on
 * destruction via `remove_sink`; without the lock held across the callbacks, a
 * sink can be destroyed and freed on another thread while we still hold (or
 * call through) its pointer. Clever alternatives like copying the sink list and
 * unlocking before dispatch do not help: the copied pointers can still become
 * use-after-free once the owning object is destroyed.
 *
 * Because the mutex is non-recursive, downstream callbacks must not call
 * `add_sink` / `remove_sink` on this same `b_timing_source` (that would
 * deadlock). Sink registration is expected at setup / teardown time, not from
 * inside event delivery.
 */
static void
b_timing_source_push_timing_event(struct t_timing_event_sink *sink, const struct t_timing_event *event)
{
	struct b_timing_source *bts = from_sink(sink);

	// Held across dispatch so sinks cannot be destroyed under us; see above.
	os_mutex_lock(&bts->mutex);
	if (!bts->running) {
		os_mutex_unlock(&bts->mutex);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(bts->sinks); i++) {
		struct t_timing_event_sink *downstream = bts->sinks[i];

		if (downstream != NULL) {
			t_timing_event_sink_push_timing_event(downstream, event);
		}
	}
	os_mutex_unlock(&bts->mutex);
}

/*!
 * @ref xrt_frame_node implementation
 */

static struct b_timing_source *
from_node(struct xrt_frame_node *node)
{
	return container_of(node, struct b_timing_source, node);
}

static void
b_timing_source_break_apart(struct xrt_frame_node *node)
{
	struct b_timing_source *bts = from_node(node);

	os_mutex_lock(&bts->mutex);
	bts->running = false;
	os_mutex_unlock(&bts->mutex);
}

static void
b_timing_source_destroy(struct xrt_frame_node *node)
{
	struct b_timing_source *bts = from_node(node);

	os_mutex_destroy(&bts->mutex);

	free(bts);
}

/*
 * Exported functions
 */

xrt_result_t
b_timing_source_create(struct xrt_frame_context *xfctx,
                       struct t_timing_event_sink **out_sink,
                       struct t_timing_event_source **out_source)
{
	struct b_timing_source *bts = U_TYPED_CALLOC(struct b_timing_source);
	if (bts == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	if (os_mutex_init(&bts->mutex) < 0) {
		free(bts);
		return XRT_ERROR_SYNC_PRIMITIVE_CREATION_FAILED;
	}

	// @ref t_timing_event_sink
	bts->sink.push_timing_event = b_timing_source_push_timing_event;

	// @ref t_timing_event_source
	bts->source.add_sink = b_timing_source_add_sink;
	bts->source.remove_sink = b_timing_source_remove_sink;

	// @ref xrt_frame_node
	bts->node.break_apart = b_timing_source_break_apart;
	bts->node.destroy = b_timing_source_destroy;

	bts->running = true;

	xrt_frame_context_add(xfctx, &bts->node);

	*out_sink = &bts->sink;
	*out_source = &bts->source;
	return XRT_SUCCESS;
}
