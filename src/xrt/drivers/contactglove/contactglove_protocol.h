// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of ContactGlove device driver.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_contactglove
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_byte_order.h"


#ifdef __cplusplus
extern "C" {
#endif

#define PING_INTERVAL_NS (1000LL * U_TIME_1MS_IN_NS) // 1000 ms
#define PING_TIMEOUT_NS (800LL * U_TIME_1MS_IN_NS)   // 800 ms

#define CONTACTGLOVE2_SENSOR_COUNT 16

enum contactglove_to_device_packet_mode
{
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_REQUEST_PAIRING = 2,
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_DEBUG = 6,
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_REQUEST_VERSION = 7,
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_PING = 8,
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_SWITCH_CHANNEL = 13,
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_BODY_HAPTICS_LEFT = 15,
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_BODY_HAPTICS_RIGHT = 16,
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_SET_MODULE_STATE = 22,
	CONTACTGLOVE_TO_DEVICE_PACKET_MODE_REQUEST_MODULE_STATE = 24,
};

enum contactglove_to_host_packet_mode
{
	CONTACTGLOVE_TO_HOST_PACKET_MODE_SENSOR_VALUE_LEFT = 4,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_SENSOR_VALUE_RIGHT = 5,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_DEBUG = 6,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_DEVICE_VERSION = 7,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_LEFT_PING = 8,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_RIGHT_PING = 9,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_GYRO_DATA_LEFT = 10,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_GYRO_DATA_RIGHT = 11,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_MODULE_STATE = 14,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_MAGNETRA2_INPUT_LEFT = 21,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_MAGNETRA2_INPUT_RIGHT = 22,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_GLOVE_CONNECTION = 30,
	CONTACTGLOVE_TO_HOST_PACKET_MODE_ERROR_LOG = 100,
};

#pragma pack(push, 1)

#define SIZE_ASSERT(type, size)                                                                                        \
	static_assert(sizeof(type) == (size), "Size of " #type " is not " #size " bytes as was expected")

struct contactglove_version
{
	uint8_t major;
	uint8_t minor;
};

struct contactglove_to_host_packet_device_version
{
	struct contactglove_version dongle;
	struct contactglove_version left;
	struct contactglove_version right;
};
SIZE_ASSERT(struct contactglove_to_host_packet_device_version, 6);

struct contactglove_raw_battery_info
{
	uint8_t battery_raw;
	uint8_t charge_status;
};

struct contactglove_to_host_packet_glove_connection
{
	struct contactglove_raw_battery_info battery_left;
	struct contactglove_raw_battery_info battery_right;
	uint8_t channel;
};
SIZE_ASSERT(struct contactglove_to_host_packet_glove_connection, 5);

struct contactglove_to_host_packet_ping
{
	uint8_t nonce;
	__le16 glove_to_dongle_ms;
};
SIZE_ASSERT(struct contactglove_to_host_packet_ping, 3);

struct contactglove_to_host_packet_hand_gyro_data
{
	__le16 quat_raw[4];
};
SIZE_ASSERT(struct contactglove_to_host_packet_hand_gyro_data, 8);

enum contactglove_sensor_position
{
	CONTACTGLOVE_SENSOR_POSITION_FINGER_PINKY_ROOT1,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_PINKY_TIP,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_PINKY_ROOT2,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_RING_ROOT1,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_RING_TIP,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_RING_ROOT2,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_MIDDLE_ROOT1,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_MIDDLE_TIP,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_MIDDLE_ROOT2,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_INDEX_ROOT1,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_INDEX_TIP,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_INDEX_ROOT2,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_THUMB_ROOT1,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_THUMB_TIP,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_THUMB_ROOT2,
	CONTACTGLOVE_SENSOR_POSITION_FINGER_THUMB_BASE,
};

struct contactglove_to_host_packet_sensor_data
{
	uint8_t mode;
	uint8_t valid;
	__le16 sensor_values[CONTACTGLOVE2_SENSOR_COUNT];
};
SIZE_ASSERT(struct contactglove_to_host_packet_sensor_data, 1 + 1 + (2 * CONTACTGLOVE2_SENSOR_COUNT));

enum contactglove_magnetra2_button_bits
{
	MAGNETRA2_BUTTON_BITS_A = (1 << 0),
	MAGNETRA2_BUTTON_BITS_B = (1 << 1),
	MAGNETRA2_BUTTON_BITS_TRIGGER_CLICK = (1 << 2),
	MAGNETRA2_BUTTON_BITS_TRACKPAD_BOTTOM = (1 << 3),
	MAGNETRA2_BUTTON_BITS_PAIRING = (1 << 4),
};

enum contactglove_magnetra2_multi_ch_ranges
{
	MAGNETRA2_MULTI_CH_RANGE_JOYSTICK_START = 0,
	MAGNETRA2_MULTI_CH_RANGE_JOYSTICK_END = 50,
	MAGNETRA2_MULTI_CH_RANGE_TRACKPAD_TOP_START = 51,
	MAGNETRA2_MULTI_CH_RANGE_TRACKPAD_TOP_END = 150,
	MAGNETRA2_MULTI_CH_RANGE_SYSTEM_START = 151,
	MAGNETRA2_MULTI_CH_RANGE_SYSTEM_END = 210,
	MAGNETRA2_MULTI_CH_RANGE_NONE_START = 211,
	MAGNETRA2_MULTI_CH_RANGE_NONE_END = 255,
};

struct contactglove_to_host_packet_magnetra2_input
{
	uint8_t button_bits;
	uint8_t joystick_x;
	uint8_t joystick_y;
	uint8_t trigger;
	uint8_t multi_ch_value;
};
SIZE_ASSERT(struct contactglove_to_host_packet_magnetra2_input, 5);

enum contactglove_module_kind
{
	CONTACTGLOVE_MODULE_MAGNETRA2 = 1,
	CONTACTGLOVE_MODULE_LED_MANAGER = 4,
	CONTACTGLOVE_MODULE_SLEEP_MANAGER = 11,
	CONTACTGLOVE_MODULE_MAX,
};

enum contactglove_device_role
{
	CONTACTGLOVE_DEVICE_ROLE_LEFT = 1,
	CONTACTGLOVE_DEVICE_ROLE_RIGHT = 2,
};

struct contactglove_to_host_packet_module_state_header
{
	uint8_t device_role; // Maps to enum contactglove_device_role
	uint8_t module_kind; // Maps to enum contactglove_module_kind
	uint8_t data_length;
};
SIZE_ASSERT(struct contactglove_to_host_packet_module_state_header, 3);

struct contactglove_module_state_magnetra2
{
	uint8_t enabled;
};
SIZE_ASSERT(struct contactglove_module_state_magnetra2, 1);

struct contactglove_module_state_sleep_manager
{
	//! How long until the glove goes into sleep mode, 255=disabled
	uint8_t duration_minutes;
	//! Unused
	uint8_t duration_sec;
};
SIZE_ASSERT(struct contactglove_module_state_sleep_manager, 2);

struct contactglove_module_state_led_manager
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};
SIZE_ASSERT(struct contactglove_module_state_led_manager, 3);

enum contactglove_debug_message_kind
{
	CONTACTGLOVE_DEBUG_MESSAGE_TEMPORARY_LOG = 1,
	CONTACTGLOVE_DEBUG_MESSAGE_SIZE_UNMATCHED = 2,
	CONTACTGLOVE_DEBUG_MESSAGE_WAITING_FOR_PAIRING = 3,
	CONTACTGLOVE_DEBUG_MESSAGE_UNKNOWN_MODE = 5,
	CONTACTGLOVE_DEBUG_MESSAGE_UNKNOWN_MAC_ADDRESS = 6,
	CONTACTGLOVE_DEBUG_MESSAGE_UNKNOWN_DEVICE = 9,
	CONTACTGLOVE_DEBUG_MESSAGE_SEND_FAILED = 10,
	CONTACTGLOVE_DEBUG_MESSAGE_CRC = 11,
};

struct contactglove_to_host_packet_debug
{
	uint8_t message_kind; // see enum contactglove_debug_message_kind
	uint8_t message_value;
};
SIZE_ASSERT(struct contactglove_to_host_packet_debug, 2);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
