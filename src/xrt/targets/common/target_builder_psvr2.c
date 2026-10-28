// Copyright 2025, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  PS VR2 prober code.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_system.h"

#include "util/u_debug.h"
#include "util/u_misc.h"
#include "util/u_logging.h"
#include "util/u_trace_marker.h"
#include "util/u_builder_search.h"

#include "target_builder_helpers.h"

#include "psvr2/psvr2_interface.h"

#ifdef XRT_BUILD_DRIVER_PSSENSE
#include "pssense/pssense_interface.h"
#endif

/*
 *
 * Internal structures
 *
 */

struct psvr2_builder
{
	struct t_builder base;

	enum u_logging_level log_level;
};

static struct psvr2_builder *
psvr2_builder(struct xrt_builder *xb)
{
	return (struct psvr2_builder *)xb;
}

/*
 *
 * Misc stuff.
 *
 */

DEBUG_GET_ONCE_LOG_OPTION(psvr2_log, "PSVR2_LOG", U_LOGGING_WARN)

#define PSVR2_ERROR(p, ...) U_LOG_IFL_E(p->log_level, __VA_ARGS__)
#define PSVR2_DEBUG(p, ...) U_LOG_IFL_D(p->log_level, __VA_ARGS__)

static const char *driver_list[] = {
    "psvr2",
    "pssense",
};


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
psvr2_estimate_system(struct xrt_builder *xb,
                      cJSON *config,
                      struct xrt_prober *xp,
                      struct xrt_builder_estimate *estimate)
{
	struct psvr2_builder *pb = psvr2_builder(xb);

	struct xrt_prober_device **xpdevs = NULL;
	size_t xpdev_count = 0;
	xrt_result_t xret = XRT_SUCCESS;

	U_ZERO(estimate);

	xret = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
	U_LOG_CHK_AND_RET(pb->log_level, xret, "xrt_prober_lock_list");

	struct xrt_prober_device *dev =
	    u_builder_find_prober_device(xpdevs, xpdev_count, PSVR2_VID, PSVR2_PID, XRT_BUS_TYPE_USB);
	if (dev != NULL) {
		estimate->certain.head = true;
		estimate->certain.dof6 = true;
	}

#ifdef XRT_BUILD_DRIVER_PSSENSE
	struct xrt_prober_device *dev_controller_left =
	    u_builder_find_prober_device(xpdevs, xpdev_count, PSSENSE_VID, PSSENSE_PID_LEFT, XRT_BUS_TYPE_ANY);
	if (dev_controller_left != NULL) {
		estimate->certain.left = true;
	}

	struct xrt_prober_device *dev_controller_right =
	    u_builder_find_prober_device(xpdevs, xpdev_count, PSSENSE_VID, PSSENSE_PID_RIGHT, XRT_BUS_TYPE_ANY);
	if (dev_controller_right != NULL) {
		estimate->certain.right = true;
	}
#endif

	PSVR2_DEBUG(pb, "PSVR2 builder estimate: head %d, left %d, right %d", estimate->certain.head,
	            estimate->certain.left, estimate->certain.right);

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	U_LOG_CHK_AND_RET(pb->log_level, xret, "xrt_prober_unlock_list");

	return xret;
}

static xrt_result_t
psvr2_open_system_impl(struct xrt_builder *xb,
                       cJSON *config,
                       struct xrt_prober *xp,
                       struct xrt_tracking_origin *origin,
                       struct xrt_system_devices *xsysd,
                       struct xrt_frame_context *xfctx,
                       struct t_builder_options *tbo)
{
	struct xrt_prober_device **xpdevs = NULL;
	size_t xpdev_count = 0;
	xrt_result_t xret = XRT_SUCCESS;

	DRV_TRACE_MARKER();

	xret = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
	if (xret != XRT_SUCCESS) {
		goto unlock_and_fail;
	}

	struct xrt_device *head_xdev = NULL;
	struct xrt_prober_device *head_xpdev =
	    u_builder_find_prober_device(xpdevs, xpdev_count, PSVR2_VID, PSVR2_PID, XRT_BUS_TYPE_USB);
	if (head_xpdev != NULL) {
		head_xdev = psvr2_hmd_create(head_xpdev);
		if (head_xdev == NULL) {
			PSVR2_ERROR(psvr2_builder(xb), "PSVR2 HMD device creation failed");
			goto unlock_and_fail;
		}

		xsysd->static_xdevs[xsysd->static_xdev_count++] = head_xdev;
	}

	tbo->head = head_xdev;
	tbo->eyes = head_xdev;
	tbo->face = head_xdev;

#ifdef XRT_BUILD_DRIVER_PSSENSE
	struct xrt_device *left_xdev = NULL;
	struct xrt_prober_device *left_xpdev =
	    u_builder_find_prober_device(xpdevs, xpdev_count, PSSENSE_VID, PSSENSE_PID_LEFT, XRT_BUS_TYPE_ANY);
	if (left_xpdev != NULL) {
		left_xdev = pssense_create(xp, left_xpdev, xfctx, NULL);
		if (left_xdev == NULL) {
			PSVR2_ERROR(psvr2_builder(xb), "PS Sense left controller device creation failed");
		} else {
			xsysd->static_xdevs[xsysd->static_xdev_count++] = left_xdev;
		}
	}

	struct xrt_device *right_xdev = NULL;
	struct xrt_prober_device *right_xpdev =
	    u_builder_find_prober_device(xpdevs, xpdev_count, PSSENSE_VID, PSSENSE_PID_RIGHT, XRT_BUS_TYPE_ANY);
	if (right_xpdev != NULL) {
		right_xdev = pssense_create(xp, right_xpdev, xfctx, NULL);
		if (right_xdev == NULL) {
			PSVR2_ERROR(psvr2_builder(xb), "PS Sense right controller device creation failed");
		} else {
			xsysd->static_xdevs[xsysd->static_xdev_count++] = right_xdev;
		}
	}

	tbo->left = left_xdev;
	tbo->right = right_xdev;
#endif

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	if (xret != XRT_SUCCESS) {
		goto fail;
	}

	return XRT_SUCCESS;


unlock_and_fail:
	xret = xrt_prober_unlock_list(xp, &xpdevs);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	/* Fallthrough */
fail:
	return XRT_ERROR_DEVICE_CREATION_FAILED;
}

static void
psvr2_destroy(struct xrt_builder *xb)
{
	free(xb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
t_builder_psvr2_create(void)
{
	struct psvr2_builder *pb = U_TYPED_CALLOC(struct psvr2_builder);

	pb->log_level = debug_get_log_option_psvr2_log();

	// xrt_builder fields.
	pb->base.base.estimate_system = psvr2_estimate_system;
	pb->base.base.open_system = t_builder_open_system_static_roles;
	pb->base.base.destroy = psvr2_destroy;
	pb->base.base.identifier = "psvr2";
	pb->base.base.name = "PlayStation VR 2";
	pb->base.base.driver_identifiers = driver_list;
	pb->base.base.driver_identifier_count = ARRAY_SIZE(driver_list);

	// t_builder fields.
	pb->base.open_system_static_roles = psvr2_open_system_impl;

	return &pb->base.base;
}
