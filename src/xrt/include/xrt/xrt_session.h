// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for session object.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_space.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_compositor_native;


/*
 *
 * Session events.
 *
 */

/*!
 * Type of a @ref xrt_session event.
 *
 * @see xrt_session_event
 * @ingroup xrt_iface
 */
enum xrt_session_event_type
{
	//! This session has no pending events.
	XRT_SESSION_EVENT_NONE = 0,

	//! The state of the session has changed.
	XRT_SESSION_EVENT_STATE_CHANGE = 1,

	//! The state of the primary session has changed.
	XRT_SESSION_EVENT_OVERLAY_CHANGE = 2,

	//! The session is about to be lost.
	XRT_SESSION_EVENT_LOSS_PENDING = 3,

	//! The session has been lost.
	XRT_SESSION_EVENT_LOST = 4,

	//! The referesh rate of session (compositor) has changed.
	XRT_SESSION_EVENT_DISPLAY_REFRESH_RATE_CHANGE = 5,

	//! A reference space for this session has a pending change.
	XRT_SESSION_EVENT_REFERENCE_SPACE_CHANGE_PENDING = 6,

	//! The performance of the session has changed
	XRT_SESSION_EVENT_PERFORMANCE_CHANGE = 7,

	//! The passthrough state of the session has changed
	XRT_SESSION_EVENT_PASSTHRU_STATE_CHANGE = 8,
};

/*!
 * Session state changes event, type @ref XRT_SESSION_EVENT_STATE_CHANGE.
 *
 * @see xrt_session_event
 * @ingroup xrt_iface
 */
struct xrt_session_event_state_change
{
	enum xrt_session_event_type type;
	bool visible;
	bool focused;
};

/*!
 * Primary session state changes event,
 * type @ref XRT_SESSION_EVENT_OVERLAY_CHANGE.
 *
 * @see xrt_session_event
 * @ingroup xrt_iface
 */
struct xrt_session_event_overlay
{
	enum xrt_session_event_type type;
	bool primary_focused;
};

/*!
 * Loss pending event, @ref XRT_SESSION_EVENT_LOSS_PENDING.
 *
 * @see xrt_session_event
 * @ingroup xrt_iface
 */
struct xrt_session_event_loss_pending
{
	enum xrt_session_event_type type;
	uint64_t loss_time_ns;
};

/*!
 * Session lost event, type @ref XRT_SESSION_EVENT_LOST.
 *
 * @see xrt_session_event
 * @ingroup xrt_iface
 */
struct xrt_session_event_lost
{
	enum xrt_session_event_type type;
};

/*!
 * Display refresh rate of compositor changed event,
 * type @ref XRT_SESSION_EVENT_DISPLAY_REFRESH_RATE_CHANGE.
 *
 * @see xrt_session_event
 * @ingroup xrt_iface
 */
struct xrt_session_event_display_refresh_rate_change
{
	enum xrt_session_event_type type;
	float from_display_refresh_rate_hz;
	float to_display_refresh_rate_hz;
};

/*!
 * Event that tells the application that the reference space has a pending
 * change to it, maps to @p XrEventDataReferenceSpaceChangePending,
 * type @ref XRT_SESSION_EVENT_REFERENCE_SPACE_CHANGE_PENDING.
 *
 * @see xrt_session_event
 * @ingroup xrt_iface
 */
struct xrt_session_event_reference_space_change_pending
{
	enum xrt_session_event_type event_type;
	enum xrt_reference_space_type ref_type;
	uint64_t timestamp_ns;
	struct xrt_pose pose_in_previous_space;
	bool pose_valid;
};

/*!
 * Performance metrics change event.
 */
struct xrt_session_event_perf_change
{
	enum xrt_session_event_type type;
	enum xrt_perf_domain domain;
	enum xrt_perf_sub_domain sub_domain;
	enum xrt_perf_notify_level from_level;
	enum xrt_perf_notify_level to_level;
};

/*!
 * Passthrough state change event.
 */
struct xrt_session_event_passthrough_state_change
{
	enum xrt_session_event_type type;
	enum xrt_passthrough_state state;
};

/*!
 * Union of all session events, used to return multiple events through one call.
 * Each event struct must start with a @ref xrt_session_event_type field.
 *
 * @see xrt_session_event
 * @ingroup xrt_iface
 */
union xrt_session_event {
	enum xrt_session_event_type type;
	struct xrt_session_event_state_change state;
	struct xrt_session_event_state_change overlay;
	struct xrt_session_event_loss_pending loss_pending;
	struct xrt_session_event_lost lost;
	struct xrt_session_event_display_refresh_rate_change display;
	struct xrt_session_event_reference_space_change_pending ref_change;
	struct xrt_session_event_perf_change performance;
	struct xrt_session_event_passthrough_state_change passthru;
};

/*!
 * Used internally from producers of events to push events into session, some
 * sinks might mutliplex events to multiple sessions.
 *
 * @ingroup xrt_iface
 */
struct xrt_session_event_sink
{
	/*!
	 * Push one event to this sink, data is copied so pointer only needs to
	 * be valid for the duration of the call.
	 *
	 * @param xses Self-pointer to event sink.
	 * @param xse  A pre-filled out event.
	 */
	xrt_result_t (*push_event)(struct xrt_session_event_sink *xses, const union xrt_session_event *xse);
};

/*!
 * @copydoc xrt_session_event_sink::push_event
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_session_event_sink
 */
XRT_CHECK_RESULT static inline xrt_result_t
xrt_session_event_sink_push(struct xrt_session_event_sink *xses, const union xrt_session_event *xse)
{
	return xses->push_event(xses, xse);
}


/*
 *
 * Session.
 *
 */

/*!
 * The XRT representation of `XrSession`, this object does not have all of the
 * functionality of a session, most are partitioned to the session level
 * compositor object. Often this is @ref xrt_compositor_native, note that
 * interface may also be a system level object depending in implementor.
 *
 * @ingroup xrt_iface
 */
struct xrt_session
{
	/*!
	 * Poll a single event from this session, if no event is available then
	 * the type of the event will be @ref XRT_SESSION_EVENT_NONE.
	 *
	 * @param      xs      Pointer to self
	 * @param[out] out_xse Event to be returned.
	 */
	xrt_result_t (*poll_events)(struct xrt_session *xs, union xrt_session_event *out_xse);

	/*!
	 * Destroy the session, must be destroyed after the native compositor.
	 *
	 * Code consuming this interface should use @ref xrt_session_destroy.
	 *
	 * @param xs Pointer to self
	 */
	void (*destroy)(struct xrt_session *xs);
};

/*!
 * @copydoc xrt_session::poll_events
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_session
 */
XRT_CHECK_RESULT static inline xrt_result_t
xrt_session_poll_events(struct xrt_session *xs, union xrt_session_event *out_xse)
{
	return xs->poll_events(xs, out_xse);
}

/*!
 * Destroy an xrt_session - helper function.
 *
 * @param[in,out] xsd_ptr A pointer to the xrt_session struct pointer.
 *
 * Will destroy the system if `*xs_ptr` is not NULL. Will then set `*xs_ptr` to
 * NULL.
 *
 * @public @memberof xrt_session
 */
static inline void
xrt_session_destroy(struct xrt_session **xs_ptr)
{
	struct xrt_session *xs = *xs_ptr;
	if (xs == NULL) {
		return;
	}

	*xs_ptr = NULL;
	xs->destroy(xs);
}


#ifdef __cplusplus
}
#endif
