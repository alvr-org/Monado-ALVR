// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A routine to automatically refine latency offsets of LED blink times using constellation samples.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "util/u_var.h"
#include "util/u_debug.h"

#include "math/m_api.h"

#include "os/os_time.h"

#include "t_led_sync_refinement.h"


DEBUG_GET_ONCE_LOG_OPTION(led_sync_refinement_log, "T_LED_SYNC_REFINEMENT_LOG", U_LOGGING_INFO)

#define LOG_TRACE(refinement, ...) U_LOG_IFL_T(refinement->log_level, __VA_ARGS__)
#define LOG_DEBUG(refinement, ...) U_LOG_IFL_D(refinement->log_level, __VA_ARGS__)
#define LOG_INFO(refinement, ...) U_LOG_IFL_I(refinement->log_level, __VA_ARGS__)
#define LOG_WARN(refinement, ...) U_LOG_IFL_W(refinement->log_level, __VA_ARGS__)
#define LOG_ERROR(refinement, ...) U_LOG_IFL_E(refinement->log_level, __VA_ARGS__)

//! The amount of frames to not see it for consider a device "not found" during an active search
#define MAX_SAMPLE_LATENCY 3

/*
 * Helper functions
 */

//! The amount of time of which a change under this amount is considered "good" and we can exit the binary search
static time_duration_ns
change_good_cutoff(struct t_led_sync_refinement *refinement)
{
	return MIN(refinement->current_blink_duration_ns, refinement->exposure_time_ns) / 50;
}

//! The amount of frames since the sample was applied to the controller
static uint32_t
frames_since_sample_apply(struct t_led_sync_refinement *refinement, timepoint_ns now_ns)
{
	return (uint32_t)((now_ns - refinement->last_sample_apply_time_ns) / refinement->exposure_interval_ns);
}

static void
set_offset_duration(struct t_led_sync_refinement *refinement,
                    time_duration_ns latency_offset_ns,
                    time_duration_ns fudge_offset_ns,
                    time_duration_ns blink_duration_ns)
{
	refinement->current_latency_offset_ns = latency_offset_ns;
	refinement->current_blink_fudge_ns = fudge_offset_ns;
	refinement->current_blink_duration_ns = blink_duration_ns;

	refinement->has_sample_for_driver = true;
	refinement->sample_applied = false;
	refinement->sample_for_driver = XRT_C11_COMPOUND(struct t_led_sync_sample){
	    .timestamp.device_host_latency_ns = latency_offset_ns,
	    .timestamp_mode = T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_DEVICE_HOST_LATENCY,
	    .fudge_offset_ns = fudge_offset_ns,
	    .blink_duration_ns = blink_duration_ns,
	};

	// If we're doing optical driven offset and have a saved offset, use that mode.
	if ((refinement->options.flags & T_LED_SYNC_REFINEMENT_FLAGS_OPTICAL_DRIVEN_OFFSET) != 0 &&
	    refinement->optical_driven_offset.has_saved_offset) {
		refinement->sample_for_driver.timestamp_mode =
		    T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_HOST_DEVICE_CLOCK_OFFSET;
		refinement->sample_for_driver.timestamp.host_device_clock_offset_ns =
		    refinement->optical_driven_offset.saved_offset + latency_offset_ns;
	}

	LOG_TRACE(refinement,
	          "Setting offset to %" PRId64 "ns with fudge %" PRId64 "ns and blink duration to %" PRId64 "ns",
	          latency_offset_ns, fudge_offset_ns, blink_duration_ns);
}

static void
set_centered_duration(struct t_led_sync_refinement *refinement, time_duration_ns blink_duration_ns)
{
	// Find the latency offset, the offset needing to be applied to get blink start to line up with exposure
	// start, this is the transfer latency from the controller->host.
	time_duration_ns new_latency_offset_ns = refinement->found_left_edge_ns;

	// Set the offset so that the center of the blink is at the center of the exposure, since latency and
	// fudge offset are added together.
	time_duration_ns new_fudge_offset_ns = (refinement->exposure_time_ns / 2) - (blink_duration_ns / 2);

	set_offset_duration(refinement, new_latency_offset_ns, new_fudge_offset_ns, blink_duration_ns);
}

//! Gets the period to iterate over when searching for a latency offset.
static time_duration_ns
get_search_period_locked(struct t_led_sync_refinement *refinement)
{
	int divisor = 4;

	if (!refinement->has_exposure_time) {
		return refinement->current_blink_duration_ns / divisor;
	}

	if (refinement->phase == T_LED_SYNC_SEARCH_PHASE_FIND_INITIAL_OFFSET) {
		// If we're in initial offset search, we can use the larger of the two, since we just need to find one
		// that overlaps.
		return MAX(refinement->current_blink_duration_ns, refinement->exposure_time_ns) / divisor;
	} else {
		return MIN(refinement->current_blink_duration_ns, refinement->exposure_time_ns) / divisor;
	}
}

static time_duration_ns
max_right_edge(struct t_led_sync_refinement *refinement)
{
	if ((refinement->options.flags & T_LED_SYNC_REFINEMENT_FLAGS_HAS_LATENCY_CAP) != 0) {
		return refinement->options.latency_cap_ns;
	} else {
		return refinement->exposure_interval_ns - refinement->current_blink_duration_ns;
	}
}

static void
set_phase_locked(struct t_led_sync_refinement *refinement, enum t_led_sync_phase new_phase)
{
	refinement->phase = new_phase;

	LOG_INFO(refinement, "Entering search phase: %d", new_phase);

	switch (new_phase) {
	case T_LED_SYNC_SEARCH_PHASE_FIND_INITIAL_OFFSET: {
		refinement->found_left_edge_ns = -1;
		refinement->found_right_edge_ns = -1;

		refinement->current_blink_duration_ns = refinement->options.initial_blink_duration_ns;

		refinement->optical_driven_offset.has_saved_offset = false;

		set_offset_duration(refinement, 0, 0, refinement->current_blink_duration_ns);

		break;
	}
	case T_LED_SYNC_SEARCH_PHASE_FIND_RIGHT_EDGE: {
		// If we're doing clock driven offset
		if (refinement->options.flags & T_LED_SYNC_REFINEMENT_FLAGS_OPTICAL_DRIVEN_OFFSET) {
			refinement->optical_driven_offset.has_saved_offset = true;
			refinement->optical_driven_offset.saved_offset =
			    refinement->optical_driven_offset.driver_host_device_clock_offset_ns;
		}

		// Left bound is the current offset
		refinement->binary_search_state.left_bound_ns = refinement->current_blink_fudge_ns;
		// Right bound is the start of the next exposure minus the blink duration, we don't want to blink into
		// the next frame, rather start at zero, since latency will always be >0.
		refinement->binary_search_state.right_bound_ns = max_right_edge(refinement);

		LOG_DEBUG(refinement,
		          "Starting binary search for right edge, left bound at: %" PRId64
		          "ns, right bound at: %" PRId64 "ns",
		          refinement->binary_search_state.left_bound_ns,
		          refinement->binary_search_state.right_bound_ns);

		time_duration_ns new_offset_ns =
		    (refinement->binary_search_state.right_bound_ns + refinement->binary_search_state.left_bound_ns) /
		    2;
		set_offset_duration(refinement, 0, new_offset_ns, refinement->current_blink_duration_ns);

		break;
	}
	case T_LED_SYNC_SEARCH_PHASE_FIND_LEFT_EDGE: {
		// We should always have a right edge by now
		assert(refinement->found_right_edge_ns > -1);

		// Left bound is when the exposure *should* be given zero latency
		refinement->binary_search_state.left_bound_ns = 0;
		// Right bound is the found right edge
		refinement->binary_search_state.right_bound_ns = refinement->found_right_edge_ns;

		LOG_DEBUG(refinement, "Starting binary search for left edge, right edge at: %" PRId64 "ns",
		          refinement->found_right_edge_ns);

		time_duration_ns new_offset_ns =
		    (refinement->binary_search_state.right_bound_ns + refinement->binary_search_state.left_bound_ns) /
		    2;
		set_offset_duration(refinement, 0, new_offset_ns, refinement->current_blink_duration_ns);

		break;
	}
	case T_LED_SYNC_SEARCH_PHASE_REFINE_BLINK_DURATION: {
		// We should have both edges by now
		assert(refinement->found_right_edge_ns > -1);
		assert(refinement->found_left_edge_ns > -1);

		set_centered_duration(refinement, refinement->current_blink_duration_ns);

		refinement->blink_time_refinement_state.backing_off = false;
		refinement->blink_time_refinement_state.last_good_blink_duration_ns =
		    refinement->current_blink_duration_ns;

		break;
	}
	case T_LED_SYNC_SEARCH_PHASE_MAINTAIN_OFFSET: {
		// We should have both edges by now
		assert(refinement->found_right_edge_ns > -1);
		assert(refinement->found_left_edge_ns > -1);

		set_centered_duration(refinement, refinement->current_blink_duration_ns);

		LOG_DEBUG(refinement, "Maintaining offset at: %" PRId64 "ns", refinement->current_latency_offset_ns);

		break;
	}
	default: break;
	}
}

static void
handle_find_initial_offset(struct t_led_sync_refinement *refinement)
{
	if (refinement->frames_since_last_visually_seen > MAX_SAMPLE_LATENCY) {
		const time_duration_ns search_period_ns = get_search_period_locked(refinement);
		const time_duration_ns new_fudge_offset_ns =
		    (refinement->current_blink_fudge_ns + search_period_ns) % max_right_edge(refinement);

		LOG_DEBUG(refinement, "Device hasn't been seen, trying offset: %" PRId64 "ns", new_fudge_offset_ns);

		// Scan the fudge offset around instead of latency, so latency isn't jumping around at random and
		// confusing time conversion math.
		set_offset_duration(refinement, 0, new_fudge_offset_ns, refinement->current_blink_duration_ns);
	} else {
		LOG_DEBUG(refinement, "Device has been seen, moving to find edge phases");

		// We got a sample recently, try to find the edges now.
		set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_FIND_RIGHT_EDGE);
	}
}

static void
handle_find_right_edge(struct t_led_sync_refinement *refinement)
{
	if (refinement->frames_since_last_visually_seen > MAX_SAMPLE_LATENCY) {
		LOG_DEBUG(refinement,
		          "Device hasn't been seen for %" PRIu32
		          " frames, advancing binary search with left side closer",
		          refinement->frames_since_last_visually_seen);

		// We didn't see the device
		refinement->binary_search_state.right_bound_ns = refinement->current_blink_fudge_ns;

		time_duration_ns new_offset_ns =
		    (refinement->binary_search_state.left_bound_ns + refinement->binary_search_state.right_bound_ns) /
		    2;

		set_offset_duration(refinement, 0, new_offset_ns, refinement->current_blink_duration_ns);

	} else {
		LOG_DEBUG(refinement, "Device has been seen, advancing binary search with right side closer");

		// We saw the device
		refinement->binary_search_state.left_bound_ns = refinement->current_blink_fudge_ns;

		time_duration_ns new_offset_ns =
		    (refinement->binary_search_state.left_bound_ns + refinement->binary_search_state.right_bound_ns) /
		    2;

		set_offset_duration(refinement, 0, new_offset_ns, refinement->current_blink_duration_ns);
	}

	bool done = refinement->binary_search_state.right_bound_ns - refinement->binary_search_state.left_bound_ns <
	            change_good_cutoff(refinement);
	if (done) {
		// We found a good enough offset for the right edge, move on to finding the left edge.
		refinement->found_right_edge_ns = refinement->current_blink_fudge_ns;

		LOG_DEBUG(refinement,
		          "Found right edge at offset: %" PRId64 "ns. Difference %" PRId64
		          "ns was lower than cutoff %" PRId64 "ns, moving to find left edge phase",
		          refinement->current_blink_fudge_ns,
		          refinement->binary_search_state.right_bound_ns -
		              refinement->binary_search_state.left_bound_ns,
		          change_good_cutoff(refinement));

		set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_FIND_LEFT_EDGE);
	}
}

static void
handle_find_left_edge(struct t_led_sync_refinement *refinement)
{
	if (refinement->frames_since_last_visually_seen > MAX_SAMPLE_LATENCY) {
		LOG_DEBUG(refinement,
		          "Device hasn't been seen for %" PRIu32
		          " frames, advancing binary search with right side closer",
		          refinement->frames_since_last_visually_seen);

		// We didn't see the device
		refinement->binary_search_state.left_bound_ns =
		    refinement->current_blink_fudge_ns + refinement->current_blink_duration_ns;

		time_duration_ns new_offset_ns =
		    ((refinement->binary_search_state.left_bound_ns + refinement->binary_search_state.right_bound_ns) /
		     2);
		// Set the new offset so that we're moving the right edge of the blink to the left edge of the exposure
		set_offset_duration(refinement, 0, new_offset_ns - refinement->current_blink_duration_ns,
		                    refinement->current_blink_duration_ns);
	} else {
		LOG_DEBUG(refinement, "Device has been seen, advancing binary search with left side closer");

		// We saw the device
		refinement->binary_search_state.right_bound_ns =
		    refinement->current_blink_fudge_ns + refinement->current_blink_duration_ns;

		time_duration_ns new_offset_ns =
		    (refinement->binary_search_state.left_bound_ns + refinement->binary_search_state.right_bound_ns) /
		    2;
		// Set the new offset so that we're moving the right edge of the blink to the left edge of the exposure
		set_offset_duration(refinement, 0, new_offset_ns - refinement->current_blink_duration_ns,
		                    refinement->current_blink_duration_ns);
	}

	bool done = refinement->binary_search_state.right_bound_ns - refinement->binary_search_state.left_bound_ns <
	            change_good_cutoff(refinement);
	if (done) {
		// We found a good enough offset for the left edge, we can maintain the offset now.
		refinement->found_left_edge_ns =
		    refinement->current_blink_fudge_ns + refinement->current_blink_duration_ns;

		LOG_DEBUG(refinement,
		          "Found left edge at offset: %" PRId64 "ns. Difference %" PRId64
		          "ns was lower than cutoff %" PRId64 "ns, moving to maintain offset phase",
		          refinement->current_blink_fudge_ns,
		          refinement->binary_search_state.right_bound_ns -
		              refinement->binary_search_state.left_bound_ns,
		          change_good_cutoff(refinement));

		// The visibility window is how much of the time window the LEDs were visible for
		time_duration_ns visibility_window_ns =
		    refinement->found_right_edge_ns - refinement->current_blink_fudge_ns;

		// The real LED on period is the amount of time the LEDs were actually visible to the camera for
		time_duration_ns real_led_on_window_ns = visibility_window_ns - (refinement->exposure_time_ns);
		LOG_DEBUG(refinement,
		          "Found left edge at offset: %" PRId64 "ns, right edge at offset: %" PRId64
		          "ns, visibility window: %" PRId64 "ns, real LED on window: %" PRId64 "ns",
		          refinement->found_left_edge_ns, refinement->found_right_edge_ns, visibility_window_ns,
		          real_led_on_window_ns);

		// We've found the latency offset, so if we're not optimizing blink duration, we can move to just
		// maintaining the offset now.
		if (refinement->options.flags & T_LED_SYNC_REFINEMENT_FLAGS_BLINK_DURATION) {
			set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_REFINE_BLINK_DURATION);
		} else {
			set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_MAINTAIN_OFFSET);
		}
	}
}

static void
handle_refine_blink_duration(struct t_led_sync_refinement *refinement)
{
	if (refinement->frames_since_last_visually_seen > MAX_SAMPLE_LATENCY) {
		LOG_DEBUG(refinement, "Device hasn't been seen for %" PRIu32 " frames, trying shorter blink duration",
		          refinement->frames_since_last_visually_seen);

		if (refinement->blink_time_refinement_state.backing_off) {
			// Backing off still didn't work, just go back to the start and hope for the best.
			time_duration_ns new_blink_duration = refinement->options.initial_blink_duration_ns;
			set_centered_duration(refinement, new_blink_duration);

			LOG_DEBUG(refinement,
			          "Backing off didn't work, resetting blink duration to initial value: %" PRId64 "ns",
			          new_blink_duration);

			set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_MAINTAIN_OFFSET);
			return;
		}

		// We didn't see the device, try increasing the blink duration again to back off
		time_duration_ns new_blink_duration =
		    refinement->blink_time_refinement_state.last_good_blink_duration_ns;

		set_centered_duration(refinement, new_blink_duration);
		refinement->blink_time_refinement_state.backing_off = true;

		LOG_DEBUG(refinement, "Trying to back off to last good blink duration: %" PRId64 "ns",
		          new_blink_duration);
	} else {
		if (refinement->blink_time_refinement_state.backing_off) {
			// We backed off and it worked, so now we found the minimum blink duration that doesn't cause
			// instability, we can stop refining.
			LOG_DEBUG(refinement,
			          "Back off worked, found good blink duration: %" PRId64 "ns, stopping refinement",
			          refinement->current_blink_duration_ns);
			set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_MAINTAIN_OFFSET);
			return;
		}

		time_duration_ns search_period_ns = get_search_period_locked(refinement);
		time_duration_ns new_blink_duration_ns = refinement->current_blink_duration_ns - search_period_ns;

		// We saw the device, this is a good blink duration, save it in case we need to back off.
		refinement->blink_time_refinement_state.last_good_blink_duration_ns =
		    refinement->current_blink_duration_ns;
		refinement->blink_time_refinement_state.backing_off = false;

		LOG_DEBUG(refinement,
		          "Device has been seen, trying shorter blink duration: %" PRId64 "ns (was %" PRId64 "ns)",
		          new_blink_duration_ns, refinement->current_blink_duration_ns);

		if (new_blink_duration_ns < refinement->options.min_blink_duration_ns) {
			LOG_DEBUG(refinement,
			          "New blink duration %" PRId64 "ns was below minimum blink duration %" PRId64
			          "ns, stopping refinement",
			          new_blink_duration_ns, refinement->options.min_blink_duration_ns);
			set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_MAINTAIN_OFFSET);
			return;
		}

		set_centered_duration(refinement, new_blink_duration_ns);
	}
}

static void
handle_maintain_offset(struct t_led_sync_refinement *refinement)
{}

static void
handle_timing_event_locked(struct t_led_sync_refinement *refinement,
                           const struct t_timing_event_camera_exposure_start *event)
{
	if (event->exposure_time_ns > 0) {
		refinement->has_exposure_time = true;
		refinement->exposure_time_ns = event->exposure_time_ns;
	}

	assert(event->frame_period_ns > 0); // No frame period present. Please estimate this externally.

	refinement->exposure_interval_ns = event->frame_period_ns;
	refinement->frames_since_last_visually_seen++;
	refinement->current_sequence_id = event->sequence_id;

	// We have an exposure interval now, start searching!
	if (refinement->phase == T_LED_SYNC_SEARCH_PHASE_INIT) {
		set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_FIND_INITIAL_OFFSET);
	}

	// Not applied yet
	if (!refinement->sample_applied) {
		return;
	}

	timepoint_ns now_ns = os_monotonic_get_ns();

	// Let the device settle for a few frames after applying a sample before we start trying to refine again
	if (frames_since_sample_apply(refinement, now_ns) < refinement->options.settle_frames) {
		return;
	}

	bool trigger_resync = refinement->frames_since_last_visually_seen >
	                          (refinement->options.time_to_resync_ns / refinement->exposure_interval_ns) &&
	                      refinement->phase > T_LED_SYNC_SEARCH_PHASE_FIND_INITIAL_OFFSET;
	// If we're past the initial find and we haven't seen the controller in some amount of
	// time, start searching for an offset again.
	if (trigger_resync) {
		LOG_DEBUG(refinement, "Triggering resync due to not being visually seen for %" PRIu32 " frames",
		          refinement->frames_since_last_visually_seen);
		set_phase_locked(refinement, T_LED_SYNC_SEARCH_PHASE_FIND_INITIAL_OFFSET);
	}

	switch (refinement->phase) {
	case T_LED_SYNC_SEARCH_PHASE_INIT: return;
	case T_LED_SYNC_SEARCH_PHASE_FIND_INITIAL_OFFSET: {
		handle_find_initial_offset(refinement);
		break;
	}
	case T_LED_SYNC_SEARCH_PHASE_FIND_RIGHT_EDGE: {
		handle_find_right_edge(refinement);
		break;
	}
	case T_LED_SYNC_SEARCH_PHASE_FIND_LEFT_EDGE: {
		handle_find_left_edge(refinement);
		break;
	}
	case T_LED_SYNC_SEARCH_PHASE_REFINE_BLINK_DURATION: {
		handle_refine_blink_duration(refinement);
		break;
	}
	case T_LED_SYNC_SEARCH_PHASE_MAINTAIN_OFFSET: {
		handle_maintain_offset(refinement);
		break;
	}
	default: break;
	}
}

static void
handle_constellation_sample_locked(struct t_led_sync_refinement *refinement,
                                   const struct t_constellation_tracker_sample *sample)
{
	refinement->frames_since_last_visually_seen = 0;
}

/*
 * Exported functions
 */

int
t_led_sync_refinement_init(struct t_led_sync_refinement *refinement,
                           const struct t_led_sync_refinement_options *options)
{
	int result;

	(*refinement) = XRT_C11_COMPOUND(struct t_led_sync_refinement){
	    .initialized = false,

	    .log_level = debug_get_log_option_led_sync_refinement_log(),

	    .options = *options,

	    .has_exposure_time = false,
	    .exposure_time_ns = 0,

	    .exposure_interval_ns = 0,

	    // Push an initial sample
	    .has_sample_for_driver = true,
	    .sample_for_driver =
	        {
	            .timestamp.device_host_latency_ns = 0,
	            .timestamp_mode = T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_DEVICE_HOST_LATENCY,
	            .fudge_offset_ns = 0,
	            .blink_duration_ns = options->initial_blink_duration_ns,
	        },
	    .sample_applied = false,

	    .phase = T_LED_SYNC_SEARCH_PHASE_INIT,
	    .current_latency_offset_ns = 0,
	    .current_blink_fudge_ns = 0,
	    .current_blink_duration_ns = options->initial_blink_duration_ns,
	    .frames_since_last_visually_seen = 0,
	    .current_sequence_id = 0,
	    .last_sample_apply_time_ns = 0,

	    .found_left_edge_ns = -1,
	    .found_right_edge_ns = -1,

	    .binary_search_state =
	        {
	            .left_bound_ns = 0,
	            .right_bound_ns = 0,
	        },
	};

	result = os_mutex_init(&refinement->lock);
	if (result < 0) {
		return result;
	}

	refinement->initialized = true;

	u_var_add_root(refinement, "LED Sync Refinement", true);

	u_var_add_gui_header(refinement, NULL, "Options");
	u_var_add_ro_i32(refinement, (int32_t *)&refinement->options.flags, "flags");
	u_var_add_ro_i64_ns(refinement, &refinement->options.initial_blink_duration_ns, "initial_blink_duration_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->options.min_blink_duration_ns, "min_blink_duration_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->options.max_blink_duration_ns, "max_blink_duration_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->options.time_to_resync_ns, "time_to_resync_ns");
	u_var_add_ro_u32(refinement, &refinement->options.settle_frames, "settle_frames");
	u_var_add_ro_i64_ns(refinement, &refinement->options.latency_cap_ns, "latency_cap_ns");

	u_var_add_gui_header(refinement, NULL, "Exposure Info");
	u_var_add_bool(refinement, &refinement->has_exposure_time, "Has Exposure Time");
	u_var_add_ro_i64_ns(refinement, &refinement->exposure_time_ns, "Exposure Time (ns)");
	u_var_add_ro_i64_ns(refinement, &refinement->exposure_interval_ns, "Exposure Interval (ns)");

	u_var_add_gui_header(refinement, NULL, "Driver Sample");
	u_var_add_bool(refinement, &refinement->has_sample_for_driver, "Has Sample For Driver");
	u_var_add_i32(refinement, (int32_t *)&refinement->sample_for_driver.timestamp_mode, "Sample Timestamp Mode");
	u_var_add_ro_i64_ns(refinement, &refinement->sample_for_driver.timestamp.device_host_latency_ns,
	                    "Sample Device Host Latency (ns)");
	u_var_add_ro_i64_ns(refinement, &refinement->sample_for_driver.fudge_offset_ns, "Sample Fudge Offset (ns)");
	u_var_add_ro_i64_ns(refinement, &refinement->sample_for_driver.blink_duration_ns, "Sample Blink Duration (ns)");
	u_var_add_bool(refinement, &refinement->sample_applied, "Sample Applied");

	u_var_add_gui_header(refinement, NULL, "State");
	u_var_add_ro_u32(refinement, (uint32_t *)&refinement->phase, "phase");
	u_var_add_ro_i64_ns(refinement, &refinement->current_latency_offset_ns, "current_latency_offset_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->current_blink_fudge_ns, "current_blink_fudge_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->current_blink_duration_ns, "current_blink_duration_ns");
	u_var_add_ro_u32(refinement, &refinement->frames_since_last_visually_seen, "frames_since_last_visually_seen");
	u_var_add_ro_u32(refinement, &refinement->current_sequence_id, "current_sequence_id");
	u_var_add_ro_i64_ns(refinement, &refinement->last_sample_apply_time_ns, "last_sample_apply_time_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->found_left_edge_ns, "found_left_edge_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->found_right_edge_ns, "found_right_edge_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->binary_search_state.left_bound_ns, "binary_search.left_bound_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->binary_search_state.right_bound_ns,
	                    "binary_search.right_bound_ns");
	u_var_add_ro_i64_ns(refinement, &refinement->blink_time_refinement_state.last_good_blink_duration_ns,
	                    "blink_time_refinement.last_good_blink_duration_ns");
	u_var_add_bool(refinement, &refinement->blink_time_refinement_state.backing_off,
	               "blink_time_refinement.backing_off");
	u_var_add_ro_i64_ns(refinement, &refinement->optical_driven_offset.driver_host_device_clock_offset_ns,
	                    "optical_driven_offset.driver_host_device_clock_offset_ns");
	u_var_add_bool(refinement, &refinement->optical_driven_offset.has_saved_offset,
	               "optical_driven_offset.has_saved_offset");
	u_var_add_ro_i64_ns(refinement, &refinement->optical_driven_offset.saved_offset,
	                    "optical_driven_offset.saved_offset");

	return 0;
}

void
t_led_sync_refinement_destroy(struct t_led_sync_refinement *refinement)
{
	if (!refinement->initialized) {
		return;
	}

	os_mutex_destroy(&refinement->lock);
}

void
t_led_sync_push_timing_event(struct t_led_sync_refinement *refinement,
                             const struct t_timing_event_camera_exposure_start *event)
{
	assert(refinement->initialized);

	os_mutex_lock(&refinement->lock);
	handle_timing_event_locked(refinement, event);
	os_mutex_unlock(&refinement->lock);
}

void
t_led_sync_push_constellation_sample(struct t_led_sync_refinement *refinement,
                                     const struct t_constellation_tracker_sample *sample)
{
	assert(refinement->initialized);

	os_mutex_lock(&refinement->lock);
	handle_constellation_sample_locked(refinement, sample);
	os_mutex_unlock(&refinement->lock);
}

bool
t_led_sync_get_sample(struct t_led_sync_refinement *refinement, struct t_led_sync_sample *out_sample)
{
	assert(refinement->initialized);

	os_mutex_lock(&refinement->lock);
	{
		if (!refinement->has_sample_for_driver) {
			os_mutex_unlock(&refinement->lock);
			return false;
		}

		*out_sample = refinement->sample_for_driver;
		refinement->has_sample_for_driver = false;
	}
	os_mutex_unlock(&refinement->lock);

	return true;
}

void
t_led_sync_mark_latest_sample_applied(struct t_led_sync_refinement *refinement, timepoint_ns apply_time_ns)
{
	assert(refinement->initialized);

	os_mutex_lock(&refinement->lock);
	refinement->sample_applied = true;
	refinement->last_sample_apply_time_ns = apply_time_ns;
	os_mutex_unlock(&refinement->lock);
}

void
t_led_sync_push_host_device_clock_offset(struct t_led_sync_refinement *refinement, time_duration_ns clock_offset_ns)
{
	assert(refinement->initialized);

	os_mutex_lock(&refinement->lock);
	{
		refinement->optical_driven_offset.driver_host_device_clock_offset_ns = clock_offset_ns;
	}
	os_mutex_unlock(&refinement->lock);
}
