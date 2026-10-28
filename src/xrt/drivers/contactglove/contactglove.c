// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of ContactGlove device driver.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_contactglove
 */

#include "xrt/xrt_device.h"

#include "util/u_logging.h"
#include "util/u_misc.h"
#include "util/u_device.h"
#include "util/u_debug.h"
#include "util/u_var.h"
#include "util/u_cobs.h"
#include "util/u_hand_simulation.h"

#include "math/m_api.h"

#include "contactglove_interface.h"
#include "contactglove_internal.h"

#include <unistd.h>
#include <errno.h>


DEBUG_GET_ONCE_LOG_OPTION(contactglove_log, "CONTACTGLOVE_LOG", U_LOGGING_INFO)
DEBUG_GET_ONCE_BOOL_OPTION(contactglove_pair, "CONTACTGLOVE_PAIR", false)

#define DONGLE_TRACE(dongle, ...) U_LOG_IFL_T(dongle->log_level, __VA_ARGS__)
#define DONGLE_DEBUG(dongle, ...) U_LOG_IFL_D(dongle->log_level, __VA_ARGS__)
#define DONGLE_INFO(dongle, ...) U_LOG_IFL_I(dongle->log_level, __VA_ARGS__)
#define DONGLE_WARN(dongle, ...) U_LOG_IFL_W(dongle->log_level, __VA_ARGS__)
#define DONGLE_ERROR(dongle, ...) U_LOG_IFL_E(dongle->log_level, __VA_ARGS__)
#define GLOVE_TRACE(glove, ...) U_LOG_XDEV_IFL_T(&glove->base, glove->dongle->log_level, __VA_ARGS__)
#define GLOVE_DEBUG(glove, ...) U_LOG_XDEV_IFL_D(&glove->base, glove->dongle->log_level, __VA_ARGS__)
#define GLOVE_INFO(glove, ...) U_LOG_XDEV_IFL_I(&glove->base, glove->dongle->log_level, __VA_ARGS__)
#define GLOVE_WARN(glove, ...) U_LOG_XDEV_IFL_W(&glove->base, glove->dongle->log_level, __VA_ARGS__)
#define GLOVE_ERROR(glove, ...) U_LOG_XDEV_IFL_E(&glove->base, glove->dongle->log_level, __VA_ARGS__)

/*
 *
 * Implementation functions.
 *
 */

static void
contactglove_dongle_destroy(struct contactglove_dongle *dongle)
{
	assert(dongle->base.count == 0);

	DONGLE_INFO(dongle, "Destroying ContactGlove dongle device");

	os_thread_helper_destroy(&dongle->thread);
	os_mutex_destroy(&dongle->data_lock);
	u_cobs_decoder_destroy(&dongle->cobs_decoder);

	os_serial_destroy(dongle->dongle_serial);

	u_var_remove_root(dongle);

	free(dongle);
}

static void
contactglove_dongle_decrement(struct contactglove_dongle *dongle)
{
	if (xrt_reference_dec_and_is_zero(&dongle->base)) {
		contactglove_dongle_destroy(dongle);
	}
}

static uint8_t
crc8(const uint8_t *data, size_t length)
{
	uint8_t crc = 0x00;

	for (size_t i = 0; i < length; i++) {
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; j++) {
			if (crc & 0x80) {
				crc = (crc << 1) ^ 0x07;
			} else {
				crc <<= 1;
			}
		}
	}

	return crc;
}

static ssize_t
contactglove_dongle_write(struct contactglove_dongle *dongle, const uint8_t *data, size_t length)
{
	ssize_t to_write = (ssize_t)length;
	while (to_write > 0) {
		ssize_t written = os_serial_write(dongle->dongle_serial, data + (length - to_write), (size_t)to_write);
		if (written < 0) {
			DONGLE_ERROR(dongle, "Failed to write to ContactGlove dongle, got %s", strerror(errno));
			os_mutex_unlock(&dongle->data_lock);
			return -1;
		}
		to_write -= written;
	}

	return (ssize_t)length;
}

static int
contactglove_dongle_send_packet_locked(struct contactglove_dongle *dongle,
                                       enum contactglove_to_device_packet_mode mode,
                                       const uint8_t *payload,
                                       size_t payload_length)
{
	// According to the specifications, the COBS encoder is limited to <=255 bytes, which means the payload
	// length must be <=253 bytes (1 byte for mode, 1 byte for CRC)

	uint8_t input_packet[255];
	uint8_t output_packet[256];

	if (payload_length > 253) {
		DONGLE_ERROR(dongle, "Payload length %zu exceeds maximum of 253 bytes", payload_length);
		return -1;
	}

	input_packet[0] = (uint8_t)mode;
	memcpy(&input_packet[1], payload, payload_length);
	input_packet[payload_length + 1] = crc8(input_packet, payload_length + 1);

	size_t encoded_length =
	    u_cobs_encode(input_packet, payload_length + 2, output_packet, sizeof(output_packet) - 1);

	ssize_t written = contactglove_dongle_write(dongle, output_packet, encoded_length);

	if (written < 0 || (size_t)written != encoded_length) {
		DONGLE_ERROR(dongle, "Failed to send ContactGlove packet of mode %u, wrote %zd of %zu bytes", mode,
		             written, encoded_length);
		return -1;
	}

	return 0;
}

static int
contactglove_send_ping(struct contactglove_dongle *dongle, uint8_t nonce)
{
	uint8_t payload[] = {nonce, 0x00, 0x00};
	int ret = contactglove_dongle_send_packet_locked(dongle, CONTACTGLOVE_TO_DEVICE_PACKET_MODE_PING, payload,
	                                                 sizeof(payload));

	if (ret < 0) {
		DONGLE_ERROR(dongle, "Failed to send ping packet, got %d", ret);
		return -1;
	}

	return 0;
}

static void
contactglove_ping_handle_glove_timeout(struct contactglove_dongle *dongle,
                                       struct contactglove_device *glove,
                                       timepoint_ns now)
{
	if ((now - dongle->last_ping_send) > PING_INTERVAL_NS &&
	    glove->last_received_ping_nonce != dongle->last_sent_ping_nonce &&
	    (now - glove->last_ping_receive) > PING_TIMEOUT_NS) {
		GLOVE_DEBUG(glove, "Did not receive nonce %d in time for %d glove.", dongle->last_sent_ping_nonce,
		            glove->hand);
	}
}

static int
contactglove_request_module_state_locked(struct contactglove_dongle *dongle,
                                         struct contactglove_device *glove,
                                         enum contactglove_module_kind module_kind)
{
	uint8_t payload[] = {(uint8_t)glove->role, (uint8_t)module_kind};
	int ret = contactglove_dongle_send_packet_locked(
	    dongle, CONTACTGLOVE_TO_DEVICE_PACKET_MODE_REQUEST_MODULE_STATE, payload, sizeof(payload));

	if (ret < 0) {
		DONGLE_ERROR(dongle, "Failed to send ping packet, got %d", ret);
		return -1;
	}

	return 0;
}

static int
contactglove_set_module_state_locked(struct contactglove_dongle *dongle,
                                     struct contactglove_device *glove,
                                     enum contactglove_module_kind module,
                                     const uint8_t *data,
                                     size_t length)
{
	struct contactglove_to_host_packet_module_state_header header = {
	    .device_role = glove->role,
	    .module_kind = module,
	    .data_length = length,
	};

	uint8_t payload[256];
	memcpy(payload, &header, sizeof(header));
	assert(length < (sizeof(payload) - sizeof(header)));
	memcpy(payload + (sizeof(header)), data, length);

	int ret = contactglove_dongle_send_packet_locked(dongle, CONTACTGLOVE_TO_DEVICE_PACKET_MODE_SET_MODULE_STATE,
	                                                 payload, sizeof(header) + length);
	if (ret < 0) {
		DONGLE_ERROR(dongle, "Failed to set module state, got %d", ret);
	}

	glove->module_state_valid[module] = false;

	// Request an update after we send the update
	ret = contactglove_request_module_state_locked(dongle, glove, module);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int
contactglove_request_update_module_state_locked(struct contactglove_dongle *dongle, struct contactglove_device *glove)
{
	int ret;
	if (!glove->module_state_valid[CONTACTGLOVE_MODULE_MAGNETRA2]) {
		ret = contactglove_request_module_state_locked(dongle, glove, CONTACTGLOVE_MODULE_MAGNETRA2);
		if (ret < 0) {
			return ret;
		}
	}
	if (!glove->module_state_valid[CONTACTGLOVE_MODULE_SLEEP_MANAGER]) {
		ret = contactglove_request_module_state_locked(dongle, glove, CONTACTGLOVE_MODULE_SLEEP_MANAGER);
		if (ret < 0) {
			return ret;
		}
	}
	if (!glove->module_state_valid[CONTACTGLOVE_MODULE_LED_MANAGER]) {
		ret = contactglove_request_module_state_locked(dongle, glove, CONTACTGLOVE_MODULE_LED_MANAGER);
		if (ret < 0) {
			return ret;
		}
	}

	if (glove->magnetra2_connected && glove->module_state_valid[CONTACTGLOVE_MODULE_MAGNETRA2] &&
	    !glove->module_state_magnetra2.enabled) {
		struct contactglove_module_state_magnetra2 module_state_magnetra2 = {.enabled = true};

		ret = contactglove_set_module_state_locked(dongle, glove, CONTACTGLOVE_MODULE_MAGNETRA2,
		                                           (const uint8_t *)&module_state_magnetra2,
		                                           sizeof(module_state_magnetra2));
		if (ret < 0) {
			return ret;
		}
	}

	// Change the LED in a pattern to show Monado has booted the glove
	if (glove->module_state_valid[CONTACTGLOVE_MODULE_LED_MANAGER]) {
		struct contactglove_module_state_led_manager next_state;

		switch (glove->module_state_led_manager.r) {
		default:
		case 0x5B: {
			next_state = (struct contactglove_module_state_led_manager){
			    .r = 0xF5,
			    .g = 0xAB,
			    .b = 0xB9,
			};
			break;
		}
		case 0xF5: {
			next_state = (struct contactglove_module_state_led_manager){
			    .r = 0xFF,
			    .g = 0xFF,
			    .b = 0xFF,
			};
			break;
		}
		case 0xFF: {
			next_state = (struct contactglove_module_state_led_manager){
			    .r = 0x5B,
			    .g = 0xCF,
			    .b = 0xFB,
			};
			break;
		}
		}

		ret = contactglove_set_module_state_locked(dongle, glove, CONTACTGLOVE_MODULE_LED_MANAGER,
		                                           (const uint8_t *)&next_state, sizeof(next_state));
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int
contactglove_ping_tick(struct contactglove_dongle *dongle, timepoint_ns now)
{
	int ret;

	if ((now - dongle->last_ping_send) > PING_INTERVAL_NS) {
		dongle->last_sent_ping_nonce++;
		ret = contactglove_send_ping(dongle, dongle->last_sent_ping_nonce) == 0;
		if (ret >= 0) {
			dongle->last_ping_send = now;
			DONGLE_TRACE(dongle, "Sent ping with nonce %u", dongle->last_sent_ping_nonce);
		} else {
			return -1;
		}

		if (dongle->left_glove) {
			ret = contactglove_request_update_module_state_locked(dongle, dongle->left_glove);
			if (ret < 0) {
				return -1;
			}
		}
		if (dongle->right_glove) {
			ret = contactglove_request_update_module_state_locked(dongle, dongle->right_glove);
			if (ret < 0) {
				return -1;
			}
		}
	}

	if (dongle->left_glove) {
		contactglove_ping_handle_glove_timeout(dongle, dongle->left_glove, now);
	}
	if (dongle->right_glove) {
		contactglove_ping_handle_glove_timeout(dongle, dongle->right_glove, now);
	}

	return 0;
}

static int
contactglove_dongle_thread_tick(struct contactglove_dongle *dongle)
{
	int ret;
	uint8_t buffer[256];
	ssize_t read_bytes = os_serial_read(dongle->dongle_serial, buffer, sizeof(buffer), 100);
	if (read_bytes < 0) {
		DONGLE_ERROR(dongle, "Failed to read from ContactGlove dongle, got %s (%zd)", strerror(errno),
		             read_bytes);
		return -1;
	} else if (read_bytes == 0) {
		// No data available
		return 0;
	}

	// Push read bytes to COBS decoder
	ret = u_cobs_push_bytes(&dongle->cobs_decoder, buffer, (size_t)read_bytes);
	if (ret != 0) {
		DONGLE_ERROR(dongle, "Failed to push bytes to COBS decoder, got %s", strerror(ret));
		return 0; // Continue anyway
	}

	timepoint_ns now = os_monotonic_get_ns();

	os_mutex_lock(&dongle->data_lock);
	ret = contactglove_ping_tick(dongle, now);
	os_mutex_unlock(&dongle->data_lock);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

static void
contactglove_dongle_handle_packet(void *user_data, const uint8_t *data, size_t length)
{
	struct contactglove_dongle *dongle = user_data;

	if (length < 3) {
		return;
	}

	timepoint_ns now_ns = os_monotonic_get_ns();

	uint8_t expected_crc = crc8(data, length - 1);

	// Check the CRC
	if (expected_crc != data[length - 1]) {
		DONGLE_ERROR(dongle, "Got bad CRC of %d when expecting %d", data[length - 1], expected_crc);
	}

	DONGLE_TRACE(dongle, "Received packet of length %zu with mode %d", length,
	             (enum contactglove_to_host_packet_mode)data[0]);

	const uint8_t *payload = data + 1;
	const size_t payload_length = length - 2; // cut off mode and crc

	switch (data[0]) {
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_DEBUG: {
		struct contactglove_to_host_packet_debug debug;
		if (payload_length != sizeof(debug)) {
			DONGLE_ERROR(dongle, "Invalid debug packet length %zu, expected %zu", payload_length,
			             sizeof(debug));
			return;
		}

		memcpy(&debug, payload, sizeof(debug));

		DONGLE_DEBUG(dongle, "Received debug message from dongle, %d:%d", debug.message_kind,
		             debug.message_value);

		break;
	}
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_DEVICE_VERSION: {
		struct contactglove_to_host_packet_device_version version_info;
		if (payload_length != sizeof(version_info)) {
			DONGLE_ERROR(dongle, "Invalid device version packet length %zu, expected %zu", payload_length,
			             sizeof(version_info));
			return;
		}

		memcpy(&version_info, payload, sizeof(version_info));

		DONGLE_DEBUG(dongle, "Received device versions - Dongle: %u.%u, Left Glove: %u.%u, Right Glove: %u.%u",
		             version_info.dongle.major, version_info.dongle.minor, version_info.left.major,
		             version_info.left.minor, version_info.right.major, version_info.right.minor);

		os_mutex_lock(&dongle->data_lock);
		dongle->version = version_info.dongle;
		if (dongle->left_glove) {
			dongle->left_glove->version = version_info.left;
		}
		if (dongle->right_glove) {
			dongle->right_glove->version = version_info.right;
		}
		os_mutex_unlock(&dongle->data_lock);

		break;
	}
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_GLOVE_CONNECTION: {
		struct contactglove_to_host_packet_glove_connection glove_info;
		if (payload_length != sizeof(glove_info)) {
			DONGLE_ERROR(dongle, "Invalid glove connection packet length %zu, expected %zu", payload_length,
			             sizeof(glove_info));
			return;
		}

		memcpy(&glove_info, payload, sizeof(glove_info));

		DONGLE_DEBUG(
		    dongle,
		    "Received glove connection info - Channel: %u, Left Battery: %u (%u), Right Battery: %u (%u)",
		    glove_info.channel, glove_info.battery_left.battery_raw, glove_info.battery_left.charge_status,
		    glove_info.battery_right.battery_raw, glove_info.battery_right.charge_status);

		os_mutex_lock(&dongle->data_lock);
		dongle->channel = glove_info.channel;
		if (dongle->left_glove) {
			dongle->left_glove->raw_battery = glove_info.battery_left;
			dongle->left_glove->connected = glove_info.battery_left.battery_raw != 0xFF;
		}
		if (dongle->right_glove) {
			dongle->right_glove->raw_battery = glove_info.battery_right;
			dongle->right_glove->connected = glove_info.battery_right.battery_raw != 0xFF;
		}
		os_mutex_unlock(&dongle->data_lock);

		break;
	}
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_LEFT_PING:
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_RIGHT_PING: {
		struct contactglove_to_host_packet_ping ping_info;
		if (payload_length != sizeof(ping_info)) {
			DONGLE_ERROR(dongle, "Invalid ping packet length %zu, expected %zu", payload_length,
			             sizeof(ping_info));
			return;
		}

		memcpy(&ping_info, payload, sizeof(ping_info));

		bool is_right_hand = (data[0] == CONTACTGLOVE_TO_HOST_PACKET_MODE_RIGHT_PING);

		os_mutex_lock(&dongle->data_lock);
		struct contactglove_device *glove = is_right_hand ? dongle->right_glove : dongle->left_glove;

		// @note The firmware seems to send response times of >=400 while the device is disconnected or just
		// connecting, that's probably a result of an internal timeout of some kind? Let's throw that offset
		// away
		if (glove && glove->connected && ping_info.glove_to_dongle_ms < 350) {
			glove->last_ping_receive = now_ns;
			glove->last_received_ping_nonce = ping_info.nonce;
			glove->glove_to_dongle_ms = __le16_to_cpu(ping_info.glove_to_dongle_ms);

			glove->avg_glove_to_dongle_offset_ns +=
			    (((timepoint_ns)glove->glove_to_dongle_ms * U_TIME_1MS_IN_NS) -
			     glove->avg_glove_to_dongle_offset_ns) *
			    0.1;

			GLOVE_DEBUG(
			    glove,
			    "Received ping response with nonce %u, glove_to_dongle_ms %u, adjusted average to %lf",
			    ping_info.nonce, glove->glove_to_dongle_ms,
			    time_ns_to_s(glove->avg_glove_to_dongle_offset_ns));
		}
		os_mutex_unlock(&dongle->data_lock);

		break;
	}
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_GYRO_DATA_LEFT:
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_GYRO_DATA_RIGHT: {
		struct contactglove_to_host_packet_hand_gyro_data hand_gyro_data;
		if (payload_length != sizeof(hand_gyro_data)) {
			DONGLE_ERROR(dongle, "Invalid hand gyro data packet length %zu, expected %zu", payload_length,
			             sizeof(hand_gyro_data));
			return;
		}

		memcpy(&hand_gyro_data, payload, sizeof(hand_gyro_data));

		bool is_right_hand = (data[0] == CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_GYRO_DATA_RIGHT);

		os_mutex_lock(&dongle->data_lock);
		struct contactglove_device *glove = is_right_hand ? dongle->right_glove : dongle->left_glove;

		if (glove) {
			struct xrt_quat quat = {
			    .w = (float)__le16_to_cpu(hand_gyro_data.quat_raw[2]) * 2.0f / 65535.0f - 1.0f,
			    .x = -(float)__le16_to_cpu(hand_gyro_data.quat_raw[0]) * 2.0f / 65535.0f + 1.0f,
			    .y = (float)__le16_to_cpu(hand_gyro_data.quat_raw[1]) * 2.0f / 65535.0f - 1.0f,
			    .z = -(float)__le16_to_cpu(hand_gyro_data.quat_raw[3]) * 2.0f / 65535.0f + 1.0f,
			};
			math_quat_normalize(&quat);

			struct xrt_space_relation relation = {
			    .pose = {.orientation = quat},
			    .relation_flags =
			        XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT,
			};

			m_relation_history_push_with_motion_estimation(glove->relation_history, &relation,
			                                               now_ns - glove->avg_glove_to_dongle_offset_ns);
			glove->latest_orientation = quat;
		}
		os_mutex_unlock(&dongle->data_lock);

		break;
	}
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_SENSOR_VALUE_LEFT:
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_SENSOR_VALUE_RIGHT: {
		struct contactglove_to_host_packet_sensor_data sensor_data;
		if (payload_length != sizeof(sensor_data)) {
			DONGLE_ERROR(dongle, "Invalid sensor values packet length %zu, expected %zu", payload_length,
			             sizeof(sensor_data));
			return;
		}

		memcpy(&sensor_data, payload, sizeof(sensor_data));

		bool is_right_hand = (data[0] == CONTACTGLOVE_TO_HOST_PACKET_MODE_SENSOR_VALUE_RIGHT);

		os_mutex_lock(&dongle->data_lock);
		struct contactglove_device *glove = is_right_hand ? dongle->right_glove : dongle->left_glove;

		if (glove) {
			memcpy(&glove->raw_flex_adc_values, sensor_data.sensor_values,
			       sizeof(sensor_data.sensor_values));

			contactglove_calibration_wizard_push_flex_sensors(&glove->calibration_wizard,
			                                                  glove->raw_flex_adc_values);
		}

		os_mutex_unlock(&dongle->data_lock);

		break;
	}
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_MODULE_STATE: {
		struct contactglove_to_host_packet_module_state_header module_state_header;
		if (payload_length < sizeof(module_state_header)) {
			DONGLE_ERROR(dongle, "Invalid module state packet length, expected >= %zu, got %zu",
			             sizeof(module_state_header), payload_length);
			return;
		}

		memcpy(&module_state_header, payload, sizeof(module_state_header));

		os_mutex_lock(&dongle->data_lock);
		struct contactglove_device *glove = module_state_header.device_role == CONTACTGLOVE_DEVICE_ROLE_RIGHT
		                                        ? dongle->right_glove
		                                        : dongle->left_glove;

		if (glove) {
			const uint8_t *module_state_payload = payload + sizeof(module_state_header);
			const size_t module_state_payload_length = payload_length - sizeof(module_state_header);

			switch (module_state_header.module_kind) {
			case CONTACTGLOVE_MODULE_MAGNETRA2: {
				struct contactglove_module_state_magnetra2 module_state_magnetra2;
				if (module_state_payload_length != sizeof(module_state_magnetra2)) {
					DONGLE_ERROR(dongle,
					             "Invalid Magnetra2 module state length, got %zu, expected %zu",
					             module_state_payload_length, sizeof(module_state_magnetra2));
					// break instead of return not to leak the mutex
					break;
				}

				memcpy(&module_state_magnetra2, module_state_payload, sizeof(module_state_magnetra2));

				glove->module_state_magnetra2 = module_state_magnetra2;
				glove->module_state_valid[CONTACTGLOVE_MODULE_MAGNETRA2] = true;

				break;
			}
			case CONTACTGLOVE_MODULE_SLEEP_MANAGER: {
				struct contactglove_module_state_sleep_manager module_state_sleep_manager;
				if (module_state_payload_length != sizeof(module_state_sleep_manager)) {
					DONGLE_ERROR(dongle,
					             "Invalid sleep manager module state length, got %zu, expected %zu",
					             module_state_payload_length, sizeof(module_state_sleep_manager));
					// break instead of return not to leak the mutex
					break;
				}

				memcpy(&module_state_sleep_manager, module_state_payload,
				       sizeof(module_state_sleep_manager));

				glove->module_state_sleep_manager = module_state_sleep_manager;
				glove->module_state_valid[CONTACTGLOVE_MODULE_SLEEP_MANAGER] = true;

				break;
			}
			case CONTACTGLOVE_MODULE_LED_MANAGER: {
				struct contactglove_module_state_led_manager module_state_led_manager;
				if (module_state_payload_length != sizeof(module_state_led_manager)) {
					DONGLE_ERROR(dongle,
					             "Invalid led manager module state length, got %zu, expected %zu",
					             module_state_payload_length, sizeof(module_state_led_manager));
					// break instead of return not to leak the mutex
					break;
				}

				memcpy(&module_state_led_manager, module_state_payload,
				       sizeof(module_state_led_manager));

				glove->module_state_led_manager = module_state_led_manager;
				glove->module_state_valid[CONTACTGLOVE_MODULE_LED_MANAGER] = true;

				break;
			}
			}
		}
		os_mutex_unlock(&dongle->data_lock);

		break;
	}
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_MAGNETRA2_INPUT_LEFT:
	case CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_MAGNETRA2_INPUT_RIGHT: {
		struct contactglove_to_host_packet_magnetra2_input magnetra2_input;
		if (payload_length != sizeof(magnetra2_input)) {
			DONGLE_ERROR(dongle, "Invalid Magnetra2 input packet length %zu, expected %zu", payload_length,
			             sizeof(magnetra2_input));
			return;
		}

		memcpy(&magnetra2_input, payload, sizeof(magnetra2_input));

		bool is_right_hand = (data[0] == CONTACTGLOVE_TO_HOST_PACKET_MODE_HAND_MAGNETRA2_INPUT_RIGHT);

		DONGLE_TRACE(
		    dongle,
		    "Received Magnetra2 input - Hand: %s, Button Bits: 0x%02X, Joystick X: %u, Joystick Y: %u, "
		    "Trigger: %u, Multi-Channel Value: %u",
		    is_right_hand ? "Right" : "Left", magnetra2_input.button_bits, magnetra2_input.joystick_x,
		    magnetra2_input.joystick_y, magnetra2_input.trigger, magnetra2_input.multi_ch_value);

		os_mutex_lock(&dongle->data_lock);
		struct contactglove_device *glove = is_right_hand ? dongle->right_glove : dongle->left_glove;

		if (glove) {
			glove->magnetra2_connected = true;

			glove->raw_input = magnetra2_input;
			glove->raw_input_update_time_ns = now_ns - glove->avg_glove_to_dongle_offset_ns;

			contactglove_calibration_wizard_push_joystick(
			    &glove->calibration_wizard, magnetra2_input.joystick_x, magnetra2_input.joystick_y);

			contactglove_calibration_wizard_push_trigger(&glove->calibration_wizard,
			                                             magnetra2_input.trigger);

			contactglove_calibration_wizard_push_button(
			    &glove->calibration_wizard,
			    (magnetra2_input.button_bits & MAGNETRA2_BUTTON_BITS_TRACKPAD_BOTTOM) == 0);
		}
		os_mutex_unlock(&dongle->data_lock);

		break;
	}
	}
}

void *
contactglove_dongle_thread(void *ptr)
{
	struct contactglove_dongle *dongle = ptr;

	os_thread_helper_lock(&dongle->thread);

	int ret = 0;
	while (ret >= 0 && os_thread_helper_is_running_locked(&dongle->thread)) {
		os_thread_helper_unlock(&dongle->thread);

		ret = contactglove_dongle_thread_tick(dongle);

		os_thread_helper_lock(&dongle->thread);
	}

	os_thread_helper_unlock(&dongle->thread);

	if (ret < 0) {
		DONGLE_ERROR(dongle, "Dongle thread exiting due to error %s", strerror(-ret));
	} else {
		DONGLE_DEBUG(dongle, "Dongle thread exiting cleanly");
	}

	return NULL;
}

static void
contactglove_get_curl_values(struct contactglove_device *glove, struct u_hand_tracking_curl_values *curl_values)
{
	(*curl_values) = (struct u_hand_tracking_curl_values){
	    .index = contactglove_apply_sensor_calibration(glove->calibration_wizard.calibration,
	                                                   CONTACTGLOVE_SENSOR_POSITION_FINGER_INDEX_ROOT1,
	                                                   glove->raw_flex_adc_values),
	    .middle = contactglove_apply_sensor_calibration(glove->calibration_wizard.calibration,
	                                                    CONTACTGLOVE_SENSOR_POSITION_FINGER_MIDDLE_ROOT1,
	                                                    glove->raw_flex_adc_values),
	    .ring = contactglove_apply_sensor_calibration(glove->calibration_wizard.calibration,
	                                                  CONTACTGLOVE_SENSOR_POSITION_FINGER_RING_ROOT1,
	                                                  glove->raw_flex_adc_values),
	    .little = contactglove_apply_sensor_calibration(glove->calibration_wizard.calibration,
	                                                    CONTACTGLOVE_SENSOR_POSITION_FINGER_PINKY_ROOT1,
	                                                    glove->raw_flex_adc_values),
	    .thumb = contactglove_apply_sensor_calibration(glove->calibration_wizard.calibration,
	                                                   CONTACTGLOVE_SENSOR_POSITION_FINGER_THUMB_ROOT1,
	                                                   glove->raw_flex_adc_values),
	};
}

/*
 *
 * Device functions.
 *
 */

static xrt_result_t
contactglove_get_tracked_pose(struct xrt_device *xdev,
                              const enum xrt_input_name name,
                              const int64_t at_timestamp_ns,
                              struct xrt_space_relation *out_relation)
{
	struct contactglove_device *glove = (struct contactglove_device *)xdev;

	switch (name) {
	case XRT_INPUT_HT_UNOBSTRUCTED_LEFT:
	case XRT_INPUT_HT_UNOBSTRUCTED_RIGHT:
	case XRT_INPUT_MAGNETRA2_GRIP_POSE:
	case XRT_INPUT_MAGNETRA2_AIM_POSE: break;
	default: return XRT_ERROR_INPUT_UNSUPPORTED;
	}

	*out_relation = (struct xrt_space_relation)XRT_SPACE_RELATION_ZERO;

	// If not connected, return empty relation
	if (!glove->connected) {
		return XRT_SUCCESS;
	}

	m_relation_history_get(glove->relation_history, at_timestamp_ns, out_relation);

	return XRT_SUCCESS;
}

static void
contactglove_destroy(struct xrt_device *xdev)
{
	struct contactglove_device *glove = (struct contactglove_device *)xdev;

	os_mutex_lock(&glove->dongle->data_lock);
	switch (glove->hand) {
	case XRT_HAND_RIGHT: glove->dongle->right_glove = NULL; break;
	case XRT_HAND_LEFT: glove->dongle->left_glove = NULL; break;
	default: break;
	}
	os_mutex_unlock(&glove->dongle->data_lock);

	contactglove_dongle_decrement(glove->dongle);

	m_relation_history_destroy(&glove->relation_history);

	contactglove_calibration_wizard_deinit(&glove->calibration_wizard);

	u_device_free(&glove->base);
}

static xrt_result_t
contactglove_get_battery_status(struct xrt_device *xdev, bool *out_present, bool *out_charging, float *out_charge)
{
	struct contactglove_device *glove = (struct contactglove_device *)xdev;

	os_mutex_lock(&glove->dongle->data_lock);

	if (glove->raw_battery.battery_raw == 0xFF || !glove->connected) {
		*out_present = false;
		*out_charge = 0.0f;
	} else {
		*out_present = true;
		*out_charge = (float)glove->raw_battery.battery_raw / 100.0f;
	}

	*out_charging = false;

	os_mutex_unlock(&glove->dongle->data_lock);

	return XRT_SUCCESS;
}

static xrt_result_t
contactglove_get_hand_tracking(struct xrt_device *xdev,
                               enum xrt_input_name name,
                               int64_t desired_timestamp_ns,
                               struct xrt_hand_joint_set *out_value,
                               int64_t *out_timestamp_ns)
{
	struct contactglove_device *glove = (struct contactglove_device *)xdev;

	os_mutex_lock(&glove->dongle->data_lock);

	struct u_hand_tracking_curl_values curl_values;
	contactglove_get_curl_values(glove, &curl_values);

	struct xrt_space_relation root_relation;
	xrt_device_get_tracked_pose(
	    xdev, glove->hand == XRT_HAND_LEFT ? XRT_INPUT_HT_UNOBSTRUCTED_LEFT : XRT_INPUT_HT_UNOBSTRUCTED_RIGHT,
	    desired_timestamp_ns, &root_relation);

	u_hand_sim_simulate_for_valve_index_knuckles(&curl_values, glove->hand, &root_relation, out_value);

	*out_timestamp_ns = desired_timestamp_ns;

	os_mutex_unlock(&glove->dongle->data_lock);

	return XRT_SUCCESS;
}

static xrt_result_t
contactglove_update_inputs(struct xrt_device *xdev)
{
	struct contactglove_device *glove = (struct contactglove_device *)xdev;

	os_mutex_lock(&glove->dongle->data_lock);

#define UPDATE_TIME(name)                                                                                              \
	glove->base.inputs[CONTACTGLOVE_DEVICE_INPUT_##name].timestamp = glove->raw_input_update_time_ns;
#define UPDATE_BOOL(name, exp)                                                                                         \
	glove->base.inputs[CONTACTGLOVE_DEVICE_INPUT_##name].value.boolean = (exp);                                    \
	UPDATE_TIME(name)

	// Device sends active-*low* inputs
	UPDATE_BOOL(A_CLICK, !(glove->raw_input.button_bits & MAGNETRA2_BUTTON_BITS_A))
	UPDATE_BOOL(B_CLICK, !(glove->raw_input.button_bits & MAGNETRA2_BUTTON_BITS_B))
	UPDATE_BOOL(TRIGGER_CLICK, !(glove->raw_input.button_bits & MAGNETRA2_BUTTON_BITS_TRIGGER_CLICK))
	UPDATE_BOOL(X_CLICK, !(glove->raw_input.button_bits & MAGNETRA2_BUTTON_BITS_TRACKPAD_BOTTOM))
	UPDATE_BOOL(PAIRING_CLICK, !(glove->raw_input.button_bits & MAGNETRA2_BUTTON_BITS_PAIRING))

	glove->base.inputs[CONTACTGLOVE_DEVICE_INPUT_THUMBSTICK].value.vec2 = contactglove_apply_stick_calibration(
	    glove->calibration_wizard.calibration.joystick, glove->raw_input.joystick_x, glove->raw_input.joystick_y);
	UPDATE_TIME(THUMBSTICK)
	glove->base.inputs[CONTACTGLOVE_DEVICE_INPUT_TRIGGER_VALUE].value.vec1 = (struct xrt_vec1){
	    .x = contactglove_apply_trigger_calibration(glove->calibration_wizard.calibration.trigger,
	                                                glove->raw_input.trigger),
	};
	UPDATE_TIME(TRIGGER_VALUE)

	struct u_hand_tracking_curl_values curl_values;
	contactglove_get_curl_values(glove, &curl_values);

	float squeeze = (curl_values.middle + curl_values.ring + curl_values.little) / 3.0f;
	glove->base.inputs[CONTACTGLOVE_DEVICE_INPUT_SQUEEZE_VALUE].value.vec1 = (struct xrt_vec1){
	    .x = CLAMP(squeeze, 0.0f, 1.0f),
	};
	UPDATE_TIME(SQUEEZE_VALUE)

#define CHECK_RANGE(range)                                                                                             \
	(glove->raw_input.multi_ch_value >= MAGNETRA2_MULTI_CH_RANGE_##range##_START &&                                \
	 glove->raw_input.multi_ch_value <= MAGNETRA2_MULTI_CH_RANGE_##range##_END)

	// @note yes, this means only one can be pressed down at once, i'm unsure why the protocol works like this,
	// since these all appear to just be binary inputs, rather than some kind of force or capacitive sensor
	UPDATE_BOOL(THUMBSTICK_CLICK, CHECK_RANGE(JOYSTICK))
	UPDATE_BOOL(Y_CLICK, CHECK_RANGE(TRACKPAD_TOP))
	UPDATE_BOOL(SYSTEM_CLICK, CHECK_RANGE(SYSTEM))

#undef CHECK_RANGE
#undef UPDATE_BOOL
#undef UPDATE_TIME

	os_mutex_unlock(&glove->dongle->data_lock);

	return XRT_SUCCESS;
}

static int
contactglove_create_glove(struct contactglove_dongle *dongle,
                          enum contactglove_type type,
                          const char *serial_number,
                          enum xrt_hand hand,
                          struct contactglove_device **out_glove)
{
	struct contactglove_device *glove = U_DEVICE_ALLOCATE(struct contactglove_device, U_DEVICE_ALLOC_TRACKING_NONE,
	                                                      CONTACTGLOVE_DEVICE_INPUT_COUNT, 1);
	if (glove == NULL) {
		U_LOG_E("Failed to allocate ContactGlove device");
		return -1;
	}

	m_relation_history_create(&glove->relation_history);

	glove->dongle = dongle;
	glove->hand = hand;
	glove->raw_battery.battery_raw = 0xFF; // unknown

	glove->base.name = XRT_DEVICE_CONTACTGLOVE2_WITH_MAGNETRA2;
	snprintf(glove->base.serial, XRT_DEVICE_NAME_LEN, "%s", serial_number);

	contactglove_calibration_wizard_init(&glove->calibration_wizard, glove);

	switch (hand) {
	case XRT_HAND_RIGHT: {
		glove->base.device_type = XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER;
		glove->base.inputs[CONTACTGLOVE_DEVICE_INPUT_HT_UNOBSTRUCTED].name = XRT_INPUT_HT_UNOBSTRUCTED_RIGHT;
		snprintf(glove->base.str, XRT_DEVICE_NAME_LEN, "ContactGlove2 (Right)");
		glove->role = CONTACTGLOVE_DEVICE_ROLE_RIGHT;
		break;
	}
	case XRT_HAND_LEFT: {
		glove->base.device_type = XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER;
		glove->base.inputs[CONTACTGLOVE_DEVICE_INPUT_HT_UNOBSTRUCTED].name = XRT_INPUT_HT_UNOBSTRUCTED_LEFT;
		snprintf(glove->base.str, XRT_DEVICE_NAME_LEN, "ContactGlove2 (Left)");
		glove->role = CONTACTGLOVE_DEVICE_ROLE_LEFT;
		break;
	}
	}

	u_device_populate_function_pointers(&glove->base, contactglove_get_tracked_pose, contactglove_destroy);
	glove->base.get_battery_status = contactglove_get_battery_status;
	glove->base.get_hand_tracking = contactglove_get_hand_tracking;
	glove->base.update_inputs = contactglove_update_inputs;

	glove->base.supported.orientation_tracking = true;
	glove->base.supported.hand_tracking = true;
	glove->base.supported.battery_status = true;

#define SET_INPUT(input_name)                                                                                          \
	glove->base.inputs[CONTACTGLOVE_DEVICE_INPUT_##input_name].name = XRT_INPUT_MAGNETRA2_##input_name;
	SET_INPUT(A_CLICK)
	SET_INPUT(B_CLICK)
	SET_INPUT(X_CLICK)
	SET_INPUT(Y_CLICK)
	SET_INPUT(SYSTEM_CLICK)
	SET_INPUT(PAIRING_CLICK)
	SET_INPUT(TRIGGER_VALUE)
	SET_INPUT(TRIGGER_CLICK)
	SET_INPUT(SQUEEZE_VALUE)
	SET_INPUT(THUMBSTICK)
	SET_INPUT(THUMBSTICK_CLICK)
	SET_INPUT(GRIP_POSE)
	SET_INPUT(AIM_POSE)
#undef SET_INPUT

	// Hold a reference to the dongle while the glove exists
	xrt_reference_inc(&dongle->base);

	*out_glove = glove;

	u_var_add_root(glove, hand == XRT_HAND_RIGHT ? "ContactGlove2 Glove (Right)" : "ContactGlove2 Glove (Left)",
	               true);
	u_var_add_u8(glove, &glove->version.major, "Version (Major)");
	u_var_add_u8(glove, &glove->version.minor, "Version (Minor)");
	u_var_add_u8(glove, &glove->raw_battery.battery_raw, "Raw Battery");
	u_var_add_u8(glove, &glove->raw_battery.charge_status, "Charge Status");
	u_var_add_u8(glove, &glove->last_received_ping_nonce, "Last Received Ping Nonce");
	u_var_add_u16(glove, &glove->glove_to_dongle_ms, "Glove to Dongle (ms)");
	u_var_add_ro_i64_ns(glove, &glove->avg_glove_to_dongle_offset_ns, "Avg Glove to Dongle Offset (ns)");
	u_var_add_ro_i64_ns(glove, &glove->last_ping_receive, "Last Ping Receive (ns)");
	u_var_add_ro_quat_f32(glove, &glove->latest_orientation, "Latest Orientation");
	u_var_add_bool(glove, &glove->connected, "Connected");
	{
		u_var_add_gui_header(glove, NULL, "Magnetra2 State");
		u_var_add_bool(glove, &glove->module_state_valid[CONTACTGLOVE_MODULE_MAGNETRA2],
		               "Magnetra2 State Valid");
		u_var_add_u8(glove, &glove->module_state_magnetra2.enabled, "Magnetra2 Enabled");
	}
	{
		u_var_add_gui_header(glove, NULL, "Sleep Manager State");
		u_var_add_bool(glove, &glove->module_state_valid[CONTACTGLOVE_MODULE_SLEEP_MANAGER],
		               "Sleep Manager State Valid");
		u_var_add_u8(glove, &glove->module_state_sleep_manager.duration_minutes, "Sleep Duration Minutes");
		u_var_add_u8(glove, &glove->module_state_sleep_manager.duration_sec, "Sleep Duration Seconds (Unused)");
	}
	{
		u_var_add_gui_header(glove, NULL, "LED Manager State");
		u_var_add_bool(glove, &glove->module_state_valid[CONTACTGLOVE_MODULE_LED_MANAGER],
		               "LED Manager State Valid");
		u_var_add_u8(glove, &glove->module_state_led_manager.r, "R");
		u_var_add_u8(glove, &glove->module_state_led_manager.g, "G");
		u_var_add_u8(glove, &glove->module_state_led_manager.b, "B");
	}
	{
		u_var_add_gui_header(glove, NULL, "Input State");
		u_var_add_u8(glove, &glove->raw_input.button_bits, "Button Bits");
		u_var_add_u8(glove, &glove->raw_input.joystick_x, "Joystick X");
		u_var_add_u8(glove, &glove->raw_input.joystick_y, "Joystick Y");
		u_var_add_u8(glove, &glove->raw_input.trigger, "Trigger");
		u_var_add_u8(glove, &glove->raw_input.multi_ch_value, "Multi Channel Value");
	}

	glove->u_var_flex_adc_values = (struct u_var_u16_arr){
	    .data = glove->raw_flex_adc_values,
	    .length = CONTACTGLOVE2_SENSOR_COUNT,
	};
	u_var_add_u16_arr(glove, &glove->u_var_flex_adc_values, "Raw Flex ADC Values");

	return 0;
}

/*
 *
 * Exported functions.
 *
 */

int
contactglove_create(enum contactglove_type type,
                    const char *serial_number,
                    struct os_serial_device *dongle_serial,
                    struct contactglove_dongle **out_dongle,
                    struct xrt_device **out_xdevs)
{
	int ret;
	assert(type == CONTACTGLOVE_TYPE_CONTACTGLOVE2); // Currently only ContactGlove2 is supported

	// Set the DTR line high, to tell the dongle that we are ready to communicate.
	if (os_serial_set_line_control(dongle_serial, true, false) != 0) {
		U_LOG_E("Failed to set line control on ContactGlove2 serial device");
		os_serial_destroy(dongle_serial);
		return -1;
	}

	struct contactglove_dongle *dongle = U_TYPED_CALLOC(struct contactglove_dongle);
	if (dongle == NULL) {
		U_LOG_E("Failed to allocate ContactGlove dongle");
		return -1;
	}

	dongle->log_level = debug_get_log_option_contactglove_log();
	dongle->dongle_serial = dongle_serial;
	dongle->type = type;

	ret = u_cobs_decoder_create(256, contactglove_dongle_handle_packet, dongle, &dongle->cobs_decoder);
	if (ret != 0) {
		DONGLE_ERROR(dongle, "Failed to create COBS decoder for ContactGlove dongle, got %d", ret);

		free(dongle);
		return -1;
	}

	ret = os_mutex_init(&dongle->data_lock);
	if (ret != 0) {
		DONGLE_ERROR(dongle, "Failed to initialize ContactGlove dongle data mutex, got %d", ret);
		u_cobs_decoder_destroy(&dongle->cobs_decoder);
		free(dongle);
		return -1;
	}

	ret = os_thread_helper_init(&dongle->thread);
	if (ret != 0) {
		DONGLE_ERROR(dongle, "Failed to initialize ContactGlove dongle thread helper, got %d", ret);
		os_mutex_destroy(&dongle->data_lock);
		free(dongle);
		return -1;
	}

	ret = contactglove_create_glove(dongle, type, serial_number, false, &dongle->left_glove);
	if (ret < 0) {
		DONGLE_ERROR(dongle, "Failed to create left glove, got %d", ret);
		contactglove_dongle_destroy(dongle);
		return -1;
	}

	ret = contactglove_create_glove(dongle, type, serial_number, true, &dongle->right_glove);
	if (ret < 0) {
		DONGLE_ERROR(dongle, "Failed to create right glove, got %d", ret);

		// Clean up left glove directly, since nothing is holding a reference to the glove yet
		struct xrt_device *left_glove_xdev = &dongle->left_glove->base;
		xrt_device_destroy(&left_glove_xdev);

		contactglove_dongle_destroy(dongle);

		return -1;
	}

	ret = os_thread_helper_start(&dongle->thread, contactglove_dongle_thread, dongle);
	if (ret != 0) {
		DONGLE_ERROR(dongle, "Failed to start ContactGlove dongle thread, got %d", ret);

		struct xrt_device *right_glove_xdev = &dongle->right_glove->base;
		xrt_device_destroy(&right_glove_xdev);
		struct xrt_device *left_glove_xdev = &dongle->left_glove->base;
		xrt_device_destroy(&left_glove_xdev);

		contactglove_dongle_destroy(dongle);
	}

	out_xdevs[0] = &dongle->left_glove->base;
	out_xdevs[1] = &dongle->right_glove->base;

	*out_dongle = dongle;

	u_var_add_root(dongle, "ContactGlove Dongle", true);
	u_var_add_log_level(dongle, &dongle->log_level, "Log Level");
	u_var_add_ro_text(dongle, dongle->type == CONTACTGLOVE_TYPE_CONTACTGLOVE1 ? "ContactGlove1" : "ContactGlove2",
	                  "Type");
	u_var_add_u8(dongle, &dongle->version.major, "Version (Major)");
	u_var_add_u8(dongle, &dongle->version.minor, "Version (Minor)");
	u_var_add_u8(dongle, &dongle->channel, "Channel");
	u_var_add_ro_i64_ns(dongle, &dongle->last_ping_send, "Last Ping Send (ns)");
	u_var_add_u8(dongle, &dongle->last_sent_ping_nonce, "Last Sent Ping Nonce");

	if (debug_get_bool_option_contactglove_pair()) {
		os_mutex_lock(&dongle->data_lock);
		ret = contactglove_dongle_pair(dongle);
		os_mutex_unlock(&dongle->data_lock);
		if (ret < 0) {
			DONGLE_ERROR(dongle, "Failed to enable pairing mode on ContactGlove dongle, got %d", ret);
			// Continue anyway
		} else {
			DONGLE_INFO(dongle, "Enabled pairing mode on ContactGlove dongle");
		}
	}

	return 2;
}

int
contactglove_dongle_pair(struct contactglove_dongle *dongle)
{
	const uint8_t payload[] = {0x52}; // enable pairing mode

	int ret = contactglove_dongle_send_packet_locked(dongle, CONTACTGLOVE_TO_DEVICE_PACKET_MODE_REQUEST_PAIRING,
	                                                 payload, sizeof(payload));
	if (ret < 0) {
		DONGLE_ERROR(dongle, "Failed to send pairing request packet, got %d", ret);
		return -1;
	}

	return 0;
}
