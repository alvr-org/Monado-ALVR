// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Shared default implementation of the instance with compositor.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

#include "main/comp_target.h"
#include "xrt/xrt_space.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_config_build.h"
#include "xrt/xrt_config_os.h"

#include "os/os_time.h"

#include "util/u_debug.h"
#include "util/u_trace_marker.h"
#include "util/u_system_helpers.h"

#include "b_system.h"
#include "b_app_policy.h"

#ifdef XRT_MODULE_COMPOSITOR_MAIN
#include "main/comp_main_interface.h"
#endif

#include "target_instance_parts.h"
#include "target_alvr_comp.h"

#include <assert.h>

#ifdef XRT_OS_ANDROID
#include "android/android_instance_base.h"
#endif

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
t_instance_is_system_available(struct xrt_instance *xinst, bool *out_available)
{
	XRT_TRACE_MARKER();

	assert(out_available != NULL);

	*out_available = true;

	return XRT_SUCCESS;
}

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

	struct b_system *bsys = NULL;
	struct xrt_system_compositor *xsysc = NULL;
	struct xrt_space_overseer *xso = NULL;
	struct xrt_system_devices *xsysd = NULL;
	xrt_result_t xret = XRT_SUCCESS;

	bsys = b_system_create();
	assert(bsys != NULL); // Should never fail.

	xret = u_system_devices_create_from_prober( //
	    xinst,                                  // xinst
	    &bsys->broadcast,                       // broadcast
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
	b_system_fill_properties(bsys, head->str);

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
		struct comp_target_factory alvr_fac = alvr_create_target_factory();
		xret = comp_main_create_system_compositor(head, &alvr_fac, NULL, &xsysc);
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
	*out_xsys = &bsys->base;
	*out_xsysd = xsysd;
	*out_xso = xso;

	if (xsysc != NULL) {
		// Tell the system about the system compositor.
		b_system_set_system_compositor(bsys, xsysc);

		assert(out_xsysc != NULL);
		*out_xsysc = xsysc;
	}

	return xret;


err_destroy:
	xrt_space_overseer_destroy(&xso);
	xrt_system_devices_destroy(&xsysd);
	b_system_destroy(&bsys);

	return xret;
}

static xrt_result_t
t_instance_create_app_instance(struct xrt_instance *xinst, struct xrt_app_instance **out_xainst)
{
	XRT_TRACE_MARKER();

	assert(out_xainst != NULL);
	assert(*out_xainst == NULL);

	struct b_app_instance *bainst = b_app_instance_create();
	if (bainst == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	*out_xainst = &bainst->base;

	// @note: We don't keep track of it internally, instead relying on external things to properly destroy their
	//        app instances before calling xrt_instance_destroy.

	return XRT_SUCCESS;
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
	tinst->base.is_system_available = t_instance_is_system_available;
	tinst->base.create_system = t_instance_create_system;
	tinst->base.get_prober = t_instance_get_prober;
	tinst->base.create_app_instance = t_instance_create_app_instance;
	tinst->base.destroy = t_instance_destroy;
	tinst->xp = xp;

	tinst->base.startup_timestamp = os_monotonic_get_ns();

#ifdef XRT_OS_ANDROID
	if (ii != NULL) {
		ret = android_instance_base_init(&tinst->android, &tinst->base, ii);
		if (ret < 0) {
			xrt_prober_destroy(&xp);
			free(tinst);
			return ret;
		}
	}
#endif // XRT_OS_ANDROID

	*out_xinst = &tinst->base;

	return XRT_SUCCESS;
}
