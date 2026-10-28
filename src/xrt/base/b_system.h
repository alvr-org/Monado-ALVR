// Copyright 2023, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_system.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_system.h"
#include "xrt/xrt_session.h"
#include "os/os_threading.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_session;
struct xrt_session_event_sink;
union xrt_session_event;

/*!
 * A pair of @ref xrt_session and @ref xrt_session_event_sink that has been
 * registered to this system, used to multiplex events to all sessions.
 *
 * @ingroup aux_util
 */
struct b_system_session_pair
{
	struct xrt_session *xs;
	struct xrt_session_event_sink *xses;
};

/*!
 * A helper to implement a @ref xrt_system, takes care of multiplexing events
 * to sessions.
 *
 * @ingroup aux_util
 * @implements xrt_system
 */
struct b_system
{
	struct xrt_system base;

	//! Pushes events to all sessions created from this system.
	struct xrt_session_event_sink broadcast;

	struct
	{
		struct os_mutex mutex;

		//! Number of session and event sink pairs.
		uint32_t count;
		//! Capacity of the session array.
		uint32_t capacity;
		//! Array of session and event sink pairs.
		struct b_system_session_pair *pairs;
	} sessions;

	/*!
	 * Used to implement @ref xrt_system::create_session, can be NULL. This
	 * field should be set with @ref b_system_set_system_compositor.
	 */
	struct xrt_system_compositor *xsysc;
};

/*!
 * Create a @ref b_system, creates a fully working system. Objects wishing to
 * use @ref b_system as a parent class should use @ref b_system_init.
 *
 * @public @memberof b_system
 * @ingroup aux_util
 * @see b_system_init
 */
struct b_system *
b_system_create(void);

/*!
 * Inits a @ref b_system struct when used as a parent class, only to be used
 * by base class. Not needed to be called if created by @ref b_system_create.
 *
 * @protected @memberof b_system
 * @ingroup aux_util
 */
bool
b_system_init(struct b_system *bsys, void (*destroy_fn)(struct xrt_system *));

/*!
 * Finalizes a @ref b_system struct when used as a parent class, only to be used
 * by base class. This will not free the @ref b_system pointer itself but will
 * free any resources created by the default implementation functions. Not
 * needed to be called if created by @ref b_system_create, instead use
 * xrt_system::destroy.
 *
 * @protected @memberof b_system
 * @ingroup aux_util
 */
void
b_system_fini(struct b_system *bsys);

/*!
 * Add a @ref xrt_session to be tracked and to receive multiplexed events.
 *
 * @public @memberof b_system
 * @ingroup aux_util
 */
void
b_system_add_session(struct b_system *bsys, struct xrt_session *xs, struct xrt_session_event_sink *xses);

/*!
 * Remove a @ref xrt_session from tracking, it will no longer receive events,
 * the given @p xses needs to match when it was added.
 *
 * @public @memberof b_system
 * @ingroup aux_util
 */
void
b_system_remove_session(struct b_system *bsys, struct xrt_session *xs, struct xrt_session_event_sink *xses);

/*!
 * Broadcast event to all sessions under this system.
 *
 * @public @memberof b_system
 * @ingroup aux_util
 */
void
b_system_broadcast_event(struct b_system *bsys, const union xrt_session_event *xse);

/*!
 * Set the system compositor, used in the @ref xrt_system_create_session call.
 *
 * @public @memberof b_system
 * @ingroup aux_util
 */
void
b_system_set_system_compositor(struct b_system *bsys, struct xrt_system_compositor *xsysc);

/*!
 * Fill system properties.
 *
 * @public @memberof b_system
 * @ingroup aux_util
 */
void
b_system_fill_properties(struct b_system *bsys, const char *name);

/*!
 * Destroy an @ref b_system_create allocated @ref b_system - helper function.
 *
 * @param[in,out] bsys_ptr A pointer to the @ref b_system_create allocated
 * struct pointer.
 *
 * Will destroy the system devices if @p *bsys_ptr is not NULL. Will then set
 * @p *bsys_ptr to NULL.
 *
 * @public @memberof b_system
 */
static inline void
b_system_destroy(struct b_system **bsys_ptr)
{
	struct b_system *bsys = *bsys_ptr;
	if (bsys == NULL) {
		return;
	}

	*bsys_ptr = NULL;
	bsys->base.destroy(&bsys->base);
}


#ifdef __cplusplus
}
#endif
