// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Radio state machine functions for the Oculus Rift.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift
 */

#include "xrt/xrt_byte_order.h"

#include "util/u_device.h"
#include "util/u_var.h"
#include "util/u_file.h"

#include "rift_internal.h"
#include "rift_radio.h"
#include "rift_usb.h"

#include <errno.h>


static void
rift_update_input(struct xrt_device *device, size_t index, union xrt_input_value value, int64_t now)
{
	device->inputs[index].value = value;
	device->inputs[index].timestamp = now;
}

/*
 * Rift Touch Controller device functions
 */

static xrt_result_t
rift_touch_controller_get_tracked_pose(struct xrt_device *xdev,
                                       const enum xrt_input_name name,
                                       int64_t at_timestamp_ns,
                                       struct xrt_space_relation *out_relation)
{
	struct rift_touch_controller *controller = (struct rift_touch_controller *)xdev;

	switch (name) {
	case XRT_INPUT_TOUCH_GRIP_POSE:
	case XRT_INPUT_TOUCH_AIM_POSE: {
		struct xrt_space_relation relation = XRT_SPACE_RELATION_ZERO;

		if (imu_fusion_get_prediction(controller->input.imu_fusion, (uint64_t)at_timestamp_ns,
		                              &relation.pose.orientation, &relation.angular_velocity) == 0) {
			relation.relation_flags = XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
			                          XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
			                          XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT;

			(*out_relation) = relation;
		}
		break;
	}
	default: return XRT_ERROR_INPUT_UNSUPPORTED;
	}

	return XRT_SUCCESS;
}

static void
rift_touch_controller_destroy(struct xrt_device *xdev)
{}

static xrt_result_t
rift_touch_controller_update_inputs(struct xrt_device *xdev)
{
	struct rift_touch_controller *controller = rift_touch_controller(xdev);

	os_mutex_lock(&controller->input.mutex);
	struct rift_touch_controller_input_state input_state = controller->input.state;
	os_mutex_unlock(&controller->input.mutex);

	uint64_t update_ns = controller->input.device_local_ns;

	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_A_CLICK,
	                  (union xrt_input_value){.boolean = input_state.buttons & RIFT_TOUCH_CONTROLLER_BUTTON_A},
	                  update_ns);
	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_B_CLICK,
	                  (union xrt_input_value){.boolean = input_state.buttons & RIFT_TOUCH_CONTROLLER_BUTTON_B},
	                  update_ns);
	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_SYSTEM_CLICK,
	                  (union xrt_input_value){.boolean = input_state.buttons & RIFT_TOUCH_CONTROLLER_BUTTON_MENU},
	                  update_ns);
	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_THUMBSTICK_CLICK,
	                  (union xrt_input_value){.boolean = input_state.buttons & RIFT_TOUCH_CONTROLLER_BUTTON_STICK},
	                  update_ns);

	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_THUMBSTICK,
	                  (union xrt_input_value){.vec2 = {CLAMP(input_state.stick.x, -1.0f, 1.0f),
	                                                   CLAMP(input_state.stick.y, -1.0f, 1.0f)}},
	                  update_ns);

	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_TRIGGER_VALUE,
	                  (union xrt_input_value){.vec1 = {CLAMP(input_state.trigger, 0.0f, 1.0f)}}, update_ns);
	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_SQUEEZE_VALUE,
	                  (union xrt_input_value){.vec1 = {CLAMP(input_state.grip, 0.0f, 1.0f)}}, update_ns);

	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_A_TOUCH,
	                  (union xrt_input_value){.boolean = input_state.cap_a_x >= 1}, update_ns);
	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_B_TOUCH,
	                  (union xrt_input_value){.boolean = input_state.cap_b_y >= 1}, update_ns);
	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_TRIGGER_TOUCH,
	                  (union xrt_input_value){.boolean = input_state.cap_trigger >= 1}, update_ns);
	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_THUMBSTICK_TOUCH,
	                  (union xrt_input_value){.boolean = input_state.cap_stick >= 1}, update_ns);
	rift_update_input(&controller->base, RIFT_TOUCH_CONTROLLER_INPUT_THUMBREST_TOUCH,
	                  (union xrt_input_value){.boolean = input_state.cap_thumbrest >= 1}, update_ns);

	return XRT_SUCCESS;
}

static xrt_result_t
rift_touch_controller_set_output(struct xrt_device *xdev,
                                 enum xrt_output_name name,
                                 const struct xrt_output_value *value)
{
	struct rift_touch_controller *controller = rift_touch_controller(xdev);

	if (name != XRT_OUTPUT_NAME_TOUCH_HAPTIC) {
		return XRT_ERROR_OUTPUT_UNSUPPORTED;
	}

	if (value->type != XRT_OUTPUT_VALUE_TYPE_VIBRATION) {
		return XRT_ERROR_OUTPUT_UNSUPPORTED;
	}

	timepoint_ns now = os_monotonic_get_ns();
	// Set the minimum time to 5ms to help guarantee we actually get a pulse of haptics
	timepoint_ns end_time = now + MAX(value->vibration.duration_ns, U_TIME_1MS_IN_NS * 5);

	// if it's higher than the middle of the two possible frequencies, pick the high frequency rumble
	bool high_freq = value->vibration.frequency > ((320.0 + 160.0) / 2.0);

	os_mutex_lock(&controller->input.mutex);
	controller->input.haptic.end_time_ns = end_time;
	controller->input.haptic.high_freq = high_freq;
	controller->input.haptic.amplitude = CLAMP(value->vibration.amplitude, 0.0f, 1.0f);
	os_mutex_unlock(&controller->input.mutex);

	return XRT_SUCCESS;
}

static xrt_result_t
rift_touch_controller_get_battery_status(struct xrt_device *xdev,
                                         bool *out_present,
                                         bool *out_charging,
                                         float *out_charge)
{
	struct rift_touch_controller *controller = rift_touch_controller(xdev);

	int32_t battery_status = xrt_atomic_s32_load(&controller->input.battery_status);

	*out_charging = false;

	if (battery_status == -1) {
		*out_present = false;
		return XRT_SUCCESS;
	}

	*out_present = true;
	*out_charge = (float)battery_status / 100.0f;

	return XRT_SUCCESS;
}

void
rift_touch_controller_node_break_apart(struct xrt_frame_node *node)
{}

void
rift_touch_controller_node_destroy(struct xrt_frame_node *node)
{
	struct rift_touch_controller *controller = rift_touch_controller_from_node(node);

	u_var_remove_root(controller);

	if (controller->input.mutex_created) {
		os_mutex_destroy(&controller->input.mutex);
	}

	if (controller->radio_data.calibration_body_json) {
		free(controller->radio_data.calibration_body_json);
	}

	if (controller->input.calibration.leds) {
		free(controller->input.calibration.leds);
	}

	if (controller->input.clock_tracker) {
		m_clock_windowed_skew_tracker_destroy(controller->input.clock_tracker);
	}

	if (controller->input.imu_fusion) {
		imu_fusion_destroy(controller->input.imu_fusion);
	}

	u_device_free(&controller->base);
}

/*
 *
 * Rift Remote device functions
 *
 */

static xrt_result_t
rift_remote_get_tracked_pose(struct xrt_device *xdev,
                             const enum xrt_input_name name,
                             int64_t at_timestamp_ns,
                             struct xrt_space_relation *out_relation)
{
	// The remote has no tracked poses
	return XRT_ERROR_INPUT_UNSUPPORTED;
}

static void
rift_remote_destroy(struct xrt_device *xdev)
{
	// stub
}

static xrt_result_t
rift_remote_update_inputs(struct xrt_device *xdev)
{
	struct rift_remote *remote = rift_remote(xdev);

	uint64_t now = os_monotonic_get_ns();

	uint16_t buttons = xrt_atomic_s32_load(&remote->buttons);

	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_DPAD_UP,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_DPAD_UP}, now);
	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_DPAD_DOWN,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_DPAD_DOWN}, now);
	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_DPAD_LEFT,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_DPAD_LEFT}, now);
	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_DPAD_RIGHT,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_DPAD_RIGHT}, now);
	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_SELECT,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_SELECT}, now);
	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_VOLUME_UP,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_VOLUME_UP}, now);
	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_VOLUME_DOWN,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_VOLUME_DOWN}, now);
	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_BACK,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_BACK}, now);
	rift_update_input(&remote->base, RIFT_REMOTE_INPUT_OCULUS,
	                  (union xrt_input_value){.boolean = buttons & RIFT_REMOTE_BUTTON_MASK_OCULUS}, now);

	return XRT_SUCCESS;
}

static void
rift_remote_node_break_apart(struct xrt_frame_node *node)
{}

static void
rift_remote_node_destroy(struct xrt_frame_node *node)
{
	struct rift_remote *remote = rift_remote_from_node(node);

	u_var_remove_root(remote);

	u_device_free(&remote->base);
}

/*
 * Implementation functions
 */

static struct rift_remote *
rift_remote_create(struct rift_hmd *hmd, struct xrt_frame_context *xfctx)
{
	struct rift_remote *remote =
	    U_DEVICE_ALLOCATE(struct rift_remote, U_DEVICE_ALLOC_TRACKING_NONE, RIFT_REMOTE_INPUT_COUNT, 1);
	if (remote == NULL) {
		HMD_ERROR(hmd, "Failed to allocate remote");
		return NULL;
	}

	remote->node.break_apart = rift_remote_node_break_apart;
	remote->node.destroy = rift_remote_node_destroy;

	snprintf(remote->base.str, XRT_DEVICE_NAME_LEN, "Oculus Rift Remote");
	remote->base.device_type = XRT_DEVICE_TYPE_GAMEPAD;
	remote->base.name = XRT_DEVICE_RIFT_REMOTE;

#define SET_INPUT(NAME)                                                                                                \
	do {                                                                                                           \
		remote->base.inputs[RIFT_REMOTE_INPUT_##NAME].name = XRT_INPUT_RIFT_REMOTE_##NAME##_CLICK;             \
	} while (0)
	SET_INPUT(DPAD_UP);
	SET_INPUT(DPAD_DOWN);
	SET_INPUT(DPAD_LEFT);
	SET_INPUT(DPAD_RIGHT);
	SET_INPUT(SELECT);
	SET_INPUT(VOLUME_UP);
	SET_INPUT(VOLUME_DOWN);
	SET_INPUT(BACK);
	SET_INPUT(OCULUS);
#undef SET_INPUT

	u_device_populate_function_pointers(&remote->base, rift_remote_get_tracked_pose, rift_remote_destroy);
	remote->base.update_inputs = rift_remote_update_inputs;

	u_var_add_root(remote, "Rift Remote", true);
	u_var_add_ro_u32(remote, (uint32_t *)&remote->buttons, "buttons");

	xrt_frame_context_add(xfctx, &remote->node);

	return remote;
}

static struct rift_touch_controller *
rift_touch_controller_create(struct rift_hmd *hmd,
                             enum rift_radio_device_type device_type,
                             struct xrt_frame_context *xfctx)
{
	int result;

	struct rift_touch_controller *controller = U_DEVICE_ALLOCATE(
	    struct rift_touch_controller, U_DEVICE_ALLOC_NO_FLAGS, RIFT_TOUCH_CONTROLLER_INPUT_COUNT, 1);
	if (controller == NULL) {
		HMD_ERROR(hmd, "Failed to allocate touch controller");
		return NULL;
	}

	controller->node.break_apart = rift_touch_controller_node_break_apart;
	controller->node.destroy = rift_touch_controller_node_destroy;

	controller->device_type = device_type;
	controller->hmd = hmd;

	controller->base.tracking_origin = hmd->base.tracking_origin;

	snprintf(controller->base.str, XRT_DEVICE_NAME_LEN, "Oculus Touch (Unknown)");
	switch (device_type) {
	case RIFT_RADIO_DEVICE_LEFT_TOUCH:
		snprintf(controller->base.str, XRT_DEVICE_NAME_LEN, "Oculus Touch (Left)");
		controller->base.device_type = XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER;
		break;
	case RIFT_RADIO_DEVICE_RIGHT_TOUCH:
		snprintf(controller->base.str, XRT_DEVICE_NAME_LEN, "Oculus Touch (Right)");
		controller->base.device_type = XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER;
		break;
	case RIFT_RADIO_DEVICE_TRACKED_OBJECT:
		snprintf(controller->base.str, XRT_DEVICE_NAME_LEN, "Oculus Touch (Tracked Object)");
		controller->base.device_type = XRT_DEVICE_TYPE_ANY_HAND_CONTROLLER;
		break;
	default: break; return controller;
	}
	controller->base.name = XRT_DEVICE_TOUCH_CONTROLLER_RIFT_CV1;

#define SET_INPUT(NAME, ACTIVE)                                                                                        \
	do {                                                                                                           \
		if (ACTIVE) {                                                                                          \
			controller->base.inputs[RIFT_TOUCH_CONTROLLER_INPUT_##NAME].name = XRT_INPUT_TOUCH_##NAME;     \
			controller->base.inputs[RIFT_TOUCH_CONTROLLER_INPUT_##NAME].active = ACTIVE;                   \
		}                                                                                                      \
	} while (0)
	SET_INPUT(X_CLICK, device_type == RIFT_RADIO_DEVICE_LEFT_TOUCH);
	SET_INPUT(X_TOUCH, device_type == RIFT_RADIO_DEVICE_LEFT_TOUCH);
	SET_INPUT(Y_CLICK, device_type == RIFT_RADIO_DEVICE_LEFT_TOUCH);
	SET_INPUT(Y_TOUCH, device_type == RIFT_RADIO_DEVICE_LEFT_TOUCH);
	SET_INPUT(MENU_CLICK, device_type == RIFT_RADIO_DEVICE_LEFT_TOUCH);

	SET_INPUT(A_CLICK, device_type == RIFT_RADIO_DEVICE_RIGHT_TOUCH);
	SET_INPUT(A_TOUCH, device_type == RIFT_RADIO_DEVICE_RIGHT_TOUCH);
	SET_INPUT(B_CLICK, device_type == RIFT_RADIO_DEVICE_RIGHT_TOUCH);
	SET_INPUT(B_TOUCH, device_type == RIFT_RADIO_DEVICE_RIGHT_TOUCH);
	SET_INPUT(SYSTEM_CLICK, device_type == RIFT_RADIO_DEVICE_RIGHT_TOUCH);

	SET_INPUT(SQUEEZE_VALUE, true);
	SET_INPUT(TRIGGER_TOUCH, true);
	SET_INPUT(TRIGGER_VALUE, true);
	SET_INPUT(THUMBSTICK_CLICK, true);
	SET_INPUT(THUMBSTICK_TOUCH, true);
	SET_INPUT(THUMBSTICK, true);
	SET_INPUT(THUMBREST_TOUCH, true);
	SET_INPUT(GRIP_POSE, true);
	SET_INPUT(AIM_POSE, true);
	SET_INPUT(TRIGGER_PROXIMITY, true);
	SET_INPUT(THUMB_PROXIMITY, true);
#undef SET_INPUT

	controller->base.outputs[0].name = XRT_OUTPUT_NAME_TOUCH_HAPTIC;

	u_device_populate_function_pointers(&controller->base, rift_touch_controller_get_tracked_pose,
	                                    rift_touch_controller_destroy);
	controller->base.update_inputs = rift_touch_controller_update_inputs;
	controller->base.set_output = rift_touch_controller_set_output;
	controller->base.get_battery_status = rift_touch_controller_get_battery_status;

	controller->base.supported.battery_status = true;
	controller->base.supported.orientation_tracking = true;

	result = os_mutex_init(&controller->input.mutex);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to init touch controller input mutex");
		rift_touch_controller_node_destroy(&controller->node);
		return NULL;
	}
	controller->input.mutex_created = true;

	controller->input.clock_tracker = m_clock_windowed_skew_tracker_alloc(64);
	if (controller->input.clock_tracker == NULL) {
		HMD_ERROR(hmd, "Failed to allocate touch controller clock tracker");
		rift_touch_controller_node_destroy(&controller->node);
		return NULL;
	}

	controller->input.imu_fusion = imu_fusion_create();
	if (controller->input.imu_fusion == NULL) {
		HMD_ERROR(hmd, "Failed to create touch controller IMU fusion");
		rift_touch_controller_node_destroy(&controller->node);
		return NULL;
	}

	// -1 means unknown battery level/no data
	xrt_atomic_s32_store(&controller->input.battery_status, -1);

	u_var_add_root(controller, "Rift Touch Controller", true);
	u_var_add_ro_i64_ns(controller, &controller->input.device_remote_ns, "Device Remote Timestamp");
	u_var_add_ro_i64_ns(controller, &controller->input.device_local_ns, "Device Local Timestamp");
	u_var_add_bool(controller, &controller->input.calibration_read, "Read Calibration");

	{
		u_var_add_gui_header(controller, NULL, "Input State");
		u_var_add_ro_vec3_f64(controller, &controller->input.last_imu_sample.accel_m_s2, "Last Accel (m/s²)");
		u_var_add_ro_vec3_f64(controller, &controller->input.last_imu_sample.gyro_rad_secs,
		                      "Last Gyro (rad/s)");
		u_var_add_ro_i32(controller, (int32_t *)&controller->input.battery_status, "battery_status");
		u_var_add_u8(controller, &controller->input.state.buttons, "buttons");
		u_var_add_f32(controller, &controller->input.state.trigger, "trigger");
		u_var_add_f32(controller, &controller->input.state.grip, "grip");
		u_var_add_f32(controller, &controller->input.state.stick.x, "stick.x");
		u_var_add_f32(controller, &controller->input.state.stick.y, "stick.y");
		u_var_add_u8(controller, &controller->input.state.haptic_counter, "haptic_counter");
		u_var_add_f32(controller, &controller->input.state.cap_stick, "cap_stick");
		u_var_add_f32(controller, &controller->input.state.cap_b_y, "cap_b_y");
		u_var_add_f32(controller, &controller->input.state.cap_a_x, "cap_a_x");
		u_var_add_f32(controller, &controller->input.state.cap_trigger, "cap_trigger");
		u_var_add_f32(controller, &controller->input.state.cap_thumbrest, "cap_thumbrest");
	}

	xrt_frame_context_add(xfctx, &controller->node);

	return controller;
}

static int
rift_radio_read_device_serial_async_locked(struct rift_hmd *hmd,
                                           enum rift_radio_device_type device_type,
                                           char serial[SERIAL_NUMBER_LENGTH + 1],
                                           bool *serial_valid)
{
	// Radio is already busy
	if (hmd->radio_state.current_command != RIFT_RADIO_COMMAND_NONE) {
		return -EBUSY;
	}

	HMD_DEBUG(hmd, "Reading serial for device type %d", device_type);

	int result = rift_send_radio_cmd(
	    hmd, true,
	    &(struct rift_radio_cmd_report){.a = 0x03, .b = RIFT_RADIO_READ_CMD_SERIAL, .c = (uint8_t)device_type});
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to send read serial command");
		return result;
	}

	hmd->radio_state.current_command = RIFT_RADIO_COMMAND_READ_SERIAL;
	hmd->radio_state.command_data.read_serial = (struct rift_radio_command_data_read_serial){
	    .serial = serial,
	    .serial_valid = serial_valid,
	};

	return 0;
}

static int
rift_radio_read_device_flash_async_locked(struct rift_hmd *hmd,
                                          void *user_data,
                                          enum rift_radio_device_type device_type,
                                          uint16_t offset,
                                          uint16_t length,
                                          uint8_t *buffer,
                                          flash_read_callback_t callback)
{
	// Radio is already busy
	if (hmd->radio_state.current_command != RIFT_RADIO_COMMAND_NONE) {
		return -EBUSY;
	}

	HMD_DEBUG(hmd, "Queueing flash read at %d of length %d for device %d", offset, length, device_type);

	int result =
	    rift_radio_send_data_read_cmd(hmd, &(struct rift_radio_data_read_cmd){.offset = offset, .length = length});
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to send data read command to radio, reason %d", result);
		return result;
	}

	result = rift_send_radio_cmd(hmd, true,
	                             &(struct rift_radio_cmd_report){
	                                 .a = 0x03, .b = RIFT_RADIO_READ_CMD_FLASH_CONTROL, .c = (uint8_t)device_type});
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to send radio command report to HMD, reason %d", result);
		return result;
	}

	hmd->radio_state.current_command = RIFT_RADIO_COMMAND_READ_FLASH;
	hmd->radio_state.command_data.read_flash = (struct rift_radio_command_data_read_flash){
	    .user_data = user_data,
	    .address = offset,
	    .length = length,
	    .buffer = buffer,
	    .read_callback = callback,
	};

	return 0;
}

static int
rift_touch_controller_calibration_body_read_callback(void *user_data, uint16_t address, uint16_t length);

static int
rift_touch_controller_calibration_read_body_async_locked(struct rift_touch_controller *controller, uint16_t address)
{
	return rift_radio_read_device_flash_async_locked(
	    controller->hmd, controller, controller->device_type, address + CALIBRATION_BODY_BYTE_OFFSET,
	    MIN(controller->radio_data.calibration_body_json_length - address, CALIBRATION_BODY_BYTE_CHUNK_LENGTH),
	    controller->radio_data.calibration_data_buffer, rift_touch_controller_calibration_body_read_callback);
}

// strlen("touchXX_.cal") + NULL == 13
#define RIFT_TOUCH_CONTROLLER_CALIBRATION_FILENAME_MAX_LEN (XRT_DEVICE_NAME_LEN + 13)

static void
rift_touch_controller_get_calibration_filename(struct rift_touch_controller *controller, char *out_filename)
{
	snprintf(out_filename, RIFT_TOUCH_CONTROLLER_CALIBRATION_FILENAME_MAX_LEN, "touch%d_%s.cal",
	         controller->device_type, controller->base.serial);
}

static int
rift_touch_controller_calibration_body_read_callback(void *user_data, uint16_t address, uint16_t length)
{
	int result;
	struct rift_touch_controller *controller = user_data;

	const uint8_t *chunk_buffer = controller->radio_data.calibration_data_buffer;

	if (address == CALIBRATION_HEADER_BYTE_OFFSET) {
		assert(length == CALIBRATION_HEADER_BYTE_LENGTH);

		if (chunk_buffer[0] != 1 || chunk_buffer[1] != 0) {
			HMD_ERROR(controller->hmd, "Got bad flash header");
			return -1;
		}

		__le16 length_raw;
		memcpy(&length_raw, chunk_buffer + 2, sizeof(length_raw));
		HMD_DEBUG(controller->hmd, "Calibration body length: %d", length_raw);

		uint16_t length = __le16_to_cpu(length_raw);

		controller->radio_data.calibration_body_json = U_TYPED_ARRAY_CALLOC(uint8_t, length + 1);
		if (controller->radio_data.calibration_body_json == NULL) {
			HMD_ERROR(controller->hmd, "Failed to allocate calibration body JSON");
			return -1;
		}
		// null terminate
		controller->radio_data.calibration_body_json[length] = '\0';
		controller->radio_data.calibration_body_json_length = length;

		result = rift_touch_controller_calibration_read_body_async_locked(controller, 0);
		assert(result != -EBUSY); // radio cannot be busy at this point
		if (result < 0) {
			return result;
		}

		return 0;
	}

	uint16_t body_offset = address - CALIBRATION_BODY_BYTE_OFFSET;

	assert(body_offset + length <= controller->radio_data.calibration_body_json_length);
	assert(length <= CALIBRATION_BODY_BYTE_CHUNK_LENGTH);

	memcpy(controller->radio_data.calibration_body_json + body_offset, chunk_buffer, length);

	if (body_offset + length < controller->radio_data.calibration_body_json_length) {
		result = rift_touch_controller_calibration_read_body_async_locked(controller, body_offset + length);
		assert(result != -EBUSY); // radio cannot be busy at this point
		if (result < 0) {
			return result;
		}

		return 0;
	}

	// Finished reading calibration body
	HMD_INFO(controller->hmd, "Finished reading touch controller calibration JSON body for controller %d",
	         controller->device_type);
#if 0
	printf("%s\n", controller->radio_data.calibration_body_json);
#endif

	os_mutex_lock(&controller->input.mutex);
	if (!rift_touch_calibration_parse((const char *)controller->radio_data.calibration_body_json,
	                                  controller->radio_data.calibration_body_json_length,
	                                  &controller->input.calibration)) {
		HMD_ERROR(controller->hmd, "Failed to parse touch controller calibration JSON");
		os_mutex_unlock(&controller->input.mutex);
		return -1;
	}

	controller->input.calibration_read = true;
	os_mutex_unlock(&controller->input.mutex);

	char calibration_filename[RIFT_TOUCH_CONTROLLER_CALIBRATION_FILENAME_MAX_LEN];
	rift_touch_controller_get_calibration_filename(controller, calibration_filename);

	FILE *calibration_file = u_file_open_file_in_config_dir_subpath(RIFT_CONFIG_SUBDIR, calibration_filename, "wb");
	if (calibration_file == NULL) {
		HMD_ERROR(controller->hmd, "Failed to open calibration file for writing");
		return 0;
	}

	if (fwrite(controller->radio_data.calibration_hash, 1, CALIBRATION_HASH_BYTE_LENGTH, calibration_file) !=
	    CALIBRATION_HASH_BYTE_LENGTH) {
		HMD_ERROR(controller->hmd, "Failed to write calibration hash to file");
		fclose(calibration_file);
		return 0;
	}

	size_t to_write = controller->radio_data.calibration_body_json_length;
	while (to_write > 0) {
		size_t written = fwrite(controller->radio_data.calibration_body_json +
		                            controller->radio_data.calibration_body_json_length - to_write,
		                        1, to_write, calibration_file);
		if (written == 0) {
			HMD_ERROR(controller->hmd, "Failed to write calibration body to file");
			fclose(calibration_file);
			return 0;
		}
		to_write -= written;
	}
	fclose(calibration_file);

	HMD_DEBUG(controller->hmd, "Wrote calibration data to %s", calibration_filename);

	return 0;
}

static int
rift_touch_controller_calibration_hash_read_callback(void *user_data, uint16_t address, uint16_t length)
{
	assert(address == CALIBRATION_HASH_BYTE_OFFSET);
	if (length != CALIBRATION_HASH_BYTE_LENGTH) {
		return -EINVAL;
	}

	struct rift_touch_controller *controller = user_data;

	assert(controller->hmd->radio_state.current_command == RIFT_RADIO_COMMAND_NONE);

	char calibration_filename[RIFT_TOUCH_CONTROLLER_CALIBRATION_FILENAME_MAX_LEN];
	rift_touch_controller_get_calibration_filename(controller, calibration_filename);

	FILE *calibration_file = u_file_open_file_in_config_dir_subpath(RIFT_CONFIG_SUBDIR, calibration_filename, "r");
	if (calibration_file != NULL) {
		size_t file_size;
		char *calibration_data = u_file_read_content(calibration_file, &file_size);
		fclose(calibration_file);

		if (memcmp(calibration_data, controller->radio_data.calibration_hash, CALIBRATION_HASH_BYTE_LENGTH) ==
		    0) {
			HMD_INFO(controller->hmd,
			         "Calibration hash matches cached file for controller %d, loading from disk",
			         controller->device_type);
			if (rift_touch_calibration_parse(calibration_data + CALIBRATION_HASH_BYTE_LENGTH,
			                                 file_size - CALIBRATION_HASH_BYTE_LENGTH,
			                                 &controller->input.calibration)) {
				controller->input.calibration_read = true;
				free(calibration_data);
				return 0;
			} else {
				HMD_ERROR(controller->hmd,
				          "Failed to parse touch controller calibration JSON from disk, reading from "
				          "device flash");
				free(calibration_data);
				// fall through to reading from device flash
			}
		} else {
			HMD_INFO(
			    controller->hmd,
			    "Calibration hash does not match cached file for controller %d, reading from device flash",
			    controller->device_type);
			free(calibration_data);
		}
	}

	int result = rift_radio_read_device_flash_async_locked(
	    controller->hmd, controller, controller->device_type, CALIBRATION_HEADER_BYTE_OFFSET,
	    CALIBRATION_HEADER_BYTE_LENGTH, controller->radio_data.calibration_data_buffer,
	    rift_touch_controller_calibration_body_read_callback);
	assert(result != -EBUSY); // radio cannot be busy at this point
	if (result < 0) {
		return result;
	}

	return 0;
}

static int
rift_radio_read_touch_calibration_hash_async_locked(struct rift_hmd *hmd, struct rift_touch_controller *controller)
{
	int result;

	result = rift_radio_read_device_flash_async_locked(
	    hmd, controller, controller->device_type, CALIBRATION_HASH_BYTE_OFFSET, CALIBRATION_HASH_BYTE_LENGTH,
	    controller->radio_data.calibration_hash, rift_touch_controller_calibration_hash_read_callback);
	if (result < 0) {
		return result;
	}

	return 0;
}

bool
rift_radio_read_device_serial(struct rift_hmd *hmd,
                              enum rift_radio_device_type device_type,
                              char *serial,
                              bool *serial_valid)
{
	// Early out if read.
	if (*serial_valid) {
		return true;
	}

	int result = rift_radio_read_device_serial_async_locked(hmd, device_type, serial, serial_valid);

	if (result == -EBUSY) {
		// try again later
		return false;
	}

	if (result < 0) {
		HMD_ERROR(hmd, "Failed to start reading serial for device type %d, reason %d", device_type, result);
		return false;
	}

	return true;
}

static bool
rift_radio_read_controller_calibration(struct rift_hmd *hmd, struct rift_touch_controller *controller)
{
	// Early out if read.
	if (controller->input.calibration_read) {
		return true;
	}

	int result = rift_radio_read_touch_calibration_hash_async_locked(hmd, controller);

	if (result == -EBUSY) {
		// try again later
		return false;
	}

	if (result < 0) {
		HMD_ERROR(hmd, "Failed to start reading calibration hash for device type %d, reason %d",
		          controller->device_type, result);
		return false;
	}

	return true;
}

static void
rift_touch_controller_handle_radio_input_report(struct rift_hmd *hmd,
                                                struct rift_touch_controller *controller,
                                                struct rift_radio_report_message message,
                                                int64_t receive_ns)
{
	struct rift_touch_controller_calibration *c = &controller->input.calibration;

	uint8_t tgs[5];
	memcpy(tgs, message.touch.touch_grip_stick_state, sizeof tgs);
	uint16_t raw_trigger = tgs[0] | ((tgs[1] & 0x03) << 8);
	uint16_t raw_grip = ((tgs[1] & 0xfc) >> 2) | ((tgs[2] & 0xf) << 6);
	uint16_t raw_stick_x = ((tgs[2] & 0xf0) >> 4) | ((tgs[3] & 0x3f) << 4);
	uint16_t raw_stick_y = ((tgs[3] & 0xc0) >> 6) | ((tgs[4] & 0xff) << 2);

	float trigger = rift_min_mid_max_range_to_float(c->trigger_range, (float)raw_trigger);
	float grip = rift_min_mid_max_range_to_float(c->middle_range, (float)raw_grip);

	struct xrt_vec2 joy;
	if (raw_stick_x >= c->joy_x_dead[0] && raw_stick_x <= c->joy_x_dead[1] && raw_stick_y >= c->joy_y_dead[0] &&
	    raw_stick_y <= c->joy_y_dead[1]) {
		joy.x = 0.0f;
		joy.y = 0.0f;
	} else {
		joy.x =
		    ((float)raw_stick_x - c->joy_x_range[0]) / (c->joy_x_range[1] - c->joy_x_range[0]) * 2.0f - 1.0f;
		joy.y =
		    ((float)raw_stick_y - c->joy_y_range[0]) / (c->joy_y_range[1] - c->joy_y_range[0]) * 2.0f - 1.0f;
	}

	struct xrt_vec3 raw_accel = {
	    MATH_GRAVITY_M_S2 / 2048.0 * (float)message.touch.accel[0],
	    MATH_GRAVITY_M_S2 / 2048.0 * (float)message.touch.accel[1],
	    MATH_GRAVITY_M_S2 / 2048.0 * (float)message.touch.accel[2],
	};

	// Gyro is MPU 6500, configured for 2000°/s.
	// The datasheet has 16.4 LSB/°/s, but I'm using
	// 32768 / 2000 = 16.384 here because that actually
	// yields a 2000°/s full range, then converting to
	// radians for the fusion.
	struct xrt_vec3 raw_gyro = {
	    message.touch.gyro[0] / (16.384 * 180.0) * M_PI,
	    message.touch.gyro[1] / (16.384 * 180.0) * M_PI,
	    message.touch.gyro[2] / (16.384 * 180.0) * M_PI,
	};
	struct xrt_vec3 gyro;
	struct xrt_vec3 accel;

	// For controllers, we apply the rotation matrix first,
	// and then add the provided factory offsets
	math_matrix_3x3_transform_vec3((struct xrt_matrix_3x3 *)&c->gyro_calibration, &raw_gyro, &gyro);
	math_vec3_accum(&c->gyro_offset, &gyro);

	math_matrix_3x3_transform_vec3((struct xrt_matrix_3x3 *)&c->accel_calibration, &raw_accel, &accel);
	math_vec3_accum(&c->accel_offset, &accel);

	controller->input.device_remote_ns +=
	    (timepoint_ns)(message.touch.timestamp - controller->input.last_device_remote_us) * U_TIME_1US_IN_NS;
	controller->input.last_device_remote_us = message.touch.timestamp;

	os_mutex_lock(&controller->input.mutex);
	m_clock_windowed_skew_tracker_push(       //
	    controller->input.clock_tracker,      //
	    receive_ns - RIFT_RADIO_LATENCY_BIAS, //
	    controller->input.device_remote_ns);  //

	if (!m_clock_windowed_skew_tracker_to_local(controller->input.clock_tracker, controller->input.device_remote_ns,
	                                            &controller->input.device_local_ns)) {
		HMD_WARN(hmd, "Failed to convert device remote time to local time");
		os_mutex_unlock(&controller->input.mutex);
		return;
	}

	controller->input.state.buttons = message.touch.buttons & 0x0F;
	controller->input.state.trigger = trigger;
	controller->input.state.grip = grip;
	controller->input.state.stick = joy;

	switch (message.touch.adc_channel) {
	case RIFT_TOUCH_CONTROLLER_ADC_STICK:
		controller->input.state.cap_stick = rift_min_mid_max_cap(c, 0, (float)message.touch.adc_value);
		break;
	case RIFT_TOUCH_CONTROLLER_ADC_A_X:
		controller->input.state.cap_a_x = rift_min_mid_max_cap(c, 3, (float)message.touch.adc_value);
		break;
	case RIFT_TOUCH_CONTROLLER_ADC_B_Y:
		controller->input.state.cap_b_y = rift_min_mid_max_cap(c, 1, (float)message.touch.adc_value);
		break;
	case RIFT_TOUCH_CONTROLLER_ADC_TRIGGER:
		controller->input.state.cap_trigger = rift_min_mid_max_cap(c, 2, (float)message.touch.adc_value);
		break;
	case RIFT_TOUCH_CONTROLLER_ADC_THUMBREST:
		controller->input.state.cap_thumbrest = rift_min_mid_max_cap(c, 7, (float)message.touch.adc_value);
		break;
	case RIFT_TOUCH_CONTROLLER_ADC_HAPTIC_COUNTER:
		controller->input.state.haptic_counter = (uint8_t)message.touch.adc_value;
		break;
	case RIFT_TOUCH_CONTROLLER_ADC_BATTERY:
		xrt_atomic_s32_store(&controller->input.battery_status, message.touch.adc_value);
		break;
	}

	struct xrt_imu_sample imu_sample = {
	    .timestamp_ns = controller->input.device_local_ns,
	    .gyro_rad_secs = {gyro.x, gyro.y, gyro.z},
	    .accel_m_s2 = {accel.x, accel.y, accel.z},
	};
	controller->input.last_imu_sample = imu_sample;

	struct xrt_vec3 accel_variance = {0.01, 0.01, 0.01};
	struct xrt_vec3 gyro_variance = {0.01, 0.01, 0.01};
	imu_fusion_incorporate_gyros_and_accelerometer(controller->input.imu_fusion, imu_sample.timestamp_ns, &gyro,
	                                               &gyro_variance, &accel, &accel_variance, NULL);

	os_mutex_unlock(&controller->input.mutex);
}

int
rift_radio_handle_read(struct rift_hmd *hmd)
{
	// int result;
	uint8_t buf[REPORT_MAX_SIZE];
	int length;

	// 1ms so the radio thread is ticking at 1khz for haptics.
	length = os_hid_read(hmd->radio_dev, buf, sizeof(buf), 1);

	timepoint_ns receive_ns = os_monotonic_get_ns() - RIFT_USB_LATENCY_BIAS;

	if (length < 0) {
		HMD_ERROR(hmd, "Got error reading from radio device, assuming fatal, reason %d", length);
		return length;
	}

	// non fatal, but nothing to do
	if (length == 0) {
		// HMD_TRACE(hmd, "Timed out waiting for packet from radio, packets should come in at 500hz");
		return 0;
	}

	// do nothing with the keepalive
	if (buf[0] == IN_REPORT_CV1_RADIO_KEEPALIVE) {
		HMD_TRACE(hmd, "Got radio keepalive(?)");
		return 0;
	}

	if (buf[0] != IN_REPORT_RADIO_DATA) {
		HMD_WARN(hmd, "Got radio IN report with bad ID (got %d, expected %d, size %d), ignoring...", buf[0],
		         IN_REPORT_RADIO_DATA, length);
		return 0;
	}

	if (length != IN_REPORT_RADIO_DATA_SIZE) {
		HMD_WARN(hmd, "Got radio IN report with bad size (got %d, expected %d), ignoring...", length,
		         IN_REPORT_RADIO_DATA_SIZE);
		return 0;
	}

	struct rift_radio_report radio_report;
	memcpy(&radio_report, buf + 1, sizeof(radio_report));

	for (uint32_t i = 0; i < ARRAY_SIZE(radio_report.messages); i++) {
		const struct rift_radio_report_message message = radio_report.messages[i];

		if (message.flags != 0x1c && message.flags != 0x05) {
			// HMD_TRACE(hmd, "Got message with unknown radio flags %04x. skipping", message.flags);

			continue;
		}

		switch (message.device_type) {
		case RIFT_RADIO_DEVICE_LEFT_TOUCH:
		case RIFT_RADIO_DEVICE_RIGHT_TOUCH:
		case RIFT_RADIO_DEVICE_TRACKED_OBJECT: {
			size_t touch_index = rift_radio_device_type_to_touch_index(message.device_type);

			struct rift_touch_controller *controller = hmd->radio_state.touch_controllers[touch_index];

			if (controller == NULL) {
				controller = hmd->radio_state.touch_controllers[touch_index] =
				    rift_touch_controller_create(hmd, message.device_type, hmd->xfctx);

				if (controller == NULL) {
					HMD_ERROR(hmd, "Failed to create touch controller for device type %d",
					          message.device_type);
					continue;
				}

				HMD_INFO(hmd, "Created touch controller for device type %d", message.device_type);

				os_mutex_lock(&hmd->device_mutex);
				hmd->devices[hmd->device_count++] = &controller->base;
				os_mutex_unlock(&hmd->device_mutex);
			}

			if (!rift_radio_read_device_serial(hmd, message.device_type, controller->base.serial,
			                                   &controller->radio_data.serial_valid)) {
				// still waiting for serial
				break;
			}

			if (!rift_radio_read_controller_calibration(hmd, controller)) {
				// still waiting for calibration
				break;
			}

			rift_touch_controller_handle_radio_input_report(hmd, controller, message, receive_ns);

			break;
		}
		case RIFT_RADIO_DEVICE_REMOTE: {
			struct rift_remote *remote = hmd->radio_state.remote;

			if (remote == NULL) {
				remote = hmd->radio_state.remote = rift_remote_create(hmd, hmd->xfctx);

				if (remote == NULL) {
					HMD_ERROR(hmd, "Failed to create remote");
					continue;
				}

				HMD_INFO(hmd, "Created remote");

				os_mutex_lock(&hmd->device_mutex);
				hmd->devices[hmd->device_count++] = &remote->base;
				os_mutex_unlock(&hmd->device_mutex);
			}

			if (!rift_radio_read_device_serial(hmd, message.device_type, remote->base.serial,
			                                   &remote->serial_valid)) {
				// still waiting for serial
				break;
			}

			xrt_atomic_s32_store(&remote->buttons, message.remote.buttons);

			break;
		}
		}
	}

	return 0;
}

static int
rift_radio_set_touch_controller_haptics_sync(struct rift_hmd *hmd,
                                             enum rift_radio_device_type device_type,
                                             bool high_freq,
                                             uint8_t amplitude)
{
	int result;
	unsigned char buf[30] = {0};

	// radio is already busy!
	if (hmd->radio_state.current_command != RIFT_RADIO_COMMAND_NONE) {
		return -EBUSY;
	}

	if (amplitude != 0) {
		buf[high_freq ? 3 /* 320hz */ : 2 /* 160hz */] = 0xa0;

		buf[6] = amplitude;
	}

	result = rift_send_radio_data(
	    hmd, &(struct rift_radio_cmd_report){.a = 0x02, .b = 0x03, .c = (uint8_t)device_type}, buf, sizeof(buf));

	if (result < 0) {
		return result;
	}

	result = rift_get_radio_cmd_response(hmd, true, true);
	if (result < 0) {
		HMD_ERROR(hmd, "Failed to send haptics command to device %d, reason %d", device_type, result);
		return result;
	}

	return 0;
}

static int
rift_radio_update_touch_haptics(struct rift_hmd *hmd, enum rift_radio_device_type device_type, timepoint_ns now)
{
	int result = 0;
	size_t touch_index = rift_radio_device_type_to_touch_index(device_type);

	struct rift_touch_controller *controller = hmd->radio_state.touch_controllers[touch_index];

	if (controller == NULL) {
		return 0;
	}

	// Haptic duration has elapsed, turn it off
	if (now > controller->input.haptic.end_time_ns) {
		if (controller->input.haptic.set_enabled) {
			result = rift_radio_set_touch_controller_haptics_sync(hmd, device_type, false, 0);
			if (result == 0) {
				controller->input.haptic.set_enabled = false;
				controller->input.haptic.set_amplitude = -1.0f;
			}

			return result;
		}

		// no haptic needed
		return 0;
	}

	if (controller->input.haptic.amplitude != controller->input.haptic.set_amplitude ||
	    controller->input.haptic.high_freq != controller->input.haptic.set_high_freq) {
		result = rift_radio_set_touch_controller_haptics_sync(
		    hmd, device_type, controller->input.haptic.high_freq,
		    CLAMP(controller->input.haptic.amplitude * 255.0f, 0, 255));

		if (result == 0) {
			controller->input.haptic.set_enabled = true;
			controller->input.haptic.set_amplitude = controller->input.haptic.amplitude;
			controller->input.haptic.set_high_freq = controller->input.haptic.high_freq;
		}

		return result;
	}

	return 0;
}

int
rift_radio_handle_haptics(struct rift_hmd *hmd)
{
	int result;

	timepoint_ns now = os_monotonic_get_ns();

	static const enum rift_radio_device_type to_check[] = {
	    RIFT_RADIO_DEVICE_LEFT_TOUCH,
	    RIFT_RADIO_DEVICE_RIGHT_TOUCH,
	    RIFT_RADIO_DEVICE_TRACKED_OBJECT,
	};
	for (size_t i = 0; i < ARRAY_SIZE(to_check); i++) {
		result = rift_radio_update_touch_haptics(hmd, to_check[i], now);

		// radio busy, try again later
		if (result == -EBUSY) {
			return 0;
		}

		if (result < 0) {
			HMD_ERROR(hmd, "Failed to update haptics for device type %d, reason %d", to_check[i], result);
			return result;
		}
	}

	return 0;
}

int
rift_radio_handle_command(struct rift_hmd *hmd)
{
	// No active command
	if (hmd->radio_state.current_command == RIFT_RADIO_COMMAND_NONE) {
		return 0;
	}

	int result = rift_get_radio_cmd_response(hmd, false, true);

	if (result == -EINPROGRESS) {
		// still waiting for response
		return 0;
	}

	if (result == -ETIMEDOUT) {
		switch (hmd->radio_state.current_command) {

		case RIFT_RADIO_COMMAND_READ_SERIAL:
			break; // @note this can be erroneous since the headset likes to send remote packets even when
			       //       there's no remote sometimes
		default:
			HMD_WARN(hmd, "Timed out waiting for radio command response %d, cancelling request",
			         hmd->radio_state.current_command);
			break;
		}

		hmd->radio_state.current_command = RIFT_RADIO_COMMAND_NONE;

		return 0;
	}

	// Unexpected error
	if (result < 0) {
		HMD_ERROR(hmd, "Unexpected error getting radio command response, reason %d", result);
		return result;
	}

	enum rift_radio_command command = hmd->radio_state.current_command;
	hmd->radio_state.current_command = RIFT_RADIO_COMMAND_NONE;

	HMD_TRACE(hmd, "Successfully received response for radio command %d", command);

	switch (command) {
	case RIFT_RADIO_COMMAND_READ_SERIAL: {
		struct rift_radio_command_data_read_serial *data = &hmd->radio_state.command_data.read_serial;

		uint8_t buf[24]; // size of data response
		result = rift_radio_read_data(hmd, (uint8_t *)&buf, sizeof(buf), false);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to read serial data from radio, reason %d", result);
			break;
		}

		const char *serial_ptr = (const char *)(buf + 5); // first 5 bytes are unknown

		(*data->serial_valid) = true;

		HMD_INFO(hmd, "Read radio serial: %s", serial_ptr);
		memcpy(data->serial, (char *)serial_ptr, SERIAL_NUMBER_LENGTH);
		data->serial[SERIAL_NUMBER_LENGTH] = 0;

		break;
	}
	case RIFT_RADIO_COMMAND_READ_FLASH: {
		struct rift_radio_command_data_read_flash *data = &hmd->radio_state.command_data.read_flash;

		result = rift_radio_read_data(hmd, data->buffer, data->length, true);
		if (result < 0) {
			HMD_ERROR(hmd, "Failed to read flash data from radio, reason %d", result);
			break;
		}

		// call user callback
		result = data->read_callback(data->user_data, data->address, data->length);
		if (result < 0) {
			HMD_ERROR(hmd, "Flash read callback returned error %d", result);
			break;
		}

		break;
	}
	default: break;
	}

	return 0;
}
