// Copyright 2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
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
 * Struct used by @ref b_session to queue up events.
 *
 * @ingroup aux_util
 */
struct b_session_event
{
	union xrt_session_event xse;
	struct b_session_event *next;
};

/*!
 * This is a helper struct that fully implements @ref xrt_session object.
 *
 * The use of @ref b_system is optional, but if not used you will need to track
 * the session and signal it's destruction yourself.
 *
 * @ingroup aux_util
 * @implements xrt_session
 */
struct b_session
{
	struct xrt_session base;

	//! Pushes events to this session, used to share to other components.
	struct xrt_session_event_sink sink;

	//! Owning system, optional.
	struct b_system *bsys;

	struct
	{
		struct os_mutex mutex;
		struct b_session_event *ptr;
	} events;
};

/*!
 * Create a session, optionally pass in a @ref b_system. If @p bsys is not NULL
 * the call register this session on that system. This function is exposed so
 * that code can reuse @ref b_session as a base class.
 *
 * @public @memberof b_session
 * @ingroup aux_util
 */
struct b_session *
b_session_create(struct b_system *bsys);

/*!
 * Push an event to this session. This function is exposed so that code can
 * reuse @ref b_session as a base class.
 *
 *
 * @public @memberof b_session
 * @ingroup aux_util
 */
void
b_session_event_push(struct b_session *bs, const union xrt_session_event *xse);

/*!
 * Pop a single event from this session, if no event is available
 * then the type of the event will be @ref XRT_SESSION_EVENT_NONE.
 *
 * @public @memberof b_session
 * @ingroup aux_util
 */
void
b_session_event_pop(struct b_session *bs, union xrt_session_event *out_xse);


#ifdef __cplusplus
}
#endif
