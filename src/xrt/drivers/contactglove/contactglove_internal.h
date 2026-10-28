// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of ContactGlove device driver.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_contactglove
 */

#pragma once

#include "xrt/xrt_device.h"
#include "xrt/xrt_defines.h"

#include "os/os_threading.h"

#include "util/u_cobs.h"
#include "util/u_var.h"
#include "util/u_logging.h"
#include "util/u_hand_tracking.h"

#include "math/m_relation_history.h"

#include "contactglove_interface.h"
#include "contactglove_protocol.h"
#include "contactglove_calibration.h"


/*!
 * A dongle for ContactGlove gloves.
 *
 * @implements xrt_reference
 */
struct contactglove_dongle
{
	struct xrt_reference base;

	enum u_logging_level log_level;

	struct os_mutex data_lock;
	struct os_thread_helper thread;

	struct os_serial_device *dongle_serial;
	enum contactglove_type type;

	struct u_cobs_decoder cobs_decoder;

	uint8_t last_sent_ping_nonce;
	timepoint_ns last_ping_send;

	struct contactglove_version version;
	uint8_t channel;

	struct contactglove_device *left_glove;
	struct contactglove_device *right_glove;
};

enum contactglove_device_inputs
{
	CONTACTGLOVE_DEVICE_INPUT_HT_UNOBSTRUCTED = 0,
	CONTACTGLOVE_DEVICE_INPUT_A_CLICK,
	CONTACTGLOVE_DEVICE_INPUT_B_CLICK,
	CONTACTGLOVE_DEVICE_INPUT_X_CLICK,
	CONTACTGLOVE_DEVICE_INPUT_Y_CLICK,
	CONTACTGLOVE_DEVICE_INPUT_SYSTEM_CLICK,
	CONTACTGLOVE_DEVICE_INPUT_PAIRING_CLICK,
	CONTACTGLOVE_DEVICE_INPUT_TRIGGER_VALUE,
	CONTACTGLOVE_DEVICE_INPUT_TRIGGER_CLICK,
	CONTACTGLOVE_DEVICE_INPUT_SQUEEZE_VALUE,
	CONTACTGLOVE_DEVICE_INPUT_THUMBSTICK,
	CONTACTGLOVE_DEVICE_INPUT_THUMBSTICK_CLICK,
	CONTACTGLOVE_DEVICE_INPUT_GRIP_POSE,
	CONTACTGLOVE_DEVICE_INPUT_AIM_POSE,
	CONTACTGLOVE_DEVICE_INPUT_COUNT,
};

/*!
 * A single ContactGlove glove.
 *
 * @implements xrt_device
 */
struct contactglove_device
{
	struct xrt_device base;

	struct contactglove_dongle *dongle;

	struct contactglove_version version;
	struct contactglove_raw_battery_info raw_battery;

	uint8_t last_received_ping_nonce;
	timepoint_ns last_ping_receive;
	uint16_t glove_to_dongle_ms;

	//! Whether this glove is actively connected to the dongle
	bool connected;

	bool magnetra2_connected;

	timepoint_ns raw_input_update_time_ns;
	struct contactglove_to_host_packet_magnetra2_input raw_input;

	timepoint_ns avg_glove_to_dongle_offset_ns;

	bool module_state_valid[CONTACTGLOVE_MODULE_MAX];
	struct contactglove_module_state_magnetra2 module_state_magnetra2;
	struct contactglove_module_state_sleep_manager module_state_sleep_manager;
	struct contactglove_module_state_led_manager module_state_led_manager;

	struct m_relation_history *relation_history;
	struct xrt_quat latest_orientation;

	uint16_t raw_flex_adc_values[CONTACTGLOVE2_SENSOR_COUNT];
	struct u_var_u16_arr u_var_flex_adc_values;

	struct contactglove_calibration_wizard calibration_wizard;

	enum xrt_hand hand;
	enum contactglove_device_role role;
};
