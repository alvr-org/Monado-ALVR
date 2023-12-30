// Copyright 2024, Jan Schmidt
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers to estimate offsets between clocks
 * @author Jan Schmidt <jan@centricular.com>
 * @ingroup aux_math
 */
#include "util/u_misc.h"
#include "m_clock_tracking.h"

/* Fixed constants for discontinuity detection and
 * subsequent hold-off. These could be made configurable
 * if that turns out to be desirable */
const time_duration_ns CLOCK_RESET_THRESHOLD = 100 * U_TIME_1MS_IN_NS;
const time_duration_ns CLOCK_RESET_HOLDOFF = 30 * U_TIME_1MS_IN_NS;

struct m_clock_observation
{
	timepoint_ns local_ts; /* Timestamp from local / reference clock */
	time_duration_ns skew; /* skew = local_ts - remote_ts */
};

static struct m_clock_observation
m_clock_observation_init(timepoint_ns local_ts, timepoint_ns remote_ts)
{
	struct m_clock_observation ret = {
	    .local_ts = local_ts,
	    .skew = local_ts - remote_ts,
	};
	return ret;
}

struct m_clock_windowed_skew_tracker
{
	/* Maximum size of the window in samples */
	size_t max_window_samples;
	/* Current size of the window in samples (smaller than maximum after init or reset) */
	size_t current_window_samples;

	/* Observations ringbuffer window */
	struct m_clock_observation *window;
	/* Position in the observations window */
	size_t current_window_pos;

	/* Track the position in the window of the smallest
	 * skew value */
	time_duration_ns current_min_skew;
	size_t current_min_skew_pos;

	/* the most recently submitted observation */
	bool have_last_observation;
	struct m_clock_observation last_observation;

	/* Last discontinuity timestamp, used for holdoff after a reset */
	timepoint_ns clock_reset_ts;

	/* Smoothing and output */
	bool have_skew_estimate;
	timepoint_ns current_local_anchor;
	time_duration_ns current_skew; /* Offset between local time and the remote */
};

struct m_clock_windowed_skew_tracker *
m_clock_windowed_skew_tracker_alloc(const size_t window_samples)
{
	struct m_clock_windowed_skew_tracker *t = U_TYPED_CALLOC(struct m_clock_windowed_skew_tracker);
	if (t == NULL) {
		return NULL;
	}

	t->window = U_TYPED_ARRAY_CALLOC(struct m_clock_observation, window_samples);
	if (t->window == NULL) {
		free(t);
		return NULL;
	}

	t->max_window_samples = window_samples;

	return t;
}

void
m_clock_windowed_skew_tracker_reset(struct m_clock_windowed_skew_tracker *t)
{
	// Clear time tracking
	t->have_last_observation = false;
	t->current_window_samples = 0;
}

void
m_clock_windowed_skew_tracker_destroy(struct m_clock_windowed_skew_tracker *t)
{
	free(t->window);
	free(t);
}

void
m_clock_windowed_skew_tracker_push(struct m_clock_windowed_skew_tracker *t,
                                   const timepoint_ns local_ts,
                                   const timepoint_ns remote_ts)
{
	struct m_clock_observation obs = m_clock_observation_init(local_ts, remote_ts);

	if (t->have_last_observation) {
		time_duration_ns skew_delta = t->last_observation.skew - obs.skew;
		if (-skew_delta >= CLOCK_RESET_THRESHOLD || skew_delta >= CLOCK_RESET_THRESHOLD) {
			// Too large a delta between observations. Reset the smoothing to adapt more quickly
			t->clock_reset_ts = local_ts;
			t->current_window_pos = 0;
			t->current_window_samples = 0;

			t->have_last_observation = true;
			t->last_observation = obs;
			return;
		}

		// After a reset, ignore all samples briefly in order to
		// let the new timeline settle.
		if (local_ts - t->clock_reset_ts < CLOCK_RESET_HOLDOFF) {
			return;
		}
		t->clock_reset_ts = 0;
	}
	t->last_observation = obs;

	if (t->current_window_samples < t->max_window_samples) {
		/* Window is still being filled */

		if (t->current_window_pos == 0) {
			/* First sample. Take it as-is */
			t->current_min_skew = t->current_skew = obs.skew;
			t->current_local_anchor = local_ts;
			t->current_min_skew_pos = 0;
		} else if (obs.skew <= t->current_min_skew) {
			/* We found a new minimum. Take it */
			t->current_min_skew = obs.skew;
			t->current_local_anchor = local_ts;
			t->current_min_skew_pos = t->current_window_pos;
		}

		/* Grow the stored observation array */
		t->window[t->current_window_samples++] = obs;

	} else if (obs.skew <= t->current_min_skew) {
		/* Found a new minimum skew. */
		t->window[t->current_window_pos] = obs;

		t->current_local_anchor = local_ts;
		t->current_min_skew = obs.skew;
		t->current_min_skew_pos = t->current_window_pos;
	} else if (t->current_window_pos == t->current_min_skew_pos) {
		/* Replacing the previous minimum skew. Find the new minimum */
		t->window[t->current_window_pos] = obs;

		struct m_clock_observation *new_min = &t->window[0];
		size_t new_min_index = 0;

		for (size_t i = 1; i < t->current_window_samples; i++) {
			struct m_clock_observation *cur = &t->window[i];
			if (cur->skew <= new_min->skew) {
				new_min = cur;
				new_min_index = i;
			}
		}

		t->current_local_anchor = new_min->local_ts;
		t->current_min_skew = new_min->skew;
		t->current_min_skew_pos = new_min_index;
	} else {
		/* Just insert the observation */
		t->window[t->current_window_pos] = obs;
	}

	/* Wrap around the window index */
	t->current_window_pos = (t->current_window_pos + 1) % t->max_window_samples;

	/* Update the moving average skew */
	size_t w = t->current_window_samples;
	t->current_skew = (t->current_min_skew + t->current_skew * (w - 1)) / w;
	t->have_skew_estimate = true;
}

bool
m_clock_windowed_skew_tracker_to_local(struct m_clock_windowed_skew_tracker *t,
                                       const timepoint_ns remote_ts,
                                       timepoint_ns *local_ts)
{
	if (!t->have_skew_estimate) {
		return false;
	}

	*local_ts = remote_ts + t->current_skew;
	return true;
}

bool
m_clock_windowed_skew_tracker_to_remote(struct m_clock_windowed_skew_tracker *t,
                                        const timepoint_ns local_ts,
                                        timepoint_ns *remote_ts)
{
	if (!t->have_skew_estimate) {
		return false;
	}

	*remote_ts = local_ts - t->current_skew;
	return true;
}
