// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for @ref xrt_builder implementations.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#include "xrt/xrt_prober.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_tracking.h"

#include "util/u_debug.h"
#include "util/u_system_helpers.h"
#include "util/u_builder_helpers.h"

#include "b_space_overseer.h"
#include "b_system_devices.h"

#include "target_builder_helpers.h"


DEBUG_GET_ONCE_FLOAT_OPTION(tracking_origin_offset_x, "XRT_TRACKING_ORIGIN_OFFSET_X", 0.0f)
DEBUG_GET_ONCE_FLOAT_OPTION(tracking_origin_offset_y, "XRT_TRACKING_ORIGIN_OFFSET_Y", 0.0f)
DEBUG_GET_ONCE_FLOAT_OPTION(tracking_origin_offset_z, "XRT_TRACKING_ORIGIN_OFFSET_Z", 0.0f)


/*
 *
 * 'Exported' functions.
 *
 */

void
t_builder_create_space_overseer_legacy(struct xrt_session_event_sink *broadcast,
                                       struct xrt_device *head,
                                       struct xrt_device *eyes,
                                       struct xrt_device *left,
                                       struct xrt_device *right,
                                       struct xrt_device *gamepad,
                                       struct xrt_device **xdevs,
                                       uint32_t xdev_count,
                                       bool root_is_unbounded,
                                       const struct xrt_pose *T_stage_local,
                                       bool per_app_local_spaces,
                                       struct xrt_space_overseer **out_xso)
{
	/*
	 * Tracking origins.
	 */

	struct xrt_vec3 global_tracking_origin_offset = {
	    debug_get_float_option_tracking_origin_offset_x(),
	    debug_get_float_option_tracking_origin_offset_y(),
	    debug_get_float_option_tracking_origin_offset_z(),
	};

	u_builder_setup_tracking_origins(    //
	    head,                            //
	    eyes,                            //
	    left,                            //
	    right,                           //
	    gamepad,                         //
	    &global_tracking_origin_offset); //


	/*
	 * Space overseer.
	 */

	struct b_space_overseer *uso = b_space_overseer_create(broadcast);

	b_space_overseer_legacy_setup( //
	    uso,                       // uso
	    xdevs,                     // xdevs
	    xdev_count,                // xdev_count
	    head,                      // head
	    T_stage_local,             // local_offset
	    root_is_unbounded,         // root_is_unbounded
	    per_app_local_spaces       // per_app_local_spaces
	);

	*out_xso = (struct xrt_space_overseer *)uso;
}

xrt_result_t
t_builder_roles_helper_open_system(struct xrt_builder *xb,
                                   cJSON *config,
                                   struct xrt_prober *xp,
                                   struct xrt_session_event_sink *broadcast,
                                   struct xrt_system_devices **out_xsysd,
                                   struct xrt_space_overseer **out_xso,
                                   t_builder_open_system_fn fn)
{
	struct t_builder_options tbo = {
	    .T_stage_local =
	        {
	            .orientation = XRT_QUAT_IDENTITY,
	            .position = {.x = 0, .y = 1.6, .z = 0},
	        },
	};
	xrt_result_t xret;

	// Use the static system devices helper, no dynamic roles.
	struct b_system_devices_static *bsysds = b_system_devices_static_allocate();
	struct xrt_tracking_origin *origin = &bsysds->base.origin;
	struct xrt_system_devices *xsysd = &bsysds->base.base;
	struct xrt_frame_context *xfctx = &bsysds->base.xfctx;

	xret = fn(  //
	    xb,     // xb
	    config, // config
	    xp,     // xp
	    origin, // origin
	    xsysd,  // xsysd
	    xfctx,  // xfctx
	    &tbo);  // tbo
	if (xret != XRT_SUCCESS) {
		xrt_system_devices_destroy(&xsysd);
		return xret;
	}

	/*
	 * Assign to role(s).
	 */

	xsysd->static_roles.head = tbo.head;
	xsysd->static_roles.eyes = tbo.eyes;
	xsysd->static_roles.face = tbo.face;
#define U_SET_HT_ROLE(SRC)                                                                                             \
	xsysd->static_roles.hand_tracking.SRC.left = tbo.hand_tracking.SRC.left;                                       \
	xsysd->static_roles.hand_tracking.SRC.right = tbo.hand_tracking.SRC.right;
	U_SET_HT_ROLE(unobstructed)
	U_SET_HT_ROLE(conforming)
#undef U_SET_HT_ROLE


	xret = b_system_devices_static_finalize( //
	    bsysds,                              // bsysds
	    tbo.left,                            // left
	    tbo.right,                           // right
	    tbo.gamepad);                        // gamepad
	if (xret != XRT_SUCCESS) {
		xrt_system_devices_destroy(&xsysd);
		return xret;
	}


	/*
	 * Create the space overseer.
	 */

	*out_xsysd = xsysd;
	t_builder_create_space_overseer_legacy( //
	    broadcast,                          // broadcast
	    tbo.head,                           // head
	    tbo.eyes,                           // eyes
	    tbo.left,                           // left
	    tbo.right,                          // right
	    tbo.gamepad,                        // gamepad
	    xsysd->static_xdevs,                // xdevs
	    xsysd->static_xdev_count,           // xdev_count
	    false,                              // root_is_unbounded
	    &tbo.T_stage_local,                 // T_stage_local
	    true,                               // per_app_local_spaces
	    out_xso);                           // out_xso

	return XRT_SUCCESS;
}

xrt_result_t
t_builder_open_system_static_roles(struct xrt_builder *xb,
                                   cJSON *config,
                                   struct xrt_prober *xp,
                                   struct xrt_session_event_sink *broadcast,
                                   struct xrt_system_devices **out_xsysd,
                                   struct xrt_space_overseer **out_xso)
{
	struct t_builder *tb = (struct t_builder *)xb;

	return t_builder_roles_helper_open_system( //
	    xb,                                    //
	    config,                                //
	    xp,                                    //
	    broadcast,                             //
	    out_xsysd,                             //
	    out_xso,                               //
	    tb->open_system_static_roles);         //
}
