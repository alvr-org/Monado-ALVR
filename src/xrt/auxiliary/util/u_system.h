// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_system.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

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
struct u_system_session_pair
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
struct u_system
{
	struct xrt_system base;

	//! Pushes events to all sessions created from this system.
	struct xrt_session_event_sink broadcast;

	struct
	{
		struct os_mutex mutex;

		//! Number of session and event sink pairs.
		uint32_t count;
		//! Array of session and event sink pairs.
		struct u_system_session_pair *pairs;
	} sessions;

	/*!
	 * Used to implement @ref xrt_system::create_session, can be NULL. This
	 * field should be set with @ref u_system_set_system_compositor.
	 */
	struct xrt_system_compositor *xsysc;
};

/*!
 * Create a @ref u_system.
 *
 * @public @memberof u_system
 * @ingroup aux_util
 */
struct u_system *
u_system_create(void);

/*!
 * Add a @ref xrt_session to be tracked and to receive multiplexed events.
 *
 * @public @memberof u_system
 * @ingroup aux_util
 */
void
u_system_add_session(struct u_system *usys, struct xrt_session *xs, struct xrt_session_event_sink *xses);

/*!
 * Remove a @ref xrt_session from tracking, it will no longer receive events,
 * the given @p xses needs to match when it was added.
 *
 * @public @memberof u_system
 * @ingroup aux_util
 */
void
u_system_remove_session(struct u_system *usys, struct xrt_session *xs, struct xrt_session_event_sink *xses);

/*!
 * Broadcast event to all sessions under this system.
 *
 * @public @memberof u_system
 * @ingroup aux_util
 */
void
u_system_broadcast_event(struct u_system *usys, const union xrt_session_event *xse);

/*!
 * Set the system compositor, used in the @ref xrt_system_create_session call.
 *
 * @public @memberof u_system
 * @ingroup aux_util
 */
void
u_system_set_system_compositor(struct u_system *usys, struct xrt_system_compositor *xsysc);

/*!
 * Fill system properties.
 *
 * @public @memberof u_system
 * @ingroup aux_util
 */
void
u_system_fill_properties(struct u_system *usys, const char *name);

/*!
 * Destroy an @ref u_system_create allocated @ref u_system - helper function.
 *
 * @param[in,out] usys_ptr A pointer to the @ref u_system_create allocated
 * struct pointer.
 *
 * Will destroy the system devices if @p *usys_ptr is not NULL. Will then set
 * @p *usys_ptr to NULL.
 *
 * @public @memberof u_system
 */
static inline void
u_system_destroy(struct u_system **usys_ptr)
{
	struct u_system *usys = *usys_ptr;
	if (usys == NULL) {
		return;
	}

	*usys_ptr = NULL;
	usys->base.destroy(&usys->base);
}


#ifdef __cplusplus
}
#endif
