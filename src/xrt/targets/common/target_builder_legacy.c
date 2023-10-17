// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Fallback builder the old method of probing devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_prober.h"

#include "util/u_misc.h"
#include "util/u_device.h"
#include "util/u_builders.h"
#include "util/u_system_helpers.h"

#include "target_builder_interface.h"

#include <assert.h>

static const char *driver_list[] = {
#ifdef XRT_BUILD_DRIVER_HYDRA
    "hydra",
#endif

#ifdef XRT_BUILD_DRIVER_HDK
    "hdk",
#endif

#ifdef XRT_BUILD_DRIVER_ULV2
    "ulv2",
#endif

#ifdef XRT_BUILD_DRIVER_DEPTHAI
    "depthai",
#endif

#ifdef XRT_BUILD_DRIVER_WMR
    "wmr",
#endif

#ifdef XRT_BUILD_DRIVER_ARDUINO
    "arduino",
#endif

#ifdef XRT_BUILD_DRIVER_DAYDREAM
    "daydream",
#endif

#ifdef XRT_BUILD_DRIVER_OHMD
    "oh",
#endif

#ifdef XRT_BUILD_DRIVER_NS
    "ns",
#endif

#ifdef XRT_BUILD_DRIVER_ANDROID
    "android",
#endif

#ifdef XRT_BUILD_DRIVER_ILLIXR
    "illixr",
#endif

#ifdef XRT_BUILD_DRIVER_REALSENSE
    "rs",
#endif

#ifdef XRT_BUILD_DRIVER_EUROC
    "euroc",
#endif

#ifdef XRT_BUILD_DRIVER_QWERTY
    "qwerty",
#endif

#if defined(XRT_BUILD_DRIVER_HANDTRACKING) && defined(XRT_BUILD_DRIVER_DEPTHAI)
    "ht",
#endif

#if defined(XRT_BUILD_DRIVER_SIMULATED)
    "simulated",
#endif
    NULL,
};


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
legacy_estimate_system(struct xrt_builder *xb,
                       cJSON *config,
                       struct xrt_prober *xp,
                       struct xrt_builder_estimate *estimate)
{
	// If no driver is enabled, there is no way to create a HMD
	bool may_create_hmd = xb->driver_identifier_count > 0;

	estimate->maybe.head = may_create_hmd;
	estimate->maybe.left = may_create_hmd;
	estimate->maybe.right = may_create_hmd;
	estimate->priority = -20;

	return XRT_SUCCESS;
}

static xrt_result_t
legacy_open_system(struct xrt_builder *xb,
                   cJSON *config,
                   struct xrt_prober *xp,
                   struct xrt_system_devices **out_xsysd,
                   struct xrt_space_overseer **out_xso)
{
	xrt_result_t xret;
	int ret;

	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);


	/*
	 * Create the devices.
	 */

	xret = xrt_prober_probe(xp);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	// Use the static system devices helper, no dynamic roles.
	struct u_system_devices_static *usysds = u_system_devices_static_allocate();
	struct xrt_system_devices *xsysd = &usysds->base.base;

	ret = xrt_prober_select(xp, xsysd->xdevs, ARRAY_SIZE(xsysd->xdevs));
	if (ret < 0) {
		xrt_system_devices_destroy(&xsysd);
		return XRT_ERROR_DEVICE_CREATION_FAILED;
	}

	// Count the xdevs.
	for (uint32_t i = 0; i < ARRAY_SIZE(xsysd->xdevs); i++) {
		if (xsysd->xdevs[i] == NULL) {
			break;
		}

		xsysd->xdev_count++;
	}


	/*
	 * Setup the roles.
	 */

	int head_idx, left_idx, right_idx;
	u_device_assign_xdev_roles(xsysd->xdevs, xsysd->xdev_count, &head_idx, &left_idx, &right_idx);

	struct xrt_device *head = NULL;
	struct xrt_device *left = NULL, *right = NULL;
	struct xrt_device *left_ht = NULL, *right_ht = NULL;

	if (head_idx >= 0) {
		head = xsysd->xdevs[head_idx];
	}
	if (left_idx >= 0) {
		left = xsysd->xdevs[left_idx];
	}
	if (right_idx >= 0) {
		right = xsysd->xdevs[right_idx];
	}

	// Find hand tracking devices.
	left_ht = u_system_devices_get_ht_device_left(xsysd);
	right_ht = u_system_devices_get_ht_device_right(xsysd);

	// Assign to role(s).
	xsysd->static_roles.head = head;
	xsysd->static_roles.hand_tracking.left = left_ht;
	xsysd->static_roles.hand_tracking.right = right_ht;

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
legacy_destroy(struct xrt_builder *xb)
{
	free(xb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
t_builder_legacy_create(void)
{
	struct xrt_builder *xb = U_TYPED_CALLOC(struct xrt_builder);
	xb->estimate_system = legacy_estimate_system;
	xb->open_system = legacy_open_system;
	xb->destroy = legacy_destroy;
	xb->identifier = "legacy";
	xb->name = "Legacy probing system";
	xb->driver_identifiers = driver_list;
	xb->driver_identifier_count = ARRAY_SIZE(driver_list) - 1;

	return xb;
}
