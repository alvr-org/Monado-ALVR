// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_session.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_session.h"
#include "os/os_threading.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Struct used by @ref u_session to queue up events.
 *
 * @ingroup aux_util
 */
struct u_session_event
{
	union xrt_session_event xse;
	struct u_session_event *next;
};

/*!
 * This is a helper struct that fully implements @ref xrt_session object.
 *
 * The use of @ref u_system is optional, but if not used you will need to track
 * the session and signal it's destruction yourself.
 *
 * @ingroup aux_util
 * @implements xrt_session
 */
struct u_session
{
	struct xrt_session base;

	//! Pushes events to this session, used to share to other components.
	struct xrt_session_event_sink sink;

	//! Owning system, optional.
	struct u_system *usys;

	struct
	{
		struct os_mutex mutex;
		struct u_session_event *ptr;
	} events;
};

/*!
 * Create a session, optionally pass in a @ref u_system. If @p usys is not NULL
 * the call register this session on that system. This function is exposed so
 * that code can re-use @ref u_session as a base class.
 *
 * @public @memberof u_session
 * @ingroup aux_util
 */
struct u_session *
u_session_create(struct u_system *usys);

/*!
 * Push an event to this session. This function is exposed so that code can
 * re-use @ref u_session as a base class.
 *
 *
 * @public @memberof u_session
 * @ingroup aux_util
 */
void
u_session_event_push(struct u_session *us, const union xrt_session_event *xse);

/*!
 * Pop a single event from this session, if no event is available
 * then the type of the event will be @ref XRT_SESSION_EVENT_NONE.
 *
 * @public @memberof u_session
 * @ingroup aux_util
 */
void
u_session_event_pop(struct u_session *us, union xrt_session_event *out_xse);


#ifdef __cplusplus
}
#endif
