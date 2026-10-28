// Copyright 2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_session.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "b_system.h"
#include "b_session.h"


/*
 *
 * Helpers.
 *
 */

static inline struct b_session *
b_session(struct xrt_session *xs)
{
	return (struct b_session *)xs;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
push_event(struct xrt_session_event_sink *xses, const union xrt_session_event *xse)
{
	struct b_session *bs = container_of(xses, struct b_session, sink);

	b_session_event_push(bs, xse);

	return XRT_SUCCESS;
}

static xrt_result_t
poll_events(struct xrt_session *xs, union xrt_session_event *out_xse)
{
	struct b_session *bs = b_session(xs);

	b_session_event_pop(bs, out_xse);

	return XRT_SUCCESS;
}

static xrt_result_t
request_exit(struct xrt_session *xs)
{
	struct b_session *bs = b_session(xs);

	union xrt_session_event xse = XRT_STRUCT_INIT;
	xse.type = XRT_SESSION_EVENT_REQUEST_EXIT;

	b_session_event_push(bs, &xse);

	return XRT_SUCCESS;
}

static void
destroy(struct xrt_session *xs)
{
	struct b_session *bs = b_session(xs);

	if (bs->bsys != NULL) {
		b_system_remove_session(bs->bsys, &bs->base, &bs->sink);
		bs->bsys = NULL;
	}

	struct b_session_event *event = bs->events.ptr;
	while (event) {
		struct b_session_event *tmp = event->next;
		free(event);
		event = tmp;
	}
	bs->events.ptr = NULL;

	os_mutex_destroy(&bs->events.mutex);

	free(xs);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct b_session *
b_session_create(struct b_system *bsys)
{
	struct b_session *bs = U_TYPED_CALLOC(struct b_session);

	// xrt_session fields.
	bs->base.poll_events = poll_events;
	bs->base.request_exit = request_exit;
	bs->base.destroy = destroy;

	// xrt_session_event_sink fields.
	bs->sink.push_event = push_event;

	// b_session fields.
	XRT_MAYBE_UNUSED int ret = os_mutex_init(&bs->events.mutex);
	assert(ret == 0);
	bs->bsys = bsys;

	// If we got a b_system.
	if (bsys != NULL) {
		b_system_add_session(bsys, &bs->base, &bs->sink);
	}

	return bs;
}

void
b_session_event_push(struct b_session *bs, const union xrt_session_event *xse)
{
	struct b_session_event *use = U_TYPED_CALLOC(struct b_session_event);
	use->xse = *xse;

	os_mutex_lock(&bs->events.mutex);

	// Find the last slot.
	struct b_session_event **slot = &bs->events.ptr;
	while (*slot != NULL) {
		slot = &(*slot)->next;
	}

	*slot = use;

	os_mutex_unlock(&bs->events.mutex);
}

void
b_session_event_pop(struct b_session *bs, union xrt_session_event *out_xse)
{
	U_ZERO(out_xse);
	out_xse->type = XRT_SESSION_EVENT_NONE;

	os_mutex_lock(&bs->events.mutex);

	if (bs->events.ptr != NULL) {
		struct b_session_event *bse = bs->events.ptr;

		*out_xse = bse->xse;
		bs->events.ptr = bse->next;
		free(bse);
	}

	os_mutex_unlock(&bs->events.mutex);
}
