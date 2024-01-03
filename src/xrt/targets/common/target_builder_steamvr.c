// Copyright 2023, Duncan Spaulding.
// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Builder for SteamVR proprietary driver wrapper.
 * @author BabbleBones <BabbleBones@protonmail.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#include "tracking/t_hand_tracking.h"
#include "tracking/t_tracking.h"

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "util/u_builders.h"
#include "util/u_config_json.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_sink.h"
#include "util/u_system_helpers.h"

#include "vive/vive_builder.h"

#include "target_builder_interface.h"

#include "steamvr_lh/steamvr_lh_interface.h"

#ifndef XRT_BUILD_DRIVER_STEAMVR_LIGHTHOUSE
#error "This builder requires the SteamVR Lighthouse driver"
#endif


/*
 *
 * Misc stuff.
 *
 */

DEBUG_GET_ONCE_LOG_OPTION(steamvr_log, "STEAMVR_LH_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_BOOL_OPTION(steamvr_enable, "STEAMVR_LH_ENABLE", false)

#define LH_TRACE(...) U_LOG_IFL_T(debug_get_log_option_steamvr_log(), __VA_ARGS__)
#define LH_DEBUG(...) U_LOG_IFL_D(debug_get_log_option_steamvr_log(), __VA_ARGS__)
#define LH_INFO(...) U_LOG_IFL_I(debug_get_log_option_steamvr_log(), __VA_ARGS__)
#define LH_WARN(...) U_LOG_IFL_W(debug_get_log_option_steamvr_log(), __VA_ARGS__)
#define LH_ERROR(...) U_LOG_IFL_E(debug_get_log_option_steamvr_log(), __VA_ARGS__)

static const char *driver_list[] = {
    "steamvr_lh",
};

struct steamvr_builder
{
	struct xrt_builder base;

	/*!
	 * Is our HMD a Valve Index?
	 */
	bool is_valve_index;
};


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
steamvr_estimate_system(struct xrt_builder *xb,
                        cJSON *config,
                        struct xrt_prober *xp,
                        struct xrt_builder_estimate *estimate)
{
	struct steamvr_builder *svrb = (struct steamvr_builder *)xb;

	// Currently no built in support for hand tracking.
	bool have_hand_tracking = false;

	if (debug_get_bool_option_steamvr_enable()) {
		return vive_builder_estimate( //
		    xp,                       // xp
		    true,                     // have_6dof
		    have_hand_tracking,       // have_hand_tracking
		    &svrb->is_valve_index,    // out_have_valve_index
		    estimate);                // out_estimate
	} else {
		return XRT_SUCCESS;
	}
}

static xrt_result_t
steamvr_open_system(struct xrt_builder *xb,
                    cJSON *config,
                    struct xrt_prober *xp,
                    struct xrt_session_event_sink *broadcast,
                    struct xrt_system_devices **out_xsysd,
                    struct xrt_space_overseer **out_xso)
{
	struct steamvr_builder *svrb = (struct steamvr_builder *)xb;
	struct u_system_devices_static *usysds = NULL;
	struct xrt_system_devices *xsysd = NULL;
	xrt_result_t result = XRT_SUCCESS;

	// Sanity checking.
	if (out_xsysd == NULL || *out_xsysd != NULL) {
		LH_ERROR("Invalid output system pointer");
		return XRT_ERROR_DEVICE_CREATION_FAILED;
	}

	// Use the static system devices helper, no dynamic roles.
	usysds = u_system_devices_static_allocate();
	xsysd = &usysds->base.base;

	// Do creation.
	xsysd->xdev_count += steamvr_lh_get_devices(&xsysd->xdevs[xsysd->xdev_count]);

	// Device indices.
	int head_idx = -1;
	int left_idx = -1;
	int right_idx = -1;

	// Look for regular devices.
	u_device_assign_xdev_roles(xsysd->xdevs, xsysd->xdev_count, &head_idx, &left_idx, &right_idx);

	// Sanity check.
	if (head_idx < 0) {
		LH_ERROR("Unable to find HMD");
		result = XRT_ERROR_DEVICE_CREATION_FAILED;
		goto end_err;
	}

	// Devices to populate.
	struct xrt_device *head = NULL;
	struct xrt_device *left = NULL, *right = NULL;
	struct xrt_device *left_ht = NULL, *right_ht = NULL;

	// Always have a head.
	head = xsysd->xdevs[head_idx];

	// It's okay if we didn't find controllers
	if (left_idx >= 0) {
		left = xsysd->xdevs[left_idx];
		left_ht = u_system_devices_get_ht_device_left(xsysd);
	}

	if (right_idx >= 0) {
		right = xsysd->xdevs[right_idx];
		right_ht = u_system_devices_get_ht_device_right(xsysd);
	}

	if (svrb->is_valve_index) {
		// This space left intentionally blank
	}

	// Assign to role(s).
	xsysd->static_roles.head = head;
	xsysd->static_roles.hand_tracking.left = left_ht;
	xsysd->static_roles.hand_tracking.right = right_ht;

	u_system_devices_static_finalize( //
	    usysds,                       // usysds
	    left,                         // left
	    right);                       // right

	*out_xsysd = xsysd;
	u_builder_create_space_overseer_legacy( //
	    broadcast,                          // broadcast
	    head,                               // head
	    left,                               // left
	    right,                              // right
	    xsysd->xdevs,                       // xdevs
	    xsysd->xdev_count,                  // xdev_count
	    false,                              // root_is_unbounded
	    out_xso);                           // out_xso

	return XRT_SUCCESS;

end_err:
	xrt_system_devices_destroy(&xsysd);

	return result;
}

static void
steamvr_destroy(struct xrt_builder *xb)
{
	struct steamvr_builder *svrb = (struct steamvr_builder *)xb;
	free(svrb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
t_builder_steamvr_create(void)
{
	struct steamvr_builder *svrb = U_TYPED_CALLOC(struct steamvr_builder);
	svrb->base.estimate_system = steamvr_estimate_system;
	svrb->base.open_system = steamvr_open_system;
	svrb->base.destroy = steamvr_destroy;
	svrb->base.identifier = "steamvr";
	svrb->base.name = "SteamVR proprietary wrapper (Vive, Index, Tundra trackers, etc.) devices builder";
	svrb->base.driver_identifiers = driver_list;
	svrb->base.driver_identifier_count = ARRAY_SIZE(driver_list);

	return &svrb->base;
}
