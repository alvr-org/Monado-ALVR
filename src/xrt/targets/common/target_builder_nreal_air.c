// Copyright 2023, Tobias Frisch
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Nreal Air prober code.
 * @author Tobias Frisch <thejackimonster@gmail.com>
 * @ingroup drv_na
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "os/os_hid.h"

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_prober.h"

#include "util/u_builders.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_system_helpers.h"
#include "util/u_trace_marker.h"

#include "nreal_air/na_hmd.h"
#include "nreal_air/na_interface.h"

enum u_logging_level nreal_air_log_level;

#define NA_WARN(...) U_LOG_IFL_W(nreal_air_log_level, __VA_ARGS__)
#define NA_ERROR(...) U_LOG_IFL_E(nreal_air_log_level, __VA_ARGS__)


/*
 *
 * Misc stuff.
 *
 */

DEBUG_GET_ONCE_LOG_OPTION(nreal_air_log, "NA_LOG", U_LOGGING_WARN)

static const char *driver_list[] = {
    "nreal_air",
};


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
nreal_air_estimate_system(struct xrt_builder *xb,
                          cJSON *config,
                          struct xrt_prober *xp,
                          struct xrt_builder_estimate *estimate)
{
	struct xrt_prober_device **xpdevs = NULL;
	size_t xpdev_count = 0;
	xrt_result_t xret = XRT_SUCCESS;

	U_ZERO(estimate);

	xret = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	struct xrt_prober_device *dev =
	    u_builder_find_prober_device(xpdevs, xpdev_count, NA_VID, NA_PID, XRT_BUS_TYPE_USB);
	if (dev != NULL) {
		estimate->certain.head = true;
	}

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	assert(xret == XRT_SUCCESS);

	return XRT_SUCCESS;
}

static xrt_result_t
nreal_air_open_system_impl(struct xrt_builder *xb,
                           cJSON *config,
                           struct xrt_prober *xp,
                           struct xrt_tracking_origin *origin,
                           struct xrt_system_devices *xsysd,
                           struct xrt_frame_context *xfctx,
                           struct u_builder_roles_helper *ubrh)
{
	struct xrt_prober_device **xpdevs = NULL;
	size_t xpdev_count = 0;
	xrt_result_t xret = XRT_SUCCESS;

	DRV_TRACE_MARKER();

	nreal_air_log_level = debug_get_log_option_nreal_air_log();

	xret = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
	if (xret != XRT_SUCCESS) {
		goto unlock_and_fail;
	}

	struct xrt_prober_device *dev_hmd =
	    u_builder_find_prober_device(xpdevs, xpdev_count, NA_VID, NA_PID, XRT_BUS_TYPE_USB);
	if (dev_hmd == NULL) {
		goto unlock_and_fail;
	}

	struct os_hid_device *hid_handle = NULL;
	int result = xrt_prober_open_hid_interface(xp, dev_hmd, NA_HANDLE_IFACE, &hid_handle);
	if (result != 0) {
		NA_ERROR("Failed to open Nreal Air handle interface");
		goto unlock_and_fail;
	}

	struct os_hid_device *hid_control = NULL;
	result = xrt_prober_open_hid_interface(xp, dev_hmd, NA_CONTROL_IFACE, &hid_control);
	if (result != 0) {
		os_hid_destroy(hid_handle);
		NA_ERROR("Failed to open Nreal Air control interface");
		goto unlock_and_fail;
	}

	unsigned char hmd_serial_no[XRT_DEVICE_NAME_LEN];
	result = xrt_prober_get_string_descriptor(xp, dev_hmd, XRT_PROBER_STRING_SERIAL_NUMBER, hmd_serial_no,
	                                          XRT_DEVICE_NAME_LEN);
	if (result < 0) {
		NA_WARN("Could not read Nreal Air serial number from USB");
		snprintf((char *)hmd_serial_no, XRT_DEVICE_NAME_LEN, "Unknown");
	}

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	if (xret != XRT_SUCCESS) {
		goto fail;
	}

	struct xrt_device *na_device = na_hmd_create_device(hid_handle, hid_control, nreal_air_log_level);
	if (na_device == NULL) {
		NA_ERROR("Failed to initialise Nreal Air driver");
		goto fail;
	}

	// Add to device list.
	xsysd->xdevs[xsysd->xdev_count++] = na_device;

	// Assign to role(s).
	ubrh->head = na_device;

	return XRT_SUCCESS;


	/*
	 * Error path.
	 */

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
nreal_air_destroy(struct xrt_builder *xb)
{
	free(xb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
nreal_air_builder_create(void)
{
	struct u_builder *ub = U_TYPED_CALLOC(struct u_builder);

	// xrt_builder fields.
	ub->base.estimate_system = nreal_air_estimate_system;
	ub->base.open_system = u_builder_open_system_static_roles;
	ub->base.destroy = nreal_air_destroy;
	ub->base.identifier = "nreal_air";
	ub->base.name = "Nreal Air";
	ub->base.driver_identifiers = driver_list;
	ub->base.driver_identifier_count = ARRAY_SIZE(driver_list);

	// u_builder fields.
	ub->open_system_static_roles = nreal_air_open_system_impl;

	return &ub->base;
}
