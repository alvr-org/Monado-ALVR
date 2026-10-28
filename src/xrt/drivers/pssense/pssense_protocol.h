// Copyright 2023, Collabora, Ltd.
// Copyright 2023, Jarett Millard
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  PlayStation Sense controller prober and driver code.
 * @author Jarett Millard <jarett.millard@gmail.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_pssense
 */

#pragma once

#include "xrt/xrt_byte_order.h"

#include "math/m_api.h"


#pragma pack(push, 1)

// @todo Remove when clang-format is updated in CI
// clang-format off
// IMU ticks are in thirds of a microsecond, so 1 tick = 333.333... nanoseconds
#define NS_TO_IMU_TICKS(ns) (((uint64_t)(ns) * 3) / 1000)
#define IMU_TICKS_TO_NS(ticks) (((uint64_t)(ticks) * 1000) / 3)

#define PERIOD_ID_TO_DURATION_NS(period_id) (time_duration_ns)(IMU_TICKS_TO_NS(150LLU * (uint64_t)period_id))
// @note: the +1 is to ensure things round correctly when dealing with converting to and back
#define DURATION_NS_TO_PERIOD_ID(duration_ns) (uint8_t)(((NS_TO_IMU_TICKS(duration_ns + 1)) / 150LLU) & 0xFF)
// clang-format on

#define STABLE_MIN_PERIOD_ID 20

#define MIN_PERIOD_ID 1
#define MAX_PERIOD_ID 42

#define PCM_SAMPLE_RATE 3000
#define PCM_HAPTIC_BUF_SIZE 32

#define INPUT_REPORT_ID_USB 0x01
#define INPUT_REPORT_ID_BLUETOOTH 0x31

#define OUTPUT_REPORT_ID_BLUETOOTH 0x31
#define OUTPUT_REPORT_TAG 0x10

#define CALIBRATION_DATA_FEATURE_REPORT_ID 0x05
#define SET_POLLING_RATE_FEATURE_REPORT_ID 0x08

#define CALIBRATION_DATA_PART_ID_1 0
#define CALIBRATION_DATA_PART_ID_2 0x81

const uint8_t INPUT_REPORT_CRC32_SEED = 0xa1;
const uint8_t OUTPUT_REPORT_CRC32_SEED = 0xa2;
const uint8_t FEATURE_REPORT_CRC32_SEED = 0xa3;
const uint8_t SET_FEATURE_REPORT_CRC32_SEED = 0x53;

const uint8_t CHARGE_STATE_DISCHARGING = 0x00;
const uint8_t CHARGE_STATE_CHARGING = 0x01;
const uint8_t CHARGE_STATE_FULL = 0x02;
const uint8_t CHARGE_STATE_ABNORMAL_VOLTAGE = 0x0A;
const uint8_t CHARGE_STATE_ABNORMAL_TEMP = 0x0B;
const uint8_t CHARGE_STATE_CHARGING_ERROR = 0x0F;

#define INPUT_REPORT_BLUETOOTH_LENGTH 78
#define INPUT_REPORT_USB_LENGTH 64

struct pssense_input_report_common
{
	uint8_t thumbstick_x;
	uint8_t thumbstick_y;
	uint8_t trigger_value;
	uint8_t trigger_proximity;
	uint8_t squeeze_proximity;
	uint8_t unknown1[2]; // Always 0x0001
	uint8_t buttons[3];
	uint8_t unknown2; // Always 0x00
	__le32 seq_no;
	__le16 gyro[3];
	__le16 accel[3];
	__le32 imu_ticks;
	uint8_t temperature;
	uint8_t unknown3[7];
	uint8_t trigger_feedback_state;
	uint8_t trigger_feedback_mode;
	uint8_t battery_state; // Low nibble charge level 0x00-0x0a, high nibble battery state
	uint8_t plug_state;    // Flags for USB data and/or power connected
	__le32 host_timestamp;
	__le32 device_timestamp_ticks;
	uint8_t unknown4[4];
	uint8_t aes_cmac[8];
};

struct pssense_usb_input_report
{
	uint8_t report_id;
	struct pssense_input_report_common common;
};
static_assert(sizeof(struct pssense_usb_input_report) == INPUT_REPORT_USB_LENGTH,
              "Incorrect input report struct length");

/*!
 * HID input report data packet.
 */
struct pssense_bluetooth_input_report
{
	uint8_t report_id;
	uint8_t bt_header;
	struct pssense_input_report_common common;
	uint8_t unknown5;
	uint8_t crc_failure_count;
	uint8_t padding[7];
	__le32 crc;
};
static_assert(sizeof(struct pssense_bluetooth_input_report) == INPUT_REPORT_BLUETOOTH_LENGTH,
              "Incorrect input report struct length");

enum pssense_output_settings_flag1
{
	PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK0 = 1 << 0,
	PSSENSE_OUTPUT_SETTINGS_FLAG1_RUMBLE_EMULATION = 1 << 1,
	PSSENSE_OUTPUT_SETTINGS_FLAG1_ADAPTIVE_TRIGGER_ENABLE = 1 << 2,
	PSSENSE_OUTPUT_SETTINGS_FLAG1_INTENSITY_INCREASE_SET_ENABLE = 1 << 3,
	PSSENSE_OUTPUT_SETTINGS_FLAG1_INTENSITY_REDUCTION_SET_ENABLE = 1 << 4,
	PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK5 = 1 << 5,
	PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK6 = 1 << 6,
	PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK7 = 1 << 7,
};

#define OUTPUT_SETTINGS_ENABLE_VIBRATION_BITS                                                                          \
	(PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK0 | PSSENSE_OUTPUT_SETTINGS_FLAG1_RUMBLE_EMULATION)

#define OUTPUT_SETTINGS_VIBRATE_MODE_HIGH_120HZ 0x00
// 0x20
#define OUTPUT_SETTINGS_VIBRATE_MODE_LOW_60HZ (PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK5)
// 0x40
#define OUTPUT_SETTINGS_VIBRATE_MODE_CLASSIC_RUMBLE (PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK6)
// 0x60
#define OUTPUT_SETTINGS_VIBRATE_MODE_DIET_RUMBLE                                                                       \
	(PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK5 | PSSENSE_OUTPUT_SETTINGS_FLAG1_UNK6)

enum pssense_output_settings_flag2
{
	PSSENSE_OUTPUT_SETTINGS_FLAG2_UNK0 = 1 << 0,
	PSSENSE_OUTPUT_SETTINGS_FLAG2_UNK1 = 1 << 1,
	//! Used to mark whether to read the status LED enable bool or not.
	PSSENSE_OUTPUT_SETTINGS_FLAG2_STATUS_LED_SET_ENABLE = 1 << 2,
	PSSENSE_OUTPUT_SETTINGS_FLAG2_UNK3 = 1 << 3,
	PSSENSE_OUTPUT_SETTINGS_FLAG2_UNK4 = 1 << 4,
	PSSENSE_OUTPUT_SETTINGS_FLAG2_UNK5 = 1 << 5,
	PSSENSE_OUTPUT_SETTINGS_FLAG2_UNK6 = 1 << 6,
	PSSENSE_OUTPUT_SETTINGS_FLAG2_UNK7 = 1 << 7,
};

enum pssense_adaptive_trigger_mode
{
	TRIGGER_FEEDBACK_MODE_OFF = 0x05,

	// simple versions of other effects, shouldn't be used
	TRIGGER_FEEDBACK_MODE_SIMPLE_FEEDBACK = 0x01,
	TRIGGER_FEEDBACK_MODE_SIMPLE_WEAPON = 0x02,
	TRIGGER_FEEDBACK_MODE_SIMPLE_VIBRATION = 0x06,

	// limited versions of official
	TRIGGER_FEEDBACK_MODE_LIMITED_FEEDBACK = 0x11,
	TRIGGER_FEEDBACK_MODE_LIMITED_WEAPON = 0x12,

	// official ones found through reverse engineering
	TRIGGER_FEEDBACK_MODE_FEEDBACK = 0x21,
	TRIGGER_FEEDBACK_MODE_SLOPE_FEEDBACK = 0x22, // aka Bow
	TRIGGER_FEEDBACK_MODE_WEAPON = 0x25,
	TRIGGER_FEEDBACK_MODE_VIBRATION = 0x26,

	// unofficial ones found through fuzzing firmware
	TRIGGER_FEEDBACK_MODE_GALLOPING = 0x23,
	TRIGGER_FEEDBACK_MODE_MACHINE = 0x27,
};

struct pssense_output_adaptive_trigger_settings
{
	uint8_t mode; // See enum pssense_adaptive_trigger_mode
	union {
		uint8_t raw_parameters[10];
	};
};

enum pssense_led_sync_phase
{
	// initializing
	LED_SYNC_PHASE_INIT = 0,
	// ???
	LED_SYNC_PHASE_PRESCAN = 1,
	// ???
	LED_SYNC_PHASE_BROAD = 2,
	// ???
	LED_SYNC_PHASE_BG = 3,
	// ???
	LED_SYNC_PHASE_STABLE = 4,
	// used during passthrough to prevent LEDs from bleeding into it
	LED_SYNC_PHASE_LED_ALL_OFF = 5,
	// probably 100% duty cycle?
	LED_SYNC_PHASE_LED_ALL_ON = 6,
	// probably also 100% duty cycle?
	LED_SYNC_PHASE_DEBUG = 7,
};

struct pssense_led_settings
{
	uint8_t phase;
	uint8_t sequence_number;
	uint8_t period_id;
	//! The position, in IMU ticks, @ref NS_PER_IMU_TICK
	__le32 cycle_position;
	//! The length, in thirds of a nanosecond.
	__le32 cycle_length;
	uint8_t led_blink[4];
};

struct pssense_output_settings
{
	//! See enum pssense_output_settings_flag1
	uint8_t flag1;
	//! See enum pssense_output_settings_flag2
	uint8_t flag2;
	//! Vibration amplitude from 0x00-0xff. Sending 0 turns vibration off.
	uint8_t vibration_amplitude;
	//! Sony driver sometimes sets to 0x82.
	uint8_t unk0;
	//! Settings for the adaptive trigger
	struct pssense_output_adaptive_trigger_settings trigger_settings;
	//! Time of packet send, in host time, in microseconds
	__le32 host_timestamp_send_time_us;
	//! Settings of the tracking LEDs.
	struct pssense_led_settings led_settings;
	//! Lower 4 bits for haptics reduction, upper 4 bits for trigger reduction. Decreases in 12.5% increments.
	uint8_t trigger_haptics_reduction;
	//! Whether to enable the status LED or not.
	uint8_t status_led_enable;
	uint8_t unk1[2];
};
static_assert(sizeof(struct pssense_output_settings) == 38, "Incorrect output settings struct length");

#define OUTPUT_REPORT_LENGTH_PS5 78
#define OUTPUT_REPORT_LENGTH_USB 39

/**
 * HID output report data packet matching the PS5 layout, with PCM haptics.
 *
 * Reference:
 * https://github.com/BnuuySolutions/PSVR2Toolkit/blob/b6ffe9f03cb2456d9d07aea74e839d9c5fd188f5/projects/psvr2_openvr_driver_ex/sense_controller.h#L44
 */
struct pssense_ps5_output_report
{
	uint8_t report_id;
	uint8_t seq_no_mode; // High bits only; low bits are always 0
	uint8_t tag;         // Needs to be 0x10 for this report
	struct pssense_output_settings settings;
	uint8_t counter;
	uint8_t haptics[PCM_HAPTIC_BUF_SIZE];
	__le32 crc;
};
static_assert(sizeof(struct pssense_ps5_output_report) == OUTPUT_REPORT_LENGTH_PS5,
              "Incorrect output report struct length");

struct pssense_usb_output_report
{
	// @note: There appears to be no dedicated report ID field here. Setting this report ID to
	//        any non-zero number seems to work, though. Zero doesn't work.
	//        I believe this controller just expects strange report IDs over USB.
	//        If USB breaks when using hidapi, this is the first place to check.
	uint8_t seq_no_mode;
	struct pssense_output_settings settings;
};
static_assert(sizeof(struct pssense_usb_output_report) == OUTPUT_REPORT_LENGTH_USB,
              "Incorrect output report struct length");

#define FEATURE_REPORT_LENGTH 64
#define CALIBRATION_DATA_LENGTH 116

/**
 * HID output report data packet.
 */
struct pssense_feature_report
{
	uint8_t report_id;
	uint8_t part_id;
	uint8_t data[CALIBRATION_DATA_LENGTH / 2];
	__le32 crc;
};
static_assert(sizeof(struct pssense_feature_report) == FEATURE_REPORT_LENGTH, "Incorrect feature report struct length");

/*
 * Playstation Sense controller calibration.
 */
struct pssense_calibration_data
{
	int16_t accel_plus_x;    // 0x00
	uint8_t _pad0[4];        // 0x02-0x05
	int16_t accel_minus_x;   // 0x06
	uint8_t _pad1[6];        // 0x08-0x0D
	int16_t accel_plus_y;    // 0x0E
	uint8_t _pad2[4];        // 0x10-0x13
	int16_t accel_minus_y;   // 0x14
	uint8_t _pad3[6];        // 0x16-0x1B
	int16_t accel_plus_z;    // 0x1C
	uint8_t _pad4[4];        // 0x1E-0x21
	int16_t accel_minus_z;   // 0x22
	int16_t gyro_bias_x;     // 0x24
	int16_t gyro_bias_y;     // 0x26
	int16_t gyro_bias_z;     // 0x28
	int16_t gyro_plus_x;     // 0x2A
	uint8_t _pad6[4];        // 0x2C-0x2F
	int16_t gyro_minus_x;    // 0x30
	uint8_t _pad7[6];        // 0x32-0x37
	int16_t gyro_plus_y;     // 0x38
	uint8_t _pad8[4];        // 0x3A-0x3D
	int16_t gyro_minus_y;    // 0x3E
	uint8_t _pad9[6];        // 0x40-0x45 (unused by the official driver)
	int16_t gyro_plus_z;     // 0x46
	uint8_t _pad10[4];       // 0x48-0x4B
	int16_t gyro_minus_z;    // 0x4C
	int16_t gyro_speed_ref1; // 0x4E
	int16_t gyro_speed_ref2; // 0x50
	uint8_t _pad_end[34];    // 0x52-0x73
};
static_assert(sizeof(struct pssense_calibration_data) == CALIBRATION_DATA_LENGTH,
              "Incorrect calibration data struct length");

#define SET_POLLING_RATE_FEATURE_REPORT_LENGTH 48
struct pssense_set_polling_rate_feature_report
{
	uint8_t report_id;
	uint8_t unk;
	__le16 rate_1;
	__le16 rate_2;
	uint8_t padding[38];
	__le32 crc;
};
static_assert(sizeof(struct pssense_set_polling_rate_feature_report) == SET_POLLING_RATE_FEATURE_REPORT_LENGTH,
              "Incorrect feature report struct length");

#pragma pack(pop)
