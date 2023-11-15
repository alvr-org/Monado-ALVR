// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface for IPC server code.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup ipc_server
 */

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_config_os.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XRT_OS_ANDROID

/*!
 * Main entrypoint to the compositor process.
 *
 * @ingroup ipc_server
 */
int
ipc_server_main(int argc, char **argv);

#endif


#ifdef XRT_OS_ANDROID

/*!
 * Main entrypoint to the server process.
 *
 * @param ps Pointer to populate with the server struct.
 * @param startup_complete_callback Function to call upon completing startup
 *                                  and populating *ps, but before entering
 *                                  the mainloop.
 * @param data user data to pass to your callback.
 *
 * @ingroup ipc_server
 */
int
ipc_server_main_android(struct ipc_server **ps, void (*startup_complete_callback)(void *data), void *data);

#endif


#ifdef __cplusplus
}
#endif
