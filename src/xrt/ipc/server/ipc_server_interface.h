// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface for IPC server code.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup ipc_server
 */

#pragma once

#include "xrt/xrt_config_os.h"
#include "xrt/xrt_results.h"

#include "util/u_debug_gui.h"


#ifdef __cplusplus
extern "C" {
#endif


struct xrt_instance;
struct ipc_server;

/*!
 * Information passed into the IPC server main function, used for customization
 * of the IPC server.
 *
 * @ingroup ipc_server
 */
struct ipc_server_main_info
{
	//! Information passed onto the debug gui.
	struct u_debug_gui_create_info udgci;

	//! Flag whether runtime should exit on app disconnect.
	bool exit_on_disconnect;

	//! Disable listening on stdin for server stop.
	bool no_stdin;
};

/*!
 *
 * @ingroup ipc_server
 */
struct ipc_server_callbacks
{
	/*!
	 * The IPC server failed to init.
	 *
	 * @param[in] xret The error code generated during init.
	 * @param[in] data User data given passed into the main function.
	 */
	void (*init_failed)(xrt_result_t xret, void *data);

	/*!
	 * The service has completed init and is entering its mainloop.
	 *
	 * @param[in] s     The IPC server.
	 * @param[in] xinst Instance that was created by the IPC server.
	 * @param[in] data  User data given passed into the main function.
	 */
	void (*mainloop_entering)(struct ipc_server *s, struct xrt_instance *xinst, void *data);

	/*!
	 * The service is leaving the mainloop, after this callback returns the
	 * IPC server will destroy all resources created.
	 *
	 * @param[in] s     The IPC server.
	 * @param[in] xinst Instance that was created by the IPC server.
	 * @param[in] data  User data given passed into the main function.
	 */
	void (*mainloop_leaving)(struct ipc_server *s, struct xrt_instance *xinst, void *data);

	/*!
	 * A new client has connected to the IPC server.
	 *
	 * param s     The IPC server.
	 * param client_id The ID of the newly connected client.
	 * param data  User data given passed into the main function.
	 */
	void (*client_connected)(struct ipc_server *s, uint32_t client_id, void *data);

	/*!
	 * A client has disconnected from the IPC server.
	 *
	 * param s     The IPC server.
	 * param client_id The ID of the newly connected client.
	 * param data  User data given passed into the main function.
	 */
	void (*client_disconnected)(struct ipc_server *s, uint32_t client_id, void *data);
};

/*!
 * Common main function for starting the IPC service.
 *
 * @ingroup ipc_server
 */
int
ipc_server_main_common(const struct ipc_server_main_info *ismi, const struct ipc_server_callbacks *iscb, void *data);

/*!
 * Asks the server to shut down, this call is asynchronous and will return
 * immediately. Use callbacks to be notified when the server stops.
 *
 * @memberof ipc_server
 */
int
ipc_server_stop(struct ipc_server *s);

#ifndef XRT_OS_ANDROID

/*!
 * Main entrypoint to the compositor process.
 *
 * @ingroup ipc_server
 */
int
ipc_server_main(int argc, char **argv, const struct ipc_server_main_info *ismi);

#endif


#ifdef __cplusplus
}
#endif
