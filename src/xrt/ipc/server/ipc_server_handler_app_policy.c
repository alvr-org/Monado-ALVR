// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC server handlers for app policy objects.
 * @ingroup ipc_server
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 */

#include "xrt/xrt_app_policy.h"

#include "shared/ipc_protocol.h"
#include "server/ipc_server.h"
#include "server/ipc_server_objects.h"

#include "ipc_server_generated.h"


/*
 *
 * Helpers.
 *
 */

#define GET_XAINST_OR_RETURN(ICS, ID, XAINST)                                                                          \
	do {                                                                                                           \
		xrt_result_t xret = ipc_server_objects_get_xainst_and_validate((ICS), ID, &(XAINST));                  \
		IPC_CHK_AND_RET((ICS)->server, xret, "ipc_server_objects_get_xainst_and_validate");                    \
	} while (0)

#define GET_XASYS_OR_RETURN(ICS, ID, XASYS)                                                                            \
	do {                                                                                                           \
		xrt_result_t xret = ipc_server_objects_get_xasys_and_validate((ICS), ID, &(XASYS));                    \
		IPC_CHK_AND_RET((ICS)->server, xret, "ipc_server_objects_get_xasys_and_validate");                     \
	} while (0)


/*
 *
 * Handle functions.
 *
 */

xrt_result_t
ipc_handle_app_instance_create(volatile struct ipc_client_state *ics, uint32_t *out_id)
{
	struct xrt_app_instance *xainst = NULL;
	xrt_result_t xret = xrt_instance_create_app_instance(ics->server->xinst, &xainst);
	IPC_CHK_AND_RET(ics->server, xret, "xrt_instance_create_app_instance");

	xret = ipc_server_objects_get_xainst_id_or_add(ics, xainst, out_id);
	if (xret != XRT_SUCCESS) {
		xrt_app_instance_destroy(&xainst);
		return xret;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_app_instance_destroy(volatile struct ipc_client_state *ics, uint32_t id)
{
	return ipc_server_objects_destroy_xainst(ics, id);
}

xrt_result_t
ipc_handle_app_instance_create_app_system(volatile struct ipc_client_state *ics,
                                          uint32_t xainst_id,
                                          uint32_t *out_xasys_id)
{
	struct xrt_app_instance *xainst = NULL;
	GET_XAINST_OR_RETURN(ics, xainst_id, xainst);

	// @todo This needs to grab the xrt_system from an ID once we have multi-system, but the IPC layer has no
	//       concept of system ID currently.
	struct xrt_system *xsys = ics->server->xsys;
	struct xrt_app_system *xasys = NULL;
	xrt_result_t xret = xrt_app_instance_create_app_system(xainst, xsys, &xasys);
	IPC_CHK_AND_RET(ics->server, xret, "xrt_app_instance_create_app_system");

	xret = ipc_server_objects_get_xasys_id_or_add(ics, xasys, out_xasys_id);
	if (xret != XRT_SUCCESS) {
		xrt_app_system_destroy(&xasys);
		return xret;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_handle_app_system_destroy(volatile struct ipc_client_state *ics, uint32_t xasys_id)
{
	return ipc_server_objects_destroy_xasys(ics, xasys_id);
}

xrt_result_t
ipc_handle_app_system_poll_events(volatile struct ipc_client_state *ics,
                                  uint32_t id,
                                  struct xrt_app_system_event *out_xasyse)
{
	struct xrt_app_system *xasys = NULL;
	GET_XASYS_OR_RETURN(ics, id, xasys);

	if (xasys == NULL) {
		return XRT_ERROR_IPC_FAILURE;
	}

	return xrt_app_system_poll_events(xasys, out_xasyse);
}

xrt_result_t
ipc_handle_app_system_get_recommended_view_configuration(
    volatile struct ipc_client_state *ics,
    uint32_t id,
    enum xrt_view_type view_type,
    struct xrt_recommended_view_config *out_recommended_view_config)
{
	struct xrt_app_system *xasys = NULL;
	GET_XASYS_OR_RETURN(ics, id, xasys);

	if (xasys == NULL) {
		return XRT_ERROR_IPC_FAILURE;
	}

	return xrt_app_system_get_recommended_view_configuration(xasys, view_type, out_recommended_view_config);
}

xrt_result_t
ipc_handle_app_system_set_recommended_view_configuration(
    volatile struct ipc_client_state *ics,
    uint32_t id,
    enum xrt_view_type view_type,
    const struct xrt_recommended_view_config *recommended_view_config)
{
	struct xrt_app_system *xasys = NULL;
	GET_XASYS_OR_RETURN(ics, id, xasys);

	if (xasys == NULL) {
		return XRT_ERROR_IPC_FAILURE;
	}

	return xrt_app_system_set_recommended_view_configuration(xasys, view_type, recommended_view_config);
}

#undef GET_XAINST_OR_RETURN
