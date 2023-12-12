// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Shared default implementation of the instance with compositor.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#include "xrt/xrt_space.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_config_build.h"

#include "os/os_time.h"

#include "util/u_debug.h"
#include "util/u_system.h"
#include "util/u_trace_marker.h"
#include "util/u_system_helpers.h"

#ifdef XRT_MODULE_COMPOSITOR_MAIN
#include "main/comp_main_interface.h"
#endif

#include "target_instance_parts.h"

#include <assert.h>


#ifdef XRT_MODULE_COMPOSITOR_MAIN
#define USE_NULL_DEFAULT (false)
#else
#define USE_NULL_DEFAULT (true)
#endif

DEBUG_GET_ONCE_BOOL_OPTION(use_null, "XRT_COMPOSITOR_NULL", USE_NULL_DEFAULT)

xrt_result_t
null_compositor_create_system(struct xrt_device *xdev, struct xrt_system_compositor **out_xsysc);



/*
 *
 * Internal functions.
 *
 */

static xrt_result_t
t_instance_create_system(struct xrt_instance *xinst,
                         struct xrt_system **out_xsys,
                         struct xrt_system_devices **out_xsysd,
                         struct xrt_space_overseer **out_xso,
                         struct xrt_system_compositor **out_xsysc)
{
	XRT_TRACE_MARKER();

	assert(out_xsys != NULL);
	assert(*out_xsys == NULL);
	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);
	assert(out_xso != NULL);
	assert(*out_xso == NULL);
	assert(out_xsysc == NULL || *out_xsysc == NULL);

	struct u_system *usys = NULL;
	struct xrt_system_compositor *xsysc = NULL;
	struct xrt_space_overseer *xso = NULL;
	struct xrt_system_devices *xsysd = NULL;
	xrt_result_t xret = XRT_SUCCESS;

	usys = u_system_create();
	assert(usys != NULL); // Should never fail.

	xret = u_system_devices_create_from_prober( //
	    xinst,                                  // xinst
	    &usys->broadcast,                       // broadcast
	    &xsysd,                                 // out_xsysd
	    &xso);                                  // out_xso
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	// Early out if we only want devices.
	if (out_xsysc == NULL) {
		goto out;
	}

	struct xrt_device *head = xsysd->static_roles.head;

	bool use_null = debug_get_bool_option_use_null();

#ifdef XRT_MODULE_COMPOSITOR_NULL
	if (use_null) {
		xret = null_compositor_create_system(head, &xsysc);
	}
#else
	if (use_null) {
		U_LOG_E("The null compositor is not compiled in!");
		xret = XRT_ERROR_VULKAN;
	}
#endif

#ifdef XRT_MODULE_COMPOSITOR_MAIN
	if (xret == XRT_SUCCESS && xsysc == NULL) {
		xret = comp_main_create_system_compositor(head, NULL, &xsysc);
	}
#else
	if (!use_null) {
		U_LOG_E("Explicitly didn't request the null compositor, but the main compositor hasn't been built!");
		xret = XRT_ERROR_VULKAN;
	}
#endif

	if (xret != XRT_SUCCESS) {
		goto err_destroy;
	}

out:
	*out_xsys = &usys->base;
	*out_xsysd = xsysd;
	*out_xso = xso;

	if (xsysc != NULL) {
		// Tell the system about the system compositor.
		u_system_set_system_compositor(usys, xsysc);

		assert(out_xsysc != NULL);
		*out_xsysc = xsysc;
	}

	return xret;


err_destroy:
	xrt_space_overseer_destroy(&xso);
	xrt_system_devices_destroy(&xsysd);
	u_system_destroy(&usys);

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
	struct xrt_prober *xp = NULL;

	u_trace_marker_init();

	XRT_TRACE_MARKER();

	int ret = xrt_prober_create_with_lists(&xp, &target_lists);
	if (ret < 0) {
		return XRT_ERROR_PROBER_CREATION_FAILED;
	}

	struct t_instance *tinst = U_TYPED_CALLOC(struct t_instance);
	tinst->base.create_system = t_instance_create_system;
	tinst->base.get_prober = t_instance_get_prober;
	tinst->base.destroy = t_instance_destroy;
	tinst->xp = xp;

	tinst->base.startup_timestamp = os_monotonic_get_ns();

	*out_xinst = &tinst->base;

	return XRT_SUCCESS;
}
