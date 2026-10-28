// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Small cli application to control IPC service.
 * @author Pete Black <pblack@collabora.com>
 * @ingroup ipc
 */

#include "util/u_file.h"

#include "client/ipc_client.h"
#include "client/ipc_client_connection.h"

#include "ipc_client_generated.h"
#include "xrt/xrt_results.h"

#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


#define P(...) fprintf(stdout, __VA_ARGS__)
#define PE(...) fprintf(stderr, __VA_ARGS__)

typedef enum op_mode
{
	MODE_GET,
	MODE_SET_PRIMARY,
	MODE_SET_FOCUSED,
	MODE_TOGGLE_IO,
	MODE_RECENTER,
	MODE_GET_BRIGHTNESS,
	MODE_SET_BRIGHTNESS,
} op_mode_t;


struct full_device_info
{
	struct ipc_device_list list;
	struct ipc_device_info info[XRT_SYSTEM_MAX_DEVICES];
};

/*
 *
 * Helper functions.
 *
 */

/*!
 * Get the device list from a given connection.
 *
 * @param ipc_c The IPC connection.
 * @param[out] out_full The full device info struct to fill out.
 * @return XRT_SUCCESS on success, or an error code.
 */
static xrt_result_t
get_device_list(struct ipc_connection *ipc_c, struct full_device_info *out_full)
{
	xrt_result_t xret = XRT_SUCCESS;
	xret = ipc_call_system_devices_get_list(ipc_c, &out_full->list);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	for (uint32_t i = 0; i < out_full->list.device_count; i++) {
		uint32_t device_id = out_full->list.devices[i].id;

		xret = ipc_call_device_get_info_no_arrays(ipc_c, device_id, &out_full->info[i]);
		if (xret != XRT_SUCCESS) {
			PE("Failed to get device info for device %u.\n", device_id);
			return xret;
		}
	}

	return XRT_SUCCESS;
}

int
get_mode(struct ipc_connection *ipc_c)
{
	struct ipc_client_list clients;

	xrt_result_t r;

	r = ipc_call_system_get_clients(ipc_c, &clients);
	if (r != XRT_SUCCESS) {
		PE("Failed to get client list.\n");
		exit(1);
	}

	P("Clients:\n");
	for (uint32_t i = 0; i < clients.id_count; i++) {
		uint32_t id = clients.ids[i];

		struct ipc_app_state cs;
		r = ipc_call_system_get_client_info(ipc_c, id, &cs);
		if (r != XRT_SUCCESS) {
			PE("Failed to get client info for client %d.\n", id);
			return 1;
		}

		P("\tid: %d"
		  "\tact: %d"
		  "\tdisp: %d"
		  "\tfoc: %d"
		  "\tposes: %d"
		  "\thands: %d"
		  "\tinputs: %d"
		  "\toutputs: %d"
		  "\tovly: %d"
		  "\tz: %d"
		  "\tpid: %d"
		  "\t%s\n",
		  clients.ids[i],                    //
		  cs.session_active,                 //
		  cs.session_visible,                //
		  cs.session_focused,                //
		  !cs.io_blocks.block_poses,         //
		  !cs.io_blocks.block_hand_tracking, //
		  !cs.io_blocks.block_inputs,        //
		  !cs.io_blocks.block_outputs,       //
		  cs.session_overlay,                //
		  cs.z_order,                        //
		  cs.pid,                            //
		  cs.info.application_name);
	}

	struct full_device_info full_device_info;
	r = get_device_list(ipc_c, &full_device_info);
	if (r != XRT_SUCCESS) {
		PE("Failed to get device list.\n");
		return 1;
	}

	P("\nDevices:\n");
	for (uint32_t i = 0; i < full_device_info.list.device_count; i++) {
		P("\tid: %d"
		  "\tname: %d"
		  "\t\"%s\"\n",
		  full_device_info.list.devices[i].id, //
		  full_device_info.info[i].name,       //
		  full_device_info.info[i].str);       //
	}

	return 0;
}

int
set_primary(struct ipc_connection *ipc_c, int client_id)
{
	xrt_result_t r;

	r = ipc_call_system_set_primary_client(ipc_c, client_id);
	if (r != XRT_SUCCESS) {
		PE("Failed to set active client to %d.\n", client_id);
		return 1;
	}

	return 0;
}

int
set_focused(struct ipc_connection *ipc_c, int client_id)
{
	xrt_result_t r;

	r = ipc_call_system_set_focused_client(ipc_c, client_id);
	if (r != XRT_SUCCESS) {
		PE("Failed to set focused client to %d.\n", client_id);
		return 1;
	}

	return 0;
}

int
toggle_io(struct ipc_connection *ipc_c, int client_id)
{
	xrt_result_t r;

	r = ipc_call_system_toggle_io_client(ipc_c, client_id);
	if (r != XRT_SUCCESS) {
		PE("Failed to toggle io for client %d.\n", client_id);
		return 1;
	}

	return 0;
}

int
recenter_local_spaces(struct ipc_connection *ipc_c)
{
	xrt_result_t r;

	r = ipc_call_space_recenter_local_spaces(ipc_c);
	if (r != XRT_SUCCESS) {
		PE("Failed to recenter local spaces.\n");
		return 1;
	}

	return 0;
}

int
get_first_device_id(struct ipc_connection *ipc_c)
{
	struct full_device_info full_device_info;
	xrt_result_t xret;

	// Get device list using the new API
	xret = get_device_list(ipc_c, &full_device_info);
	if (xret != XRT_SUCCESS) {
		PE("Failed to get device list.\n");
		return -1;
	}

	// Look for the first HMD device
	for (uint32_t i = 0; i < full_device_info.list.device_count; i++) {
		if (full_device_info.list.devices[i].device_type == XRT_DEVICE_TYPE_HMD) {
			uint32_t device_id = full_device_info.list.devices[i].id;

			// Print to stderr to enable scripting
			PE("Picked device %u: %s\n", device_id, full_device_info.info[i].str);

			return device_id;
		}
	}

	return -1;
}

int
get_brightness(struct ipc_connection *ipc_c, int device_id)
{
	device_id = device_id >= 0 ? device_id : get_first_device_id(ipc_c);
	if (device_id < 0) {
		PE("Couldn't find a HMD device!\n");
		return 1;
	}

	float out_brightness;
	xrt_result_t r = ipc_call_device_get_brightness(ipc_c, device_id, &out_brightness);
	if (r != XRT_SUCCESS) {
		PE("Failed to get brightness for device %d\n", device_id);
		return 1;
	}

	P("%d\n", (int)(out_brightness * 100));

	return 0;
}

int
set_brightness(struct ipc_connection *ipc_c, int device_id, const char *value)
{
	if (value == NULL) {
		return 1;
	}

	const int length = strlen(value);
	if (length == 0) {
		return 1;
	}

	bool relative = (value[0] == '-' || value[0] == '+');

	char *end = NULL;
	float target_brightness = strtof(value, &end);

	if ((length > (end - value)) && *end == '%') {
		target_brightness /= 100.f;
	}

	device_id = device_id >= 0 ? device_id : get_first_device_id(ipc_c);
	if (device_id < 0) {
		PE("Couldn't find a HMD device!\n");
		return 1;
	}

	xrt_result_t r = ipc_call_device_set_brightness(ipc_c, device_id, target_brightness, relative);
	if (r != XRT_SUCCESS) {
		PE("Failed to set brightness for device %d\n", device_id);
		return 1;
	}

	float out_brightness;
	r = ipc_call_device_get_brightness(ipc_c, device_id, &out_brightness);
	if (r != XRT_SUCCESS) {
		PE("Failed to get brightness for device %d\n", device_id);
		return 1;
	}

	if (relative || out_brightness != target_brightness) {
		P("Set brightness to %d%%\n", (int)(out_brightness * 100));
	}

	return 0;
}

enum LongOptions
{
	OPTION_DEVICE = 1,
	OPTION_GET_BRIGHTNESS,
	OPTION_SET_BRIGHTNESS,
};

int
main(int argc, char *argv[])
{
	op_mode_t op_mode = MODE_GET;

	// parse arguments
	int c;
	int s_val = 0;
	int device_val = -1;
	char *brightness;

	static struct option long_options[] = {
	    {"device", required_argument, NULL, OPTION_DEVICE},
	    {"get-brightness", no_argument, NULL, OPTION_GET_BRIGHTNESS},
	    {"set-brightness", required_argument, NULL, OPTION_SET_BRIGHTNESS},
	    {NULL, 0, NULL, 0},
	};

	int option_index = 0;
	opterr = 0;
	while ((c = getopt_long(argc, argv, "p:f:i:c", long_options, &option_index)) != -1) {
		switch (c) {
		case 'p':
			s_val = atoi(optarg);
			op_mode = MODE_SET_PRIMARY;
			break;
		case 'f':
			s_val = atoi(optarg);
			op_mode = MODE_SET_FOCUSED;
			break;
		case 'i':
			s_val = atoi(optarg);
			op_mode = MODE_TOGGLE_IO;
			break;
		case 'c': op_mode = MODE_RECENTER; break;
		case OPTION_DEVICE: {
			device_val = atoi(optarg);
			break;
		}
		case OPTION_GET_BRIGHTNESS: {
			op_mode = MODE_GET_BRIGHTNESS;
			break;
		}
		case OPTION_SET_BRIGHTNESS: {
			brightness = optarg;
			op_mode = MODE_SET_BRIGHTNESS;
			break;
		}
		case '?':
			if (optopt == 's') {
				PE("Option -s requires an id to set.\n");
			} else if (isprint(optopt)) {
				PE("Option `-%c' unknown. Usage:\n", optopt);
				PE("    -c: Recenter local spaces\n");
				PE("    -f <id>: Set focused client\n");
				PE("    -p <id>: Set primary client\n");
				PE("    -i <id>: Toggle whether client receives input\n");
				PE("    --device <id>: Set device for subsequent command, otherwise defaults to the "
				   "primary device\n");
				PE("    --get-brightness: Get current display brightness in percent\n");
				PE("    --set-brightness <[+-]brightness[%%]>: Set display brightness\n");
			} else {
				PE("Option `\\x%x' unknown.\n", optopt);
			}
			exit(1);
		default: exit(0);
		}
	}

	// Connection struct on the stack, super simple.
	struct ipc_connection ipc_c = {0};

	struct xrt_instance_info info = {
	    .app_info.application_name = "monado-ctl",
	};

	xrt_result_t xret = ipc_client_connection_init(&ipc_c, U_LOGGING_INFO, &info);
	if (xret != XRT_SUCCESS) {
		U_LOG_E("ipc_client_connection_init: %u", xret);
		return -1;
	}

	bool is_system_available = false;
	xret = ipc_call_instance_is_system_available(&ipc_c, &is_system_available);
	if (xret != XRT_SUCCESS) {
		U_LOG_E("ipc_call_instance_is_system_available: %u", xret);
		return -1;
	}
	if (!is_system_available) {
		PE("System isn't available, devices won't be available!");
	}

	switch (op_mode) {
	case MODE_GET: exit(get_mode(&ipc_c)); break;
	case MODE_SET_PRIMARY: exit(set_primary(&ipc_c, s_val)); break;
	case MODE_SET_FOCUSED: exit(set_focused(&ipc_c, s_val)); break;
	case MODE_TOGGLE_IO: exit(toggle_io(&ipc_c, s_val)); break;
	case MODE_RECENTER: exit(recenter_local_spaces(&ipc_c)); break;
	case MODE_GET_BRIGHTNESS: exit(get_brightness(&ipc_c, device_val)); break;
	case MODE_SET_BRIGHTNESS: exit(set_brightness(&ipc_c, device_val, brightness)); break;
	default: P("Unrecognised operation mode.\n"); exit(1);
	}

	return 0;
}
