// Copyright 2023, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_system.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_compositor.h"

#include "os/os_threading.h"

#include "util/u_misc.h"
#include "util/u_logging.h"

#include "b_system.h"
#include "b_session.h"

#include <stdio.h>


/*
 *
 * Helpers.
 *
 */

static inline struct b_system *
b_system(struct xrt_system *xsys)
{
	return (struct b_system *)xsys;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
push_event(struct xrt_session_event_sink *xses, const union xrt_session_event *xse)
{
	struct b_system *bsys = container_of(xses, struct b_system, broadcast);

	b_system_broadcast_event(bsys, xse);

	return XRT_SUCCESS;
}

static xrt_result_t
create_session(struct xrt_system *xsys,
               const struct xrt_session_info *xsi,
               struct xrt_session **out_xs,
               struct xrt_compositor_native **out_xcn)
{
	struct b_system *bsys = b_system(xsys);
	xrt_result_t xret = XRT_SUCCESS;

	if (out_xcn != NULL && bsys->xsysc == NULL) {
		U_LOG_E("No system compositor in system, can't create native compositor.");
		return XRT_ERROR_COMPOSITOR_NOT_SUPPORTED;
	}

	struct b_session *bs = b_session_create(bsys);

	// Skip making a native compositor if not asked for.
	if (out_xcn == NULL) {
		goto out_session;
	}

	xret = xrt_syscomp_create_native_compositor( //
	    bsys->xsysc,                             //
	    xsi,                                     //
	    &bs->sink,                               //
	    out_xcn);                                //
	if (xret != XRT_SUCCESS) {
		goto err;
	}

out_session:
	*out_xs = &bs->base;

	return XRT_SUCCESS;

err:
	bs->base.destroy(&bs->base);
	return xret;
}

static void
destroy(struct xrt_system *xsys)
{
	struct b_system *bsys = b_system(xsys);

	// Use shared fini function.
	b_system_fini(bsys);

	free(bsys);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct b_system *
b_system_create(void)
{
	struct b_system *bsys = U_TYPED_CALLOC(struct b_system);

	// Use init function, then add the common destroy function.
	if (!b_system_init(bsys, destroy)) {
		free(bsys);
		return NULL;
	}

	return bsys;
}

bool
b_system_init(struct b_system *bsys, void (*destroy_fn)(struct xrt_system *))
{
	// xrt_system fields.
	bsys->base.create_session = create_session;
	bsys->base.destroy = destroy_fn;

	// xrt_session_event_sink fields.
	bsys->broadcast.push_event = push_event;

	bsys->sessions.capacity = 2;
	bsys->sessions.pairs = U_TYPED_ARRAY_CALLOC(struct b_system_session_pair, bsys->sessions.capacity);
	if (bsys->sessions.pairs == NULL) {
		U_LOG_E("Failed to allocate session array");
		return false;
	}

	// b_system fields.
	XRT_MAYBE_UNUSED int ret = os_mutex_init(&bsys->sessions.mutex);
	assert(ret == 0);

	return true;
}

void
b_system_fini(struct b_system *bsys)
{
	// Just in case, should never happen.
	if (bsys->sessions.count > 0) {
		U_LOG_E("Number of sessions not zero, things will crash!");
	}

	free(bsys->sessions.pairs);
	bsys->sessions.count = 0;

	// Mutex needs to be destroyed.
	os_mutex_destroy(&bsys->sessions.mutex);
}

void
b_system_add_session(struct b_system *bsys, struct xrt_session *xs, struct xrt_session_event_sink *xses)
{
	assert(xs != NULL);
	assert(xses != NULL);

	os_mutex_lock(&bsys->sessions.mutex);

	const uint32_t new_count = bsys->sessions.count + 1;

	if (new_count > bsys->sessions.capacity) {
		const uint32_t new_capacity = bsys->sessions.capacity * 2;
		const size_t size = new_capacity * sizeof(*bsys->sessions.pairs);
		struct b_system_session_pair *tmp = realloc(bsys->sessions.pairs, size);
		if (tmp == NULL) {
			U_LOG_E("Failed to reallocate session array");
			goto add_unlock;
		}
		bsys->sessions.pairs = tmp;
		bsys->sessions.capacity = new_capacity;
	}

	bsys->sessions.pairs[bsys->sessions.count++] = (struct b_system_session_pair){xs, xses};

add_unlock:
	os_mutex_unlock(&bsys->sessions.mutex);
}

void
b_system_remove_session(struct b_system *bsys, struct xrt_session *xs, struct xrt_session_event_sink *xses)
{
	os_mutex_lock(&bsys->sessions.mutex);

	uint32_t count = bsys->sessions.count;
	uint32_t dst = 0;

	// Find where the session we are removing is.
	while (dst < count) {
		if (bsys->sessions.pairs[dst].xs == xs) {
			break;
		} else {
			dst++;
		}
	}

	// Guards against empty array as well as not finding the session.
	if (dst >= count) {
		U_LOG_E("Could not find session to remove!");
		goto remove_unlock;
	}

	// Should not be true with above check.
	assert(count > 0);

	/*
	 * Do copies if we are not removing the last session,
	 * this also guards against uint32_t::max.
	 */
	if (dst < count - 1) {
		// Copy from every follow down one.
		for (uint32_t src = dst + 1; src < count; src++, dst++) {
			bsys->sessions.pairs[dst] = bsys->sessions.pairs[src];
		}
	}

	count--;
	// This ensures that the memory returned in add is always zero initialized.
	U_ZERO(&bsys->sessions.pairs[count]);

	bsys->sessions.count = count;

remove_unlock:
	os_mutex_unlock(&bsys->sessions.mutex);
}

void
b_system_broadcast_event(struct b_system *bsys, const union xrt_session_event *xse)
{
	xrt_result_t xret;

	os_mutex_lock(&bsys->sessions.mutex);

	for (uint32_t i = 0; i < bsys->sessions.count; i++) {
		xret = xrt_session_event_sink_push(bsys->sessions.pairs[i].xses, xse);
		if (xret != XRT_SUCCESS) {
			U_LOG_W("Failed to push event to session, dropping.");
		}
	}

	os_mutex_unlock(&bsys->sessions.mutex);
}

void
b_system_set_system_compositor(struct b_system *bsys, struct xrt_system_compositor *xsysc)
{
	assert(bsys->xsysc == NULL);

	bsys->xsysc = xsysc;
}

void
b_system_fill_properties(struct b_system *bsys, const char *name)
{
	bsys->base.properties.vendor_id = 42;
	// The magical 247 number, is to silence warnings.
	snprintf(bsys->base.properties.name, XRT_MAX_SYSTEM_NAME_SIZE, "Monado: %.*s", 247, name);
	bsys->base.properties.form_factor = XRT_FORM_FACTOR_HMD;
}
