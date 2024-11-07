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
#include "util/u_builders.h"
#include "xrt/xrt_config_drivers.h"


#include <assert.h>
#include <stdbool.h>

#include "tracking/t_tracking.h"

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "util/u_debug.h"
#include "util/u_system_helpers.h"

#include "vive/vive_builder.h"

#include "target_builder_interface.h"

#include "steamvr_lh/steamvr_lh_interface.h"
#include "xrt/xrt_results.h"

#include "xrt/xrt_space.h"
#include "util/u_space_overseer.h"

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
	struct xrt_builder base;

	struct xrt_device *head;
	struct xrt_device *left_ht, *right_ht;

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
steamvr_open_system(struct xrt_builder *xb,
                    cJSON *config,
                    struct xrt_prober *xp,
                    struct xrt_session_event_sink *broadcast,
                    struct xrt_system_devices **out_xsysd,
                    struct xrt_space_overseer **out_xso)
{
	struct steamvr_builder *svrb = (struct steamvr_builder *)xb;

	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);

	enum xrt_result result = steamvr_lh_create_devices(out_xsysd);

	if (result != XRT_SUCCESS) {
		SVR_ERROR("Unable to create devices");
		steamvr_destroy(xb);
		return result;
	}

	struct xrt_system_devices *xsysd = NULL;
	xsysd = *out_xsysd;

	if (xsysd->static_roles.head == NULL) {
		SVR_ERROR("Unable to find HMD");
		steamvr_destroy(xb);
		return XRT_ERROR_DEVICE_CREATION_FAILED;
	}

	svrb->head = xsysd->static_roles.head;

	svrb->left_ht = u_system_devices_get_ht_device_left(xsysd);
	xsysd->static_roles.hand_tracking.left = svrb->left_ht;

	svrb->right_ht = u_system_devices_get_ht_device_right(xsysd);
	xsysd->static_roles.hand_tracking.right = svrb->right_ht;

	/*
	 * Space overseer.
	 */

	struct u_space_overseer *uso = u_space_overseer_create(broadcast);

	struct xrt_pose T_stage_local = XRT_POSE_IDENTITY;

	u_space_overseer_legacy_setup( //
	    uso,                       // uso
	    xsysd->xdevs,              // xdevs
	    xsysd->xdev_count,         // xdev_count
	    svrb->head,                // head
	    &T_stage_local,            // local_offset
	    false,                     // root_is_unbounded
	    true                       // per_app_local_spaces
	);

	*out_xso = (struct xrt_space_overseer *)uso;

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
	svrb->base.estimate_system = steamvr_estimate_system;
	svrb->base.open_system = steamvr_open_system;
	svrb->base.destroy = steamvr_destroy;
	svrb->base.identifier = "steamvr";
	svrb->base.name = "SteamVR proprietary wrapper (Vive, Index, Tundra trackers, etc.) devices builder";
	svrb->base.driver_identifiers = driver_list;
	svrb->base.driver_identifier_count = ARRAY_SIZE(driver_list);

	return &svrb->base;
}
