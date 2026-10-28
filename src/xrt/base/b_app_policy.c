// Copyright 2026, Beyley Cardellio.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_app_instance.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup base
 */

#include "util/u_misc.h"
#include "util/u_logging.h"

#include "math/m_api.h"

#include "b_app_policy.h"


// Forward declares
static void
app_system_destroy(struct xrt_app_system *xasys);

#define APP_SYSTEM_MAX_NONESSENTIAL_EVENTS 32

/*
 *
 * Helpers.
 *
 */

static inline struct b_app_instance *
b_app_instance(struct xrt_app_instance *xsys)
{
	return (struct b_app_instance *)xsys;
}

static inline struct b_app_system *
b_app_system(struct xrt_app_system *xsys)
{
	return (struct b_app_system *)xsys;
}

static struct b_app_system *
app_system_create(struct b_app_instance *bainst, struct xrt_system *xsys)
{
	struct b_app_system *basys = U_TYPED_CALLOC(struct b_app_system);

	// Use init function, then add the common destroy function.
	if (!b_app_system_init(&bainst->base, basys, xsys, app_system_destroy)) {
		free(basys);
		return NULL;
	}

	return basys;
}

XRT_CHECK_RESULT static xrt_result_t
app_system_add_event_locked(struct b_app_system *basys, const struct xrt_app_system_event *event)
{
	uint32_t num_events = 0;
	struct b_app_system_event **ptr = &basys->events;
	while (*ptr) {
		ptr = &(*ptr)->next;
		num_events++;
	}

	// Drop events when we have way too many, but don't make that fatal, the event we're dropping is non-essential.
	// @note If essential events are added, don't do this! Error or let the list grow.
	if (num_events >= APP_SYSTEM_MAX_NONESSENTIAL_EVENTS) {
		U_LOG_D("Dropping non-essential event of type %d since event queue already contains maximum %d events.",
		        event->type, APP_SYSTEM_MAX_NONESSENTIAL_EVENTS);
		return XRT_SUCCESS;
	}

	*ptr = U_TYPED_CALLOC(struct b_app_system_event);

	if ((*ptr) == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	(*ptr)->base = *event;

	return XRT_SUCCESS;
}

/*
 *
 * Member functions.
 *
 */

//! Destroys a single b_app_system, only used if it's using the b_app_instance infrastructure.
static void
app_system_destroy(struct xrt_app_system *xasys)
{
	struct b_app_system *basys = b_app_system(xasys);
	struct b_app_instance *bainst = b_app_instance(basys->xainst);

	// Lock before destroying the object
	os_mutex_lock(&bainst->app_systems.mutex);

	// Use shared fini function.
	b_app_system_fini(basys);
	free(basys);

	uint32_t i = 0;
	for (; i < bainst->app_systems.count; i++) {
		if (bainst->app_systems.storage[i] == xasys) {
			break;
		}
	}

	if (i == bainst->app_systems.count) {
		assert(!"b_app_system destroyed but not in parent b_app_instance?");
		os_mutex_unlock(&bainst->app_systems.mutex);
		return;
	}

	// Swap remove the last element
	bainst->app_systems.storage[i] = bainst->app_systems.storage[bainst->app_systems.count - 1];
	// Mark the last item as removed
	bainst->app_systems.count -= 1;

	os_mutex_unlock(&bainst->app_systems.mutex);
}

xrt_result_t
b_app_system_poll_events(struct xrt_app_system *xasys, struct xrt_app_system_event *out_xasyse)
{
	struct b_app_system *basys = b_app_system(xasys);

	os_mutex_lock(&basys->mutex);
	struct b_app_system_event *event = basys->events;

	if (event) {
		basys->events = event->next;
		(*out_xasyse) = event->base;
		free(event);
	} else {
		(*out_xasyse) = (struct xrt_app_system_event){
		    .type = XRT_APP_SYSTEM_EVENT_NONE,
		};
	}

	os_mutex_unlock(&basys->mutex);

	return XRT_SUCCESS;
}

static xrt_result_t
b_app_system_get_recommended_view_configuration(struct xrt_app_system *xasys,
                                                enum xrt_view_type view_type,
                                                struct xrt_recommended_view_config *out_recommended_view_config)
{
	struct b_app_system *basys = b_app_system(xasys);

	if (view_type == XRT_VIEW_TYPE_INVALID || view_type >= XRT_VIEW_TYPE_MAX) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	os_mutex_lock(&basys->mutex);
	*out_recommended_view_config = basys->view_configs[view_type];
	os_mutex_unlock(&basys->mutex);

	return XRT_SUCCESS;
}

static xrt_result_t
b_app_system_set_recommended_view_configuration(struct xrt_app_system *xasys,
                                                enum xrt_view_type view_type,
                                                const struct xrt_recommended_view_config *recommended_view_config)
{
	struct b_app_system *basys = b_app_system(xasys);
	xrt_result_t xret = XRT_SUCCESS;

	if (view_type == XRT_VIEW_TYPE_INVALID || view_type >= XRT_VIEW_TYPE_MAX) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	os_mutex_lock(&basys->mutex);
	{
		basys->view_configs[view_type] = *recommended_view_config;

		xret = app_system_add_event_locked( //
		    basys,                          //
		    &(struct xrt_app_system_event){
		        .type = XRT_APP_SYSTEM_EVENT_RECOMMENDED_VIEW_CONFIGURATION_CHANGED,
		        .recommended_view_configuration_changed = {.view_type = view_type},
		    }); //
	}
	os_mutex_unlock(&basys->mutex);

	return xret;
}


static void
b_app_instance_destroy(struct xrt_app_instance *xainst)
{
	struct b_app_instance *bainst = b_app_instance(xainst);

	// Free the app systems that are still stored in the app instance, if any.
	if (bainst->app_systems.storage != NULL) {
		// Free all app systems that haven't been freed yet.
		for (uint32_t i = 0; i < bainst->app_systems.count; i++) {
			struct xrt_app_system *xasys = bainst->app_systems.storage[i];

			b_app_system_fini(b_app_system(xasys));
			free(xasys);
		}

		free(bainst->app_systems.storage);

		bainst->app_systems.storage = NULL;
		bainst->app_systems.count = 0;
		bainst->app_systems.capacity = 0;
	}

	// Use shared fini function.
	b_app_instance_fini(bainst);

	free(bainst);
}

static xrt_result_t
b_app_instance_create_app_system(struct xrt_app_instance *xainst,
                                 struct xrt_system *xsys,
                                 struct xrt_app_system **out_xasys)
{
	struct b_app_instance *bainst = b_app_instance(xainst);

	xrt_result_t xret = XRT_SUCCESS;

	struct b_app_system *basys = app_system_create(bainst, xsys);
	if (basys == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	os_mutex_lock(&bainst->app_systems.mutex);

	if (bainst->app_systems.count >= bainst->app_systems.capacity) {
		uint32_t new_capacity = MAX(1, bainst->app_systems.capacity * 2);
		U_ARRAY_REALLOC_OR_FREE(bainst->app_systems.storage, struct xrt_app_system *, new_capacity);

		if (bainst->app_systems.storage == NULL) {
			xret = XRT_ERROR_ALLOCATION;
			goto out;
		}

		bainst->app_systems.capacity = new_capacity;
	}

	bainst->app_systems.storage[bainst->app_systems.count++] = &basys->base;

	*out_xasys = &basys->base;

out:
	if (xret != XRT_SUCCESS) {
		// Destroy the app system we just created since we won't be able to store it.
		b_app_system_fini(basys);
		free(basys);
	}

	os_mutex_unlock(&bainst->app_systems.mutex);
	return xret;
}

/*
 *
 * 'Exported' functions.
 *
 */

struct b_app_instance *
b_app_instance_create(void)
{
	struct b_app_instance *bainst = U_TYPED_CALLOC(struct b_app_instance);

	// Use init function with the common destroy function.
	if (!b_app_instance_init(bainst, b_app_instance_destroy)) {
		free(bainst);
		return NULL;
	}

	return bainst;
}

bool
b_app_instance_init(struct b_app_instance *bainst, void (*destroy_fn)(struct xrt_app_instance *))
{
	// xrt_app_instance fields.
	bainst->base.create_app_system = b_app_instance_create_app_system;
	bainst->base.destroy = destroy_fn;

	os_mutex_init(&bainst->app_systems.mutex);

	return true;
}

void
b_app_instance_fini(struct b_app_instance *bainst)
{
	os_mutex_destroy(&bainst->app_systems.mutex);
}


bool
b_app_system_init(struct xrt_app_instance *xainst,
                  struct b_app_system *basys,
                  struct xrt_system *xsys,
                  void (*destroy_fn)(struct xrt_app_system *))
{
	// xrt_app_system fields.
	basys->base.poll_events = b_app_system_poll_events;
	basys->base.get_recommended_view_configuration = b_app_system_get_recommended_view_configuration;
	basys->base.set_recommended_view_configuration = b_app_system_set_recommended_view_configuration;
	basys->base.destroy = destroy_fn;

	basys->xainst = xainst;
	basys->xsys = xsys;

	os_mutex_init(&basys->mutex);

	return true;
}

void
b_app_system_fini(struct b_app_system *basys)
{
	os_mutex_destroy(&basys->mutex);
}
