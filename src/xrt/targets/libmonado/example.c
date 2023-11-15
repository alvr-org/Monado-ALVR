// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Small cli application to demonstrate use of libmonado.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 */

#include "monado.h"

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>


#define P(...) fprintf(stdout, __VA_ARGS__)
#define PE(...) fprintf(stderr, __VA_ARGS__)
#define CHECK_ID_EXIT(ID)                                                                                              \
	do {                                                                                                           \
		if (ID < 0 || ID > INT_MAX) {                                                                          \
			PE("Invalid client index %i.\n", s_val);                                                       \
			exit(1);                                                                                       \
		}                                                                                                      \
	} while (false)

typedef enum op_mode
{
	MODE_GET,
	MODE_SET_PRIMARY,
	MODE_SET_FOCUSED,
	MODE_TOGGLE_IO,
} op_mode_t;

int
get_mode(mnd_root_t *root)
{
	mnd_result_t mret = mnd_root_update_client_list(root);
	if (mret != MND_SUCCESS) {
		PE("Failed to get client list.\n");
		exit(1);
	}

	uint32_t num_clients = 0;
	mret = mnd_root_get_number_clients(root, &num_clients);
	if (mret != MND_SUCCESS) {
		PE("Failed to get client count.\n");
		exit(1);
	}

	P("Clients: (%d)\n", num_clients);
	for (uint32_t i = 0; i < num_clients; i++) {
		uint32_t client_id = 0;
		uint32_t flags = 0;
		const char *name = NULL;

		mret = mnd_root_get_client_id_at_index(root, i, &client_id);
		if (mret != MND_SUCCESS) {
			PE("Failed to get client id for index %u", i);
			continue;
		}

		mret = mnd_root_get_client_state(root, client_id, &flags);
		if (mret != MND_SUCCESS) {
			PE("Failed to get client state for client id: %u (index: %u)", client_id, i);
			continue;
		}

		mret = mnd_root_get_client_name(root, client_id, &name);
		if (mret != MND_SUCCESS) {
			PE("Failed to get client name for client id: %u (index: %u)", client_id, i);
			continue;
		}

		P("\tid: % 8d"
		  "\tact: %d"
		  "\tdisp: %d"
		  "\tfoc: %d"
		  "\tio: %d"
		  "\tovly: %d"
		  "\t%s\n",
		  client_id,                                         //
		  (flags & MND_CLIENT_SESSION_ACTIVE) != 0 ? 1 : 0,  //
		  (flags & MND_CLIENT_SESSION_VISIBLE) != 0 ? 1 : 0, //
		  (flags & MND_CLIENT_SESSION_FOCUSED) != 0 ? 1 : 0, //
		  (flags & MND_CLIENT_IO_ACTIVE) != 0 ? 1 : 0,       //
		  (flags & MND_CLIENT_SESSION_OVERLAY) != 0 ? 1 : 0, //
		  name);
	}

	return 0;
}

int
set_primary(mnd_root_t *root, int client_index)
{
	mnd_result_t mret = mnd_root_set_client_primary(root, client_index);
	if (mret != MND_SUCCESS) {
		PE("Failed to set active client to index %d.\n", client_index);
		return 1;
	}

	return 0;
}

int
set_focused(mnd_root_t *root, int client_index)
{
	mnd_result_t mret = mnd_root_set_client_focused(root, client_index);
	if (mret != MND_SUCCESS) {
		PE("Failed to set focused client to index %d.\n", client_index);
		return 1;
	}

	return 0;
}

int
toggle_io(mnd_root_t *root, int client_index)
{
	mnd_result_t mret = mnd_root_toggle_client_io_active(root, client_index);
	if (mret != MND_SUCCESS) {
		PE("Failed to toggle io for client index %d.\n", client_index);
		return 1;
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	op_mode_t op_mode = MODE_GET;

	// parse arguments
	int c;
	int s_val = 0;

	opterr = 0;
	while ((c = getopt(argc, argv, "p:f:i:")) != -1) {
		switch (c) {
		case 'p':
			s_val = atoi(optarg);
			CHECK_ID_EXIT(s_val);
			op_mode = MODE_SET_PRIMARY;
			break;
		case 'f':
			s_val = atoi(optarg);
			CHECK_ID_EXIT(s_val);
			op_mode = MODE_SET_FOCUSED;
			break;
		case 'i':
			s_val = atoi(optarg);
			CHECK_ID_EXIT(s_val);
			op_mode = MODE_TOGGLE_IO;
			break;
		case '?':
			if (optopt == 's') {
				PE("Option -s requires a client index to set.\n");
			} else if (isprint(optopt)) {
				PE("Option `-%c' unknown. Usage:\n", optopt);
				PE("    -f <index>: Set focused client\n");
				PE("    -p <index>: Set primary client\n");
				PE("    -i <index>: Toggle whether client receives input\n");
			} else {
				PE("Option `\\x%x' unknown.\n", optopt);
			}
			exit(1);
		default: exit(0);
		}
	}

	mnd_root_t *root = NULL;
	mnd_result_t mret;

	mret = mnd_root_create(&root);
	if (mret != MND_SUCCESS) {
		PE("Failed to connect.");
		return 1;
	}

	mret = mnd_root_update_client_list(root);
	if (mret != MND_SUCCESS) {
		PE("Failed to update client list.");
		return 1;
	}

	switch (op_mode) {
	case MODE_GET: exit(get_mode(root)); break;
	case MODE_SET_PRIMARY: exit(set_primary(root, s_val)); break;
	case MODE_SET_FOCUSED: exit(set_focused(root, s_val)); break;
	case MODE_TOGGLE_IO: exit(toggle_io(root, s_val)); break;
	default: P("Unrecognised operation mode.\n"); exit(1);
	}

	return 0;
}
