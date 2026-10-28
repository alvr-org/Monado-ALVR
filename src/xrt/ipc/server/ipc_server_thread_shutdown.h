// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC client thread shutdown and blocking I/O cancellation.
 * @author Nick Kennedy <code@nick-kennedy.com>
 * @ingroup ipc_server
 */

#pragma once
#include "xrt/xrt_config_os.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ipc_server;
struct ipc_thread;

//! Do not revive a starting thread that shutdown has already stopped.
void
ipc_server_client_thread_set_running(struct ipc_server *s, struct ipc_thread *it);

//! Read the loop state under the same lock used to stop or start it.
bool
ipc_server_client_thread_is_running(struct ipc_server *s, struct ipc_thread *it);

/*!
 * Stop all client loops, wake blocking I/O, then join each thread once.
 * Called by the server main thread after it stops accepting connections.
 * Joining finishes client cleanup before the caller destroys server resources or restarts the main loop.
 * Calls must be serialized on that main thread. Safe to repeat: joined slots are READY and no longer own a thread.
 */
void
ipc_server_stop_and_join_client_threads(struct ipc_server *s);

#ifdef __cplusplus
}
#endif
