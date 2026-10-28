// Copyright 2019-2022, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Main file for sdl compositor experiments.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#include "util/u_trace_marker.h"

#include "server/ipc_server_interface.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>


// Insert the on load constructor to init trace marker.
U_TRACE_TARGET_SETUP(U_TRACE_WHICH_SERVICE)


int
main(int argc, char *argv[])
{
	u_trace_marker_init();

	struct ipc_server_main_info ismi = {
	    .udgci =
	        {
	            .window_title = "Monado SDL Test Debug GUI",
	            .open = U_DEBUG_GUI_OPEN_AUTO,
	        },
	};

	return ipc_server_main(argc, argv, &ismi);
}
