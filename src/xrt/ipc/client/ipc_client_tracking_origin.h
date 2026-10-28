// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client tracking origin management.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup ipc_client
 */

#pragma once

#include "xrt/xrt_tracking.h"
#include "util/u_hashmap.h"


#ifdef __cplusplus
extern "C" {
#endif


struct ipc_connection;

/*!
 * Tracking origin manager for IPC client.
 *
 * Maintains a hashmap of tracking origin IDs to xrt_tracking_origin objects.
 * Fetches tracking origin info from the server on-demand.
 *
 * @ingroup ipc_client
 */
struct ipc_client_tracking_origin_manager
{
	//! Connection to the IPC server
	struct ipc_connection *ipc_c;

	//! Hashmap from tracking_origin_id to xrt_tracking_origin*
	struct u_hashmap_int *tracking_origin_map;
};

/*!
 * Initialize a tracking origin manager.
 *
 * @ingroup ipc_client
 */
xrt_result_t
ipc_client_tracking_origin_manager_init(struct ipc_client_tracking_origin_manager *manager,
                                        struct ipc_connection *ipc_c);

/*!
 * Get a tracking origin by ID. If not already cached, fetches it from the server.
 *
 * @ingroup ipc_client
 */
xrt_result_t
ipc_client_tracking_origin_manager_get(struct ipc_client_tracking_origin_manager *manager,
                                       uint32_t tracking_origin_id,
                                       struct xrt_tracking_origin **out_xtrack);

/*!
 * Finalize the tracking origin manager and all cached tracking origins.
 *
 * @ingroup ipc_client
 */
void
ipc_client_tracking_origin_manager_fini(struct ipc_client_tracking_origin_manager *manager);


#ifdef __cplusplus
}
#endif
