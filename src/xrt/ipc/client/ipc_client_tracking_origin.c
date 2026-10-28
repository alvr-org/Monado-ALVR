// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client tracking origin management.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_defines.h"
#include "xrt/xrt_tracking.h"

#include "util/u_misc.h"
#include "util/u_var.h"
#include "util/u_hashmap.h"

#include <assert.h>

#include "shared/ipc_message_channel.h"
#include "shared/ipc_protocol.h"

#include "client/ipc_client.h"
#include "client/ipc_client_connection.h"
#include "client/ipc_client_tracking_origin.h"

#include "ipc_client_generated.h"


/*
 *
 * Helper functions
 *
 */

static void
tracking_origin_cleanup_callback(void *item, void *priv)
{
	struct xrt_tracking_origin *xtrack = (struct xrt_tracking_origin *)item;

	// Remove from variable tracking
	u_var_remove_root(xtrack);

	// Free the tracking origin
	free(xtrack);
}


/*
 *
 * Exported functions
 *
 */

xrt_result_t
ipc_client_tracking_origin_manager_init(struct ipc_client_tracking_origin_manager *manager,
                                        struct ipc_connection *ipc_c)
{
	assert(manager != NULL);
	assert(ipc_c != NULL);

	manager->ipc_c = ipc_c;
	manager->tracking_origin_map = NULL;

	int ret = u_hashmap_int_create(&manager->tracking_origin_map);
	if (ret != 0) {
		IPC_ERROR(ipc_c, "Failed to create tracking origin hashmap");
		return XRT_ERROR_ALLOCATION;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_client_tracking_origin_manager_get(struct ipc_client_tracking_origin_manager *manager,
                                       uint32_t tracking_origin_id,
                                       struct xrt_tracking_origin **out_xtrack)
{
	// Check if we already have this tracking origin cached
	void *cached_xtrack = NULL;
	int ret = u_hashmap_int_find(manager->tracking_origin_map, tracking_origin_id, &cached_xtrack);
	if (ret == 0) {
		// Found in cache
		*out_xtrack = (struct xrt_tracking_origin *)cached_xtrack;
		return XRT_SUCCESS;
	}

	// Not in cache, fetch from server
	struct ipc_tracking_origin_info info;
	xrt_result_t xret = ipc_call_tracking_origin_get_info(manager->ipc_c, tracking_origin_id, &info);
	IPC_CHK_AND_RET(manager->ipc_c, xret, "ipc_call_tracking_origin_get_info");

	// Create a new tracking origin
	struct xrt_tracking_origin *xtrack = U_TYPED_CALLOC(struct xrt_tracking_origin);

	memcpy(xtrack->name, info.name, sizeof(xtrack->name));
	xtrack->type = info.type;
	xtrack->initial_offset = info.offset;

	// Add to variable tracking
	u_var_add_root(xtrack, "Tracking origin", true);
	u_var_add_ro_text(xtrack, xtrack->name, "name");
	u_var_add_pose(xtrack, &xtrack->initial_offset, "offset");

	// Store in cache
	u_hashmap_int_insert(manager->tracking_origin_map, tracking_origin_id, xtrack);

	*out_xtrack = xtrack;

	return XRT_SUCCESS;
}

void
ipc_client_tracking_origin_manager_fini(struct ipc_client_tracking_origin_manager *manager)
{
	assert(manager != NULL);
	assert(manager->ipc_c != NULL);

	// Clean up all cached tracking origins
	if (manager->tracking_origin_map != NULL) {
		u_hashmap_int_clear_and_call_for_each(manager->tracking_origin_map, tracking_origin_cleanup_callback,
		                                      NULL);
		u_hashmap_int_destroy(&manager->tracking_origin_map);
		manager->tracking_origin_map = NULL;
	}

	manager->ipc_c = NULL;
}
