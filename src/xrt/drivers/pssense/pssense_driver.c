// Copyright 2023, Collabora, Ltd.
// Copyright 2023, Jarett Millard
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  PlayStation Sense controller prober and driver code.
 * @author Jarett Millard <jarett.millard@gmail.com>
 * @ingroup drv_pssense
 */

#include "xrt/xrt_prober.h"

#include "os/os_hid.h"
#include "os/os_threading.h"
#include "os/os_time.h"

#include "math/m_api.h"

#include "tracking/t_constellation.h"
#include "tracking/t_imu.h"

#include "constellation/t_constellation_tracker.h"
#include "constellation/t_led_sync_refinement.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_linux.h"
#include "util/u_logging.h"
#include "util/u_resampler.h"
#include "util/u_time.h"
#include "util/u_trace_marker.h"
#include "util/u_var.h"
#include "util/u_misc.h"

#include "math/m_clock_tracking.h"
#include "math/m_imu_3dof.h"
#include "math/m_relation_history.h"
#include "math/m_space.h"

#include "pssense_interface.h"
#include "pssense_led_model.h"
#include "pssense_protocol.h"

#include <stdio.h>
#include <errno.h>


/*!
 * @addtogroup drv_pssense
 * @{
 */

#define PSSENSE_TRACE(p, ...) U_LOG_XDEV_IFL_T(&p->base, p->log_level, __VA_ARGS__)
#define PSSENSE_DEBUG(p, ...) U_LOG_XDEV_IFL_D(&p->base, p->log_level, __VA_ARGS__)
#define PSSENSE_DEBUG_HEX(p, data, data_size) U_LOG_XDEV_IFL_D_HEX(&p->base, p->log_level, data, data_size)
#define PSSENSE_WARN(p, ...) U_LOG_XDEV_IFL_W(&p->base, p->log_level, __VA_ARGS__)
#define PSSENSE_ERROR(p, ...) U_LOG_XDEV_IFL_E(&p->base, p->log_level, __VA_ARGS__)

DEBUG_GET_ONCE_LOG_OPTION(pssense_log, "PSSENSE_LOG", U_LOGGING_INFO)
DEBUG_GET_ONCE_BOOL_OPTION(pssense_pc_polling_rate, "PSSENSE_SET_PC_POLLING_RATE", true)

/*!
 * Indices where each input is in the input list.
 */
enum pssense_input_index
{
	PSSENSE_INDEX_PS_CLICK,
	PSSENSE_INDEX_SHARE_CLICK,
	PSSENSE_INDEX_OPTIONS_CLICK,
	PSSENSE_INDEX_SQUARE_CLICK,
	PSSENSE_INDEX_SQUARE_TOUCH,
	PSSENSE_INDEX_TRIANGLE_CLICK,
	PSSENSE_INDEX_TRIANGLE_TOUCH,
	PSSENSE_INDEX_CROSS_CLICK,
	PSSENSE_INDEX_CROSS_TOUCH,
	PSSENSE_INDEX_CIRCLE_CLICK,
	PSSENSE_INDEX_CIRCLE_TOUCH,
	PSSENSE_INDEX_SQUEEZE_CLICK,
	PSSENSE_INDEX_SQUEEZE_TOUCH,
	PSSENSE_INDEX_SQUEEZE_PROXIMITY,
	PSSENSE_INDEX_SQUEEZE_PROXIMITY_FLOAT,
	PSSENSE_INDEX_TRIGGER_CLICK,
	PSSENSE_INDEX_TRIGGER_TOUCH,
	PSSENSE_INDEX_TRIGGER_VALUE,
	PSSENSE_INDEX_TRIGGER_PROXIMITY,
	PSSENSE_INDEX_TRIGGER_PROXIMITY_FLOAT,
	PSSENSE_INDEX_THUMBSTICK,
	PSSENSE_INDEX_THUMBSTICK_CLICK,
	PSSENSE_INDEX_THUMBSTICK_TOUCH,
	PSSENSE_INDEX_GRIP_POSE,
	PSSENSE_INDEX_AIM_POSE,
	PSSENSE_INPUT_COUNT,
};

/*!
 * Parsed calibration data from the PlayStation Sense controller.
 */
struct pssense_parsed_calibration
{
	struct xrt_vec3 gyro_scale;
	struct xrt_vec3 accel_scale;

	struct xrt_vec3 accel_bias;
	struct xrt_vec3_i32 gyro_bias;
};

/*!
 * PlayStation Sense state parsed from a data packet.
 */
struct pssense_input_state
{
	uint64_t timestamp_ns;
	uint32_t seq_no;

	bool ps_click;
	bool share_click;
	bool options_click;
	bool square_click;
	bool square_touch;
	bool triangle_click;
	bool triangle_touch;
	bool cross_click;
	bool cross_touch;
	bool circle_click;
	bool circle_touch;
	bool squeeze_click;
	bool squeeze_touch;
	float squeeze_proximity;
	bool trigger_click;
	bool trigger_touch;
	float trigger_value;
	float trigger_proximity;
	bool thumbstick_click;
	bool thumbstick_touch;
	struct xrt_vec2 thumbstick;

	struct xrt_vec3_i32 gyro_raw;
	struct xrt_vec3_i32 accel_raw;

	bool battery_state_valid;
	bool battery_charging;
	//! Charge level from 0..1
	float battery_charge_percent;
};

/*!
 * A single PlayStation Sense Controller.
 *
 * @implements xrt_device
 * @implements xrt_frame_node
 * @implements t_timing_event_sink
 * @implements t_constellation_tracker_led_model
 * @implements t_constellation_tracker_device
 */
struct pssense_device
{
	struct xrt_device base;
	struct xrt_frame_node node;
	struct t_timing_event_sink timing_event_sink;
	struct t_constellation_tracker_led_model led_model;
	struct t_constellation_tracker_device constellation_device;
	struct t_constellation_tracker_tracking_source constellation_tracking_source;

	bool usb;
	struct os_hid_device *hid;
	struct os_thread_helper controller_thread;

	struct
	{
		struct m_clock_windowed_skew_tracker *clock_tracker;

		timepoint_ns latest_imu_time_ns;

		uint32_t imu_ticks_last;
		uint64_t imu_ticks_total;

		timepoint_ns latest_device_time_ns;

		uint32_t device_ticks_last;
		uint64_t device_ticks_total;

		uint32_t last_sent_host_timestamp_us;
	} timing;

	struct
	{
		struct m_relation_history *imu_relation_history;
		struct m_imu_3dof fusion;
		struct xrt_pose pose;

		uint32_t received_frames;
		uint32_t last_exposure_sequence_id;
		timepoint_ns last_exposure_local_timestamp_ns;
		time_duration_ns average_exposure_interval_ns;

		bool increment_sequence_num;
		uint8_t led_sequence_num;

		int32_t timing_fudge_100us;

		//! Locked by controller_thread.
		struct pssense_led_settings led_settings;

		bool use_constellation;
		struct t_constellation_tracker *constellation_tracker;
		t_constellation_device_id_t constellation_device_id;
		struct xrt_imu_sink *constellation_imu_sink;
		struct m_relation_history *constellation_relation_history;

		struct t_led_sync_refinement led_sync_refinement;
		uint8_t period_id;

		bool led_sync_sample_needs_marking;
		bool led_sync_sample_needs_sending;
		struct t_led_sync_sample latest_led_sync_sample;

		struct xrt_pose T_led_imu;
	} tracking;

	enum xrt_hand hand;

	enum u_logging_level log_level;

	struct os_precise_sleeper sleeper;

	struct pssense_parsed_calibration calibration;
	bool has_calibration;

	//! Input state parsed from most recent packet
	struct pssense_input_state state;
	//! Pending output state to send to device
	struct
	{
		uint8_t next_seq_no;
		uint8_t packet_counter;

		struct u_resampler *pcm_haptics_resampler;

		bool send_vibration;
		uint8_t vibration_amplitude;
		uint8_t vibration_mode;

		uint64_t vibration_end_timestamp_ns;

		bool send_trigger_feedback;
		enum pssense_adaptive_trigger_mode trigger_feedback_mode;
	} output;
	struct
	{
		bool button_states;
		bool timing;
		bool tracking;
	} gui;
};

static struct pssense_device *
from_device(struct xrt_device *xdev)
{
	return container_of(xdev, struct pssense_device, base);
}

static struct pssense_device *
from_node(struct xrt_frame_node *node)
{
	return container_of(node, struct pssense_device, node);
}

static struct pssense_device *
from_timing_event_sink(struct t_timing_event_sink *sink)
{
	return container_of(sink, struct pssense_device, timing_event_sink);
}

static struct pssense_device *
from_constellation_device(struct t_constellation_tracker_device *device)
{
	return container_of(device, struct pssense_device, constellation_device);
}

static struct pssense_device *
from_constellation_tracking_source(struct t_constellation_tracker_tracking_source *tracking_source)
{
	return container_of(tracking_source, struct pssense_device, constellation_tracking_source);
}

const uint32_t CRC_POLYNOMIAL = 0xedb88320;

/*
 *
 * Internal functions
 *
 */

static uint32_t
crc32_le(uint32_t crc, uint8_t const *p, size_t len)
{
	int i;
	crc ^= 0xffffffff;
	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRC_POLYNOMIAL : 0);
	}
	return crc ^ 0xffffffff;
}

static void
pssense_add_clock_offset_sample_locked(struct pssense_device *pssense, timepoint_ns local_ns, timepoint_ns remote_ns)
{
	m_clock_windowed_skew_tracker_push(pssense->timing.clock_tracker, local_ns, remote_ns);

	time_duration_ns skew_ns;
	if (m_clock_windowed_skew_tracker_get_skew(pssense->timing.clock_tracker, &skew_ns)) {
		t_led_sync_push_host_device_clock_offset(&pssense->tracking.led_sync_refinement, -skew_ns);
	}
}

static bool
pssense_host_ts_to_device(struct pssense_device *pssense,
                          timepoint_ns host_timestamp_ns,
                          timepoint_ns *out_device_timestamp_ns)
{
	switch (pssense->tracking.latest_led_sync_sample.timestamp_mode) {
	case T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_INVALID:
	case T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_DEVICE_HOST_LATENCY: {
		if (!m_clock_windowed_skew_tracker_to_remote(pssense->timing.clock_tracker, host_timestamp_ns,
		                                             out_device_timestamp_ns)) {
			return false;
		}
		// The refinement routine only reports the device->host transfer latency in this mode, the driver
		// has to apply it on top of its own clock tracking.
		*out_device_timestamp_ns += pssense->tracking.latest_led_sync_sample.timestamp.device_host_latency_ns;
		return true;
	}
	case T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_HOST_DEVICE_CLOCK_OFFSET: {
		*out_device_timestamp_ns =
		    host_timestamp_ns + pssense->tracking.latest_led_sync_sample.timestamp.host_device_clock_offset_ns;
		return true;
	}
	}

	return false;
}

static bool
pssense_device_ts_to_host(struct pssense_device *pssense,
                          timepoint_ns device_timestamp_ns,
                          timepoint_ns *out_host_timestamp_ns)
{
	switch (pssense->tracking.latest_led_sync_sample.timestamp_mode) {
	case T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_INVALID:
	case T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_DEVICE_HOST_LATENCY: {
		if (!m_clock_windowed_skew_tracker_to_local(pssense->timing.clock_tracker, device_timestamp_ns,
		                                            out_host_timestamp_ns)) {
			return false;
		}
		*out_host_timestamp_ns -= pssense->tracking.latest_led_sync_sample.timestamp.device_host_latency_ns;
		return true;
	}
	case T_LED_SYNC_SAMPLE_TIMESTAMP_MODE_HOST_DEVICE_CLOCK_OFFSET: {
		*out_host_timestamp_ns = device_timestamp_ns -
		                         pssense->tracking.latest_led_sync_sample.timestamp.host_device_clock_offset_ns;
		return true;
	}
	}

	return false;
}

/*!
 * Reads one packet from the device, wrapping no data as EAGAIN. Does not block.
 */
static int
pssense_read_packet_data(struct pssense_device *pssense, uint8_t *buffer, size_t size)
{
	// Poll, don't block. Outer thread needs to run quick
	int ret = os_hid_read(pssense->hid, buffer, size, 0);

	// No data yet
	if (ret == 0) {
		return -EAGAIN;
	}

	if (ret < 0) {
		PSSENSE_ERROR(pssense, "Failed to read device '%i'!", ret);
		return ret;
	}

	return ret;
}

static void
pssense_update_fusion(struct pssense_device *pssense)
{
	// We don't have calibration yet, so we can't do anything
	if (!pssense->has_calibration) {
		return;
	}

	struct xrt_vec3 gyro = {
	    .x = (pssense->state.gyro_raw.x - pssense->calibration.gyro_bias.x) * pssense->calibration.gyro_scale.x,
	    .y = (pssense->state.gyro_raw.y - pssense->calibration.gyro_bias.y) * pssense->calibration.gyro_scale.y,
	    .z = (pssense->state.gyro_raw.z - pssense->calibration.gyro_bias.z) * pssense->calibration.gyro_scale.z,
	};

	struct xrt_vec3 accel = {
	    .x = (pssense->state.accel_raw.x - pssense->calibration.accel_bias.x) * pssense->calibration.accel_scale.x,
	    .y = (pssense->state.accel_raw.y - pssense->calibration.accel_bias.y) * pssense->calibration.accel_scale.y,
	    .z = (pssense->state.accel_raw.z - pssense->calibration.accel_bias.z) * pssense->calibration.accel_scale.z,
	};

	m_imu_3dof_update(&pssense->tracking.fusion, pssense->timing.latest_imu_time_ns, &accel, &gyro);
	pssense->tracking.pose.orientation = pssense->tracking.fusion.rot;

	struct xrt_space_relation space_relation = {
	    .pose = pssense->tracking.pose,
	    .relation_flags = XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
	                      XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT,
	    .angular_velocity = gyro,
	};
	m_relation_history_push(pssense->tracking.imu_relation_history, &space_relation,
	                        pssense->timing.latest_imu_time_ns);

	if (pssense->tracking.constellation_imu_sink != NULL) {
		struct xrt_imu_sample sample = {
		    .accel_m_s2 =
		        {
		            .x = accel.x,
		            .y = accel.y,
		            .z = accel.z,
		        },
		    .gyro_rad_secs =
		        {
		            .x = gyro.x,
		            .y = gyro.y,
		            .z = gyro.z,
		        },
		};

		if (pssense_device_ts_to_host(pssense,                            //
		                              pssense->timing.latest_imu_time_ns, //
		                              &sample.timestamp_ns)) {            //
			xrt_sink_push_imu(pssense->tracking.constellation_imu_sink, &sample);
		}
	}
}

static int
pssense_handle_packet(struct pssense_device *pssense,
                      timepoint_ns recv_time_ns,
                      const struct pssense_input_report_common *data)
{
	// Final input state
	struct pssense_input_state input = {
	    .timestamp_ns = recv_time_ns,
	};

#if 0 // IMU rate test
	static timepoint_ns last[2] = {0};
	static double rate[2] = {133, 133};
	rate[pssense->hand] =
	    (rate[pssense->hand] * 0.999) + ((1 / time_ns_to_s(recv_time_ns - last[pssense->hand])) * 0.001);
	printf("%d\trate: %lfhz\n", pssense->hand, rate[pssense->hand]);
	last[pssense->hand] = recv_time_ns;
#endif

	uint32_t seq_no = __le32_to_cpu(data->seq_no);
	if (input.seq_no != 0 && seq_no != input.seq_no + 1) {
		PSSENSE_WARN(pssense, "Missed seq no %u. Previous was %u", seq_no, input.seq_no);
	}
	input.seq_no = seq_no;

	// Update input state
	input.ps_click = (data->buttons[1] & 16) != 0;
	input.squeeze_touch = (data->buttons[2] & 8) != 0;
	input.squeeze_proximity = data->squeeze_proximity / 255.0f;
	input.trigger_touch = (data->buttons[1] & 128) != 0;
	input.trigger_value = data->trigger_value / 255.0f;
	input.trigger_proximity = data->trigger_proximity / 255.0f;
	input.thumbstick.x = (data->thumbstick_x - 128) / 128.0f;
	input.thumbstick.y = (data->thumbstick_y - 128) / -128.0f;
	input.thumbstick_touch = (data->buttons[2] & 4) != 0;

	if (pssense->hand == XRT_HAND_LEFT) {
		input.share_click = (data->buttons[1] & 1) != 0;
		input.square_click = (data->buttons[0] & 1) != 0;
		input.square_touch = (data->buttons[2] & 2) != 0;
		input.triangle_click = (data->buttons[0] & 8) != 0;
		input.triangle_touch = (data->buttons[2] & 1) != 0;
		input.squeeze_click = (data->buttons[0] & 16) != 0;
		input.trigger_click = (data->buttons[0] & 64) != 0;
		input.thumbstick_click = (data->buttons[1] & 4) != 0;
	} else if (pssense->hand == XRT_HAND_RIGHT) {
		input.options_click = (data->buttons[1] & 2) != 0;
		input.cross_click = (data->buttons[0] & 2) != 0;
		input.cross_touch = (data->buttons[2] & 2) != 0;
		input.circle_click = (data->buttons[0] & 4) != 0;
		input.circle_touch = (data->buttons[2] & 1) != 0;
		input.squeeze_click = (data->buttons[0] & 32) != 0;
		input.trigger_click = (data->buttons[0] & 128) != 0;
		input.thumbstick_click = (data->buttons[1] & 8) != 0;
	}

	// Update IMU data
	uint32_t imu_ticks = __le32_to_cpu(data->imu_ticks);
	int64_t imu_ticks_delta = imu_ticks - pssense->timing.imu_ticks_last;
	if (imu_ticks_delta >= 0) {
		pssense->timing.imu_ticks_total += imu_ticks_delta;
		pssense->timing.imu_ticks_last = imu_ticks;

		pssense->timing.latest_imu_time_ns = IMU_TICKS_TO_NS(pssense->timing.imu_ticks_total);

		input.gyro_raw.x = (int16_t)__le16_to_cpu(data->gyro[0]);
		input.gyro_raw.y = (int16_t)__le16_to_cpu(data->gyro[1]);
		input.gyro_raw.z = (int16_t)__le16_to_cpu(data->gyro[2]);

		input.accel_raw.x = (int16_t)__le16_to_cpu(data->accel[0]);
		input.accel_raw.y = (int16_t)__le16_to_cpu(data->accel[1]);
		input.accel_raw.z = (int16_t)__le16_to_cpu(data->accel[2]);
	} else {
		PSSENSE_WARN(pssense, "Time went backwards. Check your play area for black holes.");
	}

	uint32_t device_ticks = __le32_to_cpu(data->device_timestamp_ticks);
	int64_t device_ticks_delta = device_ticks - pssense->timing.device_ticks_last;
	if (device_ticks_delta >= 0) {
		pssense->timing.device_ticks_total += device_ticks_delta;
		pssense->timing.device_ticks_last = device_ticks;

		pssense->timing.latest_device_time_ns = IMU_TICKS_TO_NS(pssense->timing.device_ticks_total);
	} else {
		PSSENSE_WARN(pssense, "Device time went backwards. Check your play area for black holes.");
	}

	// Battery state is upper 4 bits
	uint8_t battery_state = data->battery_state >> 4;

	// Charge values go from 0..10, so add 5% and cap at 100% so we never show 0% charge
	float battery_percent = MIN(1.0f, (data->battery_state & 0xf) * .1f + .05);

	bool battery_state_valid, charging;
	if (battery_state == CHARGE_STATE_DISCHARGING) {
		battery_state_valid = true;
		charging = false;
	} else if (battery_state == CHARGE_STATE_CHARGING) {
		battery_state_valid = true;
		charging = true;
	} else if (battery_state == CHARGE_STATE_FULL) {
		battery_state_valid = true;
		charging = true;
		battery_percent = 1.0f;
	} else if (battery_state == CHARGE_STATE_ABNORMAL_VOLTAGE) {
		battery_state_valid = false;
		PSSENSE_WARN(pssense, "Unable to determine charge state: abnormal voltage");
	} else if (battery_state == CHARGE_STATE_ABNORMAL_TEMP) {
		battery_state_valid = false;
		PSSENSE_WARN(pssense, "Unable to determine charge state: abnormal temp");
	} else if (battery_state == CHARGE_STATE_CHARGING_ERROR) {
		battery_state_valid = false;
		PSSENSE_WARN(pssense, "Unable to determine charge state: charging error");
	} else {
		battery_state_valid = false;
		PSSENSE_WARN(pssense, "Unable to determine charge state: unknown reason");
	}

	input.battery_state_valid = battery_state_valid;
	if (battery_state_valid) {
		if (charging != input.battery_charging || battery_percent != input.battery_charge_percent) {
			PSSENSE_TRACE(pssense, "Battery at %.f%%, %s", battery_percent * 100,
			              charging ? "charging" : "discharging");
		}
		input.battery_charging = charging;
		input.battery_charge_percent = battery_percent;
	}

	os_thread_helper_lock(&pssense->controller_thread);

	// Mark the LED sync refinement sample as applied
	uint32_t latest_host_send_time = __le32_to_cpu(data->host_timestamp);
	if (latest_host_send_time != pssense->timing.last_sent_host_timestamp_us &&
	    pssense->tracking.led_sync_sample_needs_marking) {
		t_led_sync_mark_latest_sample_applied(&pssense->tracking.led_sync_refinement, recv_time_ns);
		pssense->tracking.led_sync_sample_needs_marking = false;
	}

	// Update the clock offset
	pssense_add_clock_offset_sample_locked(pssense, recv_time_ns, pssense->timing.latest_device_time_ns);

	pssense->state = input;
	pssense_update_fusion(pssense);

	os_thread_helper_unlock(&pssense->controller_thread);

	return 0;
}

static int
pssense_handle_read(struct pssense_device *pssense)
{
	int ret;

	// Report data
	uint8_t buf[INPUT_REPORT_BLUETOOTH_LENGTH] = {0};
	ret = pssense_read_packet_data(pssense, buf, sizeof(buf));

	// Get the receive time as close to the packet read as possible
	timepoint_ns recv_time_ns = os_monotonic_get_ns();

	if (ret == -EAGAIN) {
		// No data yet, not an error
		return 0;
	}

	if (ret < 0) {
		PSSENSE_ERROR(pssense, "Error reading from device: %d", ret);
		return ret;
	}

	switch (buf[0]) {
	case INPUT_REPORT_ID_USB: {
		struct pssense_usb_input_report data = {0};
		if (ret != sizeof(data)) {
			PSSENSE_ERROR(pssense, "Unexpected USB input report size %d (expected %zu)", ret, sizeof(data));
			return -EINVAL;
		}

		memcpy(&data, buf, sizeof(data));

		return pssense_handle_packet(pssense, recv_time_ns, &data.common);
	}
	case INPUT_REPORT_ID_BLUETOOTH: {
		struct pssense_bluetooth_input_report data = {0};
		if (ret != sizeof(data)) {
			PSSENSE_ERROR(pssense, "Unexpected Bluetooth input report size %d (expected %zu)", ret,
			              sizeof(data));
			return -EINVAL;
		}

		memcpy(&data, buf, sizeof(data));

		// Verify the CRC of the packet
		uint32_t expected_crc = __le32_to_cpu(data.crc);
		uint32_t crc = crc32_le(0, &INPUT_REPORT_CRC32_SEED, 1);
		crc = crc32_le(crc, (uint8_t *)&data, sizeof(struct pssense_bluetooth_input_report) - 4);
		if (crc != expected_crc) {
			PSSENSE_WARN(pssense, "CRC mismatch; skipping input. Expected %08X but got %08X", expected_crc,
			             crc);
			return -EINVAL;
		}

		return pssense_handle_packet(pssense, recv_time_ns, &data.common);
	}
	default: {
		PSSENSE_WARN(pssense, "Unhandled HID report id %u", buf[0]);
	}
	}

	return 0;
}

static void
pssense_set_output_report_settings_locked(struct pssense_device *pssense,
                                          struct pssense_output_settings *settings,
                                          bool do_vibration,
                                          uint64_t now_ns)
{
	if (now_ns >= pssense->output.vibration_end_timestamp_ns) {
		pssense->output.vibration_amplitude = 0;
	}

	if (pssense->output.send_vibration && do_vibration) {
		settings->flag1 |= OUTPUT_SETTINGS_ENABLE_VIBRATION_BITS | pssense->output.vibration_mode;
		settings->vibration_amplitude = pssense->output.vibration_amplitude;
		pssense->output.send_vibration = pssense->output.vibration_amplitude > 0;
	}

	if (pssense->output.send_trigger_feedback) {
		settings->flag1 |= PSSENSE_OUTPUT_SETTINGS_FLAG1_ADAPTIVE_TRIGGER_ENABLE;
		settings->trigger_settings.mode = pssense->output.trigger_feedback_mode;
		pssense->output.send_trigger_feedback = false;
	}

	// Give it some time to settle
	if (pssense->tracking.received_frames > 10) {
		settings->led_settings = pssense->tracking.led_settings;

#if 0
		PSSENSE_DEBUG(pssense, "Full LED settings: cycle length %uns, cycle position %luns, sequence number %u",
		              settings->led_settings.cycle_length / 3,
		              (timepoint_ns)IMU_TICKS_TO_NS(settings->led_settings.cycle_position),
		              pssense->tracking.led_sequence_num);

		PSSENSE_DEBUG_HEX(pssense, (uint8_t *)&settings->led_settings,
		                  sizeof(settings->led_settings));
#endif
	} else {
		settings->led_settings.phase = LED_SYNC_PHASE_LED_ALL_OFF;
	}

	pssense->output.next_seq_no = (pssense->output.next_seq_no + 1) % 16;

	uint32_t host_send_ts = os_monotonic_get_ns() / U_TIME_1US_IN_NS;

	// Set the host timestamp as *close* as possible to us sending the packet
	settings->host_timestamp_send_time_us = __cpu_to_le32(host_send_ts);

	if (pssense->tracking.led_sync_sample_needs_sending) {
		pssense->timing.last_sent_host_timestamp_us = host_send_ts;
		pssense->tracking.led_sync_sample_needs_sending = false;
		pssense->tracking.led_sync_sample_needs_marking = true;
	}
}

static int
pssense_send_bluetooth_output_report_locked(struct pssense_device *pssense)
{
	uint64_t timestamp_ns = os_monotonic_get_ns();

	struct pssense_ps5_output_report report = {
	    .report_id = OUTPUT_REPORT_ID_BLUETOOTH,
	    // low bits are always zero, to indicate we are using the PS5 packet format
	    .seq_no_mode = (pssense->output.next_seq_no << 4) | (0x0),
	    .tag = OUTPUT_REPORT_TAG,
	    // Packet counter needs to increment with every packet, or PCM haptics won't work.
	    .counter = pssense->output.packet_counter++,
	};

	float pcm_buf[PCM_HAPTIC_BUF_SIZE] = {0};
	size_t read_pcm_samples = u_resampler_read(pssense->output.pcm_haptics_resampler, pcm_buf, ARRAY_SIZE(pcm_buf));

	if (read_pcm_samples > 0) {
		for (size_t i = 0; i < read_pcm_samples; i++) {
			// Convert from float [-1, 1] to int8 [-128, 127].
			report.haptics[i] = (int8_t)(CLAMP(((pcm_buf[i] + 1.0f) * 0.5f * 255) - 128, -128, 127));
		}
	}

	pssense_set_output_report_settings_locked(pssense, &report.settings, read_pcm_samples == 0, timestamp_ns);

	uint32_t crc = crc32_le(0, &OUTPUT_REPORT_CRC32_SEED, 1);
	crc = crc32_le(crc, (uint8_t *)&report, sizeof(struct pssense_ps5_output_report) - 4);
	report.crc = __cpu_to_le32(crc);

	PSSENSE_TRACE(pssense,
	              "Setting vibration amplitude: %u, mode: %02X, trigger feedback mode: %02X. Next seq no: %u. PCM "
	              "samples: %zu",
	              pssense->output.vibration_amplitude, pssense->output.vibration_mode,
	              pssense->output.trigger_feedback_mode, pssense->output.next_seq_no, read_pcm_samples);
	int ret = os_hid_write(pssense->hid, (uint8_t *)&report, sizeof(report));
	if (ret != sizeof(report)) {
		PSSENSE_WARN(pssense, "Failed to send output report: %d", ret);
		return ret < 0 ? ret : -EIO;
	}

#if 0
	PSSENSE_DEBUG_HEX(pssense, (uint8_t *)&report, sizeof(report));
#endif

	return 0;
}

static int
pssense_send_usb_report_locked(struct pssense_device *pssense)
{
	uint64_t timestamp_ns = os_monotonic_get_ns();

	struct pssense_usb_output_report report = {
	    .seq_no_mode = (pssense->output.next_seq_no << 4) | (0x2),
	};

	pssense_set_output_report_settings_locked(pssense, &report.settings, true, timestamp_ns);

	int ret = os_hid_write(pssense->hid, (uint8_t *)&report, sizeof(report));
	if (ret != sizeof(report)) {
		PSSENSE_WARN(pssense, "Failed to send output report: %d", ret);
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

static int
pssense_send_output_report_locked(struct pssense_device *pssense)
{
	if (pssense->usb) {
		return pssense_send_usb_report_locked(pssense);
	} else {
		return pssense_send_bluetooth_output_report_locked(pssense);
	}

	assert(!"unreachable");
	return -EINVAL;
}

//! Sets the controller to use the lower 133hz polling rate
static bool
pssense_set_pc_polling_rate(struct pssense_device *pssense)
{
	struct pssense_set_polling_rate_feature_report report = {
	    .report_id = SET_POLLING_RATE_FEATURE_REPORT_ID,
	    .unk = 0x0E,
	    .rate_1 = __cpu_to_le16(0x000C),
	    .rate_2 = __cpu_to_le16(0x0002),
	};

	uint32_t crc = crc32_le(0, &SET_FEATURE_REPORT_CRC32_SEED, 1);
	crc = crc32_le(crc, (uint8_t *)&report, sizeof(struct pssense_set_polling_rate_feature_report) - 4);
	report.crc = __cpu_to_le32(crc);

	int ret = os_hid_set_feature(pssense->hid, (uint8_t *)&report, sizeof report);
	if (ret < 0) {
		PSSENSE_ERROR(pssense, "Failed to set PC polling rate, reason %d", ret);
		return false;
	}

	return true;
}

static void *
pssense_run_thread(void *ptr)
{
	U_TRACE_SET_THREAD_NAME("PS Sense");

	struct pssense_device *pssense = ptr;

#ifdef XRT_OS_LINUX
	u_linux_try_to_set_realtime_priority_on_thread(pssense->log_level, "PS Sense");
#endif

	os_thread_helper_lock(&pssense->controller_thread);

	// 32/3000hz (PCM haptic rate), this will *technically* run slightly fast, but like, that's fine.
	const time_duration_ns pcm_haptics_period_ns = 10666666;

	timepoint_ns next_output_ns = os_monotonic_get_ns();

	int result = 0;
	while (os_thread_helper_is_running_locked(&pssense->controller_thread) && result >= 0) {
		os_thread_helper_unlock(&pssense->controller_thread);

		result = pssense_handle_read(pssense);

		if (result >= 0) {
			timepoint_ns now = os_monotonic_get_ns();

			if (now >= next_output_ns) {
				os_thread_helper_lock(&pssense->controller_thread);
				result = pssense_send_output_report_locked(pssense);
				os_thread_helper_unlock(&pssense->controller_thread);

				next_output_ns = next_output_ns + pcm_haptics_period_ns;
			}
		}

		if (result >= 0) {
			// @note we don't break the earlier `now` out into the outer scope accessible from here since
			//       sending may take some non-negligible amount of time.
			timepoint_ns to_next_output = next_output_ns - os_monotonic_get_ns();

			// Only sleep if it's an increment greater than 50us, Linux doesn't like sleeping that short and
			// often oversleeps a bit, timing matters with PCM haptics and LED sync!
			if (to_next_output > (U_TIME_1US_IN_NS * 50L)) {
				// Sleep 1ms, or half the time to the next output report, whichever is smaller. We want
				// to wake up frequently enough to read incoming packets, but not so much that we waste
				// CPU time waking too often.
				os_precise_sleeper_nanosleep(&pssense->sleeper,
				                             MIN(to_next_output / 2, U_TIME_1MS_IN_NS));
			}
		}

		os_thread_helper_lock(&pssense->controller_thread);
	}

	os_thread_helper_unlock(&pssense->controller_thread);

	return NULL;
}

static void
pssense_get_imu_fusion_pose(struct pssense_device *pssense,
                            int64_t at_timestamp_ns,
                            struct xrt_space_relation *out_relation)
{
	timepoint_ns device_ts;
	if (!pssense_host_ts_to_device(pssense, at_timestamp_ns, &device_ts)) {
		(*out_relation) = (struct xrt_space_relation){0};
		return;
	}

	m_relation_history_get(pssense->tracking.imu_relation_history, device_ts, out_relation);
}

static void
pssense_get_constellation_pose(struct pssense_device *pssense,
                               int64_t at_timestamp_ns,
                               struct xrt_space_relation *out_relation)
{
	(*out_relation) = XRT_C11_COMPOUND(struct xrt_space_relation) XRT_SPACE_RELATION_ZERO;

	if (pssense->tracking.constellation_tracker &&
	    t_constellation_tracker_get_tracked_pose(pssense->tracking.constellation_tracker,
	                                             pssense->tracking.constellation_device_id, at_timestamp_ns,
	                                             out_relation) < 0) {
		(*out_relation) = XRT_C11_COMPOUND(struct xrt_space_relation) XRT_SPACE_RELATION_ZERO;
	}

	if (out_relation->relation_flags == 0) {
		timepoint_ns device_ts;
		if (!pssense_host_ts_to_device(pssense, at_timestamp_ns, &device_ts)) {
			(*out_relation) = (struct xrt_space_relation){0};
			return;
		}

		m_relation_history_get(pssense->tracking.constellation_relation_history, device_ts, out_relation);
	}
}

static void
parse_pssense_calibration(struct pssense_device *pssense,
                          const struct pssense_calibration_data *calibration_data,
                          struct pssense_parsed_calibration *out_parsed)
{
	int16_t gyro_speed_ref1 = (int16_t)__le16_to_cpu(calibration_data->gyro_speed_ref1);
	int16_t gyro_speed_ref2 = (int16_t)__le16_to_cpu(calibration_data->gyro_speed_ref2);
	float rad_per_sec_ref = ((float)gyro_speed_ref1 + (float)gyro_speed_ref2) * 0.5f * (float)(M_PI / 180.0);
	if (!(rad_per_sec_ref > 0.0f)) {
		PSSENSE_DEBUG(pssense, "Got invalid gyro speed reference of %f rad/s. Falling back to default.",
		              rad_per_sec_ref);
		rad_per_sec_ref = (float)(M_PI * 3.0);
	}

	PSSENSE_DEBUG(pssense, "Gyro speed reference: %f rad/s", rad_per_sec_ref);

	int16_t gyro_plus_x = __le16_to_cpu(calibration_data->gyro_plus_x);
	int16_t gyro_minus_x = __le16_to_cpu(calibration_data->gyro_minus_x);

	int16_t gyro_plus_y = __le16_to_cpu(calibration_data->gyro_plus_y);
	int16_t gyro_minus_y = __le16_to_cpu(calibration_data->gyro_minus_y);

	int16_t gyro_plus_z = __le16_to_cpu(calibration_data->gyro_plus_z);
	int16_t gyro_minus_z = __le16_to_cpu(calibration_data->gyro_minus_z);

	int16_t accel_plus_x = __le16_to_cpu(calibration_data->accel_plus_x);
	int16_t accel_minus_x = __le16_to_cpu(calibration_data->accel_minus_x);
	int16_t accel_plus_y = __le16_to_cpu(calibration_data->accel_plus_y);
	int16_t accel_minus_y = __le16_to_cpu(calibration_data->accel_minus_y);
	int16_t accel_plus_z = __le16_to_cpu(calibration_data->accel_plus_z);
	int16_t accel_minus_z = __le16_to_cpu(calibration_data->accel_minus_z);

	struct xrt_vec3_i32 gyro_bias = {
	    .x = (int16_t)__le16_to_cpu(calibration_data->gyro_bias_x),
	    .y = (int16_t)__le16_to_cpu(calibration_data->gyro_bias_y),
	    .z = (int16_t)__le16_to_cpu(calibration_data->gyro_bias_z),
	};

	float gyro_span_x =
	    (fabsf((float)gyro_plus_x - (float)gyro_bias.x) + fabsf((float)gyro_minus_x - (float)gyro_bias.x)) / 2.0f;
	float gyro_span_y =
	    (fabsf((float)gyro_plus_y - (float)gyro_bias.y) + fabsf((float)gyro_minus_y - (float)gyro_bias.y)) / 2.0f;
	float gyro_span_z =
	    (fabsf((float)gyro_plus_z - (float)gyro_bias.z) + fabsf((float)gyro_minus_z - (float)gyro_bias.z)) / 2.0f;

	struct xrt_vec3 gyro_scale = {
	    .x = rad_per_sec_ref / gyro_span_x,
	    .y = rad_per_sec_ref / gyro_span_y,
	    .z = rad_per_sec_ref / gyro_span_z,
	};

	struct xrt_vec3 accel_bias = {
	    .x = (accel_plus_x + accel_minus_x) / 2.0f,
	    .y = (accel_plus_y + accel_minus_y) / 2.0f,
	    .z = (accel_plus_z + accel_minus_z) / 2.0f,
	};

	struct xrt_vec3 accel_scale = {
	    .x = (1.0f / (float)(accel_plus_x - accel_bias.x)) * MATH_GRAVITY_M_S2,
	    .y = (1.0f / (float)(accel_plus_y - accel_bias.y)) * MATH_GRAVITY_M_S2,
	    .z = (1.0f / (float)(accel_plus_z - accel_bias.z)) * MATH_GRAVITY_M_S2,
	};

	PSSENSE_DEBUG(pssense, "Gyro scale: %f\t%f\t%f rad/s", gyro_scale.x, gyro_scale.y, gyro_scale.z);
	PSSENSE_DEBUG(pssense, "Gyro bias: %d\t%d\t%d", gyro_bias.x, gyro_bias.y, gyro_bias.z);
	PSSENSE_DEBUG(pssense, "Accel scale: %f\t%f\t%f m/s^2", accel_scale.x, accel_scale.y, accel_scale.z);
	PSSENSE_DEBUG(pssense, "Accel bias: %f\t%f\t%f", accel_bias.x, accel_bias.y, accel_bias.z);

	(*out_parsed) = XRT_C11_COMPOUND(struct pssense_parsed_calibration){
	    .gyro_scale = gyro_scale,
	    .accel_scale = accel_scale,

	    .accel_bias = accel_bias,
	    .gyro_bias = gyro_bias,
	};
}

/*!
 * Retrieving the calibration data report will switch the Sense controller from compat mode into full mode.
 */
static bool
pssense_get_calibration_data(struct pssense_device *pssense)
{
	struct pssense_calibration_data calibration_data = {0};

	bool invalid_crc;
	do {
		invalid_crc = false;

		// Calibration has to be read in two parts with two feature reads.
		for (int i = 0; i < 2; i++) {
			struct pssense_feature_report report_buffer = {.report_id = CALIBRATION_DATA_FEATURE_REPORT_ID};
			int ret = os_hid_get_feature(pssense->hid, report_buffer.report_id, (uint8_t *)&report_buffer,
			                             sizeof(report_buffer));

			if (ret < 0) {
				PSSENSE_ERROR(pssense, "Failed to retrieve calibration report: %d", ret);
				return false;
			}

			if (ret != sizeof(report_buffer)) {
				PSSENSE_ERROR(pssense, "Invalid byte count transferred, expected %zu got %d",
				              sizeof(report_buffer), ret);
				return false;
			}

			switch (report_buffer.part_id) {
			case CALIBRATION_DATA_PART_ID_1: {
				memcpy((uint8_t *)&calibration_data, report_buffer.data, sizeof(report_buffer.data));
				break;
			}
			case CALIBRATION_DATA_PART_ID_2: {
				memcpy(((uint8_t *)&calibration_data) + sizeof(report_buffer.data), report_buffer.data,
				       sizeof(report_buffer.data));
				break;
			}
			default: {
				PSSENSE_ERROR(pssense, "Unknown calibration data part ID %u", report_buffer.part_id);
				return false;
			}
			}

			uint32_t crc = crc32_le(0, &FEATURE_REPORT_CRC32_SEED, 1);
			crc = crc32_le(crc, (uint8_t *)&report_buffer, sizeof(report_buffer) - 4);
			uint32_t expected_crc = __le32_to_cpu(report_buffer.crc);

			// Only check CRC on Bluetooth, CRC is zeroed out on USB.
			if (crc != expected_crc && !pssense->usb) {
				PSSENSE_WARN(pssense, "Invalid feature report CRC. Expected 0x%08X, actual 0x%08X",
				             expected_crc, crc);
				invalid_crc = true;
			}
		}
	} while (invalid_crc);

	parse_pssense_calibration(pssense, &calibration_data, &pssense->calibration);
	pssense->has_calibration = true;

	PSSENSE_DEBUG(pssense, "Calibration data retrieved and parsed successfully");

	return true;
}

static uint64_t
saturating_add_uint64(uint64_t a, uint64_t b)
{
	if (UINT64_MAX - a < b) {
		return UINT64_MAX;
	} else {
		return a + b;
	}
}

/*
 *
 * Frame node implementations
 *
 */

static void
pssense_node_break_apart(struct xrt_frame_node *node)
{
	struct pssense_device *pssense = from_node(node);

	// Make sure the USB thread is stopped
	os_thread_helper_stop_and_wait(&pssense->controller_thread);
}

static void
pssense_node_destroy(struct xrt_frame_node *node)
{
	struct pssense_device *pssense = from_node(node);

	// Destroy the controller thread
	os_thread_helper_destroy(&pssense->controller_thread);

	// Deinit the precise sleeper
	os_precise_sleeper_deinit(&pssense->sleeper);

	if (pssense->output.pcm_haptics_resampler) {
		u_resampler_destroy(pssense->output.pcm_haptics_resampler);
		pssense->output.pcm_haptics_resampler = NULL;
	}

	m_imu_3dof_close(&pssense->tracking.fusion);

	if (pssense->hid != NULL) {
		os_hid_destroy(pssense->hid);
		pssense->hid = NULL;
	}

	// Relation histories are used from the frame context lifecycle in the constellation tracker device callbacks.
	m_relation_history_destroy(&pssense->tracking.imu_relation_history);
	m_relation_history_destroy(&pssense->tracking.constellation_relation_history);

	// LED sync is used on the frame context lifecycle, so it needs to be destroyed in here.
	t_led_sync_refinement_destroy(&pssense->tracking.led_sync_refinement);

	// Remove the variable tracking.
	u_var_remove_root(pssense);

	if (pssense->led_model.leds != NULL) {
		free(pssense->led_model.leds);
		pssense->led_model.leds = NULL;
	}

	// Actually free the pointer
	free(pssense);
}

/*
 *
 * Timing event sink implementation
 *
 */

static void
pssense_timing_event_sink_push(struct t_timing_event_sink *sink, const struct t_timing_event *event)
{
	struct pssense_device *pssense = from_timing_event_sink(sink);

	// We only care about exposure start.
	if (event->type != T_TIMING_EVENT_TYPE_CAMERA_EXPOSURE_START) {
		return;
	}

	struct t_timing_event_camera_exposure_start camera_exposure = event->camera_exposure_start;

	PSSENSE_TRACE(pssense, "Received timing event: %d, seq id: %" PRIu64 ", timestamp: %" PRId64 "ns", event->type,
	              camera_exposure.sequence_id, camera_exposure.timestamp_ns);

	if (pssense->tracking.received_frames++ == 0) {
		pssense->tracking.last_exposure_sequence_id = camera_exposure.sequence_id;
		pssense->tracking.last_exposure_local_timestamp_ns = camera_exposure.timestamp_ns;
		return;
	}

	uint32_t sequence_id_delta = camera_exposure.sequence_id - pssense->tracking.last_exposure_sequence_id;

	time_duration_ns estimated_interval =
	    (camera_exposure.timestamp_ns - pssense->tracking.last_exposure_local_timestamp_ns) / (sequence_id_delta);

	// If available, use the frame period from the event
	if (camera_exposure.frame_period_ns > 0) {
		estimated_interval = camera_exposure.frame_period_ns;
	}

	// If this is the second frame we've received
	if (pssense->tracking.received_frames == 2) {
		pssense->tracking.average_exposure_interval_ns = estimated_interval;
		PSSENSE_TRACE(pssense, "Initial exposure interval: %" PRId64 "ns", estimated_interval);
	} else {
		// Iteratively average out the estimated interval to try to avoid noise
		pssense->tracking.average_exposure_interval_ns =
		    (estimated_interval * 0.1f) + (pssense->tracking.average_exposure_interval_ns * 0.9f);
		PSSENSE_TRACE(pssense, "Updated exposure interval: %" PRId64 "ns",
		              pssense->tracking.average_exposure_interval_ns);
	}

	pssense->tracking.last_exposure_sequence_id = camera_exposure.sequence_id;
	pssense->tracking.last_exposure_local_timestamp_ns = camera_exposure.timestamp_ns;

	if (pssense->tracking.average_exposure_interval_ns > 0) {
		// Update the frame period to the one we're using internally and push the timing event
		struct t_timing_event_camera_exposure_start led_sync_event = event->camera_exposure_start;
		led_sync_event.frame_period_ns = pssense->tracking.average_exposure_interval_ns;
		t_led_sync_push_timing_event(&pssense->tracking.led_sync_refinement, &led_sync_event);
	}

	os_thread_helper_lock(&pssense->controller_thread);

	// update the LED settings
	if (pssense->tracking.received_frames > 10) {
		// Update the sample from the LED sync routine
		if (t_led_sync_get_sample(&pssense->tracking.led_sync_refinement,
		                          &pssense->tracking.latest_led_sync_sample)) {
			pssense->tracking.led_sync_sample_needs_sending = true;
			pssense->tracking.period_id =
			    DURATION_NS_TO_PERIOD_ID(pssense->tracking.latest_led_sync_sample.blink_duration_ns);
			pssense->tracking.led_sequence_num += 1;
		}

		uint8_t period_id = pssense->tracking.period_id;

		// Convert the timestamp, latency offset will be applied within here. Must happen after fetching the
		// sample above, so the latency and the fudge offset below come from the same sample.
		timepoint_ns next_blink_time = 0;
		if (!pssense_host_ts_to_device(pssense, pssense->tracking.last_exposure_local_timestamp_ns,
		                               &next_blink_time)) {
			os_thread_helper_unlock(&pssense->controller_thread);
			return;
		}

		next_blink_time += (int64_t)pssense->tracking.timing_fudge_100us * 100 * U_TIME_1US_IN_NS;
		// Apply the fudge offset, which will line up the blink center with exposure center
		next_blink_time += (int64_t)pssense->tracking.latest_led_sync_sample.fudge_offset_ns;

		// PSSENSE cycle position on the wire is the *center* of the exposure, but our LED sync assumes it's the
		// start of the exposure, so we need to make it blink later to account
		next_blink_time += PERIOD_ID_TO_DURATION_NS(period_id) / 2;

		// inside thirds of a nanosecond
		uint32_t cycle_length = pssense->tracking.average_exposure_interval_ns * 3;
		// in IMU ticks
		uint32_t cycle_position = NS_TO_IMU_TICKS(next_blink_time);

#if 0
		static int64_t jitter_integration = 0;

		PSSENSE_DEBUG(pssense, "(blink length %uus) drift %ldus",
		              (uint32_t)(IMU_TICKS_TO_NS(150LLU * 32 + 1250) / 1000), jitter_integration / 1000);
		jitter_integration += jitter;

		if (jitter_integration > U_TIME_1S_IN_NS) {
			jitter_integration = 0;
		}
#endif

		pssense->tracking.led_settings = (struct pssense_led_settings){
		    .phase = LED_SYNC_PHASE_PRESCAN,
		    .cycle_length = __cpu_to_le32(cycle_length),
		    .cycle_position = __cpu_to_le32(cycle_position),
		    .sequence_number = pssense->tracking.led_sequence_num,
		    .led_blink = {0xFF, 0xFF, 0xFF, 0xFF},
		    .period_id = period_id,
		};

		if (pssense->tracking.increment_sequence_num) {
			pssense->tracking.led_sequence_num += 1;
#if 0
			PSSENSE_DEBUG(pssense, "%lu\t%lu",
			              (pssense->tracking.last_exposure_local_timestamp_ns %
			               pssense->tracking.average_exposure_interval_ns) /
			                  1000,
			              (next_blink_time % pssense->tracking.average_exposure_interval_ns) / 1000);
#endif
		}
	}
	os_thread_helper_unlock(&pssense->controller_thread);
}

/*
 *
 * Constellation tracker device implementations
 *
 */

static void
pssense_push_constellation_tracker_sample(struct t_constellation_tracker_device *device,
                                          struct t_constellation_tracker_sample *sample)
{
	struct pssense_device *pssense = from_constellation_device(device);

	t_led_sync_push_constellation_sample(&pssense->tracking.led_sync_refinement, sample);

	// Don't do anything else if the sample doesn't have a world pose
	if (!sample->has_world_pose) {
		return;
	}

	os_thread_helper_lock(&pssense->controller_thread);
	timepoint_ns device_ts;
	if (!pssense_host_ts_to_device(pssense, sample->timestamp_ns, &device_ts)) {
		os_thread_helper_unlock(&pssense->controller_thread);
		return;
	}
	os_thread_helper_unlock(&pssense->controller_thread);

	struct xrt_space_relation relation = {
	    .pose = sample->world_pose,
	    .relation_flags = XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
	                      XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT,
	};

	m_relation_history_push(pssense->tracking.constellation_relation_history, &relation, device_ts);
}

/*
 *
 * Constellation tracking source implementations
 *
 */

static void
pssense_get_constellation_tracking_source_pose(struct t_constellation_tracker_tracking_source *tracking_source,
                                               int64_t when_ns,
                                               struct xrt_space_relation *out_relation)
{
	struct pssense_device *pssense = from_constellation_tracking_source(tracking_source);

	os_thread_helper_lock(&pssense->controller_thread);
	pssense_get_constellation_pose(pssense, when_ns, out_relation);
	os_thread_helper_unlock(&pssense->controller_thread);
}

/*
 *
 * Driver implementations
 *
 */

static void
pssense_device_destroy(struct xrt_device *xdev)
{
	struct pssense_device *pssense = from_device(xdev);

	// Stop the thread helper
	os_thread_helper_stop_and_wait(&pssense->controller_thread);

	// Don't free the pointer or destroy any resources,
	// since they may be needed after device destroy but before node destroy
}

static xrt_result_t
pssense_device_update_inputs(struct xrt_device *xdev)
{
	struct pssense_device *pssense = from_device(xdev);

	// Lock the data.
	os_thread_helper_lock(&pssense->controller_thread);

	timepoint_ns host_update_time_ns;
	if (!pssense_device_ts_to_host(pssense, pssense->timing.latest_device_time_ns, &host_update_time_ns)) {
		host_update_time_ns = pssense->state.timestamp_ns;
	}

	// Update all the inputs to the correct timestamp
	for (uint32_t i = 0; i < ((uint32_t)PSSENSE_INPUT_COUNT); i++) {
		pssense->base.inputs[i].timestamp = (int64_t)host_update_time_ns;
	}
	pssense->base.inputs[PSSENSE_INDEX_PS_CLICK].value.boolean = pssense->state.ps_click;
	pssense->base.inputs[PSSENSE_INDEX_SHARE_CLICK].value.boolean = pssense->state.share_click;
	pssense->base.inputs[PSSENSE_INDEX_OPTIONS_CLICK].value.boolean = pssense->state.options_click;
	pssense->base.inputs[PSSENSE_INDEX_SQUARE_CLICK].value.boolean = pssense->state.square_click;
	pssense->base.inputs[PSSENSE_INDEX_SQUARE_TOUCH].value.boolean = pssense->state.square_touch;
	pssense->base.inputs[PSSENSE_INDEX_TRIANGLE_CLICK].value.boolean = pssense->state.triangle_click;
	pssense->base.inputs[PSSENSE_INDEX_TRIANGLE_TOUCH].value.boolean = pssense->state.triangle_touch;
	pssense->base.inputs[PSSENSE_INDEX_CROSS_CLICK].value.boolean = pssense->state.cross_click;
	pssense->base.inputs[PSSENSE_INDEX_CROSS_TOUCH].value.boolean = pssense->state.cross_touch;
	pssense->base.inputs[PSSENSE_INDEX_CIRCLE_CLICK].value.boolean = pssense->state.circle_click;
	pssense->base.inputs[PSSENSE_INDEX_CIRCLE_TOUCH].value.boolean = pssense->state.circle_touch;
	pssense->base.inputs[PSSENSE_INDEX_SQUEEZE_CLICK].value.boolean = pssense->state.squeeze_click;
	pssense->base.inputs[PSSENSE_INDEX_SQUEEZE_TOUCH].value.boolean = pssense->state.squeeze_touch;
	pssense->base.inputs[PSSENSE_INDEX_SQUEEZE_PROXIMITY].value.boolean = pssense->state.squeeze_proximity > 0.7f;
	pssense->base.inputs[PSSENSE_INDEX_SQUEEZE_PROXIMITY_FLOAT].value.vec1.x = pssense->state.squeeze_proximity;
	pssense->base.inputs[PSSENSE_INDEX_TRIGGER_CLICK].value.boolean = pssense->state.trigger_click;
	pssense->base.inputs[PSSENSE_INDEX_TRIGGER_TOUCH].value.boolean = pssense->state.trigger_touch;
	pssense->base.inputs[PSSENSE_INDEX_TRIGGER_VALUE].value.vec1.x = pssense->state.trigger_value;
	pssense->base.inputs[PSSENSE_INDEX_TRIGGER_PROXIMITY].value.boolean = pssense->state.trigger_proximity > 0.7f;
	pssense->base.inputs[PSSENSE_INDEX_TRIGGER_PROXIMITY_FLOAT].value.vec1.x = pssense->state.trigger_proximity;
	pssense->base.inputs[PSSENSE_INDEX_THUMBSTICK].value.vec2 = pssense->state.thumbstick;
	pssense->base.inputs[PSSENSE_INDEX_THUMBSTICK_CLICK].value.boolean = pssense->state.thumbstick_click;
	pssense->base.inputs[PSSENSE_INDEX_THUMBSTICK_TOUCH].value.boolean = pssense->state.thumbstick_touch;

	// Done now.
	os_thread_helper_unlock(&pssense->controller_thread);

	return XRT_SUCCESS;
}

static xrt_result_t
set_vibration_output(struct pssense_device *pssense,
                     const struct xrt_output_value *value,
                     bool *send_vibration,
                     uint8_t *vibration_amplitude,
                     uint8_t *vibration_mode)
{
	switch (value->type) {
	case XRT_OUTPUT_VALUE_TYPE_VIBRATION: {
		*send_vibration = true;
		*vibration_amplitude = (uint8_t)(value->vibration.amplitude * 255.0f);
		*vibration_mode = OUTPUT_SETTINGS_VIBRATE_MODE_CLASSIC_RUMBLE;

		if (value->vibration.frequency != XRT_FREQUENCY_UNSPECIFIED) {
			if (value->vibration.frequency <= 70) {
				*vibration_mode = OUTPUT_SETTINGS_VIBRATE_MODE_LOW_60HZ;
			} else if (value->vibration.frequency >= 110) {
				*vibration_mode = OUTPUT_SETTINGS_VIBRATE_MODE_HIGH_120HZ;
			}
		}
		break;
	}
	case XRT_OUTPUT_VALUE_TYPE_PCM_VIBRATION: {
		os_thread_helper_lock(&pssense->controller_thread);
		// Reset the resampler if we're not appending.
		if (!value->pcm_vibration.append) {
			u_resampler_reset(pssense->output.pcm_haptics_resampler);
		}

		size_t samples_consumed =
		    u_resampler_write(pssense->output.pcm_haptics_resampler, value->pcm_vibration.buffer,
		                      value->pcm_vibration.buffer_size, value->pcm_vibration.sample_rate);
		os_thread_helper_unlock(&pssense->controller_thread);

		*value->pcm_vibration.samples_consumed = samples_consumed;
		break;
	}
	default: {
		U_LOG_XDEV_UNSUPPORTED_OUTPUT(&pssense->base, pssense->log_level, XRT_OUTPUT_NAME_PSSENSE_VIBRATION);
		return XRT_ERROR_OUTPUT_UNSUPPORTED;
		break;
	}
	}

	return XRT_SUCCESS;
}

static xrt_result_t
pssense_set_output(struct xrt_device *xdev, enum xrt_output_name name, const struct xrt_output_value *value)
{
	struct pssense_device *pssense = from_device(xdev);

	bool send_vibration = false;
	uint8_t vibration_amplitude;
	uint8_t vibration_mode;

	bool send_trigger_feedback = false;
	enum pssense_adaptive_trigger_mode trigger_feedback_mode;

	switch (name) {
	case XRT_OUTPUT_NAME_PSSENSE_VIBRATION: {
		xrt_result_t result =
		    set_vibration_output(pssense, value, &send_vibration, &vibration_amplitude, &vibration_mode);
		if (result != XRT_SUCCESS) {
			return result;
		}

		break;
	}
	case XRT_OUTPUT_NAME_PSSENSE_TRIGGER_FEEDBACK: {
		for (uint64_t i = 0; i < value->force_feedback.force_feedback_location_count; i++) {
			if (value->force_feedback.force_feedback[i].location ==
			    XRT_FORCE_FEEDBACK_LOCATION_LEFT_INDEX) {
				send_trigger_feedback = true;
				if (value->force_feedback.force_feedback[i].value > 0) {
					trigger_feedback_mode = TRIGGER_FEEDBACK_MODE_SIMPLE_FEEDBACK;
				} else {
					trigger_feedback_mode = TRIGGER_FEEDBACK_MODE_OFF;
				}
			}
		}

		break;
	}
	default: {
		U_LOG_XDEV_UNSUPPORTED_OUTPUT(&pssense->base, pssense->log_level, name);
		return XRT_ERROR_OUTPUT_UNSUPPORTED;
	}
	}

	timepoint_ns now = os_monotonic_get_ns();

	os_thread_helper_lock(&pssense->controller_thread);
	if (send_vibration && (vibration_amplitude != pssense->output.vibration_amplitude ||
	                       vibration_mode != pssense->output.vibration_mode)) {
		pssense->output.send_vibration = true;
		pssense->output.vibration_amplitude = vibration_amplitude;
		pssense->output.vibration_mode = vibration_mode;
		// Some applications (hello_xr has been seen doing this) will set the duration to INT64_MAX, so when
		// adding directly, it overflows and doesn't work. This prevents that.
		pssense->output.vibration_end_timestamp_ns = saturating_add_uint64(now, value->vibration.duration_ns);
	}

	if (send_trigger_feedback && trigger_feedback_mode != pssense->output.trigger_feedback_mode) {
		pssense->output.send_trigger_feedback = true;
		pssense->output.trigger_feedback_mode = trigger_feedback_mode;
	}
	os_thread_helper_unlock(&pssense->controller_thread);

	return XRT_SUCCESS;
}

xrt_result_t
pssense_get_output_limits(struct xrt_device *xdev, struct xrt_output_limits *limits)
{
	(*limits) = XRT_C11_COMPOUND(struct xrt_output_limits){
	    // PCM data is played back at 3000hz
	    .haptic_pcm_sample_rate = PCM_SAMPLE_RATE,
	};

	return XRT_SUCCESS;
}

static struct xrt_quat
quat_from_x_rot(float x)
{
	return (struct xrt_quat){
	    .x = sinf(x * 0.5f),
	    .y = 0,
	    .z = 0,
	    .w = cosf(x * 0.5f),
	};
}

static xrt_result_t
pssense_get_tracked_pose(struct xrt_device *xdev,
                         enum xrt_input_name name,
                         int64_t at_timestamp_ns,
                         struct xrt_space_relation *out_relation)
{
	struct pssense_device *pssense = from_device(xdev);

	if (name != XRT_INPUT_PSSENSE_AIM_POSE && name != XRT_INPUT_PSSENSE_GRIP_POSE) {
		U_LOG_XDEV_UNSUPPORTED_INPUT(&pssense->base, pssense->log_level, name);
		return XRT_ERROR_INPUT_UNSUPPORTED;
	}

	struct xrt_relation_chain xrc = {0};

	float sx = pssense->hand == XRT_HAND_LEFT ? 1.0f : -1.0f;
	struct xrt_pose T_steamvrroot_model = {
	    .orientation = quat_from_x_rot(DEG_TO_RAD(-39)),
	    .position = {.x = -sx * .01432f, .y = .007713f, .z = .10399f},
	};
	struct xrt_pose T_steamvrroot_xrgrip = {
	    .orientation = quat_from_x_rot(DEG_TO_RAD(20.6)),
	    .position = {.x = sx * .007, .y = -.00182941, .z = .1019482},
	};
	struct xrt_pose T_steamvrroot_xraim = {
	    .orientation = quat_from_x_rot(DEG_TO_RAD(-39.4)),
	    .position = {.x = sx * .007, .y = -.03894766, .z = .00949694},
	};

	switch (name) {
	case XRT_INPUT_PSSENSE_AIM_POSE: {
		m_relation_chain_push_pose(&xrc, &T_steamvrroot_xraim);
		break;
	}
	case XRT_INPUT_PSSENSE_GRIP_POSE: {
		m_relation_chain_push_pose(&xrc, &T_steamvrroot_xrgrip);
		break;
	}
	default: assert(!"Unreachable");
	}
	m_relation_chain_push_inverted_pose_if_not_identity(&xrc, &T_steamvrroot_model);

#if 0 // Presently unused OpenVR poses, but might be useful if we need to expose more stuff for st/openvr
	struct xrt_pose T_steamvrroot_xrhandmodel = {
	    .orientation = quat_from_x_rot(DEG_TO_RAD(39.4)),
	    .position = {.x = -sx * 11.25, .y = -1.82941, .z = 101.9482},
	};
	struct xrt_pose T_steamvrroot_handgrip = {
	    .orientation = quat_from_x_rot(DEG_TO_RAD(5.037)),
	    .position = {.x = 0, .y = 3, .z = 97},
	};
	struct xrt_pose T_steamvrroot_tip = {
	    .orientation = quat_from_x_rot(DEG_TO_RAD(37.4)),
	    .position = {.x = sx * 16.694, .y = -25.22, .z = 24.687},
	};
#endif

	m_relation_chain_push_inverted_pose_if_not_identity(&xrc, &pssense->tracking.T_led_imu);

	struct xrt_space_relation *rel = m_relation_chain_reserve(&xrc);

	os_thread_helper_lock(&pssense->controller_thread);
	if (pssense->tracking.use_constellation) {
		pssense_get_constellation_pose(pssense, at_timestamp_ns, rel);
	} else {
		pssense_get_imu_fusion_pose(pssense, at_timestamp_ns, rel);
	}
	os_thread_helper_unlock(&pssense->controller_thread);

	m_relation_chain_resolve(&xrc, out_relation);

	return XRT_SUCCESS;
}

static xrt_result_t
pssense_get_battery_status(struct xrt_device *xdev, bool *out_present, bool *out_charging, float *out_charge)
{
	struct pssense_device *pssense = from_device(xdev);

	if (!pssense->state.battery_state_valid) {
		*out_present = false;
		return XRT_SUCCESS;
	}

	*out_present = true;
	*out_charging = pssense->state.battery_charging;
	*out_charge = pssense->state.battery_charge_percent;

	return XRT_SUCCESS;
}

/*
 *
 * Exported functions
 *
 */

#define SET_INPUT(NAME) (pssense->base.inputs[PSSENSE_INDEX_##NAME].name = XRT_INPUT_PSSENSE_##NAME)

struct xrt_device *
pssense_create(struct xrt_prober *xp,
               struct xrt_prober_device *xpdev,
               struct xrt_frame_context *xfctx,
               struct t_timing_event_sink **out_timing_sink)
{
	struct os_hid_device *hid = NULL;
	int ret;

	// On USB, we need to use interface 2, rather than 0 like on Bluetooth.
	int iface = xpdev->bus == XRT_BUS_TYPE_USB ? 2 : 0;
	ret = xrt_prober_open_hid_interface(xp, xpdev, iface, &hid);
	if (ret != 0) {
		U_LOG_E("Failed to open HID interface for PlayStation Sense controller!");
		return NULL;
	}

	unsigned char product_name[128];
	ret = xrt_prober_get_string_descriptor( //
	    xp,                                 //
	    xpdev,                              //
	    XRT_PROBER_STRING_PRODUCT,          //
	    product_name,                       //
	    sizeof(product_name));              //
	if (ret <= 0) {
		U_LOG_E("Failed to get product name from Bluetooth device!");
		return NULL;
	}

	enum u_device_alloc_flags flags = U_DEVICE_ALLOC_TRACKING_NONE;
	struct pssense_device *pssense = U_DEVICE_ALLOCATE(struct pssense_device, flags, PSSENSE_INPUT_COUNT, 2);
	PSSENSE_DEBUG(pssense, "PlayStation Sense controller found");

	pssense->node.break_apart = pssense_node_break_apart;
	pssense->node.destroy = pssense_node_destroy;

	pssense->timing_event_sink.push_timing_event = pssense_timing_event_sink_push;

	pssense->constellation_device.push_constellation_tracker_sample = pssense_push_constellation_tracker_sample;

	pssense->constellation_tracking_source.get_tracked_pose = pssense_get_constellation_tracking_source_pose;

	pssense->base.name = XRT_DEVICE_PSSENSE;
	snprintf(pssense->base.str, XRT_DEVICE_NAME_LEN, "%s", product_name);
	pssense->base.update_inputs = pssense_device_update_inputs;
	pssense->base.set_output = pssense_set_output;
	pssense->base.get_output_limits = pssense_get_output_limits;
	pssense->base.get_tracked_pose = pssense_get_tracked_pose;
	pssense->base.get_battery_status = pssense_get_battery_status;
	pssense->base.destroy = pssense_device_destroy;

	pssense->base.supported.orientation_tracking = true;
	pssense->base.supported.battery_status = true;

	pssense->usb = xpdev->bus == XRT_BUS_TYPE_USB;

	m_imu_3dof_init(&pssense->tracking.fusion, M_IMU_3DOF_USE_GRAVITY_DUR_20MS);

	// pssense->tracking.timing_fudge_100us = 20; // 2.0ms fudge
	pssense->tracking.increment_sequence_num = true;
	pssense->timing.clock_tracker = m_clock_windowed_skew_tracker_alloc(2048);

	m_relation_history_create(&pssense->tracking.imu_relation_history);
	m_relation_history_create(&pssense->tracking.constellation_relation_history);

	pssense->log_level = debug_get_log_option_pssense_log();
	pssense->hid = hid;

	// Initialize the IMU orientation to be correct
	struct xrt_quat imu_orientation_quat = quat_from_x_rot(pssense_imu_angle);

	const struct t_constellation_tracker_led *led_model = NULL;
	if (xpdev->product_id == PSSENSE_PID_LEFT) {
		pssense->base.device_type = XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER;
		pssense->hand = XRT_HAND_LEFT;

		pssense->led_model.match_parameters = DEFAULT_MATCH_PARAMETERS;
		led_model = pssense_left_leds;
		pssense->led_model.led_count = ARRAY_SIZE(pssense_left_leds);

		pssense->tracking.T_led_imu = (struct xrt_pose){
		    .orientation = imu_orientation_quat,
		    .position = T_led_imu_left,
		};
	} else if (xpdev->product_id == PSSENSE_PID_RIGHT) {
		pssense->base.device_type = XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER;
		pssense->hand = XRT_HAND_RIGHT;

		pssense->led_model.match_parameters = DEFAULT_MATCH_PARAMETERS;
		led_model = pssense_right_leds;
		pssense->led_model.led_count = ARRAY_SIZE(pssense_right_leds);

		pssense->tracking.T_led_imu = (struct xrt_pose){
		    .orientation = imu_orientation_quat,
		    .position = T_led_imu_right,
		};
	} else {
		PSSENSE_ERROR(pssense, "Unable to determine controller type");
		pssense_device_destroy(&pssense->base);
		return NULL;
	}

	pssense->led_model.leds =
	    U_TYPED_ARRAY_CALLOC(struct t_constellation_tracker_led, pssense->led_model.led_count);
	if (pssense->led_model.leds == NULL) {
		PSSENSE_ERROR(pssense, "Failed to allocate memory for LED model.");
		pssense_device_destroy(&pssense->base);
		return NULL;
	}
	memcpy(pssense->led_model.leds, led_model,
	       sizeof(struct t_constellation_tracker_led) * pssense->led_model.led_count);

	struct xrt_pose T_imu_led;
	math_pose_invert(&pssense->tracking.T_led_imu, &T_imu_led);
	for (size_t i = 0; i < pssense->led_model.led_count; i++) {
		struct xrt_vec3 led_position_in_led_space = pssense->led_model.leds[i].position;
		struct xrt_vec3 led_normal_in_led_space = pssense->led_model.leds[i].normal;

		struct xrt_vec3 led_position_in_imu_space;
		struct xrt_vec3 led_normal_in_imu_space;

		math_pose_transform_point(&T_imu_led, &led_position_in_led_space, &led_position_in_imu_space);
		math_quat_rotate_vec3(&T_imu_led.orientation, &led_normal_in_led_space, &led_normal_in_imu_space);

		// Update the position/normal so that it's IMU-relative
		pssense->led_model.leds[i].position = led_position_in_imu_space;
		pssense->led_model.leds[i].normal = led_normal_in_imu_space;
	}

	SET_INPUT(PS_CLICK);
	SET_INPUT(SHARE_CLICK);
	SET_INPUT(OPTIONS_CLICK);
	SET_INPUT(SQUARE_CLICK);
	SET_INPUT(SQUARE_TOUCH);
	SET_INPUT(TRIANGLE_CLICK);
	SET_INPUT(TRIANGLE_TOUCH);
	SET_INPUT(CROSS_CLICK);
	SET_INPUT(CROSS_TOUCH);
	SET_INPUT(CIRCLE_CLICK);
	SET_INPUT(CIRCLE_TOUCH);
	SET_INPUT(SQUEEZE_CLICK);
	SET_INPUT(SQUEEZE_TOUCH);
	SET_INPUT(SQUEEZE_PROXIMITY);
	SET_INPUT(SQUEEZE_PROXIMITY_FLOAT);
	SET_INPUT(TRIGGER_CLICK);
	SET_INPUT(TRIGGER_TOUCH);
	SET_INPUT(TRIGGER_VALUE);
	SET_INPUT(TRIGGER_PROXIMITY);
	SET_INPUT(TRIGGER_PROXIMITY_FLOAT);
	SET_INPUT(THUMBSTICK);
	SET_INPUT(THUMBSTICK_CLICK);
	SET_INPUT(THUMBSTICK_TOUCH);
	SET_INPUT(GRIP_POSE);
	SET_INPUT(AIM_POSE);

	pssense->base.outputs[0].name = XRT_OUTPUT_NAME_PSSENSE_VIBRATION;
	pssense->base.outputs[1].name = XRT_OUTPUT_NAME_PSSENSE_TRIGGER_FEEDBACK;

	os_precise_sleeper_init(&pssense->sleeper);

	pssense->output.pcm_haptics_resampler = u_resampler_create(4000, PCM_SAMPLE_RATE);
	if (pssense->output.pcm_haptics_resampler == NULL) {
		PSSENSE_ERROR(pssense, "Failed to create PCM resampler");
		pssense_device_destroy(&pssense->base);
		return NULL;
	}

	// @note We don't do blink duration refinement right now because that needs to eventually adjust the latency
	//       offset as it goes and produces a worse result with the current implementation.
	struct t_led_sync_refinement_options led_sync_refinement_options = {
	    // @todo Once LED blink refinement is fixed, enable that again
	    // @todo Once optical clock sync is implemented in full, enable that here
	    .flags = T_LED_SYNC_REFINEMENT_FLAGS_HAS_LATENCY_CAP,
	    .initial_blink_duration_ns = PERIOD_ID_TO_DURATION_NS(30),
	    .min_blink_duration_ns = PERIOD_ID_TO_DURATION_NS(1),
	    .max_blink_duration_ns = PERIOD_ID_TO_DURATION_NS(MAX_PERIOD_ID),
	    .time_to_resync_ns = T_LED_SYNC_DEFAULT_RESYNC_TIME,
	    .settle_frames = 7,
	    // 8ms latency cap, something a bit overboard for bluetooth but better than searching the whole range
	    .latency_cap_ns = U_TIME_1MS_IN_NS * 8LL,
	};
	ret = t_led_sync_refinement_init(&pssense->tracking.led_sync_refinement, &led_sync_refinement_options);
	if (ret != 0) {
		PSSENSE_ERROR(pssense, "Failed to init LED sync refinement!");
		pssense_device_destroy(&pssense->base);
		return NULL;
	}

	pssense->tracking.period_id = DURATION_NS_TO_PERIOD_ID(led_sync_refinement_options.initial_blink_duration_ns);

	ret = os_thread_helper_init(&pssense->controller_thread);
	if (ret != 0) {
		PSSENSE_ERROR(pssense, "Failed to init threading!");
		pssense_device_destroy(&pssense->base);
		return NULL;
	}

	ret = os_thread_helper_start(&pssense->controller_thread, pssense_run_thread, pssense);
	if (ret != 0) {
		PSSENSE_ERROR(pssense, "Failed to start thread!");
		pssense_device_destroy(&pssense->base);
		return NULL;
	}

	// Try to set the PC polling rate if the user requested it.
	if (debug_get_bool_option_pssense_pc_polling_rate() && //
	    !pssense_set_pc_polling_rate(pssense)) {
		PSSENSE_ERROR(pssense, "PC polling rate requested, but got error when attempting to apply.");
	}

	if (!pssense_get_calibration_data(pssense)) {
		PSSENSE_ERROR(pssense, "Failed to retrieve calibration data");
		pssense_device_destroy(&pssense->base);
		return NULL;
	}

	u_var_add_root(pssense, pssense->base.str, false);
	u_var_add_log_level(pssense, &pssense->log_level, "Log level");

	u_var_add_gui_header(pssense, &pssense->gui.button_states, "Button States");
	u_var_add_bool(pssense, &pssense->state.ps_click, "PS Click");
	if (pssense->hand == XRT_HAND_LEFT) {
		u_var_add_bool(pssense, &pssense->state.share_click, "Share Click");
		u_var_add_bool(pssense, &pssense->state.square_click, "Square Click");
		u_var_add_bool(pssense, &pssense->state.square_touch, "Square Touch");
		u_var_add_bool(pssense, &pssense->state.triangle_click, "Triangle Click");
		u_var_add_bool(pssense, &pssense->state.triangle_touch, "Triangle Touch");
	} else if (pssense->hand == XRT_HAND_RIGHT) {
		u_var_add_bool(pssense, &pssense->state.options_click, "Options Click");
		u_var_add_bool(pssense, &pssense->state.cross_click, "Cross Click");
		u_var_add_bool(pssense, &pssense->state.cross_touch, "Cross Touch");
		u_var_add_bool(pssense, &pssense->state.circle_click, "Circle Click");
		u_var_add_bool(pssense, &pssense->state.circle_touch, "Circle Touch");
	}
	u_var_add_bool(pssense, &pssense->state.squeeze_click, "Squeeze Click");
	u_var_add_bool(pssense, &pssense->state.squeeze_touch, "Squeeze Touch");
	u_var_add_ro_f32(pssense, &pssense->state.squeeze_proximity, "Squeeze Proximity");
	u_var_add_bool(pssense, &pssense->state.trigger_click, "Trigger Click");
	u_var_add_bool(pssense, &pssense->state.trigger_touch, "Trigger Touch");
	u_var_add_ro_f32(pssense, &pssense->state.trigger_value, "Trigger");
	u_var_add_ro_f32(pssense, &pssense->state.trigger_proximity, "Trigger Proximity");
	u_var_add_ro_f32(pssense, &pssense->state.thumbstick.x, "Thumbstick X");
	u_var_add_ro_f32(pssense, &pssense->state.thumbstick.y, "Thumbstick Y");
	u_var_add_bool(pssense, &pssense->state.thumbstick_click, "Thumbstick Click");
	u_var_add_bool(pssense, &pssense->state.thumbstick_touch, "Thumbstick Touch");

	u_var_add_gui_header(pssense, &pssense->gui.timing, "Timing");
	u_var_add_ro_i64_ns(pssense, &pssense->timing.latest_imu_time_ns, "Latest IMU Time (ns)");
	u_var_add_ro_u64(pssense, &pssense->timing.imu_ticks_total, "Latest IMU Time (ticks)");
	u_var_add_ro_i64_ns(pssense, &pssense->timing.latest_device_time_ns, "Latest Device Time (ns)");
	u_var_add_ro_u64(pssense, &pssense->timing.device_ticks_total, "Latest Device Time (ticks)");

	u_var_add_gui_header(pssense, &pssense->gui.tracking, "Tracking");
	u_var_add_ro_vec3_i32(pssense, &pssense->state.gyro_raw, "Raw Gyro");
	u_var_add_ro_vec3_i32(pssense, &pssense->state.accel_raw, "Raw Accel");
	u_var_add_bool(pssense, &pssense->has_calibration, "Has Calibration");
	u_var_add_ro_vec3_i32(pssense, &pssense->calibration.gyro_bias, "Gyro Bias");
	u_var_add_ro_vec3_f32(pssense, &pssense->calibration.gyro_scale, "Gyro Scale");
	u_var_add_ro_vec3_f32(pssense, &pssense->calibration.accel_bias, "Accel Bias");
	u_var_add_ro_vec3_f32(pssense, &pssense->calibration.accel_scale, "Accel Scale");
	u_var_add_pose(pssense, &pssense->tracking.pose, "Pose");
	m_imu_3dof_add_vars(&pssense->tracking.fusion, pssense, "3dof Fusion");
	u_var_add_ro_u32(pssense, &pssense->tracking.received_frames, "Received Frames");
	u_var_add_ro_u32(pssense, &pssense->tracking.last_exposure_sequence_id, "Last Exposure Sequence ID");
	u_var_add_ro_i64_ns(pssense, &pssense->tracking.last_exposure_local_timestamp_ns,
	                    "Last Exposure Timestamp (ns)");
	u_var_add_ro_i64_ns(pssense, &pssense->tracking.average_exposure_interval_ns, "Average Exposure Interval (ns)");
	u_var_add_bool(pssense, &pssense->tracking.increment_sequence_num, "Increment LED Sequence Number");
	u_var_add_u8(pssense, &pssense->tracking.led_sequence_num, "LED Sequence Number");
	u_var_add_u8(pssense, &pssense->tracking.period_id, "LED Blink Period ID");
	u_var_add_i32(pssense, &pssense->tracking.timing_fudge_100us, "Timing Fudge (100us)");
	u_var_add_bool(pssense, &pssense->tracking.use_constellation, "Use Constellation Tracking");

	xrt_frame_context_add(xfctx, &pssense->node);

	if (out_timing_sink != NULL) {
		*out_timing_sink = &pssense->timing_event_sink;
	}

	return &pssense->base;
}

int
pssense_add_to_constellation_tracker(struct xrt_device *xdev, struct t_constellation_tracker *tracker)
{
	struct pssense_device *pssense = from_device(xdev);

	struct t_constellation_tracker_device_params params = {
	    .led_model = pssense->led_model,
	    .tracking_source = &pssense->constellation_tracking_source,
	};
	int ret = t_constellation_tracker_add_device(tracker, &params, &pssense->constellation_device,
	                                             &pssense->tracking.constellation_device_id);
	if (ret < 0) {
		PSSENSE_ERROR(pssense, "Failed to add device to constellation tracker: %d", ret);
		return -1;
	} else {
		pssense->tracking.use_constellation = true;
	}

	pssense->tracking.constellation_imu_sink = params.imu_sink;
	pssense->tracking.constellation_tracker = tracker;

	pssense->base.tracking_origin = t_constellation_tracker_get_tracking_origin(tracker);

	return 0;
}

/*!
 * @}
 */
