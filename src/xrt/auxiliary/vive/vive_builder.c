// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Builder helpers for Vive/Index devices.
 * @author Moses Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vive
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "util/u_misc.h"
#include "util/u_logging.h"
#include "util/u_builders.h"
#include "util/u_system_helpers.h"

#include "vive_common.h"
#include "vive_builder.h"


#define HAVE_USB_DEV(VID, PID) u_builder_find_prober_device(xpdevs, xpdev_count, VID, PID, XRT_BUS_TYPE_USB)


xrt_result_t
vive_builder_estimate(struct xrt_prober *xp,
                      bool have_6dof,
                      bool have_hand_tracking,
                      bool *out_valve_have_index,
                      struct xrt_builder_estimate *out_estimate)
{
	struct xrt_builder_estimate estimate = XRT_STRUCT_INIT;
	struct u_builder_search_results results = {0};
	struct xrt_prober_device **xpdevs = NULL;
	size_t xpdev_count = 0;
	xrt_result_t xret = XRT_SUCCESS;

	// Lock the device list
	xret = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
	if (xret != XRT_SUCCESS) {
		U_LOG_E("Failed to lock list!");
		return xret;
	}

	bool have_vive = HAVE_USB_DEV(HTC_VID, VIVE_PID);
	bool have_vive_pro = HAVE_USB_DEV(HTC_VID, VIVE_PRO_MAINBOARD_PID);
	bool have_valve_index = HAVE_USB_DEV(VALVE_VID, VIVE_PRO_LHR_PID);

	if (have_vive || have_vive_pro || have_valve_index) {
		estimate.certain.head = true;
		if (have_6dof) {
			estimate.maybe.dof6 = true;
			estimate.certain.dof6 = true;
		}
	}

	/*
	 * The Valve Index HMDs have UVC stereo cameras on the front. If we've
	 * found an Index, we'll probably be able to open the camera and use it
	 * to track hands even if we haven't found controllers.
	 */
	if (have_hand_tracking && have_valve_index) {
		estimate.maybe.left = true;
		estimate.maybe.right = true;
	}

	static struct u_builder_search_filter maybe_controller_filters[] = {
	    {VALVE_VID, VIVE_WATCHMAN_DONGLE, XRT_BUS_TYPE_USB},
	    {VALVE_VID, VIVE_WATCHMAN_DONGLE_GEN2, XRT_BUS_TYPE_USB},
	};

	// Reset the results.
	U_ZERO(&results);

	u_builder_search(                         //
	    xp,                                   //
	    xpdevs,                               //
	    xpdev_count,                          //
	    maybe_controller_filters,             //
	    ARRAY_SIZE(maybe_controller_filters), //
	    &results);                            //
	if (results.xpdev_count != 0) {
		estimate.maybe.left = true;
		estimate.maybe.right = true;

		// Good assumption that if the user has more than 2 wireless devices, two of them will be controllers
		// and the rest will be vive trackers.
		if (results.xpdev_count > 2) {
			estimate.maybe.extra_device_count = results.xpdev_count - 2;
		}
	}

	estimate.priority = 0;

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	if (xret) {
		U_LOG_E("Failed to unlock list!");
		return xret;
	}

	*out_valve_have_index = have_valve_index;
	*out_estimate = estimate;

	return XRT_SUCCESS;
}
