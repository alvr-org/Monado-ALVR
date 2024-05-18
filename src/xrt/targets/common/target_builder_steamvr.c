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
	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);

	return steamvr_lh_create_devices(broadcast, out_xsysd, out_xso);
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
