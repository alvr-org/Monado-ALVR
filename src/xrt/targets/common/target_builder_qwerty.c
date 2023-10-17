// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Qwerty devices builder.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup drv_qwerty
 */

#include "xrt/xrt_config_drivers.h"

#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_builders.h"
#include "util/u_system_helpers.h"

#include "target_builder_interface.h"

#include "qwerty/qwerty_interface.h"

#include <assert.h>


#ifndef XRT_BUILD_DRIVER_QWERTY
#error "Must only be built with XRT_BUILD_DRIVER_QWERTY set"
#endif

// Using INFO as default to inform events real devices could report physically
DEBUG_GET_ONCE_LOG_OPTION(qwerty_log, "QWERTY_LOG", U_LOGGING_INFO)

// Driver disabled by default for being experimental
DEBUG_GET_ONCE_BOOL_OPTION(enable_qwerty, "QWERTY_ENABLE", false)


/*
 *
 * Helper functions.
 *
 */

static const char *driver_list[] = {
    "qwerty",
};


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
qwerty_estimate_system(struct xrt_builder *xb,
                       cJSON *config,
                       struct xrt_prober *xp,
                       struct xrt_builder_estimate *estimate)
{
	if (!debug_get_bool_option_enable_qwerty()) {
		return XRT_SUCCESS;
	}

	estimate->certain.head = true;
	estimate->certain.left = true;
	estimate->certain.right = true;
	estimate->priority = -25;

	return XRT_SUCCESS;
}

static xrt_result_t
qwerty_open_system(struct xrt_builder *xb,
                   cJSON *config,
                   struct xrt_prober *xp,
                   struct xrt_system_devices **out_xsysd,
                   struct xrt_space_overseer **out_xso)
{
	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);

	struct xrt_device *head = NULL;
	struct xrt_device *left = NULL;
	struct xrt_device *right = NULL;
	enum u_logging_level log_level = debug_get_log_option_qwerty_log();

	xrt_result_t xret = qwerty_create_devices(log_level, &head, &left, &right);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	// Use the static system devices helper, no dynamic roles.
	struct u_system_devices_static *usysds = u_system_devices_static_allocate();
	struct xrt_system_devices *xsysd = &usysds->base.base;

	// Add to device list.
	xsysd->xdevs[xsysd->xdev_count++] = head;
	if (left != NULL) {
		xsysd->xdevs[xsysd->xdev_count++] = left;
	}
	if (right != NULL) {
		xsysd->xdevs[xsysd->xdev_count++] = right;
	}

	// Assign to role(s).
	xsysd->static_roles.head = head;

	u_system_devices_static_finalize( //
	    usysds,                       // usysds
	    left,                         // left
	    right);                       // right


	/*
	 * Done.
	 */

	*out_xsysd = xsysd;
	u_builder_create_space_overseer_legacy( //
	    head,                               // head
	    left,                               // left
	    right,                              // right
	    xsysd->xdevs,                       // xdevs
	    xsysd->xdev_count,                  // xdev_count
	    out_xso);                           // out_xso

	return XRT_SUCCESS;
}

static void
qwerty_destroy(struct xrt_builder *xb)
{
	free(xb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
t_builder_qwerty_create(void)
{
	struct xrt_builder *xb = U_TYPED_CALLOC(struct xrt_builder);
	xb->estimate_system = qwerty_estimate_system;
	xb->open_system = qwerty_open_system;
	xb->destroy = qwerty_destroy;
	xb->identifier = "qwerty";
	xb->name = "Qwerty devices builder";
	xb->driver_identifiers = driver_list;
	xb->driver_identifier_count = ARRAY_SIZE(driver_list);
	xb->exclude_from_automatic_discovery = !debug_get_bool_option_enable_qwerty();

	return xb;
}
