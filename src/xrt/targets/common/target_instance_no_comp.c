// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Shared default implementation of the instance, but with no compositor
 * usage
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#include "xrt/xrt_system.h"

#include "util/u_system.h"
#include "util/u_trace_marker.h"
#include "util/u_system_helpers.h"

#include "target_instance_parts.h"

#include <assert.h>


static xrt_result_t
t_instance_create_system(struct xrt_instance *xinst,
                         struct xrt_system **out_xsys,
                         struct xrt_system_devices **out_xsysd,
                         struct xrt_space_overseer **out_xso,
                         struct xrt_system_compositor **out_xsysc)
{
	XRT_TRACE_MARKER();

	struct u_system *usys = NULL;
	struct xrt_system_devices *xsysd = NULL;
	struct xrt_space_overseer *xso = NULL;
	xrt_result_t xret = XRT_SUCCESS;

	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);
	assert(out_xso != NULL);
	assert(*out_xso == NULL);
	assert(out_xsysc == NULL || *out_xsysc == NULL);

	// Can't create a system compositor.
	if (out_xsysc != NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	usys = u_system_create();
	assert(usys != NULL); // Should never fail.

	xret = u_system_devices_create_from_prober( //
	    xinst,                                  //
	    &usys->broadcast,                       //
	    &xsysd,                                 //
	    &xso);                                  //
	if (xret != XRT_SUCCESS) {
		u_system_destroy(&usys);
		return xret;
	}

	*out_xsys = &usys->base;
	*out_xsysd = xsysd;
	*out_xso = xso;

	return xret;
}


/*
 *
 * Exported function(s).
 *
 */

xrt_result_t
xrt_instance_create(struct xrt_instance_info *ii, struct xrt_instance **out_xinst)
{
	XRT_TRACE_MARKER();

	struct xrt_prober *xp = NULL;

	int ret = xrt_prober_create_with_lists(&xp, &target_lists);
	if (ret < 0) {
		return XRT_ERROR_PROBER_CREATION_FAILED;
	}

	struct t_instance *tinst = U_TYPED_CALLOC(struct t_instance);
	tinst->base.create_system = t_instance_create_system;
	tinst->base.get_prober = t_instance_get_prober;
	tinst->base.destroy = t_instance_destroy;
	tinst->xp = xp;

	*out_xinst = &tinst->base;

	return XRT_SUCCESS;
}
