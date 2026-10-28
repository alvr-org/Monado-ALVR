// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining interfaces for time synchronization in Monado.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"


#ifdef __cplusplus
extern "C" {
#endif

enum t_timing_event_type
{
	//! Marks the beginning of an exposure.
	T_TIMING_EVENT_TYPE_CAMERA_EXPOSURE_START,
};

/*!
 * Marks beginning of a camera exposure, pushed from the source to the sink.
 */
struct t_timing_event_camera_exposure_start
{
	/*!
	 * The sequence ID of this frame. These may have semantic meaning depending on the source, for example, on a
	 * device with a SLAM/CONTROLLER/CONTROLLER cadence, `sequence_id % 3` will indicate which type of frame is
	 * being exposed.
	 *
	 * This is intentionally left as a generic uint64_t, and not an enum to allow for matching source/sync types to
	 * smuggle this data through while keeping the interface generic to any sink that wants to listen anyway, since
	 * WMR controllers sync to each SLAM frame, and blink on their own for the next two frames.
	 */
	uint64_t sequence_id;
	//! The timestamp of the event, in local monotonic domain. See @ref os_monotonic_get_ns.
	int64_t timestamp_ns;
	//! Estimated frame period. May be 0 if unknown.
	uint64_t frame_period_ns;
	//! Estimated exposure time of the frame. May be 0 if unknown.
	uint64_t exposure_time_ns;
};

struct t_timing_event
{
	enum t_timing_event_type type;

	union {
		struct t_timing_event_camera_exposure_start camera_exposure_start;
	};
};

/*!
 * @interface t_timing_event_sink
 *
 * A time sync sink is a component that receives timing events from a stable timing source, such as the exposure of
 * a camera frame. This allows the sink to attempt to synchronize its internal timing to the source. This is most
 * useful for devices such as the PS Sense controllers, or the WMR controllers, which need to synchronize their LED
 * flashes with a camera.
 *
 * Objects implementing t_timing_event_sink should likely also implement @ref xrt_frame_node and live within the
 * lifetime of an @ref xrt_frame_context.
 */
struct t_timing_event_sink
{
	/*!
	 * A function that the timing event sink will call to push a new timing event.
	 *
	 * @param sink  The sink to push the timing event to.
	 * @param event The timing event being pushed.
	 */
	void (*push_timing_event)(struct t_timing_event_sink *sink, const struct t_timing_event *event);
};

/*!
 * Helper function for @ref t_timing_event_sink::push_timing_event.
 *
 * @copydoc t_timing_event_sink::push_timing_event
 *
 * @public @memberof t_timing_event_sink
 */
XRT_NONNULL_ALL static inline void
t_timing_event_sink_push_timing_event(struct t_timing_event_sink *sink, const struct t_timing_event *event)
{
	sink->push_timing_event(sink, event);
}


/*!
 * @interface t_timing_event_source
 *
 * A time sync source is a component that generates timing events for an @ref t_timing_event_sink to consume.
 *
 * Objects implementing t_timing_event_source should likely also implement @ref xrt_frame_node and live within the
 * lifetime of an @ref xrt_frame_context.
 */
struct t_timing_event_source
{
	/*!
	 * Adds an event sink to this source
	 *
	 * @param source The source to add the sink to.
	 * @param sink   The sink to add to the source.
	 *
	 * @return 0 on success, or a negative value on failure (for example, the source is full).
	 */
	int (*add_sink)(struct t_timing_event_source *source, struct t_timing_event_sink *sink);

	/*!
	 * Removes an event sink from this source
	 *
	 * @param source The source to remove the sink from.
	 * @param sink   The sink to remove from the source.
	 */
	void (*remove_sink)(struct t_timing_event_source *source, struct t_timing_event_sink *sink);
};

/*!
 * Helper function for @ref t_timing_event_source::add_sink.
 *
 * @copydoc t_timing_event_source::add_sink
 *
 * @public @memberof t_timing_event_source
 */
XRT_NONNULL_ALL static inline int
t_timing_event_source_add_sink(struct t_timing_event_source *source, struct t_timing_event_sink *sink)
{
	return source->add_sink(source, sink);
}

/*!
 * Helper function for @ref t_timing_event_source::remove_sink.
 *
 * @copydoc t_timing_event_source::remove_sink
 *
 * @public @memberof t_timing_event_source
 */
XRT_NONNULL_ALL static inline void
t_timing_event_source_remove_sink(struct t_timing_event_source *source, struct t_timing_event_sink *sink)
{
	source->remove_sink(source, sink);
}

#ifdef __cplusplus
}
#endif
