// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  ContactGlove device calibration implementation.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_contactglove
 */

#include "xrt/xrt_defines.h"

#include "util/u_file.h"

#include "contactglove_calibration.h"
#include "contactglove_internal.h"

#include <errno.h>


#define CONTACTGLOVE_CONFIG_DIR "contactglove"

#define WIZARD_TRACE(wizard, ...) U_LOG_IFL_T(wizard->log_level, __VA_ARGS__)
#define WIZARD_DEBUG(wizard, ...) U_LOG_IFL_D(wizard->log_level, __VA_ARGS__)
#define WIZARD_INFO(wizard, ...) U_LOG_IFL_I(wizard->log_level, __VA_ARGS__)
#define WIZARD_WARN(wizard, ...) U_LOG_IFL_W(wizard->log_level, __VA_ARGS__)
#define WIZARD_ERROR(wizard, ...) U_LOG_IFL_E(wizard->log_level, __VA_ARGS__)

/*
 *
 * Helper functions
 *
 */

static void
get_config_filename(char *out_name, size_t out_name_size, enum xrt_hand hand, const char *device_serial)
{
	const char *hand_str = NULL;
	switch (hand) {
	case XRT_HAND_LEFT: hand_str = "l"; break;
	case XRT_HAND_RIGHT: hand_str = "r"; break;
	default:
		assert(hand_str);
		hand_str = "";
		break;
	}

	snprintf(out_name, out_name_size, "%s-%s.cgc", hand_str, device_serial);
}

static FILE *
open_config_file(struct contactglove_calibration_wizard *wizard, const char *mode)
{
	char config_filename[256 + 2 + 4];
	get_config_filename(config_filename, sizeof(config_filename), wizard->glove->hand, wizard->glove->base.serial);

	WIZARD_DEBUG(wizard, "Opening config with name %s using mode %s", config_filename, mode);

	return u_file_open_file_in_config_dir_subpath(CONTACTGLOVE_CONFIG_DIR, config_filename, mode);
}

static void
fill_default_calib(struct contactglove_calibration *calibration)
{
	(*calibration) = (struct contactglove_calibration){
	    .flex_sensors = {0},
	    .joystick =
	        {
	            .stick_center_x = 127,
	            .stick_center_y = 127,
	            .range = 127,
	            .forward_vector = {.x = 1.0f, .y = 0.0f},
	            .deadzone = 0.1f,
	        },
	    .trigger =
	        {
	            // Conservative values based on my unit
	            .min = 80,
	            .max = 110,
	        },
	};

	// Likely bad but better than literally nothing
	uint16_t uncurled[CONTACTGLOVE2_SENSOR_COUNT] = {1061, 1375, 1017, 1158, 1409, 1205, 1197, 1358,
	                                                 1192, 1185, 1293, 1192, 890,  1319, 890,  2536};
	uint16_t curled[CONTACTGLOVE2_SENSOR_COUNT] = {1764, 1624, 1565, 1622, 1967, 1629, 1812, 1967,
	                                               1745, 1662, 1690, 1745, 1114, 1376, 1152, 3016};

	for (int i = 0; i < CONTACTGLOVE2_SENSOR_COUNT; i++) {
		calibration->flex_sensors[i] = (struct contactglove_sensor_calibration){
		    .min = uncurled[i],
		    .max = curled[i],
		};
	}
}

static bool
contactglove_calibration_load_file(struct contactglove_calibration_wizard *wizard)
{
	FILE *file = open_config_file(wizard, "rb");
	if (file == NULL) {
		WIZARD_DEBUG(wizard, "ContactGlove calibration file does not exist");
		return false;
	}

	size_t size;
	uint8_t *file_contents = (uint8_t *)u_file_read_content(file, &size);

	if (file_contents == NULL) {
		WIZARD_ERROR(wizard, "Failed to open ContactGlove calibration file");
		return false;
	}

	bool ret = true;

	size_t ptr = 0;

#define READ_U8(out)                                                                                                   \
	do {                                                                                                           \
		if (ptr >= size) {                                                                                     \
			WIZARD_ERROR(wizard, "Tried to read u8, reached end of file (len=%zu).", size);                \
			ret = false;                                                                                   \
			goto load_out;                                                                                 \
		}                                                                                                      \
		memcpy(&out, (file_contents + ptr), sizeof(uint8_t));                                                  \
		ptr += sizeof(uint8_t);                                                                                \
	} while (0)
#define READ_LE16(out)                                                                                                 \
	do {                                                                                                           \
		if (ptr >= size) {                                                                                     \
			WIZARD_ERROR(wizard, "Tried to read le16, reached end of file (len=%zu).", size);              \
			ret = false;                                                                                   \
			goto load_out;                                                                                 \
		}                                                                                                      \
		memcpy(&out, (file_contents + ptr), sizeof(__le16));                                                   \
		out = __le16_to_cpu(out);                                                                              \
		ptr += sizeof(__le16);                                                                                 \
	} while (0)
#define READ_LE32(out)                                                                                                 \
	do {                                                                                                           \
		if (ptr >= size) {                                                                                     \
			WIZARD_ERROR(wizard, "Tried to read le32, reached end of file (len=%zu).", size);              \
			ret = false;                                                                                   \
			goto load_out;                                                                                 \
		}                                                                                                      \
		memcpy(&out, (file_contents + ptr), sizeof(__le32));                                                   \
		out = __le32_to_cpu(out);                                                                              \
		ptr += sizeof(__le32);                                                                                 \
	} while (0)
#define READ_BE32(out)                                                                                                 \
	do {                                                                                                           \
		if (ptr >= size) {                                                                                     \
			WIZARD_ERROR(wizard, "Tried to read be32, reached end of file (len=%zu).", size);              \
			ret = false;                                                                                   \
			goto load_out;                                                                                 \
		}                                                                                                      \
		memcpy(&out, (file_contents + ptr), sizeof(__be32));                                                   \
		out = __be32_to_cpu(out);                                                                              \
		ptr += sizeof(__be32);                                                                                 \
	} while (0)

	uint32_t magic;
	READ_BE32(magic);
	if (magic != CONTACTGLOVE_CALIBRATION_MAGIC) {
		WIZARD_ERROR(wizard, "ContactGlove calibration file has invalid magic %x", magic);
		ret = false;
		goto load_out;
	}

	uint32_t version;
	READ_LE32(version);
	if (version > CONTACTGLOVE_CALIBRATION_CURRENT_VERSION) {
		WIZARD_ERROR(wizard, "ContactGlove calibration file has invalid version %d, current is %d", version,
		             CONTACTGLOVE_CALIBRATION_CURRENT_VERSION);
		ret = false;
		goto load_out;
	}

	struct contactglove_calibration calibration;
	fill_default_calib(&calibration);

	// Read the flex sensors
	for (int i = 0; i < CONTACTGLOVE2_SENSOR_COUNT; i++) {
		READ_LE16(calibration.flex_sensors[i].min);
		READ_LE16(calibration.flex_sensors[i].max);
	}

	{ // Read the joystick calibration
		READ_U8(calibration.joystick.stick_center_x);
		READ_U8(calibration.joystick.stick_center_y);
		READ_U8(calibration.joystick.range);

		int8_t fwd_x;
		int8_t fwd_y;
		READ_U8(fwd_x);
		READ_U8(fwd_y);
		calibration.joystick.forward_vector = (struct xrt_vec2){
		    .x = fwd_x,
		    .y = fwd_y,
		};
		m_vec2_normalize(&calibration.joystick.forward_vector);

		uint8_t deadzone;
		READ_U8(deadzone);
		calibration.joystick.deadzone = deadzone / 100.0f;
	}

	{ // Read the trigger calibration
		READ_U8(calibration.trigger.min);
		READ_U8(calibration.trigger.max);
	}

#undef READ_U8
#undef READ_LE16
#undef READ_LE32
#undef READ_BE32

	// Load the new calibration
	wizard->calibration = calibration;

	WIZARD_INFO(wizard, "Loaded ContactGlove calibration from file.");

load_out:
	fclose(file);
	return ret;
}

static int
fwrite_all(FILE *f, void *raw_ptr, size_t n)
{
	assert(raw_ptr != NULL);

	uint8_t *ptr = raw_ptr;

	while (n > 0) {
		size_t written = fwrite(ptr, 1, n, f);

		n -= written;
		ptr += written;

		if (written == 0) {
			return ferror(f);
		}
	}

	return 0;
}

static bool
contactglove_calibration_write_file(struct contactglove_calibration_wizard *wizard)
{
	FILE *file = open_config_file(wizard, "wb");
	if (file == NULL) {
		WIZARD_DEBUG(wizard, "ContactGlove calibration file does not exist");
		return false;
	}

	bool ret = true;

#define WRITE_U8(value)                                                                                                \
	do {                                                                                                           \
		uint8_t real = (value);                                                                                \
		int ret = fwrite_all(file, &real, sizeof(real));                                                       \
		if (ret < 0) {                                                                                         \
			WIZARD_ERROR(wizard, "Failed to write u8 to stream, got err %s", strerror(ret));               \
			ret = false;                                                                                   \
			goto write_out;                                                                                \
		}                                                                                                      \
	} while (0)
#define WRITE_LE16(out)                                                                                                \
	do {                                                                                                           \
		__le16 real = __cpu_to_le16(out);                                                                      \
		int ret = fwrite_all(file, &real, sizeof(real));                                                       \
		if (ret < 0) {                                                                                         \
			WIZARD_ERROR(wizard, "Failed to write le16 to stream, got err %s", strerror(ret));             \
			ret = false;                                                                                   \
			goto write_out;                                                                                \
		}                                                                                                      \
	} while (0)
#define WRITE_LE32(out)                                                                                                \
	do {                                                                                                           \
		__le32 real = __cpu_to_le32(out);                                                                      \
		int ret = fwrite_all(file, &real, sizeof(real));                                                       \
		if (ret < 0) {                                                                                         \
			WIZARD_ERROR(wizard, "Failed to write le32 to stream, got err %s", strerror(ret));             \
			ret = false;                                                                                   \
			goto write_out;                                                                                \
		}                                                                                                      \
	} while (0)
#define WRITE_BE32(out)                                                                                                \
	do {                                                                                                           \
		__be32 real = __cpu_to_be32(out);                                                                      \
		int ret = fwrite_all(file, &real, sizeof(real));                                                       \
		if (ret < 0) {                                                                                         \
			WIZARD_ERROR(wizard, "Failed to write be32 to stream, got err %s", strerror(ret));             \
			ret = false;                                                                                   \
			goto write_out;                                                                                \
		}                                                                                                      \
	} while (0)

	// Header
	WRITE_BE32(CONTACTGLOVE_CALIBRATION_MAGIC);
	WRITE_LE32(CONTACTGLOVE_CALIBRATION_CURRENT_VERSION);

	// Flex Sensors
	for (int i = 0; i < CONTACTGLOVE2_SENSOR_COUNT; i++) {
		WRITE_LE16(wizard->calibration.flex_sensors[i].min);
		WRITE_LE16(wizard->calibration.flex_sensors[i].max);
	}

	{ // Joystick Calibration
		WRITE_U8(wizard->calibration.joystick.stick_center_x);
		WRITE_U8(wizard->calibration.joystick.stick_center_y);
		WRITE_U8(wizard->calibration.joystick.range);

		int8_t fwd_x = (int8_t)(wizard->calibration.joystick.forward_vector.x * 127.0f);
		int8_t fwd_y = (int8_t)(wizard->calibration.joystick.forward_vector.y * 127.0f);
		WRITE_U8(fwd_x);
		WRITE_U8(fwd_y);

		WRITE_U8((uint8_t)(wizard->calibration.joystick.deadzone * 100.0f));
	}

	{ // Trigger Calibration
		WRITE_U8(wizard->calibration.trigger.min);
		WRITE_U8(wizard->calibration.trigger.max);
	}

#undef WRITE_U8
#undef WRITE_LE16
#undef WRITE_LE32
#undef WRITE_BE32

	WIZARD_INFO(wizard, "Written ContactGlove calibration to file.");

	fflush(file);

write_out:
	fclose(file);
	return ret;
}

static void
contactglove_calibration_wizard_set_step(struct contactglove_calibration_wizard *wizard,
                                         enum contactglove_calibration_wizard_step step)
{
	wizard->step = step;

	switch (step) {
	case CONTACTGLOVE_CALIBRATION_WIZARD_NOT_CALIBRATING: break;
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_FLEX_SENSORS: {
		// Wipe the working-state calibration
		fill_default_calib(&wizard->working);

		wizard->state.flex_sensors = (struct contactglove_calibration_wizard_flex_sensor_state){
		    .sensors = {0},
		};

		// Initialize to default states that will unconditionally get overwritten on next sample
		for (int i = 0; i < CONTACTGLOVE2_SENSOR_COUNT; i++) {
			wizard->state.flex_sensors.sensors[i].max = 0;
			wizard->state.flex_sensors.sensors[i].min = UINT16_MAX;
		}

		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_CENTER: {
		static_assert(sizeof(wizard->working.flex_sensors) == sizeof(wizard->state.flex_sensors.sensors),
		              "Calibration and working data have fallen out of sync.");

		memcpy(wizard->working.flex_sensors, wizard->state.flex_sensors.sensors,
		       sizeof(wizard->state.flex_sensors.sensors));

		wizard->state.joystick_center = (struct contactglove_calibration_wizard_joystick_center_state){0};
		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_RANGE: {
		wizard->working.joystick.stick_center_x = wizard->state.joystick_center.x;
		wizard->working.joystick.stick_center_y = wizard->state.joystick_center.y;

		wizard->state.joystick_range = (struct contactglove_calibration_wizard_joystick_range_state){0};

		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_DEADZONE: {
		wizard->working.joystick.range =
		    MIN(MIN(wizard->state.joystick_range.up_range, wizard->state.joystick_range.down_range),
		        MIN(wizard->state.joystick_range.left_range, wizard->state.joystick_range.right_range));

		wizard->state.joystick_deadzone = (struct contactglove_calibration_wizard_joystick_deadzone_state){
		    .deadzone = CONTACTGLOVE2_DEFAULT_DEADZONE,
		};

		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_FORWARD: {
		wizard->working.joystick.deadzone = wizard->state.joystick_deadzone.deadzone;

		wizard->state.joystick_forward = (struct xrt_vec2){
		    .x = 0,
		    .y = 0,
		};

		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_TRIGGER: {
		wizard->working.joystick.forward_vector = wizard->state.joystick_forward;
		m_vec2_normalize(&wizard->working.joystick.forward_vector);

		wizard->state.trigger = (struct contactglove_trigger_calibration){
		    .max = 0,
		    .min = UINT8_MAX,
		};

		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_WRITING_OUTPUT: {
		wizard->working.trigger = wizard->state.trigger;

		// We have a calibration!
		wizard->calibration = wizard->working;

		contactglove_calibration_write_file(wizard);

		contactglove_calibration_wizard_set_step(wizard, CONTACTGLOVE_CALIBRATION_WIZARD_NOT_CALIBRATING);

		break;
	}
	default: {
		assert(!"Unreachable: invalid calibration wizard step");
		return;
	}
	}
}

static void
start_button_cb(void *ptr)
{
	struct contactglove_calibration_wizard *wizard = ptr;

	contactglove_calibration_wizard_start(wizard, wizard->calibrating_magnetra2);
}

/*
 *
 * Exported functions
 *
 */

void
contactglove_calibration_wizard_init(struct contactglove_calibration_wizard *wizard, struct contactglove_device *glove)
{
	(*wizard) = (struct contactglove_calibration_wizard){
	    .log_level = glove->dongle->log_level,
	    .glove = glove,
	    .button_state = false,
	    .calibrating_magnetra2 = true,
	    .step = CONTACTGLOVE_CALIBRATION_WIZARD_NOT_CALIBRATING,
	    .state = {0},
	    .calibration = {0},
	    .u_var_start_button =
	        {
	            .cb = start_button_cb,
	            .ptr = wizard,
	        },
	};

	WIZARD_DEBUG(wizard, "Loading default calibration");
	fill_default_calib(&wizard->calibration);

	if (!contactglove_calibration_load_file(wizard)) {
		WIZARD_DEBUG(wizard, "Failed to load ContactGlove calibration from file");
	}

	u_var_add_root(wizard, "ContactGlove Calibration Wizard", true);
	u_var_add_log_level(wizard, &wizard->log_level, "Log Level");
	u_var_add_bool(wizard, &wizard->button_state, "Advance Button");
	u_var_add_bool(wizard, &wizard->calibrating_magnetra2, "Calibrating Magnetra2");
	u_var_add_ro_i32(wizard, (int32_t *)&wizard->step, "Step");
	u_var_add_button(wizard, &wizard->u_var_start_button, "Start Calibration");

	u_var_add_gui_header(wizard, NULL, "Flex Sensors State");
	{
		for (size_t i = 0; i < ARRAY_SIZE(wizard->state.flex_sensors.sensors); i++) {
			char buf[] = "Flex Sensor NN NNN";
			snprintf(buf, sizeof(buf), "Flex Sensor %zu Min", i);
			u_var_add_ro_u16(wizard, &wizard->state.flex_sensors.sensors[i].min, buf);

			snprintf(buf, sizeof(buf), "Flex Sensor %zu Max", i);
			u_var_add_ro_u16(wizard, &wizard->state.flex_sensors.sensors[i].max, buf);
		}
	}

	u_var_add_gui_header(wizard, NULL, "Joystick Center State");
	{
		u_var_add_ro_u8(wizard, &wizard->state.joystick_center.x, "Center X");
		u_var_add_ro_u8(wizard, &wizard->state.joystick_center.y, "Center Y");
	}

	u_var_add_gui_header(wizard, NULL, "Joystick Range State");
	{
		u_var_add_ro_u8(wizard, &wizard->state.joystick_range.up_range, "Up Range");
		u_var_add_ro_u8(wizard, &wizard->state.joystick_range.down_range, "Down Range");
		u_var_add_ro_u8(wizard, &wizard->state.joystick_range.left_range, "Left Range");
		u_var_add_ro_u8(wizard, &wizard->state.joystick_range.right_range, "Right Range");
	}

	u_var_add_gui_header(wizard, NULL, "Joystick Deadzone State");
	{
		u_var_add_ro_f32(wizard, &wizard->state.joystick_deadzone.deadzone, "Deadzone");
	}

	u_var_add_gui_header(wizard, NULL, "Joystick Forward State");
	{
		u_var_add_ro_f32(wizard, &wizard->state.joystick_forward.x, "Forward X");
		u_var_add_ro_f32(wizard, &wizard->state.joystick_forward.y, "Forward Y");
	}

	u_var_add_gui_header(wizard, NULL, "Trigger State");
	{
		u_var_add_ro_u8(wizard, &wizard->state.trigger.min, "Trigger Min");
		u_var_add_ro_u8(wizard, &wizard->state.trigger.max, "Trigger Max");
	}

#define ADD_CALIBRATION(calibration, prefix)                                                                           \
	do {                                                                                                           \
		for (size_t i = 0; i < ARRAY_SIZE(calibration.flex_sensors); i++) {                                    \
			char buf[] = prefix " Flex Sensor NN NNN";                                                     \
			snprintf(buf, sizeof(buf), prefix " Flex Sensor %zu Min", i);                                  \
			u_var_add_ro_u16(wizard, &calibration.flex_sensors[i].min, buf);                               \
                                                                                                                       \
			snprintf(buf, sizeof(buf), prefix " Flex Sensor %zu Max", i);                                  \
			u_var_add_ro_u16(wizard, &calibration.flex_sensors[i].max, buf);                               \
		}                                                                                                      \
                                                                                                                       \
		u_var_add_ro_u8(wizard, &calibration.joystick.stick_center_x, prefix " Stick Center X");               \
		u_var_add_ro_u8(wizard, &calibration.joystick.stick_center_y, prefix " Stick Center Y");               \
		u_var_add_ro_u8(wizard, &calibration.joystick.range, prefix " Stick Range");                           \
		u_var_add_ro_f32(wizard, &calibration.joystick.forward_vector.x, prefix " Stick Forward X");           \
		u_var_add_ro_f32(wizard, &calibration.joystick.forward_vector.y, prefix " Stick Forward Y");           \
		u_var_add_ro_f32(wizard, &calibration.joystick.deadzone, prefix " Stick Deadzone");                    \
                                                                                                                       \
		u_var_add_ro_u8(wizard, &calibration.trigger.min, prefix " Trigger Min");                              \
		u_var_add_ro_u8(wizard, &calibration.trigger.max, prefix " Trigger Max");                              \
	} while (0)

	u_var_add_gui_header(wizard, NULL, "Working Calibration");
	ADD_CALIBRATION(wizard->working, "Working");
	u_var_add_gui_header(wizard, NULL, "Calibration");
	ADD_CALIBRATION(wizard->calibration, "In-use");

#undef ADD_CALIBRATION
}

void
contactglove_calibration_wizard_deinit(struct contactglove_calibration_wizard *wizard)
{
	u_var_remove_root(wizard);
}

void
contactglove_calibration_wizard_start(struct contactglove_calibration_wizard *wizard, bool calibrating_magnetra2)
{
	wizard->calibrating_magnetra2 = calibrating_magnetra2;

	contactglove_calibration_wizard_set_step(wizard, CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_FLEX_SENSORS);
}

void
contactglove_calibration_wizard_push_button(struct contactglove_calibration_wizard *wizard, bool button)
{
	bool just_pressed = (button != wizard->button_state) && button;

	wizard->button_state = button;

	// Don't do anything if the button was not just pressed
	if (!just_pressed) {
		return;
	}

	switch (wizard->step) {
	case CONTACTGLOVE_CALIBRATION_WIZARD_WRITING_OUTPUT:
	case CONTACTGLOVE_CALIBRATION_WIZARD_NOT_CALIBRATING: return;
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_FLEX_SENSORS:
		if (wizard->calibrating_magnetra2) {
			contactglove_calibration_wizard_set_step(
			    wizard, CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_CENTER);
		} else {
			contactglove_calibration_wizard_set_step(wizard,
			                                         CONTACTGLOVE_CALIBRATION_WIZARD_WRITING_OUTPUT);
		}
		break;
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_CENTER:
		contactglove_calibration_wizard_set_step(wizard,
		                                         CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_RANGE);
		break;
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_RANGE:
		contactglove_calibration_wizard_set_step(wizard,
		                                         CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_DEADZONE);
		break;
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_DEADZONE:
		contactglove_calibration_wizard_set_step(wizard,
		                                         CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_FORWARD);
		break;
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_FORWARD:
		contactglove_calibration_wizard_set_step(wizard, CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_TRIGGER);
		break;
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_TRIGGER:
		contactglove_calibration_wizard_set_step(wizard, CONTACTGLOVE_CALIBRATION_WIZARD_WRITING_OUTPUT);
		break;
	}
}

void
contactglove_calibration_wizard_push_flex_sensors(struct contactglove_calibration_wizard *wizard,
                                                  const uint16_t *adc_values)
{
	// We don't need the flex sensors otherwise
	if (wizard->step != CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_FLEX_SENSORS) {
		return;
	}

	for (int i = 0; i < CONTACTGLOVE2_SENSOR_COUNT; i++) {
		struct contactglove_sensor_calibration *sensor = &wizard->state.flex_sensors.sensors[i];

		sensor->max = MAX(sensor->max, adc_values[i]);
		sensor->min = MIN(sensor->min, adc_values[i]);
	}
}

void
contactglove_calibration_wizard_push_joystick(struct contactglove_calibration_wizard *wizard,
                                              uint8_t adc_x,
                                              uint8_t adc_y)
{
	int8_t x = (int8_t)(((int16_t)adc_x) - wizard->working.joystick.stick_center_x);
	int8_t y = (int8_t)(((int16_t)adc_y) - wizard->working.joystick.stick_center_y);

	switch (wizard->step) {
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_CENTER: {
		wizard->state.joystick_center.x = adc_x;
		wizard->state.joystick_center.y = adc_y;
		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_RANGE: {
		if (x > 0) {
			wizard->state.joystick_range.right_range = MAX(x, wizard->state.joystick_range.right_range);
		} else {
			wizard->state.joystick_range.left_range = MAX(-x, wizard->state.joystick_range.left_range);
		}

		if (y > 0) {
			wizard->state.joystick_range.up_range = MAX(x, wizard->state.joystick_range.up_range);
		} else {
			wizard->state.joystick_range.down_range = MAX(-x, wizard->state.joystick_range.down_range);
		}

		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_FORWARD: {
		struct xrt_vec2 vec = {
		    .x = x,
		    .y = y,
		};
		vec = m_vec2_div_scalar(vec, wizard->working.joystick.range);

		if (m_vec2_len_sqrd(vec) > m_vec2_len_sqrd(wizard->state.joystick_forward)) {
			wizard->state.joystick_forward = vec;
		}

		break;
	}
	case CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_JOYSTICK_DEADZONE: {
		// @todo Implement some way to actually have the user control this

		break;
	}
	default: return; // We don't need the joystick otherwise
	}
}

void
contactglove_calibration_wizard_push_trigger(struct contactglove_calibration_wizard *wizard, uint8_t adc)
{
	if (wizard->step != CONTACTGLOVE_CALIBRATION_WIZARD_CALIBRATING_TRIGGER) {
		return;
	}

	struct contactglove_trigger_calibration *calib = &wizard->state.trigger;
	calib->min = MIN(calib->min, adc);
	calib->max = MAX(calib->max, adc);
}
