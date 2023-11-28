// Copyright 2019-2023, Collabora, Ltd.
// Copyright 2022, Jan Schmidt
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Oculus Rift S prober code.
 * @author Jan Schmidt <jan@centricular.com>
 * @ingroup drv_rift_s
 */

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

#ifdef XRT_BUILD_DRIVER_HANDTRACKING
#include "ht_ctrl_emu/ht_ctrl_emu_interface.h"
#endif

#include "rift_s/rift_s_interface.h"
#include "rift_s/rift_s.h"

enum u_logging_level rift_s_log_level;

/* Interfaces for the various reports / HID controls */
#define RIFT_S_INTF_HMD 6
#define RIFT_S_INTF_STATUS 7
#define RIFT_S_INTF_CONTROLLERS 8

/*
 *
 * Misc stuff.
 *
 */

DEBUG_GET_ONCE_LOG_OPTION(rift_s_log, "RIFT_S_LOG", U_LOGGING_WARN)

#ifdef XRT_BUILD_DRIVER_HANDTRACKING
DEBUG_GET_ONCE_BOOL_OPTION(rift_s_hand_tracking_as_controller, "RIFT_S_HAND_TRACKING_AS_CONTROLLERS", false)
#endif

static const char *driver_list[] = {
    "rift-s",
};


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
rift_s_estimate_system(struct xrt_builder *xb,
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
	    u_builder_find_prober_device(xpdevs, xpdev_count, OCULUS_VR_INC_VID, OCULUS_RIFT_S_PID, XRT_BUS_TYPE_USB);
	if (dev != NULL) {
		estimate->certain.head = true;
		estimate->certain.left = true;
		estimate->certain.right = true;
	}

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	assert(xret == XRT_SUCCESS);

	return XRT_SUCCESS;
}

static xrt_result_t
rift_s_open_system_impl(struct xrt_builder *xb,
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

	rift_s_log_level = debug_get_log_option_rift_s_log();

	xret = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
	if (xret != XRT_SUCCESS) {
		goto unlock_and_fail;
	}

	struct xrt_prober_device *dev_hmd =
	    u_builder_find_prober_device(xpdevs, xpdev_count, OCULUS_VR_INC_VID, OCULUS_RIFT_S_PID, XRT_BUS_TYPE_USB);
	if (dev_hmd == NULL) {
		goto unlock_and_fail;
	}

	struct os_hid_device *hid_hmd = NULL;
	int result = xrt_prober_open_hid_interface(xp, dev_hmd, RIFT_S_INTF_HMD, &hid_hmd);
	if (result != 0) {
		RIFT_S_ERROR("Failed to open Rift S HMD interface");
		goto unlock_and_fail;
	}

	struct os_hid_device *hid_status = NULL;
	result = xrt_prober_open_hid_interface(xp, dev_hmd, RIFT_S_INTF_STATUS, &hid_status);
	if (result != 0) {
		os_hid_destroy(hid_hmd);
		RIFT_S_ERROR("Failed to open Rift S status interface");
		goto unlock_and_fail;
	}

	struct os_hid_device *hid_controllers = NULL;
	result = xrt_prober_open_hid_interface(xp, dev_hmd, RIFT_S_INTF_CONTROLLERS, &hid_controllers);
	if (result != 0) {
		os_hid_destroy(hid_hmd);
		os_hid_destroy(hid_status);
		RIFT_S_ERROR("Failed to open Rift S controllers interface");
		goto unlock_and_fail;
	}

	unsigned char hmd_serial_no[XRT_DEVICE_NAME_LEN];
	result = xrt_prober_get_string_descriptor(xp, dev_hmd, XRT_PROBER_STRING_SERIAL_NUMBER, hmd_serial_no,
	                                          XRT_DEVICE_NAME_LEN);
	if (result < 0) {
		RIFT_S_WARN("Could not read Rift S serial number from USB");
		snprintf((char *)hmd_serial_no, XRT_DEVICE_NAME_LEN, "Unknown");
	}

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	if (xret != XRT_SUCCESS) {
		goto fail;
	}

	struct rift_s_system *sys = rift_s_system_create(xp, hmd_serial_no, hid_hmd, hid_status, hid_controllers);
	if (sys == NULL) {
		RIFT_S_ERROR("Failed to initialise Oculus Rift S driver");
		goto fail;
	}


	// Create and add to list.
	struct xrt_device *hmd_xdev = rift_s_system_get_hmd(sys);
	xsysd->xdevs[xsysd->xdev_count++] = hmd_xdev;

	struct xrt_device *left_xdev = rift_s_system_get_controller(sys, 0);
	xsysd->xdevs[xsysd->xdev_count++] = left_xdev;

	struct xrt_device *right_xdev = rift_s_system_get_controller(sys, 1);
	xsysd->xdevs[xsysd->xdev_count++] = right_xdev;


	struct xrt_device *left_ht = NULL;
	struct xrt_device *right_ht = NULL;

#ifdef XRT_BUILD_DRIVER_HANDTRACKING
	struct xrt_device *ht_xdev = rift_s_system_get_hand_tracking_device(sys);
	if (ht_xdev != NULL) {
		// Create hand-tracked controllers
		RIFT_S_DEBUG("Creating emulated hand tracking controllers");

		struct xrt_device *two_hands[2];
		cemu_devices_create(hmd_xdev, ht_xdev, two_hands);

		left_ht = two_hands[0];
		right_ht = two_hands[1];

		xsysd->xdevs[xsysd->xdev_count++] = two_hands[0];
		xsysd->xdevs[xsysd->xdev_count++] = two_hands[1];

		if (debug_get_bool_option_rift_s_hand_tracking_as_controller()) {
			left_xdev = two_hands[0];
			right_xdev = two_hands[1];
		}
	}
#endif

	// Assign to role(s).
	ubrh->head = hmd_xdev;
	ubrh->left = left_xdev;
	ubrh->right = right_xdev;
	ubrh->hand_tracking.left = left_ht;
	ubrh->hand_tracking.right = right_ht;

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
rift_s_destroy(struct xrt_builder *xb)
{
	free(xb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
rift_s_builder_create(void)
{
	struct u_builder *ub = U_TYPED_CALLOC(struct u_builder);

	// xrt_builder fields.
	ub->base.estimate_system = rift_s_estimate_system;
	ub->base.open_system = u_builder_open_system_static_roles;
	ub->base.destroy = rift_s_destroy;
	ub->base.identifier = "rift_s";
	ub->base.name = "Oculus Rift S";
	ub->base.driver_identifiers = driver_list;
	ub->base.driver_identifier_count = ARRAY_SIZE(driver_list);

	// u_builder fields.
	ub->open_system_static_roles = rift_s_open_system_impl;

	return &ub->base;
}
