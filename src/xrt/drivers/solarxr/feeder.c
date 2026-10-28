// Copyright 2025, rcelyte
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  SolarXR pose feeder
 * @ingroup drv_solarxr
 */

#include "feeder.h"
#include "solarxr_ipc_message.h"

#include "util/u_hashmap.h"
#include "xrt/xrt_device.h"

#include <math.h>
#include <pb.h>

static const char manufacturer[] = "Monado";

#define PROTOBUF_FLOAT(v_) ((uint8_t *)&(v_))[0], ((uint8_t *)&(v_))[1], ((uint8_t *)&(v_))[2], ((uint8_t *)&(v_))[3]
#define PROTOBUF_INT32(v_)                                                                                             \
	(uint8_t)(0x80 | (uint32_t)(v_)), (uint8_t)(0x80 | (uint32_t)(v_) >> 7),                                       \
	    (uint8_t)(0x80 | (uint32_t)(v_) >> 14), (uint8_t)(0x80 | (uint32_t)(v_) >> 21), (uint32_t)(v_) >> 28

struct feeder_device
{
	struct feeder_device *next_to_erase;
	struct xrt_device *xdev;
	enum xrt_input_name input_name;
	uint32_t id;
	bool destroyed;
	uint8_t last_status;
	bool battery_charging;
	float battery_charge;
};

bool
feeder_add_device(struct feeder *const feeder, struct xrt_device *const xdev)
{
	if (xdev == NULL || !xdev->supported.orientation_tracking) {
		return false;
	}
	os_mutex_lock(&feeder->mutex);

	bool result = false;
	if (!solarxr_ipc_socket_is_connected(&feeder->socket) &&
	    !solarxr_ipc_socket_connect(&feeder->socket,
	                                solarxr_ipc_socket_find(feeder->socket.log_level, "SlimeVRInput"))) {
		goto unlock;
	}

	// find a suitable pose input to report
	enum xrt_input_name input_name = XRT_INPUT_GENERIC_TRACKER_POSE;
	for (size_t input = 0, input_count = xdev->input_count; input < input_count; ++input) {
		if (XRT_GET_INPUT_TYPE(xdev->inputs[input].name) == XRT_INPUT_TYPE_POSE) {
			input_name = xdev->inputs[input].name;
			break;
		}
	}

	uint8_t role = 0;
	const char *role_name = "NONE";
	switch (xdev->device_type) {
	case XRT_DEVICE_TYPE_HMD: {
		role = 19;
		role_name = "HMD";
	} break;
	case XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER: {
		role = 14;
		role_name = "RIGHT_CONTROLLER";
	} break;
	case XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER: {
		role = 13;
		role_name = "LEFT_CONTROLLER";
	} break;
	case XRT_DEVICE_TYPE_ANY_HAND_CONTROLLER: {
		role = 21;
		role_name = "GENERIC_CONTROLLER";
	} break;
	default:;
	}

	// serialize announcement packet
	const uint32_t serial_len = strlen(xdev->serial), name_len = strlen(xdev->str);
	const uint8_t message_1[] = {
	    (3 << 3) | PB_WT_STRING, PROTOBUF_INT32(0),               // ProtobufMessage::tracker_added
	    (1 << 3) | PB_WT_VARINT, PROTOBUF_INT32(feeder->next_id), // TrackerAdded::tracker_id
	    (4 << 3) | PB_WT_VARINT, role,                            // TrackerAdded::tracker_role
	    (2 << 3) | PB_WT_STRING, PROTOBUF_INT32(serial_len),
	};
	static_assert(sizeof(manufacturer) - 1 < 0x80, "");
	uint8_t message_2[] = {
	    (5 << 3) | PB_WT_STRING,
	    sizeof(manufacturer) - 1,
	    // TrackerAdded::manufacturer
	    [2 + (sizeof(manufacturer) - 1)] = (3 << 3) | PB_WT_STRING,
	    PROTOBUF_INT32(name_len),
	};
	memcpy(&message_2[2], manufacturer, sizeof(manufacturer) - 1);
	uint8_t packet[sizeof(struct solarxr_ipc_message) + sizeof(message_1) + sizeof(xdev->serial) +
	               sizeof(message_2) + sizeof(xdev->str)];
	struct solarxr_ipc_message *const message = solarxr_ipc_message_start(packet, &packet[ARRAY_SIZE(packet)]);
	solarxr_ipc_message_write(message, &packet[ARRAY_SIZE(packet)], message_1, sizeof(message_1));
	solarxr_ipc_message_write(message, &packet[ARRAY_SIZE(packet)], (const uint8_t *)xdev->serial,
	                          serial_len); // TrackerAdded::tracker_serial
	solarxr_ipc_message_write(message, &packet[ARRAY_SIZE(packet)], message_2, sizeof(message_2));
	solarxr_ipc_message_write(message, &packet[ARRAY_SIZE(packet)], (const uint8_t *)xdev->str,
	                          name_len); // TrackerAdded::tracker_name
	const uint32_t packet_len = solarxr_ipc_message_end(message, &(uint8_t *){packet});
	if (packet_len == 0) {
		U_LOG_IFL_E(feeder->socket.log_level, "solarxr_ipc_message_end() failed");
		assert(false);
		goto unlock;
	}
	const uint8_t tracker_added[] = {
	    PROTOBUF_INT32(packet_len - sizeof(*message) - sizeof((uint8_t[]){0, PROTOBUF_INT32(0)})),
	};
	memcpy(&message->body[1], tracker_added, sizeof(tracker_added));

	// initialize tracked state for the device
	struct feeder_device *const device = malloc(sizeof(*device));
	if (device == NULL) {
		U_LOG_IFL_E(feeder->socket.log_level, "malloc() failed");
		goto unlock;
	}
	*device = (struct feeder_device){
	    .xdev = xdev,
	    .input_name = input_name,
	    .id = feeder->next_id++,
	    .last_status = UINT8_MAX,
	};
	u_hashmap_int_insert(feeder->devices, (uint64_t)(uintptr_t)xdev, device);

	// send the announcement
	solarxr_ipc_socket_send_raw(&feeder->socket, packet, packet_len);
	U_LOG_IFL_D(feeder->socket.log_level, "    \"%s\" [id=%" PRIu32 " serial=\"%s\" role=TrackerRole::%s]",
	            xdev->str, device->id, xdev->serial, role_name);
	result = true;

unlock:
	os_mutex_unlock(&feeder->mutex);
	return result;
}

static void
feeder_device_unlink(struct feeder_device *const device,
                     struct xrt_device *const xdev,
                     const enum u_logging_level log_level)
{
	if (device != NULL) {
		device->destroyed = true;
		U_LOG_IFL_D(log_level, "device \"%.*s\" removed", (unsigned)ARRAY_SIZE(xdev->str), xdev->str);
	}
}

void
feeder_remove_device(struct feeder *const feeder, struct xrt_device *const xdev)
{
	os_mutex_lock(&feeder->mutex);

	struct feeder_device *device = NULL;
	u_hashmap_int_find(feeder->devices, (uint64_t)(uintptr_t)xdev, (void **)&device);
	feeder_device_unlink(device, xdev, feeder->socket.log_level);

	os_mutex_unlock(&feeder->mutex);
}

static void
feeder_device_remove_cb(const uint64_t key, const void *const value, void *const userptr)
{
	struct feeder_device *const device = (struct feeder_device *)value;
	if (!device->destroyed) { // xdev pointer may be dangling if destroyed
		feeder_device_unlink(device, (struct xrt_device *)(uintptr_t)key,
		                     ((struct feeder *)userptr)->socket.log_level);
	}
}

void
feeder_clear_devices(struct feeder *const feeder)
{
	os_mutex_lock(&feeder->mutex);
	u_hashmap_int_for_each(feeder->devices, feeder_device_remove_cb, feeder);
	os_mutex_unlock(&feeder->mutex);
}

struct send_feedback_userdata
{
	struct feeder_device *to_erase;
	timepoint_ns time;
	uint8_t *packet_end, packet[0x10000];
};

static void
feeder_device_send_feedback_cb(const uint64_t key, const void *const value, void *const userptr)
{
	struct feeder_device *const device = (struct feeder_device *)value;
	struct xrt_device *const xdev = device->destroyed ? NULL : (struct xrt_device *)(uintptr_t)key;
	struct send_feedback_userdata *const data = (struct send_feedback_userdata *)userptr;
	uint8_t status = 0; // Status::DISCONNECTED
	if (xdev == NULL) {
		device->next_to_erase = data->to_erase;
		data->to_erase = device;
	} else {
		// report meaningful changes in battery level
		bool present = false, charging = false;
		float charge = 0;
		if (xdev->supported.battery_status &&
		    xrt_device_get_battery_status(xdev, &present, &charging, &charge) == XRT_SUCCESS &&
		    (charging != device->battery_charging || fabsf(charge - device->battery_charge) >= 1e-05)) {
			device->battery_charging = charging;
			device->battery_charge = charge;
			uint8_t message[] = {
			    (5 << 3) | PB_WT_STRING, 0,                          // ProtobufMessage::battery
			    (1 << 3) | PB_WT_VARINT, PROTOBUF_INT32(device->id), // Battery::tracker_id
			    (2 << 3) | PB_WT_32BIT,  PROTOBUF_FLOAT(charge),     // Battery::battery_level
			    (3 << 3) | PB_WT_VARINT, charging,                   // Battery::is_charging
			};
			message[1] = sizeof(message) - 2;
			solarxr_ipc_message_write_single(&data->packet_end, &data->packet[ARRAY_SIZE(data->packet)],
			                                 message, sizeof(message));
		}

		// report pose
		struct xrt_space_relation relation = {0};
		assert(xdev->get_tracked_pose != NULL);
		if (xrt_device_get_tracked_pose(xdev, device->input_name, data->time, &relation) == XRT_SUCCESS &&
		    (relation.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0) {
			uint8_t message[30 + 15] = {
			    (1 << 3) | PB_WT_STRING, 0,                          // ProtobufMessage::position
			    (1 << 3) | PB_WT_VARINT, PROTOBUF_INT32(device->id), // Position::tracker_id
			    (5 << 3) | PB_WT_32BIT,  PROTOBUF_FLOAT(relation.pose.orientation.x), // Position::qx
			    (6 << 3) | PB_WT_32BIT,  PROTOBUF_FLOAT(relation.pose.orientation.y), // Position::qy
			    (7 << 3) | PB_WT_32BIT,  PROTOBUF_FLOAT(relation.pose.orientation.z), // Position::qz
			    (8 << 3) | PB_WT_32BIT,  PROTOBUF_FLOAT(relation.pose.orientation.w), // Position::qw
			    (9 << 3) | PB_WT_VARINT, 3, // Position::data_source = DataSource::FULL
			};
			uint32_t message_len = sizeof(message);
			if ((relation.relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) != 0) {
				const uint8_t position[15] = {
				    (2 << 3) | PB_WT_32BIT, PROTOBUF_FLOAT(relation.pose.position.x), // Position::x
				    (3 << 3) | PB_WT_32BIT, PROTOBUF_FLOAT(relation.pose.position.y), // Position::y
				    (4 << 3) | PB_WT_32BIT, PROTOBUF_FLOAT(relation.pose.position.z), // Position::z
				};
				memcpy(&message[sizeof(message) - sizeof(position)], position, sizeof(position));
			} else {
				message_len -= 15;
			}
			message[1] = message_len - 2;
			solarxr_ipc_message_write_single(&data->packet_end, &data->packet[ARRAY_SIZE(data->packet)],
			                                 message, message_len);
			if ((relation.relation_flags & XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT) != 0) {
				status = 1; // Status::OK
			}
		}
	}

	// report tracking status
	if (status != device->last_status) {
		device->last_status = status;
		uint8_t message[] = {
		    (4 << 3) | PB_WT_STRING, 0,                          // ProtobufMessage::tracker_status
		    (1 << 3) | PB_WT_VARINT, PROTOBUF_INT32(device->id), // TrackerStatus::tracker_id
		    (2 << 3) | PB_WT_VARINT, status,                     // TrackerStatus::status
		};
		message[1] = sizeof(message) - 2;
		solarxr_ipc_message_write_single(&data->packet_end, &data->packet[ARRAY_SIZE(data->packet)], message,
		                                 sizeof(message));
	}
}

void
feeder_send_feedback(struct feeder *const feeder)
{
	if (!solarxr_ipc_socket_is_connected(&feeder->socket)) {
		return;
	}

	// check for errors on the socket
	if (!solarxr_ipc_socket_wait_timeout(&feeder->socket, 0)) {
		U_LOG_IFL_E(feeder->socket.log_level, "connection lost");
		solarxr_ipc_socket_destroy(&feeder->socket);
		return;
	}

	os_mutex_lock(&feeder->mutex);

	struct send_feedback_userdata data;
	data.to_erase = NULL;
	data.time = os_monotonic_get_ns();
	data.packet_end = data.packet;
	u_hashmap_int_for_each(feeder->devices, feeder_device_send_feedback_cb, &data);

	// flush queued messages to socket
	if (data.packet_end != data.packet) {
		solarxr_ipc_socket_send_raw(&feeder->socket, data.packet, data.packet_end - data.packet);
	}

	while (data.to_erase != NULL) {
		struct feeder_device *const entry = data.to_erase;
		data.to_erase = entry->next_to_erase;
		u_hashmap_int_erase(feeder->devices, (uint64_t)(uintptr_t)entry->xdev);
		free(entry);
	}

	os_mutex_unlock(&feeder->mutex);
}

bool
feeder_init(struct feeder *const feeder, const enum u_logging_level log_level)
{
	feeder->devices = NULL;
	solarxr_ipc_socket_init(&feeder->socket, log_level);
	if (os_mutex_init(&feeder->mutex) != 0) {
		return false;
	}
	u_hashmap_int_create(&feeder->devices);
	return true;
}

static void
feeder_device_free_cb(const uint64_t key, const void *const value, void *const userptr)
{
	(void)key;
	(void)userptr;
	free((void *)value);
}

void
feeder_fini(struct feeder *const feeder)
{
	if (feeder->devices != NULL) {
		feeder_clear_devices(feeder);
		feeder_send_feedback(feeder);
		u_hashmap_int_for_each(feeder->devices, feeder_device_free_cb, NULL);
		u_hashmap_int_destroy(&feeder->devices);
	}
	os_mutex_destroy(&feeder->mutex);
	solarxr_ipc_socket_destroy(&feeder->socket);
}
