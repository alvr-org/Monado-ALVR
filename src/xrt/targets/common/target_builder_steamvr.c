// Copyright 2023, Duncan Spaulding.
// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Builder for SteamVR proprietary driver wrapper.
 * @author BabbleBones <BabbleBones@protonmail.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */
#include "xrt/xrt_config_drivers.h"

#include <assert.h>
#include <stdbool.h>

#include "tracking/t_tracking.h"

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_system_helpers.h"

#include "vive/vive_builder.h"

#include "target_builder_interface.h"
#include "target_builder_helpers.h"

#include "steamvr_lh/steamvr_lh_interface.h"
#include "xrt/xrt_results.h"

#include "xrt/xrt_space.h"
#include "b_space_overseer.h"

#ifndef XRT_BUILD_DRIVER_STEAMVR_LIGHTHOUSE
#error "This builder requires the SteamVR Lighthouse driver"
#endif

DEBUG_GET_ONCE_LOG_OPTION(svr_log, "STEAMVR_LH_LOG", U_LOGGING_INFO)

#define SVR_TRACE(...) U_LOG_IFL_T(debug_get_log_option_svr_log(), __VA_ARGS__)
#define SVR_DEBUG(...) U_LOG_IFL_D(debug_get_log_option_svr_log(), __VA_ARGS__)
#define SVR_INFO(...) U_LOG_IFL_I(debug_get_log_option_svr_log(), __VA_ARGS__)
#define SVR_WARN(...) U_LOG_IFL_W(debug_get_log_option_svr_log(), __VA_ARGS__)
#define SVR_ERROR(...) U_LOG_IFL_E(debug_get_log_option_svr_log(), __VA_ARGS__)
#define SVR_ASSERT(predicate, ...)                                                                                     \
	do {                                                                                                           \
		bool p = predicate;                                                                                    \
		if (!p) {                                                                                              \
			U_LOG(U_LOGGING_ERROR, __VA_ARGS__);                                                           \
			assert(false && "SVR_ASSERT failed: " #predicate);                                             \
			exit(EXIT_FAILURE);                                                                            \
		}                                                                                                      \
	} while (false);
#define SVR_ASSERT_(predicate) SVR_ASSERT(predicate, "Assertion failed " #predicate)


/*
 *
 * Misc stuff.
 *
 */

DEBUG_GET_ONCE_BOOL_OPTION(steamvr_enable, "STEAMVR_LH_ENABLE", false)

static const char *driver_list[] = {
    "steamvr_lh",
};

struct steamvr_builder
{
	struct t_builder base;

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

static void
steamvr_destroy(struct xrt_builder *xb)
{
	struct steamvr_builder *svrb = (struct steamvr_builder *)xb;
	free(svrb);
}

static xrt_result_t
steamvr_open_system_impl(struct xrt_builder *xb,
                         cJSON *config,
                         struct xrt_prober *xp,
                         struct xrt_tracking_origin *origin,
                         struct xrt_system_devices *xsysd,
                         struct xrt_frame_context *xfctx,
                         struct t_builder_options *tbo)
{
	enum xrt_result result = steamvr_lh_create_devices(xp, xsysd);

	if (result != XRT_SUCCESS) {
		SVR_ERROR("Unable to create devices");
		return result;
	}

	int head, eyes, face, left, right, gamepad;
	u_device_assign_xdev_roles(xsysd->static_xdevs, xsysd->static_xdev_count, &head, &eyes, &face, &left, &right,
	                           &gamepad);

	if (head == XRT_DEVICE_ROLE_UNASSIGNED) {
		SVR_ERROR("Unable to find HMD");
		return XRT_ERROR_DEVICE_CREATION_FAILED;
	}

#define SET_HT_ROLES(SRC)                                                                                              \
	tbo->hand_tracking.SRC.left = u_system_devices_get_ht_device_##SRC##_left(xsysd);                              \
	tbo->hand_tracking.SRC.right = u_system_devices_get_ht_device_##SRC##_right(xsysd);
	SET_HT_ROLES(unobstructed)
	SET_HT_ROLES(conforming)
#undef SET_HT_ROLES

	tbo->head = xsysd->static_xdevs[head];

	if (left != XRT_DEVICE_ROLE_UNASSIGNED) {
		tbo->left = xsysd->static_xdevs[left];
	}
	if (right != XRT_DEVICE_ROLE_UNASSIGNED) {
		tbo->right = xsysd->static_xdevs[right];
	}
	if (gamepad != XRT_DEVICE_ROLE_UNASSIGNED) {
		tbo->gamepad = xsysd->static_xdevs[gamepad];
	}

	tbo->T_stage_local = (struct xrt_pose)XRT_POSE_IDENTITY;

	return result;
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

	svrb->base.base.estimate_system = steamvr_estimate_system;
	svrb->base.base.open_system = t_builder_open_system_static_roles;
	svrb->base.base.destroy = steamvr_destroy;
	svrb->base.base.identifier = "steamvr";
	svrb->base.base.name = "SteamVR proprietary wrapper (Vive, Index, Tundra trackers, etc.) devices builder";
	svrb->base.base.driver_identifiers = driver_list;
	svrb->base.base.driver_identifier_count = ARRAY_SIZE(driver_list);

	svrb->base.open_system_static_roles = steamvr_open_system_impl;

	return &svrb->base.base;
}
