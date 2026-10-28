// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to parse and handle the Rift Touch configuration data.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift
 */

#include "util/u_json.hpp"
#include "util/u_logging.h"
#include "util/u_debug.h"

#include "math/m_api.h"

#include "rift_internal.h"

#include <string.h>
#include <math.h>


using xrt::auxiliary::util::json::JSONNode;


extern "C" bool
rift_touch_calibration_parse(const char *calibration_data,
                             size_t calibration_size,
                             struct rift_touch_controller_calibration *out_calibration)
{
	memset(out_calibration, 0, sizeof(*out_calibration));

	try {
		JSONNode root{std::string{calibration_data, calibration_size}};
		if (root.isInvalid()) {
			U_LOG_E("Failed to parse JSON config data.");
			return false;
		}

		auto tracked_object = root["TrackedObject"];

		if (tracked_object["JsonVersion"].asInt() != 2) {
			U_LOG_E("Unsupported JSON version %d", tracked_object["JsonVersion"].asInt());
			return false;
		}

		out_calibration->joy_x_range[0] = static_cast<uint16_t>(tracked_object["JoyXRangeMin"].asInt());
		out_calibration->joy_x_range[1] = static_cast<uint16_t>(tracked_object["JoyXRangeMax"].asInt());
		out_calibration->joy_x_dead[0] = static_cast<uint16_t>(tracked_object["JoyXDeadMin"].asInt());
		out_calibration->joy_x_dead[1] = static_cast<uint16_t>(tracked_object["JoyXDeadMax"].asInt());

		out_calibration->joy_y_range[0] = static_cast<uint16_t>(tracked_object["JoyYRangeMin"].asInt());
		out_calibration->joy_y_range[1] = static_cast<uint16_t>(tracked_object["JoyYRangeMax"].asInt());
		out_calibration->joy_y_dead[0] = static_cast<uint16_t>(tracked_object["JoyYDeadMin"].asInt());
		out_calibration->joy_y_dead[1] = static_cast<uint16_t>(tracked_object["JoyYDeadMax"].asInt());

		out_calibration->trigger_range[0] = static_cast<uint16_t>(tracked_object["TriggerMinRange"].asInt());
		out_calibration->trigger_range[1] = static_cast<uint16_t>(tracked_object["TriggerMidRange"].asInt());
		out_calibration->trigger_range[2] = static_cast<uint16_t>(tracked_object["TriggerMaxRange"].asInt());

		out_calibration->middle_range[0] = static_cast<uint16_t>(tracked_object["MiddleMinRange"].asInt());
		out_calibration->middle_range[1] = static_cast<uint16_t>(tracked_object["MiddleMidRange"].asInt());
		out_calibration->middle_range[2] = static_cast<uint16_t>(tracked_object["MiddleMaxRange"].asInt());
		out_calibration->middle_flipped = tracked_object["MiddleFlipped"].asBool();

		auto cap_sense_min = tracked_object["CapSenseMin"];
		auto cap_sense_touch = tracked_object["CapSenseTouch"];
		for (int i = 0; i < 8; i++) {
			out_calibration->cap_sense_min[i] = static_cast<uint16_t>(cap_sense_min[i].asInt());
			out_calibration->cap_sense_touch[i] = static_cast<uint16_t>(cap_sense_touch[i].asInt());
		}

		auto gyro_calibration_array = tracked_object["GyroCalibration"];
		for (int i = 0; i < 9; i++) {
			out_calibration->gyro_calibration[i / 3][i % 3] = gyro_calibration_array[i].asDouble();
		}
		out_calibration->gyro_offset.x = gyro_calibration_array[9 + 0].asDouble();
		out_calibration->gyro_offset.y = gyro_calibration_array[9 + 1].asDouble();
		out_calibration->gyro_offset.z = gyro_calibration_array[9 + 2].asDouble();

		auto accel_calibration_array = tracked_object["AccCalibration"];
		for (int i = 0; i < 9; i++) {
			out_calibration->accel_calibration[i / 3][i % 3] = accel_calibration_array[i].asDouble();
		}
		out_calibration->accel_offset.x = accel_calibration_array[9 + 0].asDouble();
		out_calibration->accel_offset.y = accel_calibration_array[9 + 1].asDouble();
		out_calibration->accel_offset.z = accel_calibration_array[9 + 2].asDouble();

		auto imu_position_array = tracked_object["ImuPosition"];
		out_calibration->imu_position.x = imu_position_array[0].asDouble();
		out_calibration->imu_position.y = imu_position_array[1].asDouble();
		out_calibration->imu_position.z = imu_position_array[2].asDouble();

		auto leds = tracked_object["ModelPoints"];
		out_calibration->num_leds = leds.asObject().size();
		out_calibration->leds =
		    U_TYPED_ARRAY_CALLOC(struct rift_touch_controller_led, out_calibration->num_leds);

		for (size_t i = 0; i < out_calibration->num_leds; i++) {
			auto led_object = leds["Point" + std::to_string(i)];

			auto &led = out_calibration->leds[i];

			led.position.x = led_object[0].asDouble();
			led.position.y = led_object[1].asDouble();
			led.position.z = led_object[2].asDouble();
			led.normal.x = led_object[3].asDouble();
			led.normal.y = led_object[4].asDouble();
			led.normal.z = led_object[5].asDouble();
			led.angles.x = led_object[6].asDouble();
			led.angles.y = led_object[7].asDouble();
			led.angles.z = led_object[8].asDouble();
		}
	} catch (const std::exception &e) {
		U_LOG_E("Exception while parsing touch controller calibration JSON: %s", e.what());
		return false;
	}

	return true;
}
