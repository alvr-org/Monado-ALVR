// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Tracking objects to IDs.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup ipc_server
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_tracking.h"
#include "xrt/xrt_space.h"
#include "xrt/xrt_body_tracker.h"
#include "xrt/xrt_hand_tracker.h"
#include "xrt/xrt_io_cache.h"

#include "shared/ipc_protocol.h"
#include "server/ipc_server.h"
#include "server/ipc_server_objects.h"

#include <assert.h>
#include <string.h>


/*
 *
 * Device functions.
 *
 */

xrt_result_t
ipc_server_objects_get_xdev_and_validate(volatile struct ipc_client_state *ics,
                                         uint32_t id,
                                         struct xrt_device **out_xdev)
{
	if (id >= XRT_SYSTEM_MAX_DEVICES) {
		IPC_ERROR(ics->server, "Invalid device ID %u (>= XRT_SYSTEM_MAX_DEVICES)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_device *xdev = ics->objects.xdevs[id];
	if (xdev == NULL) {
		IPC_ERROR(ics->server, "Device ID %u not found (NULL)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xdev = xdev;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xdev_id_or_add(volatile struct ipc_client_state *ics, struct xrt_device *xdev, uint32_t *out_id)
{
	assert(out_id != NULL);
	assert(xdev != NULL);

	// Check if device is already tracked and return its ID.
	for (uint32_t index = 0; index < XRT_SYSTEM_MAX_DEVICES; index++) {
		if (ics->objects.xdevs[index] == xdev) {
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	// If not, find a free slot for it, filled below.
	uint32_t index = 0;
	for (; index < XRT_SYSTEM_MAX_DEVICES; index++) {
		// Found a free slot.
		if (ics->objects.xdevs[index] == NULL) {
			break;
		}
	}

	if (index >= XRT_SYSTEM_MAX_DEVICES) {
		IPC_ERROR(ics->server, "Failed to find available slot for device: '%s'", xdev->str);
		return XRT_ERROR_IPC_FAILURE;
	}

	// Check that we can also get the tracking origin allocated.
	uint32_t tracking_origin_id = UINT32_MAX;
	xrt_result_t xret = ipc_server_objects_get_xtrack_id_or_add(ics, xdev->tracking_origin, &tracking_origin_id);
	IPC_CHK_AND_RET(ics->server, xret, "ipc_server_objects_get_xtrack_id_or_add");

	ics->objects.xdevs[index] = xdev;

	*out_id = index;

	return XRT_SUCCESS;
}


/*
 *
 * Tracking origin functions.
 *
 */

xrt_result_t
ipc_server_objects_get_xtrack_and_validate(volatile struct ipc_client_state *ics,
                                           uint32_t id,
                                           struct xrt_tracking_origin **out_xtrack)
{
	if (id >= XRT_SYSTEM_MAX_DEVICES) {
		IPC_ERROR(ics->server, "Invalid tracking origin ID %u (>= XRT_SYSTEM_MAX_DEVICES)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_tracking_origin *xtrack = ics->objects.xtracks[id];
	if (xtrack == NULL) {
		IPC_ERROR(ics->server, "Tracking origin ID %u not found (NULL)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xtrack = xtrack;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xtrack_id_or_add(volatile struct ipc_client_state *ics,
                                        struct xrt_tracking_origin *xtrack,
                                        uint32_t *out_id)
{
	assert(out_id != NULL);

	// Check if tracking origin is already tracked and return its ID.
	for (uint32_t index = 0; index < XRT_SYSTEM_MAX_DEVICES; index++) {
		if (ics->objects.xtracks[index] == xtrack) {
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	// If not, find a free slot for it, filled below.
	for (uint32_t index = 0; index < XRT_SYSTEM_MAX_DEVICES; index++) {
		if (ics->objects.xtracks[index] == NULL) {
			ics->objects.xtracks[index] = xtrack;
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	IPC_ERROR(ics->server, "Failed to find available slot for tracking origin: '%s'", xtrack->name);

	return XRT_ERROR_IPC_FAILURE;
}


/*
 *
 * Space functions.
 *
 */

xrt_result_t
ipc_server_objects_get_xspc_and_validate(volatile struct ipc_client_state *ics,
                                         uint32_t id,
                                         struct xrt_space **out_xspc)
{
	if (id >= IPC_MAX_CLIENT_SPACES) {
		IPC_ERROR(ics->server, "Invalid space ID %u (>= IPC_MAX_CLIENT_SPACES)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_space *xspc = ics->xspcs[id];
	if (xspc == NULL) {
		IPC_ERROR(ics->server, "Space ID %u not found (NULL)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xspc = xspc;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xspc_id_or_add(volatile struct ipc_client_state *ics, struct xrt_space *xspc, uint32_t *out_id)
{
	assert(out_id != NULL);
	assert(xspc != NULL);

	// Check if space is already tracked and return its ID.
	for (uint32_t index = 0; index < IPC_MAX_CLIENT_SPACES; index++) {
		if (ics->xspcs[index] == xspc) {
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	// If not, find a free slot for it, filled below.
	for (uint32_t index = 0; index < IPC_MAX_CLIENT_SPACES; index++) {
		if (ics->xspcs[index] == NULL) {
			struct xrt_space **xspc_ptr = (struct xrt_space **)&ics->xspcs[index];
			xrt_space_reference(xspc_ptr, xspc);
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	IPC_ERROR(ics->server, "Failed to find available slot for space");
	return XRT_ERROR_IPC_FAILURE;
}

xrt_result_t
ipc_server_objects_destroy_xspc(volatile struct ipc_client_state *ics, uint32_t id)
{
	if (id >= IPC_MAX_CLIENT_SPACES) {
		IPC_ERROR(ics->server, "Invalid space ID %u (>= IPC_MAX_CLIENT_SPACES)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	if (ics->xspcs[id] == NULL) {
		IPC_ERROR(ics->server, "Client tried to destroy non-existent space!");
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_space **xspc_ptr = (struct xrt_space **)&ics->xspcs[id];
	xrt_space_reference(xspc_ptr, NULL);

	return XRT_SUCCESS;
}


/*
 *
 * Body tracker functions.
 *
 */

xrt_result_t
ipc_server_objects_get_xbt_and_validate(volatile struct ipc_client_state *ics,
                                        uint32_t id,
                                        struct xrt_body_tracker **out_xbt)
{
	if (id >= IPC_MAX_CLIENT_BODY_TRACKERS) {
		IPC_ERROR(ics->server, "Invalid body tracker ID %u (>= IPC_MAX_CLIENT_BODY_TRACKERS)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_body_tracker *xbt = ics->objects.xbts[id];
	if (xbt == NULL) {
		IPC_ERROR(ics->server, "Body tracker ID %u not found (NULL)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xbt = xbt;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xbt_id_or_add(volatile struct ipc_client_state *ics,
                                     struct xrt_body_tracker *xbt,
                                     uint32_t *out_id)
{
	assert(out_id != NULL);
	assert(xbt != NULL);

	for (uint32_t index = 0; index < IPC_MAX_CLIENT_BODY_TRACKERS; index++) {
		if (ics->objects.xbts[index] == NULL) {
			ics->objects.xbts[index] = xbt;
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	IPC_ERROR(ics->server, "Failed to find available slot for body tracker");
	return XRT_ERROR_IPC_FAILURE;
}

xrt_result_t
ipc_server_objects_destroy_xbt(volatile struct ipc_client_state *ics, uint32_t id)
{
	if (id >= IPC_MAX_CLIENT_BODY_TRACKERS) {
		IPC_ERROR(ics->server, "Invalid body tracker ID %u (>= IPC_MAX_CLIENT_BODY_TRACKERS)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	if (ics->objects.xbts[id] == NULL) {
		IPC_ERROR(ics->server, "Client tried to destroy non-existent body tracker!");
		return XRT_ERROR_IPC_FAILURE;
	}

	xrt_body_tracker_destroy((struct xrt_body_tracker **)&ics->objects.xbts[id]);

	return XRT_SUCCESS;
}


/*
 *
 * Hand tracker functions.
 *
 */

xrt_result_t
ipc_server_objects_get_xht_and_validate(volatile struct ipc_client_state *ics,
                                        uint32_t id,
                                        struct xrt_hand_tracker **out_xht)
{
	if (id >= IPC_MAX_CLIENT_HAND_TRACKERS) {
		IPC_ERROR(ics->server, "Invalid hand tracker ID %u (>= IPC_MAX_CLIENT_HAND_TRACKERS)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_hand_tracker *xht = ics->objects.xhts[id];
	if (xht == NULL) {
		IPC_ERROR(ics->server, "Hand tracker ID %u not found (NULL)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xht = xht;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xht_id_or_add(volatile struct ipc_client_state *ics,
                                     struct xrt_hand_tracker *xht,
                                     uint32_t *out_id)
{
	assert(out_id != NULL);
	assert(xht != NULL);

	for (uint32_t index = 0; index < IPC_MAX_CLIENT_HAND_TRACKERS; index++) {
		if (ics->objects.xhts[index] == NULL) {
			ics->objects.xhts[index] = xht;
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	IPC_ERROR(ics->server, "Failed to find available slot for hand tracker");
	return XRT_ERROR_IPC_FAILURE;
}

xrt_result_t
ipc_server_objects_destroy_xht(volatile struct ipc_client_state *ics, uint32_t id)
{
	if (id >= IPC_MAX_CLIENT_HAND_TRACKERS) {
		IPC_ERROR(ics->server, "Invalid hand tracker ID %u (>= IPC_MAX_CLIENT_HAND_TRACKERS)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	if (ics->objects.xhts[id] == NULL) {
		IPC_ERROR(ics->server, "Client tried to destroy non-existent hand tracker!");
		return XRT_ERROR_IPC_FAILURE;
	}

	xrt_hand_tracker_destroy((struct xrt_hand_tracker **)&ics->objects.xhts[id]);

	return XRT_SUCCESS;
}


/*
 *
 * App policy functions.
 *
 */

xrt_result_t
ipc_server_objects_get_xainst_and_validate(volatile struct ipc_client_state *ics,
                                           uint32_t id,
                                           struct xrt_app_instance **out_xainst)
{
	if (id >= IPC_MAX_CLIENT_APP_INSTANCES) {
		IPC_ERROR(ics->server, "Invalid app instance ID %u (>= IPC_MAX_CLIENT_APP_INSTANCES)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_app_instance *xainst = ics->objects.xainsts[id];
	if (xainst == NULL) {
		IPC_ERROR(ics->server, "App instance ID %u not found (NULL)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xainst = xainst;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xainst_id_or_add(volatile struct ipc_client_state *ics,
                                        struct xrt_app_instance *xainst,
                                        uint32_t *out_id)
{
	assert(out_id != NULL);
	assert(xainst != NULL);

	for (uint32_t index = 0; index < IPC_MAX_CLIENT_APP_INSTANCES; index++) {
		if (ics->objects.xainsts[index] == NULL) {
			ics->objects.xainsts[index] = xainst;
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	IPC_ERROR(ics->server, "Failed to find available slot for app instance");
	return XRT_ERROR_IPC_FAILURE;
}

xrt_result_t
ipc_server_objects_destroy_xainst(volatile struct ipc_client_state *ics, uint32_t id)
{
	if (id >= IPC_MAX_CLIENT_APP_INSTANCES) {
		IPC_ERROR(ics->server, "Invalid app instance ID %u (>= IPC_MAX_CLIENT_APP_INSTANCES)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	if (ics->objects.xainsts[id] == NULL) {
		IPC_ERROR(ics->server, "Client tried to destroy non-existent app instance!");
		return XRT_ERROR_IPC_FAILURE;
	}

	xrt_app_instance_destroy((struct xrt_app_instance **)&ics->objects.xainsts[id]);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xasys_and_validate(volatile struct ipc_client_state *ics,
                                          uint32_t id,
                                          struct xrt_app_system **out_xasys)
{
	if (id >= IPC_MAX_CLIENT_APP_SYSTEMS) {
		IPC_ERROR(ics->server, "Invalid app system ID %u (>= IPC_MAX_CLIENT_APP_SYSTEMS)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_app_system *xasys = ics->objects.xasys[id];
	if (xasys == NULL) {
		IPC_ERROR(ics->server, "App system ID %u not found (NULL)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xasys = xasys;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xasys_id_or_add(volatile struct ipc_client_state *ics,
                                       struct xrt_app_system *xasys,
                                       uint32_t *out_id)
{
	assert(out_id != NULL);
	assert(xasys != NULL);

	for (uint32_t index = 0; index < IPC_MAX_CLIENT_APP_SYSTEMS; index++) {
		if (ics->objects.xasys[index] == NULL) {
			ics->objects.xasys[index] = xasys;
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	IPC_ERROR(ics->server, "Failed to find available slot for app system");
	return XRT_ERROR_IPC_FAILURE;
}

xrt_result_t
ipc_server_objects_destroy_xasys(volatile struct ipc_client_state *ics, uint32_t id)
{
	if (id >= IPC_MAX_CLIENT_APP_SYSTEMS) {
		IPC_ERROR(ics->server, "Invalid app system ID %u (>= IPC_MAX_CLIENT_APP_SYSTEMS)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	if (ics->objects.xasys[id] == NULL) {
		IPC_ERROR(ics->server, "Client tried to destroy non-existent app system!");
		return XRT_ERROR_IPC_FAILURE;
	}

	xrt_app_system_destroy((struct xrt_app_system **)&ics->objects.xasys[id]);

	return XRT_SUCCESS;
}


/*
 *
 * Input/output cache functions.
 *
 */

xrt_result_t
ipc_server_objects_get_xic_and_validate(volatile struct ipc_client_state *ics,
                                        uint32_t id,
                                        struct xrt_io_cache **out_xic)
{
	if (id >= IPC_MAX_CLIENT_IO_CACHES) {
		IPC_ERROR(ics->server, "Invalid io cache ID %u (>= IPC_MAX_CLIENT_IO_CACHES)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_io_cache *xic = ics->objects.xics[id];
	if (xic == NULL) {
		IPC_ERROR(ics->server, "Io cache ID %u not found (NULL)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xic = xic;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_server_objects_get_xic_id_or_add(volatile struct ipc_client_state *ics, struct xrt_io_cache *xic, uint32_t *out_id)
{
	assert(out_id != NULL);
	assert(xic != NULL);

	for (uint32_t index = 0; index < IPC_MAX_CLIENT_IO_CACHES; index++) {
		if (ics->objects.xics[index] == NULL) {
			ics->objects.xics[index] = xic;
			*out_id = index;
			return XRT_SUCCESS;
		}
	}

	IPC_ERROR(ics->server, "Failed to find available slot for io cache");
	return XRT_ERROR_IPC_FAILURE;
}

xrt_result_t
ipc_server_objects_destroy_xic(volatile struct ipc_client_state *ics, uint32_t id)
{
	if (id >= IPC_MAX_CLIENT_IO_CACHES) {
		IPC_ERROR(ics->server, "Invalid io cache ID %u (>= IPC_MAX_CLIENT_IO_CACHES)", id);
		return XRT_ERROR_IPC_FAILURE;
	}

	if (ics->objects.xics[id] == NULL) {
		IPC_ERROR(ics->server, "Client tried to destroy non-existent io cache!");
		return XRT_ERROR_IPC_FAILURE;
	}

	xrt_io_cache_destroy((struct xrt_io_cache **)&ics->objects.xics[id]);

	return XRT_SUCCESS;
}
