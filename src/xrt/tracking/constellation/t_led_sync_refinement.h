// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A routine to automatically refine latency offsets of LED blink times using constellation samples.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "util/u_time.h"
#include "util/u_logging.h"

#include "os/os_threading.h"

#include "tracking/t_time_sync.h"
#include "tracking/t_constellation.h"


//! 10 seconds after last visually seen sample to resync
#define T_LED_SYNC_DEFAULT_RESYNC_TIME (time_duration_ns)(U_TIME_1S_IN_NS * 5LL)

//! Flags to control the behavior of the LED sync refinement routine.
enum t_led_sync_refinement_flags
{
	T_LED_SYNC_REFINEMENT_FLAGS_NONE = 0,
	//! Whether to try to optimize the blink duration after finding an offset, to lessen power usage.
	T_LED_SYNC_REFINEMENT_FLAGS_BLINK_DURATION = 1 << 0,
	/*!
	 * Whether to lock in a clock offset and have it be driven entirely by the sync refinement, utilizing the
	 * optical samples to measure clock skew/offset between the device and host, rather than the driver's normal
	 * remote clock synchronization.
	 *
	 * Use this flag when the driver's clock tracking is very noisy and is causing LED sync to be unreliable. But
	 * currently will cause LED sync to eventually drift as clock skew between the devices takes effect.
	 */
	T_LED_SYNC_REFINEMENT_FLAGS_OPTICAL_DRIVEN_OFFSET = 1 << 1,
	/*!
	 * Whether the device has specified a maximum latency cap. This prevents the sync refinement from searching
	 * parts of the exposure that aren't possible, which can cause refinement to sometimes take longer.
	 */
	T_LED_SYNC_REFINEMENT_FLAGS_HAS_LATENCY_CAP = 1 << 2,
};

//! Options for the LED sync refinement.
struct t_led_sync_refinement_options
{
	//! The flags to use when refining.
	enum t_led_sync_refinement_flags flags;

	//! How long the LEDs should be initially blinking for, in nanoseconds.
	time_duration_ns initial_blink_duration_ns;

	//! The minimum duration to use for the LED blinks, if using reduction.
	time_duration_ns min_blink_duration_ns;
	//! The maximum duration to use for the LED blinks, if using reduction.
	time_duration_ns max_blink_duration_ns;

	//! Time to wait after the last visually seen sample before resyncing.
	time_duration_ns time_to_resync_ns;
	//! Frames to wait after the sample was applied to let the device settle.
	uint32_t settle_frames;

	//! The maximum latency offset to use, if using a latency cap.
	time_duration_ns latency_cap_ns;
};

enum t_led_sync_sample_timestamp_mode
{
	T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_INVALID = 0,
	//! Used when the LED sync refinement is just computing a basic latency offset.
	T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_DEVICE_HOST_LATENCY = 1,
	//! Used when the LED sync refinement is fully deriving the clock offset, latency included.
	T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_HOST_DEVICE_CLOCK_OFFSET = 2,
};

//! A sample read out from the driver, to pass to the device in question.
struct t_led_sync_sample
{
	union {
		//! The latency offset from the device timestamps to the host.
		time_duration_ns device_host_latency_ns;

		//! The clock offset from the host timestamp to the device.
		time_duration_ns host_device_clock_offset_ns;
	} timestamp;
	enum t_led_sync_sample_timestamp_mode timestamp_mode;

	//! The latency offset to apply to fudge the blink to line up as best as possible with the exposure.
	time_duration_ns fudge_offset_ns;
	//! The duration to make the LEDs blink for, in nanoseconds.
	time_duration_ns blink_duration_ns;
};

enum t_led_sync_phase
{
	//! The initial phase, where we haven't made any adjustments yet.
	T_LED_SYNC_SEARCH_PHASE_INIT = 0,
	//! Trying to find some offset that gets us in sync at all.
	T_LED_SYNC_SEARCH_PHASE_FIND_INITIAL_OFFSET = 1,
	//! Trying to align the rising edge of the LED blinks with the falling edge of the camera exposure.
	T_LED_SYNC_SEARCH_PHASE_FIND_RIGHT_EDGE = 2,
	//! Trying to align the falling edge of the LED blinks with the rising edge of the camera exposure.
	T_LED_SYNC_SEARCH_PHASE_FIND_LEFT_EDGE = 3,
	//! We've found an offset, now we can optimize the blink duration to something that keeps it tracking.
	T_LED_SYNC_SEARCH_PHASE_REFINE_BLINK_DURATION = 4,
	//! We've found an offset and optimized the center of the LED blink to the center of the exposure.
	T_LED_SYNC_SEARCH_PHASE_MAINTAIN_OFFSET = 5,
};

struct t_led_sync_refinement
{
	//! Whether the structure has been fully initialized.
	bool initialized;

	enum u_logging_level log_level;

	//! The options to use for refinement
	struct t_led_sync_refinement_options options;

	struct os_mutex lock;

	/*!
	 * Whether we know the actual exposure time of the frames. We may not, so we have to work without it in some
	 * cases.
	 */
	bool has_exposure_time;
	//! The known time the frame was exposed for.
	time_duration_ns exposure_time_ns;

	//! The known interval between frames.
	time_duration_ns exposure_interval_ns;

	//! Whether we have a sample ready to be sent to the driver/device.
	bool has_sample_for_driver;
	//! The sample to be sent to the driver/device on it's next convenience.
	struct t_led_sync_sample sample_for_driver;
	//! Whether the sample has been applied
	bool sample_applied;

	//! The current search phase
	enum t_led_sync_phase phase;

	/*!
	 * The current estimated latency offset between the device and the host, in nanoseconds. This is what we are
	 * trying to refine.
	 */
	time_duration_ns current_latency_offset_ns;
	/*!
	 * The amount of fudge between the latency offset and the actual blink start time, in nanoseconds, so we can
	 * place the blink at the optimal time for the exposure.
	 */
	time_duration_ns current_blink_fudge_ns;

	//! How long the LEDs blink for, each frame.
	time_duration_ns current_blink_duration_ns;

	//! The amount of frames since the controller was last visually seen.
	uint32_t frames_since_last_visually_seen;

	//! The sequence ID of the latest timing event we processed.
	uint32_t current_sequence_id;

	//! The time the latest sample was applied to the driver/device
	timepoint_ns last_sample_apply_time_ns;

	//! The found left edge of the exposure, -1 if not found yet.
	time_duration_ns found_left_edge_ns;
	//! The found right edge of the exposure, -1 if not found yet.
	time_duration_ns found_right_edge_ns;

	struct
	{
		//! The current left bound of the binary search, if we're in a find edge phase.
		time_duration_ns left_bound_ns;
		//! The current right bound of the binary search, if we're in a find edge phase.
		time_duration_ns right_bound_ns;
	} binary_search_state;

	struct
	{
		//! The last blink duration that didn't cause the device to become unstable.
		time_duration_ns last_good_blink_duration_ns;

		//! Whether we're currently trying to back off a lower blink duration.
		bool backing_off;
	} blink_time_refinement_state;

	struct
	{
		//! The latest estimated host->device clock offset from the driver.
		time_duration_ns driver_host_device_clock_offset_ns;

		bool has_saved_offset;
		time_duration_ns saved_offset;
	} optical_driven_offset;
};

int
t_led_sync_refinement_init(struct t_led_sync_refinement *refinement,
                           const struct t_led_sync_refinement_options *options);

void
t_led_sync_refinement_destroy(struct t_led_sync_refinement *refinement);

void
t_led_sync_push_timing_event(struct t_led_sync_refinement *refinement,
                             const struct t_timing_event_camera_exposure_start *event);

void
t_led_sync_push_constellation_sample(struct t_led_sync_refinement *refinement,
                                     const struct t_constellation_tracker_sample *sample);

bool
t_led_sync_get_sample(struct t_led_sync_refinement *refinement, struct t_led_sync_sample *out_sample);

void
t_led_sync_mark_latest_sample_applied(struct t_led_sync_refinement *refinement, timepoint_ns apply_time_ns);

/*!
 * Pushes a clock offset to the refinement routine, this is the offset *from* the host *to* the device.
 *
 * To convert a timestamp from host -> device domain, you would do:
 * `device_timestamp = host_timestamp + clock_offset_ns`
 */
void
t_led_sync_push_host_device_clock_offset(struct t_led_sync_refinement *refinement, time_duration_ns clock_offset_ns);
