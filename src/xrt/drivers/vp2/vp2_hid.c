// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the Vive Pro 2 HID interface.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_vp2
 */

#include "xrt/xrt_byte_order.h"
#include "xrt/xrt_compiler.h"

#include "os/os_hid.h"
#include "os/os_time.h"
#include "os/os_threading.h"

#include "util/u_logging.h"
#include "util/u_debug.h"
#include "util/u_misc.h"
#include "util/u_file.h"

#include "math/m_api.h"

#include <inttypes.h>

#include "vp2_hid.h"
#include "vp2_config.h"
#include "vp2_protocol.h"


DEBUG_GET_ONCE_LOG_OPTION(vp2_log, "VP2_LOG", U_LOGGING_WARN)

// NOTE: VP2_RESOLUTION_3680_1836_90_02 is the safest resolution that works without needing the kernel patches.
DEBUG_GET_ONCE_NUM_OPTION(vp2_resolution, "VP2_RESOLUTION", VP2_RESOLUTION_3680_1836_90_02)
DEBUG_GET_ONCE_FLOAT_OPTION(vp2_default_brightness, "VP2_DEFAULT_BRIGHTNESS", 1.0f)
DEBUG_GET_ONCE_BOOL_OPTION(vp2_noise_cancelling, "VP2_NOISE_CANCELLING", false)

#define VP2_TRACE(vp2, ...) U_LOG_IFL_T(vp2->log_level, __VA_ARGS__)
#define VP2_DEBUG(vp2, ...) U_LOG_IFL_D(vp2->log_level, __VA_ARGS__)
#define VP2_INFO(vp2, ...) U_LOG_IFL_I(vp2->log_level, __VA_ARGS__)
#define VP2_WARN(vp2, ...) U_LOG_IFL_W(vp2->log_level, __VA_ARGS__)
#define VP2_ERROR(vp2, ...) U_LOG_IFL_E(vp2->log_level, __VA_ARGS__)

struct vp2_hid
{
	enum u_logging_level log_level;

	struct os_hid_device *hid;
	enum vp2_resolution resolution;

	struct vp2_config config;

	float brightness;

	char serial[64];

	struct os_thread_helper thread;
	timepoint_ns last_status_report_ns;
	xrt_atomic_s64_t status;
};

#define VP2_CONFIG_FOLDER "vive_pro_2"

/*!
 * Delay after every write so the MCU can handle it before the next one, the HTC driver waits this long between display
 * requests.
 */
#define VP2_WRITE_DELAY_NS (U_TIME_1MS_IN_NS * 10)

/*
 *
 * Helper functions
 *
 */

static int
vp2_write(struct vp2_hid *vp2, uint8_t id, const uint8_t *data, size_t size)
{
	uint8_t buffer[VP2_DATA_SIZE] = {0};
	buffer[0] = id;
	if (size > sizeof(buffer) - 1) {
		VP2_ERROR(vp2, "Data size too large to write.");
		return -1;
	}

	memcpy(&buffer[1], data, size);
	int ret = os_hid_write(vp2->hid, buffer, sizeof(buffer));

	os_nanosleep(VP2_WRITE_DELAY_NS);

	return ret;
}

static int
vp2_write_feature(struct vp2_hid *vp2, uint8_t id, uint16_t sub_id, const uint8_t *data, size_t size)
{
	struct vp2_feature_header header = {
	    .id = id,
	    .sub_id = __cpu_to_le16(sub_id),
	    .size = (uint8_t)size,
	};

	uint8_t buffer[VP2_DATA_SIZE] = {0};
	memcpy(buffer, &header, sizeof(header));
	if (size > sizeof(buffer) - sizeof(header)) {
		VP2_ERROR(vp2, "Data size too large to write.");
		return -1;
	}

	memcpy(&buffer[sizeof(header)], data, size);
	int ret = os_hid_set_feature(vp2->hid, buffer, sizeof(buffer));

	os_nanosleep(VP2_WRITE_DELAY_NS);

	return ret;
}

static int
vp2_set_resolution(struct vp2_hid *vp2, enum vp2_resolution resolution)
{
	const uint8_t *wireless_command = (const uint8_t *)"wireless,0";
	int ret = vp2_write_feature(vp2, VP2_REPORT_ID_FEATURE, VP2_COMMAND_DISPLAY_REQUEST, wireless_command,
	                            strlen((const char *)wireless_command));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to write no wireless command.");
		return ret;
	}

	uint8_t resolution_command[16];
	int command_length =
	    snprintf((char *)resolution_command, sizeof(resolution_command), "dtd,%d", (uint8_t)resolution);

	ret = vp2_write_feature(vp2, VP2_REPORT_ID_FEATURE, VP2_COMMAND_DISPLAY_REQUEST, resolution_command,
	                        command_length);
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to write resolution command.");
		return ret;
	}

	const uint8_t *reset_command = (const uint8_t *)"chipreset";
	ret = vp2_write_feature(vp2, VP2_REPORT_ID_FEATURE, VP2_COMMAND_DISPLAY_REQUEST, reset_command,
	                        strlen((const char *)reset_command));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to write chip reset command.");
		return ret;
	}

	return 0;
}

static int
vp2_read(struct vp2_hid *vp2, uint8_t id, const uint8_t *prefix, size_t prefix_size, uint8_t *out_data, size_t out_size)
{
	uint8_t buffer[VP2_DATA_SIZE] = {0};
	int ret = os_hid_read(vp2->hid, buffer, sizeof(buffer), 1000);
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to read from HID device.");
		return ret;
	}
	if (ret == 0) {
		VP2_WARN(vp2, "Timeout reading from HID device.");
		return -1;
	}

	if (buffer[0] != id) {
		VP2_ERROR(vp2, "Unexpected report ID: got %02x, expected %02x", buffer[0], id);
		return -1;
	}

	if (prefix_size > 0 && memcmp(&buffer[1], prefix, prefix_size) != 0) {
		VP2_ERROR(vp2, "Unexpected report prefix.");
		U_LOG_IFL_D_HEX(vp2->log_level, &buffer[1], prefix_size);
		U_LOG_IFL_D_HEX(vp2->log_level, prefix, prefix_size);
		return -1;
	}

	uint8_t size = buffer[1 + prefix_size];
	if (size > out_size) {
		VP2_ERROR(vp2, "Output buffer too small: got %zu, need %u", out_size, size);
		return -1;
	}

	memcpy(out_data, &buffer[2 + prefix_size], size);

	return size;
}

//! Reads and drops any reports that are already queued, for example status reports from an earlier session.
static void
vp2_drain_reads(struct vp2_hid *vp2)
{
	uint8_t buffer[VP2_DATA_SIZE];

	// Bounded in case the reports never stop.
	for (int i = 0; i < 128; i++) {
		// Longer than the 50ms between status reports.
		if (os_hid_read(vp2->hid, buffer, sizeof(buffer), 100) <= 0) {
			return;
		}
	}

	VP2_WARN(vp2, "Reports are still coming in after draining.");
}

// This code is now disabled since we no longer need it, it's still possibly useful in the future, but isn't now.
#if 0
static int
vp2_read_int(struct vp2_hid *vp2, const char *command, int *out_value)
{
	int ret;

	ret = vp2_write(vp2, VP2_REPORT_ID_STRING, (const uint8_t *)command, strlen((const char *)command));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to write IPD read command.");
		return ret;
	}

	uint8_t response[VP2_DATA_SIZE] = {0};
	ret = vp2_read(vp2, VP2_REPORT_ID_STRING, NULL, 0, response, sizeof(response));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to read IPD response.");
		return ret;
	}

	// Null-terminate the response string
	response[ret] = '\0';

	*out_value = strtol((const char *)response, NULL, 10);

	return 0;
}
#endif

static int
vp2_get_config_size(struct vp2_hid *vp2, size_t *out_size)
{
	uint8_t buf[VP2_DATA_SIZE];

	__le16 request = __cpu_to_le16(VP2_COMMAND_CONFIG_READ_START);
	int ret = vp2_write(vp2, VP2_REPORT_ID_COMMAND, (const uint8_t *)&request, sizeof(request));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to write config size command.");
		return ret;
	}

	ret = vp2_read(vp2, VP2_REPORT_ID_COMMAND, (const uint8_t *)&request, sizeof(request), buf, sizeof(buf));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to read config size response.");
		return ret;
	}
	if (ret != 4) {
		VP2_ERROR(vp2, "Unexpected config size response length: got %d, expected 4", ret);
		return -1;
	}

	__le32 size;
	memcpy(&size, buf, sizeof(size));
	*out_size = __le32_to_cpu(size);

	return 0;
}

static int
vp2_read_config(struct vp2_hid *vp2, char **out_config, size_t *out_size)
{
	uint8_t buf[VP2_DATA_SIZE];
	int ret = 0;

	size_t config_size;
	ret = vp2_get_config_size(vp2, &config_size);
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to get config size.");
		return ret;
	}

	// skip the header in front of the JSON
	config_size -= VP2_CONFIG_HEADER_SIZE;

	struct vp2_config_read_request request = {
	    .sub_id = __cpu_to_le16(VP2_COMMAND_CONFIG_READ),
	    .size = sizeof(request.offset),
	};

	char *config = U_TYPED_ARRAY_CALLOC(char, config_size + 1);

	size_t read_offset = 0;
	while (read_offset < config_size) {
		request.offset = __cpu_to_le32((uint32_t)(read_offset + VP2_CONFIG_HEADER_SIZE));

		ret = vp2_write(vp2, VP2_REPORT_ID_COMMAND, (const uint8_t *)&request, sizeof(request));
		if (ret < 0) {
			VP2_ERROR(vp2, "Failed to write config read command.");
			goto read_config_end;
		}

		// NOTE: the reply starts with the same command as the request.
		ret = vp2_read(vp2, VP2_REPORT_ID_COMMAND, (const uint8_t *)&request.sub_id, sizeof(request.sub_id),
		               buf, sizeof(buf));
		if (ret < 0) {
			VP2_ERROR(vp2, "Failed to read config chunk.");
			goto read_config_end;
		}

		size_t to_copy = (size_t)ret;
		to_copy = MIN(to_copy, config_size - read_offset);
		memcpy(config + read_offset, buf, to_copy);
		read_offset += to_copy;
	}

read_config_end:
	if (ret < 0) {
		free(config);
	} else {
		*out_config = config;
		*out_size = config_size;
	}

	return ret;
}

//! Sends an ASCII command on VP2_REPORT_ID_STRING and reads back its reply as a null terminated string.
static int
vp2_string_command(struct vp2_hid *vp2, const char *command, char *out, size_t out_size)
{
	int ret = vp2_write(vp2, VP2_REPORT_ID_STRING, (const uint8_t *)command, strlen(command));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to write string command %s.", command);
		return ret;
	}

	ret = vp2_read(vp2, VP2_REPORT_ID_STRING, NULL, 0, (uint8_t *)out, out_size - 1);
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to read reply of string command %s.", command);
		return ret;
	}
	out[ret] = '\0'; // Null-terminate the string

	return 0;
}

static int
vp2_read_serial(struct vp2_hid *vp2, char *out, size_t out_size)
{
	return vp2_string_command(vp2, "mfg-r-devsn", out, out_size);
}

static int
vp2_load_serial(struct vp2_hid *vp2)
{
	int ret = -1;

	for (int attempt = 0; attempt < 3; attempt++) {
		VP2_TRACE(vp2, "Reading serial number, attempt %d...", attempt + 1);

		ret = vp2_read_serial(vp2, vp2->serial, sizeof(vp2->serial));
		if (ret == 0) {
			VP2_DEBUG(vp2, "Read Vive Pro 2 serial number: %s", vp2->serial);
			return 0;
		} else {
			VP2_WARN(vp2, "Failed to read serial number on attempt %d.", attempt + 1);
		}
	}

	// All attempts failed
	return ret;
}

static int
vp2_load_config(struct vp2_hid *vp2)
{
	char config_filename[128];
	snprintf(config_filename, sizeof(config_filename), "%s.json", vp2->serial);

	FILE *file = u_file_open_file_in_config_dir_subpath(VP2_CONFIG_FOLDER, config_filename, "r");
	if (file != NULL) {
		// Read the contents and close the file
		size_t config_size = 0;
		char *contents = u_file_read_content(file, &config_size);
		fclose(file);

		// If loading the contents was successful
		if (contents != NULL) {
			// And it successfully parses, then we're done
			if (vp2_config_parse(contents, config_size, &vp2->config)) {
				VP2_DEBUG(vp2, "Loaded Vive Pro 2 config from %s", config_filename);
				free(contents);
				return 0;
			} else {
				VP2_ERROR(vp2, "Failed to parse config file contents.");
				free(contents);
			}
		} else {
			VP2_ERROR(vp2, "Failed to read config file contents.");
		}
	}

	// Loading from file failed, try reading from the device
	char *config = NULL;
	size_t config_size = 0;
	for (int attempt = 0; attempt < 3; attempt++) {
		VP2_TRACE(vp2, "Reading config from device, attempt %d...", attempt + 1);

		int ret = vp2_read_config(vp2, &config, &config_size);
		if (ret >= 0) {
			VP2_DEBUG(vp2, "Read Vive Pro 2 config from device (%zu bytes)", config_size);
			if (!vp2_config_parse(config, config_size, &vp2->config)) {
				VP2_ERROR(vp2, "Failed to parse config, trying again...");
				free(config);
				config = NULL;
			} else {
				VP2_DEBUG(vp2, "Parsed Vive Pro 2 config successfully.");
				break;
			}
		} else {
			VP2_WARN(vp2, "Failed to read config from device on attempt %d.", attempt + 1);
		}
	}

	// No config still..
	if (config == NULL) {
		VP2_ERROR(vp2, "Failed to read config from device.");
		return -1;
	}

	int ret = 0;

	// Open the output file we want to write
	file = u_file_open_file_in_config_dir_subpath(VP2_CONFIG_FOLDER, config_filename, "wb");
	if (file == NULL) {
		VP2_WARN(vp2, "Failed to open config file for writing: %s, this is non-fatal", config_filename);
		goto load_config_end;
	}

	// Write the config to the file
	size_t to_write = config_size;
	while (to_write > 0) {
		size_t written = fwrite(config + (config_size - to_write), 1, to_write, file);
		if (written == 0) {
			VP2_WARN(vp2, "Failed to write to config file: %s, this is non-fatal", config_filename);
			break;
		}
		to_write -= written;
	}

	fflush(file);
	fclose(file);

	VP2_DEBUG(vp2, "Wrote Vive Pro 2 config file.");

load_config_end:
	free(config);

	return ret;
}

static int
vp2_send_noise_cancelling_commands(struct vp2_hid *vp2, const char **commands, size_t command_count)
{
	for (size_t i = 0; i < command_count; i++) {
		const char *command = commands[i];

		int ret = vp2_write_feature(vp2, VP2_REPORT_ID_FEATURE, VP2_COMMAND_CODEC_REQUEST,
		                            (const uint8_t *)command, strlen(command));
		if (ret < 0) {
			VP2_ERROR(vp2, "Failed to write noise cancelling command: %s", command);
			return ret;
		}
	}
	return 0;
}

static int
vp2_enable_noise_cancelling(struct vp2_hid *vp2)
{
	const char *commands[] = {
	    "codecreg=9c9,80",   "codecreg=9c8,a5",   "codecreg=9d0,a4",
	    "codecreg=1c008f,1", "codecreg=1c0005,9", "codecreg=1c0005,8000",
	};

	return vp2_send_noise_cancelling_commands(vp2, commands, ARRAY_SIZE(commands));
}

static int
vp2_disable_noise_cancelling(struct vp2_hid *vp2)
{
	const char *commands[] = {
	    "codecreg=9c9,8c",   "codecreg=9c8,a4",   "codecreg=9d0,0",
	    "codecreg=1c008f,0", "codecreg=1c0005,9", "codecreg=1c0005,8000",
	};

	return vp2_send_noise_cancelling_commands(vp2, commands, ARRAY_SIZE(commands));
}

static int
vp2_write_feature_event(struct vp2_hid *vp2, const struct vp2_feature_event *event)
{
	return vp2_write_feature(vp2, VP2_REPORT_ID_FEATURE, VP2_COMMAND_FEATURE_EVENT, (const uint8_t *)event,
	                         sizeof(*event));
}

static int
vp2_set_status_polling(struct vp2_hid *vp2, bool enable)
{
	struct vp2_feature_event event = {
	    .valid_mask = __cpu_to_le64(VP2_FEATURE_EVENT_STATUS_REPORT),
	    .status_report_enable = enable ? 1 : 0,
	};
	return vp2_write_feature_event(vp2, &event);
}

static int
vp2_set_power_state(struct vp2_hid *vp2, enum vp2_power_state state)
{
	// HTC sends 122/123/124 for ready/idle/standby.
	struct vp2_feature_event event = {
	    .valid_mask = __cpu_to_le64(VP2_FEATURE_EVENT_POWER_STATE),
	    .power_state = (uint8_t)state ^ VP2_POWER_STATE_XOR,
	};
	return vp2_write_feature_event(vp2, &event);
}

//! Any GET_FEATURE of VP2_REPORT_ID_FEATURE returns the state block, no request is needed.
static int
vp2_get_feature_state(struct vp2_hid *vp2, struct vp2_feature_state *out_state)
{
	int ret = os_hid_get_feature(vp2->hid, VP2_REPORT_ID_FEATURE, (uint8_t *)out_state, sizeof(*out_state));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to get feature state, got %d.", ret);
		return ret;
	}

	if (ret < (int)sizeof(*out_state) || out_state->id != VP2_REPORT_ID_FEATURE ||
	    __le16_to_cpu(out_state->sub_id) != VP2_COMMAND_FEATURE_STATE) {
		VP2_ERROR(vp2, "Unexpected feature state report.");
		return -1;
	}

	return 0;
}

//! Sends a display request getter and reads back its reply.
static int
vp2_display_query(struct vp2_hid *vp2, const char *request, char *out, size_t out_size)
{
	int ret = vp2_write_feature(vp2, VP2_REPORT_ID_FEATURE, VP2_COMMAND_DISPLAY_REQUEST, (const uint8_t *)request,
	                            strlen(request));
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to write display request %s.", request);
		return ret;
	}

	return vp2_string_command(vp2, "mfg-s-hdmi", out, out_size);
}

static const char *
vp2_power_state_to_string(uint8_t state)
{
	switch (state) {
	case 1:
	case VP2_POWER_STATE_READY: return "ready";
	case VP2_POWER_STATE_IDLE: return "idle";
	case VP2_POWER_STATE_STANDBY: return "standby";
	case VP2_POWER_STATE_SLEEP: return "sleep";
	default: return "unknown";
	}
}

//! Some replies are several lines, put them on one.
static void
vp2_flatten_reply(char *reply)
{
	for (char *c = reply; *c != '\0'; c++) {
		if (*c == '\n' || *c == '\r') {
			*c = ' ';
		}
	}

	size_t length = strlen(reply);
	while (length > 0 && reply[length - 1] == ' ') {
		reply[--length] = '\0';
	}
}

static void
vp2_log_feature_state(struct vp2_hid *vp2, const struct vp2_feature_state *state)
{
	uint8_t flags = state->display_flags;

	VP2_DEBUG(vp2, "\tMCU version: %" PRIx32, __le32_to_cpu(state->mcu_version));
	VP2_DEBUG(vp2, "\tBootloader version: %" PRIx32, __le32_to_cpu(state->bootloader_version));
	VP2_DEBUG(vp2, "\tCPLD version: %u", state->cpld_version);
	VP2_DEBUG(vp2, "\tAudio version: %u", state->audio_version);
	VP2_DEBUG(vp2, "\tVideo bridge version: %u.%u, revision 0x%04x", state->bridge_version >> 4,
	          state->bridge_version & 0xF, __le16_to_cpu(state->bridge_revision));
	VP2_DEBUG(vp2, "\tUnknown version: %" PRIx64, (uint64_t)__le64_to_cpu(state->unknown_version));
	VP2_DEBUG(vp2, "\tHardware ID: 0x%02x", state->hardware_id);
	VP2_DEBUG(vp2, "\tFirmware variant: %u", state->firmware_variant);
	VP2_DEBUG(vp2, "\tWireless mode: %u", state->wireless_mode);
	VP2_DEBUG(vp2, "\tFactory IPD ADC range: %u to %u", __le16_to_cpu(state->ipd_adc_min),
	          __le16_to_cpu(state->ipd_adc_max));
	VP2_DEBUG(vp2, "\tFactory proximity calibration: %u %u %u %u %u %u %u %u",
	          __le16_to_cpu(state->proximity_calibration[0]), __le16_to_cpu(state->proximity_calibration[1]),
	          __le16_to_cpu(state->proximity_calibration[2]), __le16_to_cpu(state->proximity_calibration[3]),
	          __le16_to_cpu(state->proximity_calibration[4]), __le16_to_cpu(state->proximity_calibration[5]),
	          __le16_to_cpu(state->proximity_calibration[6]), __le16_to_cpu(state->proximity_calibration[7]));
	VP2_DEBUG(vp2, "\tDisplay flags: 0x%02x (on %d, bridge on %d, panels initialized %d, wireless %d)", flags,
	          (flags & VP2_DISPLAY_FLAG_ON) != 0, (flags & VP2_DISPLAY_FLAG_BRIDGE_ON) != 0,
	          (flags & VP2_DISPLAY_FLAG_PANELS_INITIALIZED) != 0, (flags & VP2_DISPLAY_FLAG_WIRELESS) != 0);
	VP2_DEBUG(vp2, "\tPower state: %u (%s)", state->power_state, vp2_power_state_to_string(state->power_state));
	VP2_DEBUG(vp2, "\tCPLD state: %u", state->cpld_state);
	VP2_DEBUG(vp2, "\tBrightness: %u", state->brightness);
	VP2_DEBUG(vp2, "\tAmp gain: 0x%02x", state->amp_gain);
	VP2_DEBUG(vp2, "\tAmp power limit: %u", state->amp_power_limit);
	VP2_DEBUG(vp2, "\tStatus reports enabled: %u", state->status_report_enabled);
}

/*!
 * Logs everything the headset reports about itself. This reads replies, so it has to run while the status thread
 * isn't running and the status reports are off. Only read-only commands are used.
 */
static void
vp2_log_status(struct vp2_hid *vp2)
{
	static const struct
	{
		const char *name;
		const char *command;
	} string_commands[] = {
	    {"PCB serial", "mfg-r-pcbsn"},
	    {"PCB ID", "mfg-r-pcbid"},
	    {"SKU ID", "mfg-r-skuid"},
	    {"Video bridge (chip ID, firmware, hotplug)", "mfg-s-bridgeid"},
	    {"Audio codec ID", "mfg-s-codecid"},
	    {"Audio", "mfg-s-audioinfo"},
	    {"Proximity sensor", "mfg-r-ps_info"},
	    {"Panels (flags, power state, 0, CPLD 3, frame periods, bridge IRQ)", "mfg-s-panel"},
	};

	static const struct
	{
		const char *name;
		const char *request;
	} display_queries[] = {
	    {"Bridge firmware", "fwname"},
	    {"DP link state", "mlstate"},
	    {"DP RX status", "rx"},
	    {"DP bandwidth", "bw"},
	    {"DP lane errors", "errcnt"},
	    {"HDCP", "hdcp"},
	    {"Input timing 0", "rfrm0"},
	    {"Input timing 1", "rfrm1"},
	    {"Output timing 0", "tx0"},
	    {"Output timing 1", "tx1"},
	    {"Frame rate", "framerate"},
	    {"Bridge TX GPIO", "gpio"},
	    {"Backlight brightness", "getbrightness"},
	    {"Panel ready, sleep in sent", "panelstatus"},
	};

	char reply[VP2_DATA_SIZE];

	VP2_DEBUG(vp2, "Headset status:");

	struct vp2_feature_state state;
	if (vp2_get_feature_state(vp2, &state) == 0) {
		U_LOG_IFL_T_HEX(vp2->log_level, (const uint8_t *)&state, sizeof(state));
		vp2_log_feature_state(vp2, &state);
	}

	for (size_t i = 0; i < ARRAY_SIZE(string_commands); i++) {
		if (vp2_string_command(vp2, string_commands[i].command, reply, sizeof(reply)) < 0) {
			continue;
		}
		vp2_flatten_reply(reply);
		VP2_DEBUG(vp2, "\t%s: %s", string_commands[i].name, reply);
	}

	bool panel_ready = false;
	for (size_t i = 0; i < ARRAY_SIZE(display_queries); i++) {
		if (vp2_display_query(vp2, display_queries[i].request, reply, sizeof(reply)) < 0) {
			continue;
		}
		VP2_DEBUG(vp2, "\t%s: %s", display_queries[i].name, reply);

		if (strcmp(display_queries[i].request, "panelstatus") == 0) {
			panel_ready = strtol(reply, NULL, 10) != 0;
		}
	}

	// Reads from the panels, the firmware's own panel reads check that they are powered first.
	if (panel_ready) {
		if (vp2_display_query(vp2, "powermode", reply, sizeof(reply)) == 0) {
			VP2_DEBUG(vp2, "\tPanel power mode (DCS 0x0A, 0x9c is healthy): %s", reply);
		}
	} else {
		VP2_DEBUG(vp2, "\tPanel power mode: panels are off");
	}
}

/*!
 * The buttons glitch for single status reports, for example mute flashing on while pressing the volume buttons, so a
 * new button state is only accepted once it has been seen in this many reports in a row. A status report comes in
 * roughly every 52ms.
 */
#define VP2_BUTTON_DEBOUNCE_REPORTS 2

//! Button bytes from a status report, system in the high byte and the audio button in the low byte.
static uint16_t
get_raw_buttons(const struct vp2_status_report *report)
{
	return (uint16_t)(report->system_button << 8) | report->audio_button;
}

static void
handle_status_packet(struct vp2_hid *vp2, const struct vp2_status_report *report, uint16_t buttons)
{
	uint16_t proximity = __le16_to_cpu(report->proximity);
	uint16_t lens_separation = __le16_to_cpu(report->ipd);

	bool system = (buttons >> 8) != 0;
	bool volume_up = buttons == VP2_AUDIO_BUTTON_VOLUME_UP;
	bool volume_down = buttons == VP2_AUDIO_BUTTON_VOLUME_DOWN;
	bool mute_mic = buttons == VP2_AUDIO_BUTTON_MUTE_MIC;

	int64_t packed = proximity |                        //
	                 ((int64_t)lens_separation << 16) | //
	                 ((int64_t)volume_down << 32) |     //
	                 ((int64_t)volume_up << 33) |       //
	                 ((int64_t)system << 34) |          //
	                 ((int64_t)mute_mic << 35);         //

	// store the status
	xrt_atomic_s64_store(&vp2->status, packed);
}

static void *
status_thread_runner(void *ptr)
{
	int ret;
	struct vp2_hid *vp2 = ptr;

	//! See @ref VP2_BUTTON_DEBOUNCE_REPORTS.
	uint16_t buttons = 0;
	uint16_t buttons_candidate = 0;
	int buttons_candidate_count = 0;

	os_thread_helper_lock(&vp2->thread);
	while (os_thread_helper_is_running_locked(&vp2->thread)) {
		os_thread_helper_unlock(&vp2->thread);

		timepoint_ns now = os_monotonic_get_ns();
		if (now - vp2->last_status_report_ns > (U_TIME_1S_IN_NS * 2L)) {
			// re-enable status polling
			ret = vp2_set_status_polling(vp2, true);
			if (ret < 0) {
				VP2_WARN(vp2, "Unable to enable status polling, got %d.", ret);
			}
		}

		struct vp2_status_report report;
		ret = os_hid_read(vp2->hid, (uint8_t *)&report, sizeof(report), 500);
		if (ret < 0) {
			VP2_ERROR(vp2, "Failed to read from HID, got %d.", ret);
			os_thread_helper_lock(&vp2->thread);
			break;
		}

		if (ret == 0) {
			os_thread_helper_lock(&vp2->thread);
			continue;
		}

		bool is_good = true;
		// Only the fields up to the IPD are used.
		size_t used_size = offsetof(struct vp2_status_report, ipd) + sizeof(report.ipd);
		if (ret < (int)used_size || report.id != VP2_REPORT_ID_STATUS ||
		    __le16_to_cpu(report.sub_id) != VP2_COMMAND_STATUS_REPORT) {
			VP2_DEBUG(vp2, "Got non-status packet:");
			// U_LOG_IFL_D_HEX(vp2->log_level, (uint8_t *)&report, ret);
			is_good = false;
		}

		if (is_good) {
			uint16_t raw_buttons = get_raw_buttons(&report);
			if (raw_buttons != buttons_candidate) {
				// VP2_DEBUG(vp2, "Raw buttons changed: %04x", raw_buttons);
				buttons_candidate = raw_buttons;
				buttons_candidate_count = 0;
			}
			if (++buttons_candidate_count >= VP2_BUTTON_DEBOUNCE_REPORTS) {
				buttons = buttons_candidate;
			}

			handle_status_packet(vp2, &report, buttons);
			vp2->last_status_report_ns = os_monotonic_get_ns();
		}

		os_thread_helper_lock(&vp2->thread);
	}
	os_thread_helper_unlock(&vp2->thread);

	return NULL;
}

/*
 *
 * 'Exported' functions.
 *
 */

int
vp2_hid_open(struct os_hid_device *hid_dev, struct vp2_hid **out_hid)
{
	int ret;

	struct vp2_hid *vp2 = U_TYPED_CALLOC(struct vp2_hid);

	vp2->log_level = debug_get_log_option_vp2_log();
	vp2->hid = hid_dev;
	vp2->resolution = (enum vp2_resolution)debug_get_num_option_vp2_resolution();

	VP2_INFO(vp2, "Opened Vive Pro 2 HID device.");

	// The status stream stays on across sessions, its reports would get in the way of the replies we read below.
	ret = vp2_set_status_polling(vp2, false);
	if (ret < 0) {
		VP2_WARN(vp2, "Failed to disable status polling.");
	}
	// Drain reads to prevent stale data from borking serial/config reads.
	vp2_drain_reads(vp2);

	ret = vp2_load_serial(vp2);
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to read serial number.");
		vp2_hid_destroy(vp2);
		return ret;
	}

	ret = vp2_load_config(vp2);
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to load config.");
		vp2_hid_destroy(vp2);
		return ret;
	}

	ret = vp2_set_resolution(vp2, vp2->resolution);
	if (ret < 0) {
		VP2_WARN(vp2, "Failed to set resolution.");
	}

	// The display stays off after idle (from vp2_prepare_shutdown or the HTC driver) until ready is sent, the HTC
	// driver also sends this on activate.
	ret = vp2_set_power_state(vp2, VP2_POWER_STATE_READY);
	if (ret < 0) {
		VP2_WARN(vp2, "Failed to set ready power state.");
	}

	ret = vp2_set_noise_cancelling(vp2, debug_get_bool_option_vp2_noise_cancelling());
	if (ret < 0) {
		VP2_WARN(vp2, "Failed to set noise cancelling.");
	}

	vp2->brightness = 1.0f;
	ret = vp2_set_brightness(vp2, debug_get_float_option_vp2_default_brightness());
	if (ret < 0) {
		VP2_WARN(vp2, "Failed to set default brightness.");
	}

	// @note This sleep should not be necessary here. monado needs to gain the ability to wait for displays to
	//       appear.
	os_nanosleep(U_TIME_1S_IN_NS * 10llu); // wait for display chip to reset

	// Needs to be done before the status thread starts, since it reads replies. Skipped when it wouldn't be logged.
	if (vp2->log_level <= U_LOGGING_DEBUG) {
		vp2_log_status(vp2);
	}

	ret = os_thread_helper_init(&vp2->thread);
	if (ret != 0) {
		VP2_ERROR(vp2, "Failed to init status thread.");
		vp2_hid_destroy(vp2);
		return ret;
	}

	ret = os_thread_helper_start(&vp2->thread, status_thread_runner, vp2);
	if (ret != 0) {
		VP2_ERROR(vp2, "Failed to start status thread.");
		vp2_hid_destroy(vp2);
		return ret;
	}

	*out_hid = vp2;

	return 0;
}

enum vp2_resolution
vp2_get_resolution(struct vp2_hid *vp2)
{
	assert(vp2 != NULL);

	return vp2->resolution;
}

struct vp2_config *
vp2_get_config(struct vp2_hid *vpd)
{
	assert(vpd != NULL);

	return &vpd->config;
}

const char *
vp2_get_serial(struct vp2_hid *vp2)
{
	assert(vp2 != NULL);

	return vp2->serial;
}

int
vp2_set_noise_cancelling(struct vp2_hid *vp2, bool enable)
{
	return enable ? vp2_enable_noise_cancelling(vp2) : vp2_disable_noise_cancelling(vp2);
}

float
vp2_get_brightness(struct vp2_hid *vp2)
{
	return vp2->brightness;
}

int
vp2_set_brightness(struct vp2_hid *vp2, float brightness)
{
	brightness = CLAMP(brightness, 0.0f, 1.3f);

	int brightness_value = (int)(brightness * 100.0f);

	char command[VP2_DATA_SIZE] = {0};
	// NOTE: this is safe because brightness_value is clamped to [0, 130]
	int command_length = snprintf(command, sizeof(command), "setbrightness,%d", brightness_value);

	int ret = vp2_write_feature(vp2, VP2_REPORT_ID_FEATURE, VP2_COMMAND_DISPLAY_REQUEST, (const uint8_t *)command,
	                            command_length);

	if (ret >= 0) {
		vp2->brightness = brightness;
	}

	return ret;
}

int
vp2_prepare_shutdown(struct vp2_hid *vp2)
{
	/*
	 * The MCU only sends display off and sleep in to the panels if video is still coming in when it powers them
	 * off, otherwise it just cuts their power, which can leave charge in the LCD pixels. So this needs to be called
	 * while the compositor is still scanning out, the same as the HTC driver going idle at the start of deactivate.
	 */
	int ret = vp2_set_power_state(vp2, VP2_POWER_STATE_IDLE);
	if (ret < 0) {
		VP2_ERROR(vp2, "Failed to set idle power state.");
		return ret;
	}

	// The power off sequence takes about 20ms once the MCU picks up the request, keep video going until it's done.
	os_nanosleep(U_TIME_1MS_IN_NS * 50);

	return 0;
}

bool
vp2_get_status(struct vp2_hid *vp2,
               bool *out_proximity,
               float *out_lens_separation_meters,
               bool *out_system,
               bool *out_volume_up,
               bool *out_volume_down,
               bool *out_mute_mic)
{
	int64_t packed = xrt_atomic_s64_load(&vp2->status);
	if (packed == 0) {
		return false;
	}

	uint16_t proximity = packed & 0xffff;
	uint16_t lens_separation = (packed >> 16) & 0xffff;
	uint16_t volume_down = (packed >> 32) & 1;
	uint16_t volume_up = (packed >> 33) & 1;
	uint16_t system = (packed >> 34) & 1;
	uint16_t mute_mic = (packed >> 35) & 1;

	if (out_proximity) {
		*out_proximity = proximity > VP2_PROXIMITY_THRESHOLD;
	}
	if (out_lens_separation_meters) {
		*out_lens_separation_meters = (float)lens_separation * VP2_IPD_UNIT_METERS;
	}
	if (out_system) {
		*out_system = system;
	}
	if (out_volume_up) {
		*out_volume_up = volume_up;
	}
	if (out_volume_down) {
		*out_volume_down = volume_down;
	}
	if (out_mute_mic) {
		*out_mute_mic = mute_mic;
	}

	return true;
}

void
vp2_hid_destroy(struct vp2_hid *vp2)
{
	if (vp2->thread.initialized) {
		os_thread_helper_destroy(&vp2->thread);
	}

	if (vp2->hid != NULL) {
		os_hid_destroy(vp2->hid);
		vp2->hid = NULL;
	}

	VP2_TRACE(vp2, "Destroyed Vive Pro 2 HID device.");

	free(vp2);
}
