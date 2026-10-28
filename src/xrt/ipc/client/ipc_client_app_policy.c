// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Client side wrapper of app policy objects.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_instance.h"

#include "os/os_threading.h"

#include "ipc_client_generated.h"


/*
 *
 * Internal structs and helpers.
 *
 */

/*!
 * Client proxy for xrt_app_instance
 * @implements xrt_app_instance
 */
struct ipc_client_app_instance
{
	struct xrt_app_instance base;

	//! The IPC connection
	struct ipc_connection *ipc_c;

	uint32_t id;
};

/*!
 * Client proxy for xrt_app_system
 * @implements xrt_app_system
 */
struct ipc_client_app_system
{
	struct xrt_app_system base;

	//! The IPC connection
	struct ipc_connection *ipc_c;

	uint32_t id;
};


/*
 *
 * Helper functions
 *
 */

static inline struct ipc_client_app_instance *
ipc_client_app_instance(struct xrt_app_instance *xainst)
{
	return container_of(xainst, struct ipc_client_app_instance, base);
}

static inline struct ipc_client_app_system *
ipc_client_app_system(struct xrt_app_system *xasys)
{
	return container_of(xasys, struct ipc_client_app_system, base);
}


/*
 *
 * Member functions
 *
 */

static void
app_system_destroy(struct xrt_app_system *xasys)
{
	struct ipc_client_app_system *icasys = ipc_client_app_system(xasys);

	ipc_call_app_system_destroy(icasys->ipc_c, icasys->id);

	free(icasys);
}

static xrt_result_t
app_system_poll_events(struct xrt_app_system *xasys, struct xrt_app_system_event *out_xasyse)
{
	struct ipc_client_app_system *icasys = ipc_client_app_system(xasys);

	xrt_result_t xret = ipc_call_app_system_poll_events(icasys->ipc_c, icasys->id, out_xasyse);
	IPC_CHK_AND_RET(icasys->ipc_c, xret, "ipc_call_poll_events");

	return XRT_SUCCESS;
}

static xrt_result_t
app_system_get_recommended_view_configuration(struct xrt_app_system *xasys,
                                              enum xrt_view_type view_type,
                                              struct xrt_recommended_view_config *out_view_config)
{
	struct ipc_client_app_system *icasys = ipc_client_app_system(xasys);

	xrt_result_t xret = ipc_call_app_system_get_recommended_view_configuration(icasys->ipc_c, icasys->id, view_type,
	                                                                           out_view_config);
	IPC_CHK_AND_RET(icasys->ipc_c, xret, "ipc_call_app_system_get_recommended_view_configuration");

	return XRT_SUCCESS;
}

static xrt_result_t
app_system_set_recommended_view_configuration(struct xrt_app_system *xasys,
                                              enum xrt_view_type view_type,
                                              const struct xrt_recommended_view_config *view_config)
{
	struct ipc_client_app_system *icasys = ipc_client_app_system(xasys);

	xrt_result_t xret =
	    ipc_call_app_system_set_recommended_view_configuration(icasys->ipc_c, icasys->id, view_type, view_config);
	IPC_CHK_AND_RET(icasys->ipc_c, xret, "ipc_call_app_system_set_recommended_view_configuration");

	return XRT_SUCCESS;
}

static xrt_result_t
app_instance_create_app_system(struct xrt_app_instance *xainst,
                               struct xrt_system *xsys,
                               struct xrt_app_system **out_xasys)
{
	struct ipc_client_app_instance *icainst = ipc_client_app_instance(xainst);

	xrt_result_t xret = XRT_SUCCESS;
	struct ipc_client_app_system *icasys = NULL;

	icasys = U_TYPED_CALLOC(struct ipc_client_app_system);
	if (icasys == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	uint32_t xasys_id;
	xret = ipc_call_app_instance_create_app_system(icainst->ipc_c, icainst->id, &xasys_id);
	if (xret != XRT_SUCCESS) {
		free(icasys);
		return xret;
	}

	icasys->base.poll_events = app_system_poll_events;
	icasys->base.get_recommended_view_configuration = app_system_get_recommended_view_configuration;
	icasys->base.set_recommended_view_configuration = app_system_set_recommended_view_configuration;
	icasys->base.destroy = app_system_destroy;

	icasys->ipc_c = icainst->ipc_c;
	icasys->id = xasys_id;

	(*out_xasys) = &icasys->base;

	return xret;
}

static void
app_instance_destroy(struct xrt_app_instance *xainst)
{
	struct ipc_client_app_instance *icainst = ipc_client_app_instance(xainst);

	ipc_call_app_instance_destroy(icainst->ipc_c, icainst->id);

	free(icainst);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_app_instance *
ipc_client_create_app_instance(struct ipc_connection *ipc_c)
{
	struct ipc_client_app_instance *icainst = U_TYPED_CALLOC(struct ipc_client_app_instance);
	if (icainst == NULL) {
		return NULL;
	}

	uint32_t id;
	xrt_result_t xret = ipc_call_app_instance_create(ipc_c, &id);
	if (xret != XRT_SUCCESS) {
		free(icainst);
		return NULL;
	}

	icainst->base.create_app_system = app_instance_create_app_system;
	icainst->base.destroy = app_instance_destroy;

	icainst->ipc_c = ipc_c;
	icainst->id = id;

	return &icainst->base;
}
