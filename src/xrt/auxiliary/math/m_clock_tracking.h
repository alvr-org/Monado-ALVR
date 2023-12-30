// Copyright 2022, Collabora, Ltd.
// Copyright 2024, Jan Schmidt
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers to estimate offsets between clocks
 * @author Mateo de Mayo <mateo.demayo@collabora.com>
 * @ingroup aux_math
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "util/u_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Helper to estimate the offset between two clocks using exponential smoothing.
 *
 * Given a sample from two timestamp domains A and B that should have been
 * sampled as close as possible, together with an estimate of the offset between
 * A clock and B clock (or zero), it applies a smoothing average on the
 * estimated offset and returns @p a in B clock.
 *
 * This estimator can be used when clock observations are arriving with a low
 * delay and small jitter, or when accuracy is less important (on the order of
 * the jitter that is present). It is very computationally cheap.
 *
 * @param freq About how many times per second this function is called. Helps setting a good decay value.
 * @param a Timestamp in clock A of the event
 * @param b Timestamp in clock B of the event
 * @param[in,out] inout_a2b Pointer to the current offset estimate from A to B, or 0 if unknown.
 * Value pointed-to will be updated.
 * @return timepoint_ns @p a in B clock
 */
static inline timepoint_ns
m_clock_offset_a2b(float freq, timepoint_ns a, timepoint_ns b, time_duration_ns *inout_a2b)
{
	// This formulation of exponential filtering uses a fixed-precision integer for the
	// alpha value and operates on the delta between the old and new a2b to avoid
	// precision / overflow problems.

	// Totally arbitrary way of computing alpha, if you have a better one, replace it
	const time_duration_ns alpha = 1000 * (1.0 - 12.5 / freq); // Weight to put on accumulated a2b
	time_duration_ns old_a2b = *inout_a2b;
	time_duration_ns got_a2b = b - a;
	time_duration_ns new_a2b;
	if (old_a2b == 0) { // a2b has not been set yet
		new_a2b = got_a2b;
	} else {
		new_a2b = ((old_a2b - got_a2b) * alpha) / 1000 + got_a2b;
	}
	*inout_a2b = new_a2b;
	return a + new_a2b;
}

/*!
 * Helper to estimate the offset between two clocks using a windowed
 * minimum-skew estimation plus exponential smoothing. The algorithm
 * tracks the smallest offset within the window, on the theory that
 * minima represent samples with the lowest transmission delay and jitter.
 *
 * More computationally intensive than the simple m_clock_offset_a2b estimator,
 * but can estimate a clock with accuracy in the microsecond range
 * even in the presence of 10s of milliseconds of jitter.
 *
 * Based on the approach in Dominique Fober, Yann Orlarey, Stéphane Letz.
 * Real Time Clock Skew Estimation over Network Delays. [Technical Report] GRAME. 2005.
 * https://hal.science/hal-02158803/document
 */
struct m_clock_windowed_skew_tracker;

/*!
 * Allocate a struct m_clock_windowed_skew_tracker with a
 * window of @param window_samples samples.
 */
struct m_clock_windowed_skew_tracker *
m_clock_windowed_skew_tracker_alloc(const size_t window_samples);
void
m_clock_windowed_skew_tracker_reset(struct m_clock_windowed_skew_tracker *t);
void
m_clock_windowed_skew_tracker_destroy(struct m_clock_windowed_skew_tracker *t);

void
m_clock_windowed_skew_tracker_push(struct m_clock_windowed_skew_tracker *t,
                                   const timepoint_ns local_ts,
                                   const timepoint_ns remote_ts);

bool
m_clock_windowed_skew_tracker_to_local(struct m_clock_windowed_skew_tracker *t,
                                       const timepoint_ns remote_ts,
                                       timepoint_ns *local_ts);
bool
m_clock_windowed_skew_tracker_to_remote(struct m_clock_windowed_skew_tracker *t,
                                        const timepoint_ns local_ts,
                                        timepoint_ns *remote_ts);

#ifdef __cplusplus
}
#endif
