// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  More-internal client side code.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup ipc_client
 */

#pragma once

#include "xrt/xrt_results.h"
#include "ipc_client.h"


/*!
 * Set up the basics of the client connection: socket and shared mem
 * @param ipc_c     Empty IPC connection struct
 * @param log_level Log level for IPC messages
 * @param i_info    Instance info to send to server
 * @return XRT_SUCCESS on success
 *
 * @ingroup ipc_client
 */
xrt_result_t
ipc_client_connection_init(struct ipc_connection *ipc_c,
                           enum u_logging_level log_level,
                           const struct xrt_instance_info *i_info);

/*!
 * Locks the connection, allows sending complex messages.
 *
 * @param ipc_c The IPC connection to lock.
 *
 * @ingroup ipc_client
 */
static inline void
ipc_client_connection_lock(struct ipc_connection *ipc_c)
{
	os_mutex_lock(&ipc_c->mutex);
}

/*!
 * Unlocks the connection.
 *
 * @param ipc_c A locked IPC connection to unlock.
 *
 * @ingroup ipc_client
 */
static inline void
ipc_client_connection_unlock(struct ipc_connection *ipc_c)
{
	os_mutex_unlock(&ipc_c->mutex);
}

/*!
 * Tear down the basics of the client connection: socket and shared mem
 * @param ipc_c initialized IPC connection struct
 *
 * @ingroup ipc_client
 */
void
ipc_client_connection_fini(struct ipc_connection *ipc_c);
