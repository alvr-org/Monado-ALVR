// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC server handlers for @ref xrt_body_tracker.
 * @ingroup ipc_server
 */

#include "xrt/xrt_body_tracker.h"
#include "xrt/xrt_space.h"
#include "xrt/xrt_system.h"

#include "shared/ipc_protocol.h"

#include "ipc_server_generated.h"

#include "server/ipc_server.h"
#include "server/ipc_server_objects.h"


/*
 *
 * Helpers.
 *
 */

#define GET_XBT_OR_RETURN(ICS, ID, XBT)                                                                                \
	do {                                                                                                           \
		xrt_result_t xret = ipc_server_objects_get_xbt_and_validate((ICS), ID, &(XBT));                        \
		IPC_CHK_AND_RET((ICS)->server, xret, "ipc_server_objects_get_xbt_and_validate");                       \
	} while (0)


/*
 *
 * Handle functions.
 *
 */

xrt_result_t
ipc_handle_body_tracker_create(volatile struct ipc_client_state *ics,
                               const struct ipc_body_tracker_create_info *ipc_info,
                               uint32_t *out_id,
                               struct xrt_body_tracker_supported *out_supported)
{
	struct xrt_body_tracker_create_info info = {
	    .body_tracking_type = ipc_info->body_tracking_type,
	    .locked_xdev = NULL, // filled in later
	};

	if (ipc_info->has_locked_xdev) {
		xrt_result_t xret = ipc_server_objects_get_xdev_and_validate( //
		    ics,                                                      //
		    ipc_info->locked_xdev_id,                                 //
		    &info.locked_xdev);                                       //
		IPC_CHK_AND_RET(ics->server, xret, "ipc_server_objects_get_xdev_and_validate");
	}

	struct xrt_body_tracker *xbt = NULL;
	xrt_result_t xret = xrt_system_devices_create_body_tracker( //
	    ics->server->xsysd,                                     //
	    &info,                                                  //
	    &xbt);                                                  //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	xret = ipc_server_objects_get_xbt_id_or_add(ics, xbt, out_id);
	if (xret != XRT_SUCCESS) {
		xrt_body_tracker_destroy(&xbt);
		return xret;
	}

	*out_supported = xbt->supported;

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_body_tracker_locate(volatile struct ipc_client_state *ics,
                               uint32_t id,
                               uint32_t base_space_id,
                               const struct xrt_pose *base_offset,
                               int64_t at_timestamp_ns,
                               struct xrt_body_tracker_location *out_location)
{
	struct xrt_body_tracker *xbt = NULL;
	GET_XBT_OR_RETURN(ics, id, xbt);

	struct xrt_space *base_space = NULL;
	xrt_result_t xret = ipc_server_objects_get_xspc_and_validate(ics, base_space_id, &base_space);
	IPC_CHK_AND_RET(ics->server, xret, "ipc_server_objects_get_xspc_and_validate");

	return xrt_body_tracker_locate(xbt, ics->server->xso, base_space, base_offset, at_timestamp_ns, out_location);
}

xrt_result_t
ipc_handle_body_tracker_get_skeleton(volatile struct ipc_client_state *ics,
                                     uint32_t id,
                                     struct xrt_body_skeleton *out_skeleton)
{
	struct xrt_body_tracker *xbt = NULL;
	GET_XBT_OR_RETURN(ics, id, xbt);

	return xrt_body_tracker_get_skeleton(xbt, out_skeleton);
}

xrt_result_t
ipc_handle_body_tracker_reset_calibration_meta(volatile struct ipc_client_state *ics, uint32_t id)
{
	struct xrt_body_tracker *xbt = NULL;
	GET_XBT_OR_RETURN(ics, id, xbt);

	return xrt_body_tracker_reset_calibration_meta(xbt);
}

xrt_result_t
ipc_handle_body_tracker_set_calibration_override_meta(volatile struct ipc_client_state *ics,
                                                      uint32_t id,
                                                      float body_height)
{
	struct xrt_body_tracker *xbt = NULL;
	GET_XBT_OR_RETURN(ics, id, xbt);

	return xrt_body_tracker_set_calibration_override_meta(xbt, body_height);
}

xrt_result_t
ipc_handle_body_tracker_set_fidelity_meta(volatile struct ipc_client_state *ics,
                                          uint32_t id,
                                          enum xrt_body_tracking_fidelity_meta fidelity)
{
	struct xrt_body_tracker *xbt = NULL;
	GET_XBT_OR_RETURN(ics, id, xbt);

	return xrt_body_tracker_set_fidelity_meta(xbt, fidelity);
}

xrt_result_t
ipc_handle_body_tracker_destroy(volatile struct ipc_client_state *ics, uint32_t id)
{
	return ipc_server_objects_destroy_xbt(ics, id);
}

#undef GET_XBT_OR_RETURN
