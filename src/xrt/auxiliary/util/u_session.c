// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_session.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "util/u_system.h"
#include "util/u_session.h"


/*
 *
 * Helpers.
 *
 */

static inline struct u_session *
u_session(struct xrt_session *xs)
{
	return (struct u_session *)xs;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
push_event(struct xrt_session_event_sink *xses, const union xrt_session_event *xse)
{
	struct u_session *us = container_of(xses, struct u_session, sink);

	u_session_event_push(us, xse);

	return XRT_SUCCESS;
}

static xrt_result_t
poll_events(struct xrt_session *xs, union xrt_session_event *out_xse)
{
	struct u_session *us = u_session(xs);

	u_session_event_pop(us, out_xse);

	return XRT_SUCCESS;
}

static void
destroy(struct xrt_session *xs)
{
	struct u_session *us = u_session(xs);

	if (us->usys != NULL) {
		u_system_remove_session(us->usys, &us->base, &us->sink);
	}

	free(xs);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct u_session *
u_session_create(struct u_system *usys)
{
	struct u_session *us = U_TYPED_CALLOC(struct u_session);

	// xrt_session fields.
	us->base.poll_events = poll_events;
	us->base.destroy = destroy;

	// xrt_session_event_sink fields.
	us->sink.push_event = push_event;

	// u_session fields.
	XRT_MAYBE_UNUSED int ret = os_mutex_init(&us->events.mutex);
	assert(ret == 0);
	us->usys = usys;

	// If we got a u_system.
	if (usys != NULL) {
		u_system_add_session(usys, &us->base, &us->sink);
	}

	return us;
}

void
u_session_event_push(struct u_session *us, const union xrt_session_event *xse)
{
	struct u_session_event *use = U_TYPED_CALLOC(struct u_session_event);
	use->xse = *xse;

	os_mutex_lock(&us->events.mutex);

	// Find the last slot.
	struct u_session_event **slot = &us->events.ptr;
	while (*slot != NULL) {
		slot = &(*slot)->next;
	}

	*slot = use;

	os_mutex_unlock(&us->events.mutex);
}

void
u_session_event_pop(struct u_session *us, union xrt_session_event *out_xse)
{
	U_ZERO(out_xse);
	out_xse->type = XRT_SESSION_EVENT_NONE;

	os_mutex_lock(&us->events.mutex);

	if (us->events.ptr != NULL) {
		struct u_session_event *use = us->events.ptr;

		*out_xse = use->xse;
		us->events.ptr = use->next;
		free(use);
	}

	os_mutex_unlock(&us->events.mutex);
}
