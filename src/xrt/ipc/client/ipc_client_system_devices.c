// Copyright 2023, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client system devices.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

#include "util/u_system_helpers.h"
#include "util/u_system_devices.h"
#include "util/u_var.h"

#include "b_system_devices.h"

#include "ipc_client.h"
#include "ipc_client_generated.h"
#include "ipc_client_body_tracker.h"
#include "ipc_client_hand_tracker.h"
#include "ipc_client_io_cache.h"
#include "ipc_client_system_devices.h"
#include "ipc_client_xdev.h"


/*
 *
 * Helpers
 *
 */

static inline struct ipc_client_system_devices *
ipc_system_devices(struct xrt_system_devices *xsysd)
{
	return (struct ipc_client_system_devices *)xsysd;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
ipc_client_system_devices_get_roles(struct xrt_system_devices *xsysd, struct xrt_system_roles *out_roles)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);

	return ipc_call_system_devices_get_roles(usysd->ipc_c, out_roles);
}

static xrt_result_t
ipc_client_system_devices_feature_inc(struct xrt_system_devices *xsysd, enum xrt_device_feature_type type)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);
	xrt_result_t xret;

	assert(type < XRT_DEVICE_FEATURE_MAX_ENUM);

	// If it wasn't zero nothing to do.
	if (!xrt_reference_inc_and_was_zero(&usysd->feature_use[type])) {
		return XRT_SUCCESS;
	}

	xret = ipc_call_system_devices_begin_feature(usysd->ipc_c, type);
	IPC_CHK_ALWAYS_RET(usysd->ipc_c, xret, "ipc_call_system_devices_begin_feature");
}

static xrt_result_t
ipc_client_system_devices_feature_dec(struct xrt_system_devices *xsysd, enum xrt_device_feature_type type)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);
	xrt_result_t xret;

	assert(type < XRT_DEVICE_FEATURE_MAX_ENUM);

	// If it is not zero we are done.
	if (!xrt_reference_dec_and_is_zero(&usysd->feature_use[type])) {
		return XRT_SUCCESS;
	}

	xret = ipc_call_system_devices_end_feature(usysd->ipc_c, type);
	IPC_CHK_ALWAYS_RET(usysd->ipc_c, xret, "ipc_call_system_devices_end_feature");
}

static xrt_result_t
ipc_client_system_devices_create_body_tracker(struct xrt_system_devices *xsysd,
                                              const struct xrt_body_tracker_create_info *info,
                                              struct xrt_body_tracker **out_xbt)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);

	struct ipc_body_tracker_create_info ipc_info = {
	    .body_tracking_type = info->body_tracking_type,
	};

	if (info->locked_xdev != NULL) {
		ipc_info.has_locked_xdev = true;
		ipc_info.locked_xdev_id = ipc_client_xdev(info->locked_xdev)->device_id;
	}

	uint32_t id = 0;
	struct xrt_body_tracker_supported supported = {0};
	xrt_result_t xret = ipc_call_body_tracker_create(usysd->ipc_c, &ipc_info, &id, &supported);
	IPC_CHK_AND_RET(usysd->ipc_c, xret, "ipc_call_body_tracker_create");

	return ipc_client_body_tracker_create(usysd->ipc_c, id, &supported, out_xbt);
}

static xrt_result_t
ipc_client_system_devices_create_hand_tracker(struct xrt_system_devices *xsysd,
                                              const struct xrt_hand_tracker_create_info *info,
                                              struct xrt_hand_tracker **out_xht)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);

	struct ipc_hand_tracker_create_info ipc_info = {
	    .hand = info->hand,
	    .requested_sources = {info->requested_sources[0], info->requested_sources[1]},
	    .requested_source_count = info->requested_source_count,
	};

	if (info->locked_xdev != NULL) {
		ipc_info.has_locked_xdev = true;
		ipc_info.locked_xdev_id = ipc_client_xdev(info->locked_xdev)->device_id;
	}

	uint32_t id = 0;
	xrt_result_t xret = ipc_call_hand_tracker_create(usysd->ipc_c, &ipc_info, &id);
	IPC_CHK_AND_RET(usysd->ipc_c, xret, "ipc_call_hand_tracker_create");

	return ipc_client_hand_tracker_create(usysd->ipc_c, id, out_xht);
}

static xrt_result_t
ipc_client_system_devices_create_io_cache(struct xrt_system_devices *xsysd,
                                          struct xrt_space_overseer *xso,
                                          struct xrt_io_cache **out_xic)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);
	(void)xso;

	uint32_t id = 0;
	xrt_result_t xret = ipc_call_io_cache_create(usysd->ipc_c, &id);
	IPC_CHK_AND_RET(usysd->ipc_c, xret, "ipc_call_io_cache_create");

	return ipc_client_io_cache_create(usysd->ipc_c, id, out_xic);
}


static void
ipc_client_system_devices_destroy(struct xrt_system_devices *xsysd)
{
	struct ipc_client_system_devices *usysd = ipc_system_devices(xsysd);

	// Finalize the tracking origin manager (which cleans up all cached tracking origins)
	ipc_client_tracking_origin_manager_fini(&usysd->tracking_origin_manager);

	b_system_devices_close(&usysd->base.base);

	free(usysd);
}


/*
 *
 * 'Exported' functions.
 *
 */

xrt_result_t
ipc_client_system_devices_create(struct ipc_connection *ipc_c, struct ipc_client_system_devices **out_icsd)
{
	struct ipc_client_system_devices *icsd = U_TYPED_CALLOC(struct ipc_client_system_devices);

	u_system_devices_populate_function_pointers(&icsd->base.base, ipc_client_system_devices_get_roles,
	                                            ipc_client_system_devices_destroy);
	icsd->base.base.create_body_tracker = ipc_client_system_devices_create_body_tracker;
	icsd->base.base.create_hand_tracker = ipc_client_system_devices_create_hand_tracker;
	icsd->base.base.create_io_cache = ipc_client_system_devices_create_io_cache;
	icsd->base.base.feature_inc = ipc_client_system_devices_feature_inc;
	icsd->base.base.feature_dec = ipc_client_system_devices_feature_dec;

	icsd->ipc_c = ipc_c;

	// Initialize tracking origin manager
	xrt_result_t xret = ipc_client_tracking_origin_manager_init(&icsd->tracking_origin_manager, ipc_c);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_client_tracking_origin_manager_init", err_free);

	*out_icsd = icsd;

	return XRT_SUCCESS;

err_free:
	free(icsd);

	return xret;
}
