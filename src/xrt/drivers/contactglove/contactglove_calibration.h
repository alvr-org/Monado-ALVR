// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  ContactGlove device calibration implementation.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_contactglove
 */

#pragma once

#include "math/m_api.h"
#include "math/m_vec2.h"

#include "util/u_logging.h"
#include "util/u_var.h"

#include "contactglove_protocol.h"

#include <float.h>


#ifdef __cplusplus
extern "C" {
#endif

// Forward declare
struct contactglove_device;


//! The default deadzone used on the ContactGlove2, straight from the spec document.
#define CONTACTGLOVE2_DEFAULT_DEADZONE 0.1f

/*
 *
 * File structures
 *
 */

//! The magic for the calibration file, written to disk in big endian so on-disk visibly matches source code.
#define CONTACTGLOVE_CALIBRATION_MAGIC 0x15D04C50

enum contactglove_calibration_version
{
	//! Initial structure is added.
	CONTACTGLOVE_CALIBRATION_VERSION_INITIAL = 0,
};

//! The current version of the calibration file, increment with breaking changes
#define CONTACTGLOVE_CALIBRATION_CURRENT_VERSION CONTACTGLOVE_CALIBRATION_VERSION_INITIAL

//! Represents the calibration of a single flex sensor
struct contactglove_sensor_calibration
{
	//! The minimum value, 0.0
	uint16_t min;
	//! The max value recorded, 1.0
	uint16_t max;
};

//! Represents the calibration of the joystick
struct contactglove_stick_calibration
{
	//! The X value when the stick is at rest
	uint8_t stick_center_x;
	//! The Y value when the stick is at rest
	uint8_t stick_center_y;

	//! The distance from center where 1.0 is defined.
	uint8_t range;

	//! The normalized calibrated forward vector, in uncalibrated space.
	struct xrt_vec2 forward_vector;

	//! Deadzone from 0-1
	float deadzone;
};

//! Represents the calibration of the trigger
struct contactglove_trigger_calibration
{
	//! The minimum value of the trigger, 0.0
	uint8_t min;
	//! The maximum value of the trigger, 1.0
	uint8_t max;
};

//! A full calibration
struct contactglove_calibration
{
	struct contactglove_sensor_calibration flex_sensors[CONTACTGLOVE2_SENSOR_COUNT];
	struct contactglove_stick_calibration joystick;
	struct contactglove_trigger_calibration trigger;
};

/*
 *
 * Helper functions
 *
 */

//! Applies a flex sensor calibration to a raw ADC value, clamped between 0 and 1.
static inline float
contactglove_apply_sensor_calibration(const struct contactglove_calibration calibration,
                                      int index,
                                      const uint16_t *adc_values)
{
	const struct contactglove_sensor_calibration sensor_calib = calibration.flex_sensors[index];

	return (CLAMP(adc_values[index], sensor_calib.min, sensor_calib.max) - sensor_calib.min) /
	       (float)(sensor_calib.max - sensor_calib.min);
}

//! Applies joystick calibration, returns an OpenXR joystick position
static inline struct xrt_vec2
contactglove_apply_stick_calibration(const struct contactglove_stick_calibration calibration,
                                     const uint8_t stick_x,
                                     const uint8_t stick_y)
{
	const int8_t from_center_x = (int8_t)(((int16_t)stick_x) - calibration.stick_center_x);
	const int8_t from_center_y = (int8_t)(((int16_t)stick_y) - calibration.stick_center_y);

	const struct xrt_vec2 from_center = {
	    .x = (float)from_center_x,
	    .y = (float)from_center_y,
	};

	const struct xrt_vec2 raw_dir =
	    m_vec2_clamp_scalar(m_vec2_div_scalar(from_center, (float)calibration.range), -1.0f, 1.0f);

	// Rotate raw_dir by the inverse of forward_vector so that if raw_dir == forward_vector we get (0.0, 1.0)
	const struct xrt_vec2 rotated = {
	    .x = m_vec2_cross(raw_dir, calibration.forward_vector),
	    .y = m_vec2_dot(raw_dir, calibration.forward_vector),
	};

	// Apply deadzone
	const float raw_len = m_vec2_len(rotated);
	const float deadzone = CLAMP(calibration.deadzone, 0.0f, 1.0f - FLT_EPSILON);
	if (raw_len <= deadzone) {
		return XRT_C11_COMPOUND(struct xrt_vec2) XRT_VEC2_ZERO;
	}

	// Remap distance deadzone -> 1 to 0 -> 1, clamping length to 1.0
	const float remapped_len = CLAMP((raw_len - deadzone) / (1.0f - deadzone), 0.0f, 1.0f);

	// Remap the vector's length, inverting X (seems necessary here)
	return m_vec2_mul(m_vec2_mul_scalar(rotated, remapped_len / raw_len), (struct xrt_vec2){.x = -1, .y = 1});
}

//! Applies a trigger calibration to the raw ADC value, following the algorithm laid out in the spec document.
static inline float
contactglove_apply_trigger_calibration(struct contactglove_trigger_calibration calibration, uint8_t adc_value)
{
	// SPEC: To avoid immediate saturation at the extremes, the host applies a small internal margin when converting
	//       raw values
	if (calibration.max > calibration.min) {
		calibration.min += 5;
		calibration.max -= 5;
	} else if (calibration.max <= calibration.min) {
		calibration.min -= 5;
		calibration.max += 5;
	}

	// The stored values are used to normalize trigger input:
	// v = (raw - min) / (max - min)

	float f = (CLAMP(adc_value, calibration.min, calibration.max) - calibration.min) /
	          (float)(calibration.max - calibration.min);

	// SPEC: The normalized value is then clamped to [0, 1] and passed through a log curve for response shaping
	// @note We don't apply the clamp 0-1 in the normalized range since we do it beforehand.

	// @note They don't specify which log curve but this one seems very reasonable and likely close enough.
	f = log10f(1.0f + (9.0f * f));

	return f;
}

/*
 *
 * Wizard
 *
 */

enum contactglove_calibration_wizard_step
{
	CONTACTGLOVE_CALIBRATION_WIZARD_NOT_CALIBRATING,
	CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_FLEX_SENSORS,
	CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_CENTER,
	CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_RANGE,
	CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_DEADZONE,
	CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_FORWARD,
	CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_TRIGGER,
	CONTACTGLOVE_CALIBRATION_WIZARD_WRITING_OUTPUT,
};

struct contactglove_calibration_wizard_flex_sensor_state
{
	struct contactglove_sensor_calibration sensors[CONTACTGLOVE2_SENSOR_COUNT];
};

struct contactglove_calibration_wizard_joystick_center_state
{
	uint8_t x;
	uint8_t y;
};

struct contactglove_calibration_wizard_joystick_range_state
{
	uint8_t up_range;
	uint8_t down_range;
	uint8_t left_range;
	uint8_t right_range;
};

struct contactglove_calibration_wizard_joystick_deadzone_state
{
	float deadzone;
};

/*!
 * A type which manages the state of the currently-running calibration step
 */
struct contactglove_calibration_wizard
{
	enum u_logging_level log_level;
	struct contactglove_device *glove;

	bool button_state;

	bool calibrating_magnetra2;

	enum contactglove_calibration_wizard_step step;
	union {
		struct contactglove_calibration_wizard_flex_sensor_state flex_sensors;
		struct contactglove_calibration_wizard_joystick_center_state joystick_center;
		struct contactglove_calibration_wizard_joystick_range_state joystick_range;
		struct contactglove_calibration_wizard_joystick_deadzone_state joystick_deadzone;
		struct xrt_vec2 joystick_forward;
		struct contactglove_trigger_calibration trigger;
	} state;
	struct contactglove_calibration working;

	//! The currently applied calibration
	struct contactglove_calibration calibration;

	struct u_var_button u_var_start_button;
};


void
contactglove_calibration_wizard_init(struct contactglove_calibration_wizard *wizard, struct contactglove_device *glove);

void
contactglove_calibration_wizard_deinit(struct contactglove_calibration_wizard *wizard);

void
contactglove_calibration_wizard_start(struct contactglove_calibration_wizard *wizard, bool calibrating_magnetra2);

void
contactglove_calibration_wizard_push_button(struct contactglove_calibration_wizard *wizard, bool button);

void
contactglove_calibration_wizard_push_flex_sensors(struct contactglove_calibration_wizard *wizard,
                                                  const uint16_t *adc_values);

void
contactglove_calibration_wizard_push_joystick(struct contactglove_calibration_wizard *wizard,
                                              uint8_t adc_x,
                                              uint8_t adc_y);

void
contactglove_calibration_wizard_push_trigger(struct contactglove_calibration_wizard *wizard, uint8_t adc);

#ifdef __cplusplus
}
#endif
