// Copyright 2020-2025, Collabora, Ltd.
// Copyright 2025-2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Driver for the Oculus Rift.
 *
 * Based largely on simulated_hmd.c, with reference to the DK2 firmware and OpenHMD's rift driver.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift
 */

#include "os/os_time.h"

#include "xrt/xrt_defines.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_results.h"
#include "xrt/xrt_compiler.h"

#include "math/m_relation_history.h"
#include "math/m_clock_tracking.h"
#include "math/m_api.h"
#include "math/m_space.h"
#include "math/m_vec3.h"
#include "math/m_mathinclude.h" // IWYU pragma: keep

#include "tracking/t_dead_reckoning.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_distortion_mesh.h"
#include "util/u_logging.h"
#include "util/u_misc.h"
#include "util/u_time.h"
#include "util/u_var.h"
#include "util/u_visibility_mask.h"
#include "util/u_trace_marker.h"
#include "util/u_linux.h"
#include "util/u_truncate_printf.h"

#include "b_timing_source.h"

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#include "rift_internal.h"
#include "rift_usb.h"
#include "rift_radio.h"


/*
 *
 * Structs and defines.
 *
 */

DEBUG_GET_ONCE_LOG_OPTION(rift_log, "RIFT_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_FLOAT_OPTION(rift_override_icd_mm, "RIFT_OVERRIDE_ICD", -1.0f)
DEBUG_GET_ONCE_BOOL_OPTION(rift_use_firmware_distortion, "RIFT_USE_FIRMWARE_DISTORTION", false)
DEBUG_GET_ONCE_BOOL_OPTION(rift_power_override, "RIFT_POWER_OVERRIDE", false)
DEBUG_GET_ONCE_FLOAT_OPTION(rift_startup_wait_time, "RIFT_STARTUP_WAIT_TIME", 5.0f)
DEBUG_GET_ONCE_BOOL_OPTION(rift_enable_back_strap_leds, "RIFT_ENABLE_BACK_STRAP_LEDS", false)

//! LEDs further behind the origin than this, along the device's backward (+Z) axis, are considered part of the back
//! strap and disabled by @ref rift_upload_back_strap_disable_pattern.
#define RIFT_BACK_STRAP_THRESHOLD_M 0.1f // 100mm

// The headset doesn't like it when you give a sequence of length 1 for some reason, so let's give it what it wants.
#define RIFT_CUSTOM_PATTERN_SEQUENCE_LENGTH 10

//! Set to 1 to dump IMU data to @ref RIFT_IMU_DUMP_PATH
#define RIFT_IMU_DUMP 0
//! The path where IMU data is dumped to.
#define RIFT_IMU_DUMP_PATH "/tmp/cv1_imu_dump.csv"

/*
 *
 * Headset functions
 *
 */

//! Repeats a 2-bit LED state across all @ref RIFT_CUSTOM_PATTERN_SEQUENCE_LENGTH phases of a custom pattern sequence.
static uint32_t
rift_custom_pattern_sequence_repeat(enum rift_custom_pattern_state state)
{
	uint32_t sequence = 0;
	for (int phase = 0; phase < RIFT_CUSTOM_PATTERN_SEQUENCE_LENGTH; phase++) {
		sequence |= ((uint32_t)state & 0x3) << (phase * 2);
	}
	return sequence;
}

static int
rift_sensor_thread_tick(struct rift_hmd *hmd)
{
	uint8_t buf[REPORT_MAX_SIZE];
	int result;

	int64_t now = os_monotonic_get_ns();

	if (now - hmd->last_keepalive_time > KEEPALIVE_SEND_RATE_NS) {
		result = rift_send_keepalive(hmd);

		if (result < 0) {
			HMD_ERROR(hmd, "Got error sending keepalive, assuming fatal, reason %d", result);
			return result;
		}
	}

	result = os_hid_read(hmd->hmd_dev, buf, sizeof(buf), IMU_SAMPLE_RATE);
	timepoint_ns recv_time_ns = os_monotonic_get_ns() - RIFT_USB_LATENCY_BIAS;

	if (result < 0) {
		HMD_ERROR(hmd, "Got error reading from device, assuming fatal, reason %d", result);
		return result;
	}

	if (result == 0) {
		HMD_WARN(hmd, "Timed out waiting for packet from headset, packets should come in at %dhz",
		         IMU_SAMPLE_RATE);
		return 0;
	}

	switch (hmd->variant) {
	case RIFT_VARIANT_CV1:
	case RIFT_VARIANT_DK2: {
		// CV1 sends this every couple seconds when there's nothing connected to the radio
		if (buf[0] == IN_REPORT_CV1_RADIO_KEEPALIVE && hmd->variant == RIFT_VARIANT_CV1) {
			break;
		}

		// skip unknown commands
		if (buf[0] != IN_REPORT_DK2) {
			HMD_WARN(hmd, "Skipping unknown IN command %d", buf[0]);
			break;
		}

		struct dk2_in_report report;

		// don't treat invalid IN reports as fatal, just ignore them
		if (result < (int)sizeof(report)) {
			HMD_WARN(hmd, "Got malformed DK2 IN report with size %d", result);
			break;
		}

		// TODO: handle endianness
		memcpy(&report, buf + 1, sizeof(report));

		if (hmd->variant == RIFT_VARIANT_CV1) {
			hmd->presence = report.cv1.presence_sensor > 3;

			// TODO: figure out the actual algorithm for this, this one is too silly
			float iad_mm = (float)(report.cv1.iad_adc_value - 400) / 300.0f + 59.0f;

			// round to nearest half mm
			iad_mm = roundf(iad_mm * 2.0f) / 2.0f;

			// convert to meters
			hmd->default_icd = iad_mm / 1000.0f;
		}

		// if there's no samples, just do nothing.
		if (report.num_samples == 0) {
			break;
		}

		m_clock_bit_extender_update(&hmd->clock_extender, report.sample_timestamp, U_TIME_1US_IN_NS);

		m_clock_windowed_skew_tracker_push( //
		    hmd->clock_tracker,             //
		    recv_time_ns,                   //
		    hmd->clock_extender.clock);     //

		int64_t local_timestamp_ns;
		// if we haven't synchronized our clocks, just do nothing
		if (!m_clock_windowed_skew_tracker_to_local(hmd->clock_tracker, hmd->clock_extender.clock,
		                                            &local_timestamp_ns)) {
			break;
		}

		timepoint_ns remote_exposure_time_ns =
		    m_clock_bit_extender_peek(&hmd->clock_extender, report.tracking_timestamp, U_TIME_1US_IN_NS);

		// Update the exposure time
		uint16_t tracking_count_delta = report.tracking_count - hmd->last_tracking_count;
		hmd->last_tracking_count = report.tracking_count;
		hmd->exposure_counter += tracking_count_delta;

		if (tracking_count_delta != 0) {
			os_thread_helper_lock(&hmd->sensor_thread);
			timepoint_ns local_exposure_time_ns;
			bool have_local_exposure_time = m_clock_windowed_skew_tracker_to_local(
			    hmd->clock_tracker, remote_exposure_time_ns, &local_exposure_time_ns);

			// Record the exposure so that frames which arrive after the next exposure has already been
			// reported can still find the exposure they were taken during. Skipped until the clocks are
			// synchronized, since before that there is no local time to record.
			if (have_local_exposure_time) {
				hmd->exposure_history[hmd->exposure_history_pushed % RIFT_EXPOSURE_HISTORY_SIZE] =
				    (struct rift_exposure_event){
				        .sequence_id = hmd->exposure_counter,
				        .timestamp_ns = local_exposure_time_ns,
				        .recv_timestamp_ns = recv_time_ns,
				    };
				hmd->exposure_history_pushed++;
			}
			os_thread_helper_unlock(&hmd->sensor_thread);

			if (have_local_exposure_time && hmd->timing_event_sink) {
				struct t_timing_event event = {
				    .type = T_TIMING_EVENT_TYPE_CAMERA_EXPOSURE_START,
				    .camera_exposure_start =
				        {
				            .sequence_id = hmd->exposure_counter,
				            .timestamp_ns = local_exposure_time_ns,
				            .frame_period_ns = hmd->tracking.frame_interval * U_TIME_1US_IN_NS,
				            .exposure_time_ns =
				                (time_duration_ns)(hmd->tracking.exposure_length * U_TIME_1US_IN_NS),
				        },
				};

				t_timing_event_sink_push_timing_event(hmd->timing_event_sink, &event);
			}
		}

		if (report.num_samples > 1)
			HMD_TRACE(hmd,
			          "Had more than one sample queued! We aren't receiving IN reports fast enough, HMD "
			          "had %d samples in the queue! Having to work back that first sample...",
			          report.num_samples);

		const int num_samples = MIN(DK2_MAX_SAMPLES, report.num_samples);

		for (int i = 0; i < num_samples; i++) {
			struct rift_dk2_sample_pack latest_sample_pack = report.samples[i];

			struct xrt_vec3 accel, gyro;
			rift_unpack_float_sample(latest_sample_pack.accel.data, 0.0001f, &accel);
			rift_unpack_float_sample(latest_sample_pack.gyro.data, 0.0001f, &gyro);

			// If the HMD is not doing it's own calibration, we need to apply it now
			if (hmd->imu_needs_calibration) {
				math_matrix_3x3_transform_vec3(&hmd->imu_calibration.gyro_matrix, &gyro, &gyro);
				math_vec3_subtract(&hmd->imu_calibration.gyro_offset, &gyro);

				math_matrix_3x3_transform_vec3(&hmd->imu_calibration.accel_matrix, &accel, &accel);
				math_vec3_subtract(&hmd->imu_calibration.accel_offset, &accel);
			}

			// work back the likely timestamp of the current sample
			// if there's only one sample, then this will always be zero, if there's two or more samples,
			// the previous samples will be offset by the sample rate of the IMU
			int64_t sample_local_timestamp_ns =
			    local_timestamp_ns - ((num_samples - 1 - i) * NS_PER_SAMPLE);

			// drop packets which are in the past (TODO: figure out why these happen..)
			if (sample_local_timestamp_ns < hmd->last_sample_local_timestamp_ns) {
				break;
			}

			os_thread_helper_lock(&hmd->sensor_thread);
			{
				// update the IMU for that sample
				m_imu_3dof_update(&hmd->fusion, sample_local_timestamp_ns, &accel, &gyro);

				// Ensure these are monotonic. Sometimes due to the super fast IMU and clock jitter
				// we might estimate samples out of order a bit.
				if (sample_local_timestamp_ns > hmd->last_ff_timestamp_ns) {
					m_ff_vec3_f32_push(hmd->gyro_ff, &gyro, sample_local_timestamp_ns);
					m_ff_vec3_f32_push(hmd->accel_ff, &accel, sample_local_timestamp_ns);

					if (!hmd->fusion.grav.is_accel) {
						double grav_length = m_vec3_len(accel);
						m_ff_f64_push(hmd->gravity_correction, &grav_length,
						              sample_local_timestamp_ns);
					}

					hmd->last_ff_timestamp_ns = sample_local_timestamp_ns;
				}
			}
			os_thread_helper_unlock(&hmd->sensor_thread);

			if (hmd->constellation_imu_sink) {
				xrt_sink_push_imu(hmd->constellation_imu_sink,
				                  &(struct xrt_imu_sample){
				                      .timestamp_ns = sample_local_timestamp_ns,
				                      .accel_m_s2 = {accel.x, accel.y, accel.z},
				                      .gyro_rad_secs = {gyro.x, gyro.y, gyro.z},
				                  });
			}

#if RIFT_IMU_DUMP
			static FILE *imu_dump_file = NULL;
			if (imu_dump_file == NULL) {
				imu_dump_file = fopen(RIFT_IMU_DUMP_PATH, "wb");
				assert(imu_dump_file != NULL);
				fprintf(imu_dump_file, "ts,ax,ay,az,gx,gy,gz\n");
			}

			fprintf(imu_dump_file, "%" PRIi64 ",%f,%f,%f,%f,%f,%f\n", sample_local_timestamp_ns, accel.x,
			        accel.y, accel.z, gyro.x, gyro.y, gyro.z);
			fflush(imu_dump_file);
#endif

			hmd->last_sample_local_timestamp_ns = sample_local_timestamp_ns;

			// push the pose of the IMU for that sample, doing so per sample
			struct xrt_space_relation relation = XRT_SPACE_RELATION_ZERO;
			relation.relation_flags = (enum xrt_space_relation_flags)(
			    XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT | XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
			    XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT);
			relation.pose.orientation = hmd->fusion.rot;
			relation.angular_velocity = gyro;

			m_relation_history_push(hmd->relation_hist, &relation, sample_local_timestamp_ns);
		}

		break;
	}
	}

	return 0;
}

static void *
rift_sensor_thread(void *ptr)
{
	U_TRACE_SET_THREAD_NAME("Rift sensor thread");

	struct rift_hmd *hmd = (struct rift_hmd *)ptr;

	os_thread_helper_lock(&hmd->sensor_thread);

	// uncomment this to be able to see if things are actually progressing as expected in a debugger, without having
	// to count yourself
	// #define TICK_DEBUG

	int result = 0;
#ifdef TICK_DEBUG
	int ticks = 0;
#endif

	while (os_thread_helper_is_running_locked(&hmd->sensor_thread) && result >= 0) {
		os_thread_helper_unlock(&hmd->sensor_thread);

		result = rift_sensor_thread_tick(hmd);

		os_thread_helper_lock(&hmd->sensor_thread);
#ifdef TICK_DEBUG
		ticks += 1;
#endif
	}

	os_thread_helper_unlock(&hmd->sensor_thread);

	if (result < 0) {
		HMD_ERROR(hmd, "Sensor thread exiting due to error %d", result);
	} else {
		HMD_DEBUG(hmd, "Sensor thread exiting cleanly");
	}

	return NULL;
}

static int
rift_usb_thread_radio_tick(struct rift_hmd *hmd)
{
	int result;

	result = rift_radio_handle_read(hmd);
	if (result < 0) {
		return result;
	}

	result = rift_radio_handle_command(hmd);
	if (result < 0) {
		return result;
	}

	result = rift_radio_handle_haptics(hmd);
	if (result < 0) {
		return result;
	}

	return 0;
}

static void *
rift_radio_thread(void *ptr)
{
	struct rift_hmd *hmd = (struct rift_hmd *)ptr;

	const char *thread_name = "Rift Radio";

	U_TRACE_SET_THREAD_NAME(thread_name);
	os_thread_name_self(thread_name);

#ifdef XRT_OS_LINUX
	// Try to raise priority of this thread.
	u_linux_try_to_set_realtime_priority_on_thread(hmd->log_level, thread_name);
#endif

	os_thread_helper_lock(&hmd->radio_state.thread);

	int result = 0;
#if 0
	int ticks = 0;
#endif

	while (os_thread_helper_is_running_locked(&hmd->radio_state.thread) && result >= 0) {
		os_thread_helper_unlock(&hmd->radio_state.thread);

		result = rift_usb_thread_radio_tick(hmd);

		os_thread_helper_lock(&hmd->radio_state.thread);
#if 0
		ticks += 1;
#endif
	}

	os_thread_helper_unlock(&hmd->radio_state.thread);

	return NULL;
}

#define PARSE_MICROMETER_TRIPLET(out, values)                                                                          \
	do {                                                                                                           \
		out.x = MICROMETERS_TO_METERS(values[0]);                                                              \
		out.y = MICROMETERS_TO_METERS(values[1]);                                                              \
		out.z = MICROMETERS_TO_METERS(values[2]);                                                              \
	} while (0)

static int
rift_read_led_model(struct rift_hmd *hmd)
{
	int result;

	struct rift_position_calibration_report position_report;

	result = rift_get_position_calibration_report(hmd, &position_report);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to read position calibration report, reason %d", result);
		return result;
	}

	hmd->led_model.match_parameters = (struct t_constellation_tracker_led_model_match_parameters){
	    // Require a real good one if we don't have a prior, avoid ambiguities
	    .min_leds_for_correspondence_search_without_prior = 9,
	    .min_leds_for_correspondence_search_with_prior = 5,
	};

	// minus one to get rid of the IMU, which is also in here
	hmd->led_model.leds = U_TYPED_ARRAY_CALLOC(struct t_constellation_tracker_led, position_report.position_count);

	for (uint32_t i = 0; i < position_report.position_count; i++) {
		result = rift_get_position_calibration_report(hmd, &position_report);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to read position calibration report, reason %d", result);
			goto read_led_model_err;
		}

		if (position_report.position_type == RIFT_POSITION_CALIBRATION_TYPE_INERTIAL_SENSOR) {
			PARSE_MICROMETER_TRIPLET(hmd->T_device_imu.position, position_report.position);
			hmd->T_device_imu.position.x = -hmd->T_device_imu.position.x;
			hmd->T_device_imu.position.y = hmd->T_device_imu.position.y;
			hmd->T_device_imu.position.z = -hmd->T_device_imu.position.z;

			hmd->T_device_imu.orientation = (struct xrt_quat)XRT_QUAT_IDENTITY;

			// NOTE: we ignore the IMU orientation since it's always zeroed out..

			// Invert the pose for device relative to IMU
			math_pose_invert(&hmd->T_device_imu, &hmd->T_imu_device);
		} else {
			struct xrt_vec3 pos;
			PARSE_MICROMETER_TRIPLET(pos, position_report.position);

			struct xrt_vec3 normal;
			PARSE_MICROMETER_TRIPLET(normal, position_report.normal);
			math_vec3_normalize(&normal); // normalize the direction

			pos.x = -pos.x;
			pos.y = pos.y;
			pos.z = -pos.z;
			normal.x = -normal.x;
			normal.y = normal.y;
			normal.z = -normal.z;

			hmd->led_model.leds[hmd->led_model.led_count++] = (struct t_constellation_tracker_led){
			    .position = pos,
			    .normal = normal,
			    .radius_m = 0.0035f,                   // 3.5mm
			    .visibility_angle = DEG_TO_RAD(90.0f), // TODO: tune this value properly
			    .id = (t_constellation_led_id_it)position_report.position_index,
			};
		}
	}

	// Make the LEDs relative to the IMU
	for (size_t i = 0; i < hmd->led_model.led_count; i++) {
		struct xrt_vec3 led_pos = hmd->led_model.leds[i].position;
		hmd->led_model.leds[i].position = m_vec3_add(hmd->T_imu_device.position, led_pos);
	}

	return 0;

read_led_model_err:
	free(hmd->led_model.leds);
	hmd->led_model.leds = NULL;

	return result;
}

#undef PARSE_MICROMETER_TRIPLET

/*!
 * Uploads a custom pattern that holds every LED high except the back strap LEDs, which are held off, then drops
 * the back strap LEDs from @ref rift_hmd::led_model so the constellation tracker never tries to match blobs
 * against LEDs that are physically dark. The caller must have already set RIFT_TRACKING_CUSTOM_PATTERN on the
 * tracking report: writes to the CustomPattern report are no-ops until that bit is set.
 */
static int
rift_upload_back_strap_disable_pattern(struct rift_hmd *hmd)
{
	int result;
	const size_t total_count = hmd->led_model.led_count;
	size_t kept_count = 0;

	const uint32_t sequence_high = rift_custom_pattern_sequence_repeat(RIFT_CUSTOM_PATTERN_STAT_HIGH);
	const uint32_t sequence_off = rift_custom_pattern_sequence_repeat(RIFT_CUSTOM_PATTERN_STAT_OFF);

	for (size_t i = 0; i < total_count; i++) {
		// Copied out since the compaction below writes to index `kept_count`, which trails `i`.
		struct t_constellation_tracker_led led = hmd->led_model.leds[i];

		// Monado device space is -Z forward, so a LED further than the threshold along +Z sits behind the
		// origin by more than the threshold, i.e. on the back strap.
		bool is_back_strap = led.position.z > RIFT_BACK_STRAP_THRESHOLD_M;

		struct rift_custom_pattern_report pattern = {
		    .command_id = 0,
		    .sequence_length = RIFT_CUSTOM_PATTERN_SEQUENCE_LENGTH,
		    .sequence = is_back_strap ? sequence_off : sequence_high,
		    .led_index = led.id,
		    .led_count = (uint16_t)total_count,
		};

		result = rift_set_custom_pattern(hmd, &pattern);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to upload custom pattern for LED %d, reason %d", led.id, result);
			return result;
		}

		if (is_back_strap) {
			HMD_DEBUG(hmd, "Disabling LED %d as back strap, %.1fmm behind origin", led.id,
			          (double)(led.position.z * 1000.0f));
			continue;
		}

		// Dropped from the model as well as the hardware.
		hmd->led_model.leds[kept_count++] = led;
	}

	hmd->led_model.led_count = kept_count;

	HMD_INFO(hmd, "Disabled %zu back strap LED(s), %zu of %zu LEDs remain in the model.", total_count - kept_count,
	         kept_count, total_count);

	return 0;
}

static void
get_raw_pose(struct rift_hmd *hmd, timepoint_ns when_ns, struct xrt_relation_chain *xrc)
{
	struct xrt_space_relation relation = XRT_SPACE_RELATION_ZERO;
	if (hmd->use_constellation_poses) {
		if (t_constellation_tracker_get_tracked_pose(hmd->constellation_tracker, hmd->constellation_device_id,
		                                             when_ns, &relation) < 0) {
			relation = XRT_C11_COMPOUND(struct xrt_space_relation) XRT_SPACE_RELATION_ZERO;
		}
	}

	if (relation.relation_flags == 0) {
		m_relation_history_get(hmd->relation_hist, when_ns, &relation);
	}

	(*m_relation_chain_reserve(xrc)) = relation;
}

/*
 *
 * Distortion
 *
 */

// @todo remove clang-format off when CI is updated
// clang-format off
static const struct rift_lens_distortion DK2_DISTORTIONS[] = {
    {
        .distortion_version = RIFT_LENS_DISTORTION_LCSV_CATMULL_ROM_10_VERSION_1,
        .eye_relief = 0.008f,
        .data =
            {
                .lcsv_catmull_rom_10 =
                    {
                        .meters_per_tan_angle_at_center = 0.036f,
                        .max_r = 1.0f,
                        .chromatic_abberation = {-0.0112f, -0.015f, 0.0187f, 0.015f},
                        .k =
                            {
                                1.003f,
                                1.02f,
                                1.042f,
                                1.066f,
                                1.094f,
                                1.126f,
                                1.162f,
                                1.203f,
                                1.25f,
                                1.31f,
                                1.38f,
                            },
                    },
            },
    },
    {
        .distortion_version = RIFT_LENS_DISTORTION_LCSV_CATMULL_ROM_10_VERSION_1,
        .eye_relief = 0.018f,
        .data =
            {
                .lcsv_catmull_rom_10 =
                    {
                        .meters_per_tan_angle_at_center = 0.036f,
                        .max_r = 1.0f,
                        .chromatic_abberation = {-0.015f, -0.02f, 0.025f, 0.02f},
                        .k =
                            {
                                1.003f,
                                1.02f,
                                1.042f,
                                1.066f,
                                1.094f,
                                1.126f,
                                1.162f,
                                1.203f,
                                1.25f,
                                1.31f,
                                1.38f,
                            },
                    },
            },
    }};

static const struct rift_lens_distortion CV1_DISTORTIONS[] = {{
    .distortion_version = RIFT_LENS_DISTORTION_LCSV_CATMULL_ROM_10_VERSION_1,
    .eye_relief = 0.015f,
    .data =
        {
            .lcsv_catmull_rom_10 =
                {
                    .meters_per_tan_angle_at_center = 0.0438f,
                    .max_r = 1.0f,
                    .chromatic_abberation = {-0.008f, -0.005f, 0.015f, 0.005f},
                    .k =
                        {
                            1.000f,
                            1.0312999f,
                            1.0698f,
                            1.1155f,
                            1.173f,
                            1.2460001f,
                            1.336f,
                            1.457f,
                            1.630f,
                            1.900f,
                            2.3599999f,
                        },
                },
        },
}};
// clang-format on

static void
rift_fill_in_default_distortions(struct rift_hmd *hmd)
{
	switch (hmd->variant) {
	case RIFT_VARIANT_DK2: {
		hmd->num_lens_distortions = ARRAY_SIZE(DK2_DISTORTIONS);
		hmd->lens_distortions = DK2_DISTORTIONS;

		// TODO: let the user specify which distortion is in use with an env var,
		//       and interpolate the distortions for the user's specific eye relief setting
		hmd->distortion_in_use = 1;

		break;
	}
	case RIFT_VARIANT_CV1: {
		hmd->num_lens_distortions = ARRAY_SIZE(CV1_DISTORTIONS);
		hmd->lens_distortions = CV1_DISTORTIONS;

		// TODO: let the user specify which distortion is in use with an env var,
		//       and interpolate the distortions for the user's specific eye relief setting
		hmd->distortion_in_use = 0;

		break;
	}
	}
}

static void
rift_fill_in_distortion_inputs(struct rift_hmd *hmd)
{
	assert(hmd->lens_distortions != NULL);
	assert(hmd->distortion_in_use < hmd->num_lens_distortions);

	float lens_diameter_m = 0.0f;
	switch (hmd->variant) {
	// TODO: figure out the *real* value for CV1 by dumping it from LibOVR somehow
	case RIFT_VARIANT_CV1: lens_diameter_m = 0.05f; break;
	case RIFT_VARIANT_DK2: lens_diameter_m = 0.04f; break;
	}

	hmd->panel = XRT_C11_COMPOUND(struct u_rift_panel){
	    .size_m =
	        {
	            MICROMETERS_TO_METERS(hmd->display_info.display_width),
	            MICROMETERS_TO_METERS(hmd->display_info.display_height),
	        },
	    // No headset seen so far reports a nonzero gap.
	    .eye_gap_m = 0.0f,
	    .size_px =
	        {
	            (float)hmd->display_info.resolution_x,
	            (float)hmd->display_info.resolution_y,
	        },
	    .lens_center_separation_m = MICROMETERS_TO_METERS(hmd->display_info.lens_separation),
	    .lens_center_from_panel_top = MICROMETERS_TO_METERS(hmd->display_info.center_v),
	    .lens_diameter_m = lens_diameter_m,
	};

	const struct rift_lens_distortion *distortion = &hmd->lens_distortions[hmd->distortion_in_use];
	const struct rift_catmull_rom_distortion_data *data = &distortion->data.lcsv_catmull_rom_10;

	hmd->eye_profile = XRT_C11_COMPOUND(struct u_rift_eye_profile){
	    .eye_relief_m = distortion->eye_relief,
	    .k = data->k,
	    .k_count = CATMULL_COEFFICIENTS,
	    .chromatic_abberation =
	        {
	            data->chromatic_abberation[0],
	            data->chromatic_abberation[1],
	            data->chromatic_abberation[2],
	            data->chromatic_abberation[3],
	        },
	    .max_r = data->max_r,
	    .meters_per_tan_angle = data->meters_per_tan_angle_at_center,
	};
}

/*
 *
 * Driver functions
 *
 */

static void
rift_hmd_destroy(struct xrt_device *xdev)
{
	struct rift_hmd *hmd = rift_hmd(xdev);

	// Don't do anything but break apart the device
	hmd->node.break_apart(&hmd->node);
}

static xrt_result_t
rift_hmd_update_inputs(struct xrt_device *xdev)
{
	struct rift_hmd *hmd = rift_hmd(xdev);

	// Only the CV1 has a presence sensor
	if (hmd->variant != RIFT_VARIANT_CV1) {
		return XRT_SUCCESS;
	}

	hmd->base.inputs[1].value = (union xrt_input_value){.boolean = hmd->presence};
	hmd->base.inputs[1].timestamp = os_monotonic_get_ns();
	return XRT_SUCCESS;
}

static xrt_result_t
rift_hmd_get_tracked_pose(struct xrt_device *xdev,
                          enum xrt_input_name name,
                          int64_t at_timestamp_ns,
                          struct xrt_space_relation *out_relation)
{
	struct rift_hmd *hmd = rift_hmd(xdev);

	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		U_LOG_XDEV_UNSUPPORTED_INPUT(&hmd->base, hmd->log_level, name);
		return XRT_ERROR_INPUT_UNSUPPORTED;
	}

	struct xrt_relation_chain xrc = {0};

	if (hmd->use_constellation_poses) {
		// Constellation poses are in IMU space
		m_relation_chain_push_pose(&xrc, &hmd->T_imu_device);
	}

	get_raw_pose(hmd, at_timestamp_ns, &xrc);

	struct xrt_space_relation relation;
	m_relation_chain_resolve(&xrc, &relation);

	if ((relation.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0) {
		// If we provide an orientation, make sure that it is normalized.
		math_quat_normalize(&relation.pose.orientation);
	}

	*out_relation = relation;
	return XRT_SUCCESS;
}

static xrt_result_t
rift_hmd_get_view_poses(struct xrt_device *xdev,
                        const struct xrt_vec3 *default_eye_relation,
                        int64_t at_timestamp_ns,
                        enum xrt_view_type view_type,
                        uint32_t view_count,
                        struct xrt_space_relation *out_head_relation,
                        struct xrt_fov *out_fovs,
                        struct xrt_pose *out_poses)
{
	struct rift_hmd *hmd = rift_hmd(xdev);

	struct xrt_vec3 eye_relation = XRT_VEC3_ZERO;
	if (hmd->icd_override_m >= 0.0f) {
		eye_relation.x = hmd->icd_override_m;
	} else {
		eye_relation.x = hmd->default_icd;
	}

	return u_device_get_view_poses( //
	    xdev,                       //
	    &eye_relation,              //
	    at_timestamp_ns,            //
	    view_type,                  //
	    view_count,                 //
	    out_head_relation,          //
	    out_fovs,                   //
	    out_poses);
}

static xrt_result_t
rift_hmd_get_visibility_mask(struct xrt_device *xdev,
                             enum xrt_visibility_mask_type type,
                             uint32_t view_index,
                             struct xrt_visibility_mask **out_mask)
{
	struct xrt_fov fov = xdev->hmd->distortion.fov[view_index];
	u_visibility_mask_get_default(type, &fov, out_mask);
	return XRT_SUCCESS;
}

static xrt_result_t
rift_hmd_compute_distortion(struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
{
	struct rift_hmd *hmd = rift_hmd(xdev);

	u_compute_distortion_rift(&hmd->panel, &hmd->eye_profile, view, u, v, out_result);

	return XRT_SUCCESS;
}

static void
rift_hmd_constellation_tracking_source_get_tracked_pose(struct t_constellation_tracker_tracking_source *tracking_source,
                                                        timepoint_ns when_ns,
                                                        struct xrt_space_relation *out_relation)
{
	struct rift_hmd *hmd = container_of(tracking_source, struct rift_hmd, constellation_tracking_source);

	struct xrt_relation_chain xrc = {0};
	get_raw_pose(hmd, when_ns, &xrc);
	m_relation_chain_resolve(&xrc, out_relation);
}

static void
rift_hmd_constellation_device_push_constellation_tracker_sample(struct t_constellation_tracker_device *connection,
                                                                struct t_constellation_tracker_sample *sample)
{
	struct rift_hmd *hmd = container_of(connection, struct rift_hmd, constellation_device);

	if (!sample->has_world_pose) {
		return;
	}

	struct xrt_space_relation relation = XRT_SPACE_RELATION_ZERO;
	relation.pose = sample->world_pose;
	relation.relation_flags = XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
	                          XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT |
	                          XRT_SPACE_RELATION_POSITION_VALID_BIT;

	struct xrt_space_relation relation_3dof = XRT_SPACE_RELATION_ZERO;
	m_relation_history_get(hmd->relation_hist, sample->timestamp_ns, &relation_3dof);

	if (relation_3dof.relation_flags & XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT) {
		relation.angular_velocity = relation_3dof.angular_velocity;
		relation.relation_flags |= XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT;
	}

	m_relation_history_push_with_motion_estimation(hmd->raw_constellation_relation_hist, &relation,
	                                               sample->timestamp_ns);
}

static void
rift_frame_node_break_apart(struct xrt_frame_node *node)
{
	struct rift_hmd *hmd = rift_hmd_from_node(node);

	if (hmd->sensor_thread.initialized) {
		os_thread_helper_stop_and_wait(&hmd->sensor_thread);
	}

	if (hmd->radio_state.thread.initialized) {
		os_thread_helper_stop_and_wait(&hmd->radio_state.thread);
	}
}

static void
rift_frame_node_destroy(struct xrt_frame_node *node)
{
	struct rift_hmd *hmd = rift_hmd_from_node(node);

	if (hmd->constellation_tracker) {
		t_constellation_tracker_remove_device(hmd->constellation_tracker, hmd->constellation_device_id);
	}

	// Remove the variable tracking.
	u_var_remove_root(hmd);

	if (hmd->sensor_thread.initialized) {
		os_thread_helper_destroy(&hmd->sensor_thread);
	}

	if (hmd->clock_tracker) {
		m_clock_windowed_skew_tracker_destroy(hmd->clock_tracker);
	}

	m_relation_history_destroy(&hmd->relation_hist);
	m_relation_history_destroy(&hmd->raw_constellation_relation_hist);

	m_ff_vec3_f32_free(&hmd->gyro_ff);
	m_ff_vec3_f32_free(&hmd->accel_ff);
	m_ff_f64_free(&hmd->gravity_correction);

	// Only free if we are definitely the ones that allocated it
	if (hmd->lens_distortions && debug_get_bool_option_rift_use_firmware_distortion())
		free((void *)hmd->lens_distortions);

	os_hid_destroy(hmd->hmd_dev);
	if (hmd->radio_dev != NULL) {
		if (hmd->radio_state.thread.initialized) {
			os_thread_helper_destroy(&hmd->radio_state.thread);
		}

		os_hid_destroy(hmd->radio_dev);
	}

	if (hmd->device_count >= 0) {
		os_mutex_destroy(&hmd->device_mutex);

		// Free any sub-devices we created that weren't returned to the caller
		if (hmd->added_devices > 1) {
			for (int i = (hmd->added_devices - 1); i < hmd->device_count; i++) {
				u_device_free(hmd->devices[i]);
			}
		}
	}

	if (hmd->led_model.leds != NULL) {
		free(hmd->led_model.leds);

		hmd->led_model.leds = NULL;
		hmd->led_model.led_count = 0;
	}

	u_device_free(&hmd->base);
}

static const char *
variant_name(enum rift_variant variant)
{
	switch (variant) {
	case RIFT_VARIANT_DK2: return RIFT_DK2_PRODUCT_STRING;
	case RIFT_VARIANT_CV1: return RIFT_CV1_PRODUCT_STRING;
	}

	return "UNKNOWN";
}

/*
 *
 * Exported functions
 *
 */

int
rift_devices_create(struct os_hid_device *hmd_dev,
                    struct os_hid_device *radio_dev,
                    enum rift_variant variant,
                    const char *serial_number,
                    struct xrt_frame_context *xfctx,
                    struct rift_hmd **out_hmd,
                    struct xrt_device **out_xdevs)
{
	int result;

	// This indicates you won't be using Monado's built-in tracking algorithms.
	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);

	bool has_presence = variant == RIFT_VARIANT_CV1;
	struct rift_hmd *hmd = U_DEVICE_ALLOCATE(struct rift_hmd, flags, has_presence ? 2 : 1, 0);

	hmd->xfctx = xfctx;

	hmd->node.break_apart = rift_frame_node_break_apart;
	hmd->node.destroy = rift_frame_node_destroy;

	// Mark mutex as not initialized yet
	hmd->device_count = -1;

	hmd->variant = variant;
	hmd->hmd_dev = hmd_dev;
	hmd->radio_dev = radio_dev;

	// 4 seconds of IMU samples at 1000hz, should be more than enough
	m_ff_vec3_f32_alloc(&hmd->gyro_ff, 4096);
	m_ff_vec3_f32_alloc(&hmd->accel_ff, 4096);
	m_ff_f64_alloc(&hmd->gravity_correction, 4096);

	xrt_result_t xret = b_timing_source_create(xfctx, &hmd->timing_event_sink, &hmd->timing_event_source);
	U_LOG_CHK_ONLY_PRINT(hmd->log_level, xret, "b_timing_source_create");

	result = rift_send_keepalive(hmd);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to send keepalive to spin up headset, reason %d", result);
		goto error;
	}

	if (variant == RIFT_VARIANT_CV1) {
		// On CV1, we need to send a command to enable headset features
		rift_enable_components( //
		    hmd,                //
		    &(struct rift_enable_components_report){
		        .flags = RIFT_COMPONENT_DISPLAY | RIFT_COMPONENT_AUDIO | RIFT_COMPONENT_LEDS,
		    });

		// Read the radio address, this also enables the radio
		result = rift_get_radio_address(hmd, hmd->radio_address);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to get radio address, reason %d", result);
			goto error;
		}

		HMD_DEBUG(hmd, "Got radio address %02X:%02X:%02X:%02X:%02X", hmd->radio_address[0],
		          hmd->radio_address[1], hmd->radio_address[2], hmd->radio_address[3], hmd->radio_address[4]);
	}

	result = rift_get_display_info(hmd, &hmd->display_info);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to get device config, reason %d", result);
		goto error;
	}
	HMD_DEBUG(hmd, "Got display info from hmd, res: %dx%d", hmd->display_info.resolution_x,
	          hmd->display_info.resolution_y);

	result = rift_get_config(hmd, &hmd->config);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to get device config, reason %d", result);
		goto error;
	}
	HMD_DEBUG(hmd, "Got config from hmd, config flags: %X", hmd->config.config_flags);

	// Set a sane new config
	hmd->config.config_flags = RIFT_CONFIG_REPORT_USE_CALIBRATION | RIFT_CONFIG_REPORT_AUTO_CALIBRATION |
	                           RIFT_CONFIG_REPORT_COMMAND_KEEP_ALIVE | RIFT_CONFIG_REPORT_MOTION_KEEP_ALIVE;

	if (debug_get_bool_option_rift_power_override()) {
		hmd->config.config_flags |= RIFT_CONFIG_REPORT_OVERRIDE_POWER;
		HMD_INFO(hmd, "Enabling the override power config flag.");
	} else {
		hmd->config.config_flags &= ~RIFT_CONFIG_REPORT_OVERRIDE_POWER;
		HMD_DEBUG(hmd, "Disabling the override power config flag.");
	}

	// @todo figure out why we have to set these to zero
	hmd->config.interval = 0;
	hmd->config.command_id = 0;

	// update the config
	result = rift_set_config(hmd, &hmd->config);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to set the device config, reason %d", result);
		goto error;
	}

	// read it back
	result = rift_get_config(hmd, &hmd->config);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to set the device config, reason %d", result);
		goto error;
	}
	HMD_DEBUG(hmd, "After writing, HMD has config flags: %X", hmd->config.config_flags);

	if ((hmd->config.config_flags & RIFT_CONFIG_REPORT_USE_CALIBRATION) == 0) {
		HMD_INFO(hmd,
		         "Headset calibration enabling ignored by the headset, "
		         "reading out calibration to do ourselves");

		result = rift_get_imu_calibration(hmd, &hmd->imu_calibration);
		if (result < 0) {
			HMD_ERROR(hmd,
			          "failed to get IMU calibration, this is non-fatal, but headset IMU might drift more "
			          "than expected, reason %d",
			          result);
			hmd->imu_needs_calibration = false;
		} else {
			hmd->imu_needs_calibration = true;
			HMD_DEBUG(hmd, "Got IMU calibration from headset");
		}
	}

	if (debug_get_bool_option_rift_use_firmware_distortion()) {
		// get the lens distortions
		struct rift_lens_distortion_report lens_distortion_report;
		result = rift_get_lens_distortion(hmd, &lens_distortion_report);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to get lens distortion, reason %d", result);
			goto error;
		}

		hmd->num_lens_distortions = lens_distortion_report.num_distortions;
		struct rift_lens_distortion *lens_distortions =
		    calloc(lens_distortion_report.num_distortions, sizeof(struct rift_lens_distortion));

		rift_parse_distortion_report(&lens_distortion_report,
		                             &lens_distortions[lens_distortion_report.distortion_idx]);
		// TODO: actually verify we initialize all the distortions. if the headset is working correctly,
		// this should have happened, but you never know.
		for (uint16_t i = 1; i < hmd->num_lens_distortions; i++) {
			result = rift_get_lens_distortion(hmd, &lens_distortion_report);
			if (result < 0) {
				HMD_ERROR(hmd, "Failed to get lens distortion idx %d, reason %d", i, result);
				free(lens_distortions);
				goto error;
			}

			rift_parse_distortion_report(&lens_distortion_report,
			                             &lens_distortions[lens_distortion_report.distortion_idx]);
		}

		hmd->lens_distortions = lens_distortions;

		// TODO: pick the correct distortion for the eye relief setting the user has picked
		hmd->distortion_in_use = 0;
	} else {
		rift_fill_in_default_distortions(hmd);
	}

	result = rift_get_tracking_report(hmd, &hmd->tracking);
	if (result == 0) {
		// @note Intentionally we don't enable CUSTOM_PATTERN bit so that we don't inherit external custom
		//       patterns that may have been written by other processes or prior Monado runs
		hmd->tracking.flags = RIFT_TRACKING_ENABLE | RIFT_TRACKING_USE_CARRIER;
		hmd->tracking.pattern_idx = 0xff;

		hmd->tracking.vsync_offset = 0;
		hmd->tracking.duty_cycle = 0x7f;

		switch (hmd->variant) {
		case RIFT_VARIANT_DK2:
			hmd->tracking.exposure_length = 350;
			hmd->tracking.frame_interval = 16666;
			break;
		case RIFT_VARIANT_CV1:
			// @todo we need to pull the real configured exposure time we're using somehow, 495us is the
			//       actual exposure time we configure for CV1 sensors.
			hmd->tracking.exposure_length = 495;
			hmd->tracking.frame_interval = 19200;
			break;
		}

		result = rift_set_tracking(hmd, &hmd->tracking);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to enable tracking.");
		}
	}

	result = rift_read_led_model(hmd);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to read LED model, reason %d", result);
		goto error;
	} else {
		HMD_DEBUG(hmd, "Read LED model from Rift.");
	}

	if (!debug_get_bool_option_rift_enable_back_strap_leds()) {
		// Writes to the CustomPattern report only work with CUSTOM_PATTERN set to true
		hmd->tracking.flags |= RIFT_TRACKING_CUSTOM_PATTERN | RIFT_TRACKING_AUTO_INCREMENT;
		hmd->tracking.pattern_idx = 0;

		result = rift_set_tracking(hmd, &hmd->tracking);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to enable custom pattern mode, reason %d", result);
			goto error;
		}

		result = rift_upload_back_strap_disable_pattern(hmd);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to upload back strap disable pattern, reason %d", result);
			goto error;
		}

		HMD_INFO(hmd, "Disabled back strap LEDs via custom pattern.");
	}

	hmd->constellation_tracking_source.get_tracked_pose = rift_hmd_constellation_tracking_source_get_tracked_pose;
	hmd->constellation_device.push_constellation_tracker_sample =
	    rift_hmd_constellation_device_push_constellation_tracker_sample;

	// fill in extra display info about the headset

	size_t idx = 0;
	hmd->base.hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	hmd->base.hmd->blend_mode_count = idx;

	hmd->base.update_inputs = rift_hmd_update_inputs;
	hmd->base.get_tracked_pose = rift_hmd_get_tracked_pose;
	hmd->base.get_view_poses = rift_hmd_get_view_poses;
	hmd->base.get_visibility_mask = rift_hmd_get_visibility_mask;
	hmd->base.destroy = rift_hmd_destroy;

	// Has to happen before the mesh is filled in below, which calls straight into compute_distortion.
	rift_fill_in_distortion_inputs(hmd);

	hmd->base.hmd->distortion.models = XRT_DISTORTION_MODEL_COMPUTE;
	hmd->base.hmd->distortion.preferred = XRT_DISTORTION_MODEL_COMPUTE;
	hmd->base.compute_distortion = rift_hmd_compute_distortion;
	u_distortion_mesh_fill_in_compute(&hmd->base);

	hmd->log_level = debug_get_log_option_rift_log();

	// Print name.
	u_truncate_snprintf(hmd->base.str, XRT_DEVICE_NAME_LEN, "%s", variant_name(hmd->variant));
	u_truncate_snprintf(hmd->base.serial, XRT_DEVICE_NAME_LEN, "%s", serial_number);

	m_relation_history_create(&hmd->relation_hist);
	m_relation_history_create(&hmd->raw_constellation_relation_hist);

	// Setup input.
	hmd->base.name = XRT_DEVICE_GENERIC_HMD;
	hmd->base.device_type = XRT_DEVICE_TYPE_HMD;
	hmd->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	if (has_presence) {
		hmd->base.inputs[1].name = XRT_INPUT_GENERIC_HEAD_DETECT;
	}
	hmd->base.supported.orientation_tracking = true;
	// @todo Disable this when the user doesn't have sensors connected.
	hmd->base.supported.position_tracking = true;
	hmd->base.supported.presence = variant == RIFT_VARIANT_CV1;

	// Set up display details
	switch (hmd->variant) {
	case RIFT_VARIANT_DK2: hmd->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 75.0f); break;
	case RIFT_VARIANT_CV1: hmd->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 90.0f); break;
	}

	hmd->default_icd = MICROMETERS_TO_METERS(hmd->display_info.lens_separation);

	hmd->icd_override_m = debug_get_float_option_rift_override_icd_mm() / 1000.0f;
	if (hmd->icd_override_m >= 0.0f) {
		HMD_INFO(hmd, "Applying ICD override of %f", hmd->icd_override_m);
	} else {
		HMD_DEBUG(hmd, "Using default ICD of %f", hmd->default_icd);
	}

	switch (hmd->variant) {
	case RIFT_VARIANT_CV1: {
		hmd->base.hmd->screens[0].w_pixels = hmd->display_info.resolution_x;
		hmd->base.hmd->screens[0].h_pixels = hmd->display_info.resolution_y;

		// TODO: properly apply using u_rift_panel::eye_gap_m, but this isn't necessary, as
		//       observed gap is always zero
		uint16_t view_width = hmd->display_info.resolution_x / 2;
		uint16_t view_height = hmd->display_info.resolution_y;

		for (uint32_t i = 0; i < 2; ++i) {
			hmd->base.hmd->views[i].display.w_pixels = view_width;
			hmd->base.hmd->views[i].display.h_pixels = view_height;

			hmd->base.hmd->views[i].viewport.x_pixels = i * (hmd->display_info.resolution_x / 2);
			hmd->base.hmd->views[i].viewport.y_pixels = 0;
			hmd->base.hmd->views[i].viewport.w_pixels = view_width;
			hmd->base.hmd->views[i].viewport.h_pixels = view_height;
			hmd->base.hmd->views[i].rot = u_device_rotation_ident;
		}

		// TODO: figure out how to calculate this programmatically, right now this is hardcoded with data dumped
		//       from oculus' OpenXR runtime, some of the math for this is in rift_distortion.c, used for
		//       calculating distortion
		hmd->base.hmd->distortion.fov[0].angle_up = 0.7269826;
		hmd->base.hmd->distortion.fov[0].angle_down = -0.8378981;
		hmd->base.hmd->distortion.fov[0].angle_left = -0.76754993;
		hmd->base.hmd->distortion.fov[0].angle_right = 0.6208969;

		hmd->base.hmd->distortion.fov[1].angle_up = 0.7269826;
		hmd->base.hmd->distortion.fov[1].angle_down = -0.8378981;
		hmd->base.hmd->distortion.fov[1].angle_left = -0.6208969;
		hmd->base.hmd->distortion.fov[1].angle_right = 0.76754993;

		break;
	}
	case RIFT_VARIANT_DK2: {
		// screen is rotated, so we need to undo that here
		hmd->base.hmd->screens[0].h_pixels = hmd->display_info.resolution_x;
		hmd->base.hmd->screens[0].w_pixels = hmd->display_info.resolution_y;

		// TODO: properly apply using u_rift_panel::eye_gap_m, but this isn't necessary, as
		//       observed gap is always zero
		uint16_t view_width = hmd->display_info.resolution_x / 2;
		uint16_t view_height = hmd->display_info.resolution_y;

		for (uint32_t i = 0; i < 2; ++i) {
			hmd->base.hmd->views[i].display.w_pixels = view_width;
			hmd->base.hmd->views[i].display.h_pixels = view_height;

			hmd->base.hmd->views[i].viewport.x_pixels = 0;
			hmd->base.hmd->views[i].viewport.y_pixels = (1 - i) * (hmd->display_info.resolution_x / 2);
			hmd->base.hmd->views[i].viewport.w_pixels = view_height; // screen is rotated, so swap w and h
			hmd->base.hmd->views[i].viewport.h_pixels = view_width;
			hmd->base.hmd->views[i].rot = u_device_rotation_left;
		}

		// TODO: figure out how to calculate this programmatically, right now this is hardcoded with data dumped
		//       from oculus' OpenXR runtime, some of the math for this is in rift_distortion.c, used for
		//       calculating distortion
		hmd->base.hmd->distortion.fov[0].angle_up = 0.92667186;
		hmd->base.hmd->distortion.fov[0].angle_down = -0.92667186;
		hmd->base.hmd->distortion.fov[0].angle_left = -0.8138836;
		hmd->base.hmd->distortion.fov[0].angle_right = 0.82951474;

		hmd->base.hmd->distortion.fov[1].angle_up = 0.92667186;
		hmd->base.hmd->distortion.fov[1].angle_down = -0.92667186;
		hmd->base.hmd->distortion.fov[1].angle_left = -0.82951474;
		hmd->base.hmd->distortion.fov[1].angle_right = 0.8138836;

		break;
	}
	}

	m_imu_3dof_init(&hmd->fusion, M_IMU_3DOF_USE_GRAVITY_DUR_20MS);
	hmd->clock_tracker = m_clock_windowed_skew_tracker_alloc(2048);

	result = os_thread_helper_init(&hmd->sensor_thread);

	if (result < 0) {
		HMD_ERROR(hmd, "Failed to init os thread helper");
		goto error;
	}

	result = os_thread_helper_start(&hmd->sensor_thread, rift_sensor_thread, hmd);

	if (result < 0) {
		HMD_ERROR(hmd, "Failed to start sensor thread");
		goto error;
	}

	result = os_mutex_init(&hmd->device_mutex);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to init radio mutex");
		goto error;
	}
	hmd->device_count = 0;

	if (hmd->radio_dev != NULL) {
		result = os_thread_helper_init(&hmd->radio_state.thread);

		if (result < 0) {
			HMD_ERROR(hmd, "Failed to init os thread helper for radio");
			goto error;
		}

		result = os_thread_helper_start(&hmd->radio_state.thread, rift_radio_thread, hmd);

		if (result < 0) {
			HMD_ERROR(hmd, "Failed to start radio thread");
			goto error;
		}
	}

	// Setup variable tracker: Optional but useful for debugging
	u_var_add_root(hmd, "Rift HMD", true);
	u_var_add_log_level(hmd, &hmd->log_level, "log_level");
	u_var_add_ro_i32(hmd, (int32_t *)&hmd->variant, "variant");
	u_var_add_f32(hmd, &hmd->default_icd, "default_icd");
	u_var_add_ro_i64_ns(hmd, &hmd->last_sample_local_timestamp_ns, "last_sample_local_timestamp_ns");
	u_var_add_f32(hmd, &hmd->icd_override_m, "icd_override_m");
	u_var_add_bool(hmd, &hmd->presence, "presence");

	u_var_add_gui_header(hmd, NULL, "IMU Fusion");
	m_imu_3dof_add_vars(&hmd->fusion, hmd, "3dof_");

	u_var_add_gui_header(hmd, NULL, "IMU Calibration");
	u_var_add_bool(hmd, &hmd->imu_needs_calibration, "imu_needs_calibration");
	u_var_add_vec3_f32(hmd, &hmd->imu_calibration.gyro_offset, "gyro_offset");
	u_var_add_vec3_f32(hmd, &hmd->imu_calibration.accel_offset, "accel_offset");
	u_var_add_vec3_f32(hmd, (struct xrt_vec3 *)&hmd->imu_calibration.gyro_matrix.v[0], "gyro_matrix[0..3]");
	u_var_add_vec3_f32(hmd, (struct xrt_vec3 *)&hmd->imu_calibration.gyro_matrix.v[3], "gyro_matrix[3..6]");
	u_var_add_vec3_f32(hmd, (struct xrt_vec3 *)&hmd->imu_calibration.gyro_matrix.v[6], "gyro_matrix[6..9]");
	u_var_add_vec3_f32(hmd, (struct xrt_vec3 *)&hmd->imu_calibration.accel_matrix.v[0], "accel_matrix[0..3]");
	u_var_add_vec3_f32(hmd, (struct xrt_vec3 *)&hmd->imu_calibration.accel_matrix.v[3], "accel_matrix[3..6]");
	u_var_add_vec3_f32(hmd, (struct xrt_vec3 *)&hmd->imu_calibration.accel_matrix.v[6], "accel_matrix[6..9]");
	u_var_add_f32(hmd, &hmd->imu_calibration.temperature, "temperature");

	// wait for display/controller init
	// @note once monado *has* the capabilities to signal to us when a display is ready, we should use them!
	os_nanosleep(time_s_to_ns(debug_get_float_option_rift_startup_wait_time()));

	*out_hmd = hmd;

	os_mutex_lock(&hmd->device_mutex);
	out_xdevs[hmd->added_devices++] = &hmd->base;
	for (int i = 0; i < hmd->device_count; i++) {
		out_xdevs[hmd->added_devices++] = hmd->devices[i];
	}
	os_mutex_unlock(&hmd->device_mutex);

	return hmd->added_devices;
error:
	rift_frame_node_destroy(&hmd->node);
	return -1;
}

bool
rift_get_radio_id(struct rift_hmd *hmd, uint8_t out_radio_id[5])
{
	if (hmd->variant != RIFT_VARIANT_CV1) {
		return false;
	}

	memcpy(out_radio_id, hmd->radio_address, sizeof(hmd->radio_address));

	return true;
}

bool
rift_hmd_frame_timestamp_callback(void *user_data,             //
                                  timepoint_ns *out_timestamp, //
                                  uint64_t *out_sequence_id,   //
                                  timepoint_ns frame_start_ns, //
                                  uint32_t pts)                //
{
	struct rift_hmd *hmd = (struct rift_hmd *)user_data;

	/*
	 * A frame belongs to the exposure whose IN report arrives alongside its first packet, which is the match
	 * OpenHMD makes. Searching for that exposure directly does not work here: the gap between the two arrivals
	 * sits near zero, the ~1 ms quantisation of the 1 kHz report stream walks it either side, and whenever it goes
	 * negative the report has not been pushed into the history yet and the frame finds nothing.
	 *
	 * So the search is centred one frame interval earlier, where the exposure is always already in the history and
	 * has half an interval of margin each way, and one interval is added back to the timestamp it hands out. The
	 * exposure that lands on is the one the search cannot reliably see, extrapolated across a single interval of a
	 * clock that the HMD holds to well under a microsecond of that per frame.
	 *
	 * Frames with nothing in the window are left alone rather than guessed at.
	 */
	const time_duration_ns interval_ns = (time_duration_ns)hmd->tracking.frame_interval * U_TIME_1US_IN_NS;
	const time_duration_ns window_ns = interval_ns / 2;

	struct rift_exposure_event match = {0};
	bool have_match = false;

	os_thread_helper_lock(&hmd->sensor_thread);

	const uint64_t oldest_held = hmd->exposure_history_pushed > RIFT_EXPOSURE_HISTORY_SIZE
	                                 ? hmd->exposure_history_pushed - RIFT_EXPOSURE_HISTORY_SIZE
	                                 : 0;

	for (uint64_t i = oldest_held; i < hmd->exposure_history_pushed; i++) {
		const struct rift_exposure_event *event = &hmd->exposure_history[i % RIFT_EXPOSURE_HISTORY_SIZE];

		const time_duration_ns offset_ns = frame_start_ns - event->recv_timestamp_ns - interval_ns;
		if (offset_ns > -window_ns && offset_ns < window_ns) {
			match = *event;
			have_match = true;
		}
	}

	os_thread_helper_unlock(&hmd->sensor_thread);

	if (!have_match) {
		// Either the clocks have not synchronized yet, so there is nothing to match against, or the frame
		// landed between two exposures. Leaving the frame with the receive time the UVC driver already gave it
		// is far better than attributing it to an exposure it may not belong to.
		HMD_TRACE(hmd, "Frame with PTS %u matched no exposure", pts);
		return false;
	}

	*out_timestamp = match.timestamp_ns + interval_ns;
	*out_sequence_id = match.sequence_id + 1;

	HMD_TRACE(hmd,
	          "Frame with PTS %u matched exposure %u, taken at %" PRId64 " ns (sequence ID %" PRIu64
	          "), report %" PRId64 " ns before frame, %" PRId64 " ns from window centre",
	          pts, match.sequence_id + 1, *out_timestamp, *out_sequence_id,
	          frame_start_ns - match.recv_timestamp_ns, frame_start_ns - match.recv_timestamp_ns - interval_ns);

	return true;
}

int
rift_add_to_constellation_tracker(struct rift_hmd *hmd, struct t_constellation_tracker *tracker)
{
	struct t_constellation_tracker_device_params params = {
	    .led_model = hmd->led_model,
	    .tracking_source = &hmd->constellation_tracking_source,
	};

	hmd->constellation_tracker = tracker;

	int ret = t_constellation_tracker_add_device(tracker, &params, &hmd->constellation_device,
	                                             &hmd->constellation_device_id);
	if (ret < 0) {
		HMD_ERROR(hmd, "Failed to add Rift HMD to constellation tracker, reason %d", ret);
		return ret;
	}

	struct xrt_tracking_origin *tracking_origin = t_constellation_tracker_get_tracking_origin(tracker);
	hmd->base.tracking_origin = tracking_origin;
	hmd->constellation_imu_sink = params.imu_sink;

	// Mark that we're using constellation poses now
	hmd->use_constellation_poses = true;

	return 0;
}

struct t_timing_event_source *
rift_hmd_get_timing_event_source(struct rift_hmd *hmd)
{
	return hmd->timing_event_source;
}
