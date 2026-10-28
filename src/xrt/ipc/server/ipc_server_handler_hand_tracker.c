// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC server handlers for @ref xrt_hand_tracker.
 * @ingroup ipc_server
 */

#include "xrt/xrt_hand_tracker.h"
#include "xrt/xrt_space.h"
#include "xrt/xrt_system.h"

#include "shared/ipc_protocol.h"
#include "server/ipc_server.h"
#include "server/ipc_server_objects.h"

#include "ipc_server_generated.h"


/*
 *
 * Helpers.
 *
 */

#define GET_XHT_OR_RETURN(ICS, ID, XHT)                                                                                \
	do {                                                                                                           \
		xrt_result_t xret = ipc_server_objects_get_xht_and_validate((ICS), ID, &(XHT));                        \
		IPC_CHK_AND_RET((ICS)->server, xret, "ipc_server_objects_get_xht_and_validate");                       \
	} while (0)

static bool
is_valid_hand_tracker_source(enum xrt_input_name input_name, enum xrt_hand hand)
{
	switch (input_name) {
	case XRT_INPUT_HT_UNOBSTRUCTED_LEFT:
	case XRT_INPUT_HT_CONFORMING_LEFT: return hand == XRT_HAND_LEFT;
	case XRT_INPUT_HT_UNOBSTRUCTED_RIGHT:
	case XRT_INPUT_HT_CONFORMING_RIGHT: return hand == XRT_HAND_RIGHT;
	default: return false;
	}
}

static xrt_result_t
validate_create_info(volatile struct ipc_client_state *ics, const struct ipc_hand_tracker_create_info *ipc_info)
{
	if (ipc_info->hand != XRT_HAND_LEFT && ipc_info->hand != XRT_HAND_RIGHT) {
		IPC_ERROR(ics->server, "Invalid hand tracker hand %u", (uint32_t)ipc_info->hand);
		return XRT_ERROR_IPC_FAILURE;
	}

	if (ipc_info->requested_source_count > ARRAY_SIZE(ipc_info->requested_sources)) {
		IPC_ERROR(ics->server, "Requested source count %u is too large", ipc_info->requested_source_count);
		return XRT_ERROR_IPC_FAILURE;
	}

	for (uint32_t i = 0; i < ipc_info->requested_source_count; i++) {
		if (!is_valid_hand_tracker_source(ipc_info->requested_sources[i], ipc_info->hand)) {
			IPC_ERROR(ics->server, "Invalid requested hand tracker source %u for hand %u",
			          (uint32_t)ipc_info->requested_sources[i], (uint32_t)ipc_info->hand);
			return XRT_ERROR_IPC_FAILURE;
		}
	}

	return XRT_SUCCESS;
}


/*
 *
 * Handle functions.
 *
 */

xrt_result_t
ipc_handle_hand_tracker_create(volatile struct ipc_client_state *ics,
                               const struct ipc_hand_tracker_create_info *ipc_info,
                               uint32_t *out_id)
{
	xrt_result_t xret = validate_create_info(ics, ipc_info);
	IPC_CHK_AND_RET(ics->server, xret, "validate_create_info");

	struct xrt_hand_tracker_create_info info = {
	    .hand = ipc_info->hand,
	    .requested_sources = {ipc_info->requested_sources[0], ipc_info->requested_sources[1]},
	    .requested_source_count = ipc_info->requested_source_count,
	};

	if (ipc_info->has_locked_xdev) {
		xret = ipc_server_objects_get_xdev_and_validate( //
		    ics,                                         //
		    ipc_info->locked_xdev_id,                    //
		    &info.locked_xdev);                          //
		IPC_CHK_AND_RET(ics->server, xret, "ipc_server_objects_get_xdev_and_validate");
	}

	struct xrt_hand_tracker *xht = NULL;
	xret = xrt_system_devices_create_hand_tracker( //
	    ics->server->xsysd,                        //
	    &info,                                     //
	    &xht);                                     //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	xret = ipc_server_objects_get_xht_id_or_add(ics, xht, out_id);
	if (xret != XRT_SUCCESS) {
		xrt_hand_tracker_destroy(&xht);
		return xret;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_hand_tracker_locate(volatile struct ipc_client_state *ics,
                               uint32_t id,
                               uint32_t base_space_id,
                               const struct xrt_pose *base_offset,
                               int64_t at_timestamp_ns,
                               struct xrt_hand_tracker_location *out_location)
{
	struct xrt_hand_tracker *xht = NULL;
	GET_XHT_OR_RETURN(ics, id, xht);

	struct xrt_space *base_space = NULL;
	xrt_result_t xret = ipc_server_objects_get_xspc_and_validate(ics, base_space_id, &base_space);
	IPC_CHK_AND_RET(ics->server, xret, "ipc_server_objects_get_xspc_and_validate");

	return xrt_hand_tracker_locate(xht, ics->server->xso, base_space, base_offset, at_timestamp_ns, out_location);
}

xrt_result_t
ipc_handle_hand_tracker_set_output(volatile struct ipc_client_state *ics,
                                   uint32_t id,
                                   enum xrt_output_name name,
                                   const struct xrt_output_value *value)
{
	struct xrt_hand_tracker *xht = NULL;
	GET_XHT_OR_RETURN(ics, id, xht);

	return xrt_hand_tracker_set_output(xht, name, value);
}

xrt_result_t
ipc_handle_hand_tracker_destroy(volatile struct ipc_client_state *ics, uint32_t id)
{
	return ipc_server_objects_destroy_xht(ics, id);
}

#undef GET_XHT_OR_RETURN
