// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for @ref xrt_builder implementations.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_prober.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_tracking.h"

#include "util/u_debug.h"
#include "util/u_builders.h"
#include "util/u_system_helpers.h"
#include "util/u_space_overseer.h"


DEBUG_GET_ONCE_FLOAT_OPTION(tracking_origin_offset_x, "XRT_TRACKING_ORIGIN_OFFSET_X", 0.0f)
DEBUG_GET_ONCE_FLOAT_OPTION(tracking_origin_offset_y, "XRT_TRACKING_ORIGIN_OFFSET_Y", 0.0f)
DEBUG_GET_ONCE_FLOAT_OPTION(tracking_origin_offset_z, "XRT_TRACKING_ORIGIN_OFFSET_Z", 0.0f)


/*
 *
 * Helper functions.
 *
 */

static void
apply_offset(struct xrt_vec3 *position, struct xrt_vec3 *offset)
{
	position->x += offset->x;
	position->y += offset->y;
	position->z += offset->z;
}


/*
 *
 * 'Exported' function.
 *
 */

struct xrt_prober_device *
u_builder_find_prober_device(struct xrt_prober_device *const *xpdevs,
                             size_t xpdev_count,
                             uint16_t vendor_id,
                             uint16_t product_id,
                             enum xrt_bus_type bus_type)
{
	for (size_t i = 0; i < xpdev_count; i++) {
		struct xrt_prober_device *xpdev = xpdevs[i];
		if (xpdev->product_id != product_id || //
		    xpdev->vendor_id != vendor_id ||   //
		    xpdev->bus != bus_type) {
			continue;
		}

		return xpdev;
	}

	return NULL;
}

void
u_builder_search(struct xrt_prober *xp,
                 struct xrt_prober_device *const *xpdevs,
                 size_t xpdev_count,
                 const struct u_builder_search_filter *filters,
                 size_t filter_count,
                 struct u_builder_search_results *results)
{
	for (size_t i = 0; i < xpdev_count; i++) {
		struct xrt_prober_device *xpdev = xpdevs[i];
		bool match = false;

		for (size_t k = 0; k < filter_count; k++) {
			struct u_builder_search_filter f = filters[k];

			if (xpdev->product_id != f.product_id || //
			    xpdev->vendor_id != f.vendor_id ||   //
			    xpdev->bus != f.bus_type) {          //
				continue;
			}

			match = true;
			break;
		}

		if (!match) {
			continue;
		}

		results->xpdevs[results->xpdev_count++] = xpdev;

		// Exit if full.
		if (results->xpdev_count >= ARRAY_SIZE(results->xpdevs)) {
			return;
		}
	}
}

void
u_builder_setup_tracking_origins(struct xrt_device *head,
                                 struct xrt_device *left,
                                 struct xrt_device *right,
                                 struct xrt_vec3 *global_tracking_origin_offset)
{
	struct xrt_tracking_origin *head_origin = head ? head->tracking_origin : NULL;
	struct xrt_tracking_origin *left_origin = left ? left->tracking_origin : NULL;
	struct xrt_tracking_origin *right_origin = right ? right->tracking_origin : NULL;

	if (left_origin != NULL && left_origin->type == XRT_TRACKING_TYPE_NONE) {
		left_origin->offset.position.x = -0.2f;
		left_origin->offset.position.y = 1.3f;
		left_origin->offset.position.z = -0.5f;
	}

	if (right_origin != NULL && right_origin->type == XRT_TRACKING_TYPE_NONE) {
		right_origin->offset.position.x = 0.2f;
		right_origin->offset.position.y = 1.3f;
		right_origin->offset.position.z = -0.5f;
	}

	// Head comes last, because left and right may share tracking origin.
	if (head_origin != NULL && head_origin->type == XRT_TRACKING_TYPE_NONE) {
		// "nominal height" 1.6m
		head_origin->offset.position.x = 0.0f;
		head_origin->offset.position.y = 1.6f;
		head_origin->offset.position.z = 0.0f;
	}

	if (head_origin) {
		apply_offset(&head_origin->offset.position, global_tracking_origin_offset);
	}
	if (left_origin && left_origin != head_origin) {
		apply_offset(&left->tracking_origin->offset.position, global_tracking_origin_offset);
	}
	if (right_origin && right_origin != head_origin && right_origin != left_origin) {
		apply_offset(&right->tracking_origin->offset.position, global_tracking_origin_offset);
	}
}

void
u_builder_create_space_overseer_legacy(struct xrt_session_event_sink *broadcast,
                                       struct xrt_device *head,
                                       struct xrt_device *left,
                                       struct xrt_device *right,
                                       struct xrt_device **xdevs,
                                       uint32_t xdev_count,
                                       bool root_is_unbounded,
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
	    left,                            //
	    right,                           //
	    &global_tracking_origin_offset); //


	/*
	 * Space overseer.
	 */

	struct u_space_overseer *uso = u_space_overseer_create(broadcast);

	struct xrt_pose T_stage_local = XRT_POSE_IDENTITY;
	T_stage_local.position.y = 1.6;

	u_space_overseer_legacy_setup( //
	    uso,                       // uso
	    xdevs,                     // xdevs
	    xdev_count,                // xdev_count
	    head,                      // head
	    &T_stage_local,            // local_offset
	    root_is_unbounded);        // root_is_unbounded

	*out_xso = (struct xrt_space_overseer *)uso;
}

xrt_result_t
u_builder_roles_helper_open_system(struct xrt_builder *xb,
                                   cJSON *config,
                                   struct xrt_prober *xp,
                                   struct xrt_session_event_sink *broadcast,
                                   struct xrt_system_devices **out_xsysd,
                                   struct xrt_space_overseer **out_xso,
                                   u_builder_open_system_fn fn)
{
	struct u_builder_roles_helper ubrh = XRT_STRUCT_INIT;
	xrt_result_t xret;

	// Use the static system devices helper, no dynamic roles.
	struct u_system_devices_static *usysds = u_system_devices_static_allocate();
	struct xrt_tracking_origin *origin = &usysds->base.origin;
	struct xrt_system_devices *xsysd = &usysds->base.base;
	struct xrt_frame_context *xfctx = &usysds->base.xfctx;

	xret = fn(  //
	    xb,     // xb
	    config, // config
	    xp,     // xp
	    origin, // origin
	    xsysd,  // xsysd
	    xfctx,  // xfctx
	    &ubrh); // ubrh
	if (xret != XRT_SUCCESS) {
		xrt_system_devices_destroy(&xsysd);
		return xret;
	}

	/*
	 * Assign to role(s).
	 */

	xsysd->static_roles.head = ubrh.head;
	xsysd->static_roles.hand_tracking.left = ubrh.hand_tracking.left;
	xsysd->static_roles.hand_tracking.right = ubrh.hand_tracking.right;

	u_system_devices_static_finalize( //
	    usysds,                       // usysds
	    ubrh.left,                    // left
	    ubrh.right);                  // right


	/*
	 * Create the space overseer.
	 */

	*out_xsysd = xsysd;
	u_builder_create_space_overseer_legacy( //
	    broadcast,                          // broadcast
	    ubrh.head,                          // head
	    ubrh.left,                          // left
	    ubrh.right,                         // right
	    xsysd->xdevs,                       // xdevs
	    xsysd->xdev_count,                  // xdev_count
	    false,                              // root_is_unbounded
	    out_xso);                           // out_xso

	return XRT_SUCCESS;
}

xrt_result_t
u_builder_open_system_static_roles(struct xrt_builder *xb,
                                   cJSON *config,
                                   struct xrt_prober *xp,
                                   struct xrt_session_event_sink *broadcast,
                                   struct xrt_system_devices **out_xsysd,
                                   struct xrt_space_overseer **out_xso)
{
	struct u_builder *ub = (struct u_builder *)xb;

	return u_builder_roles_helper_open_system( //
	    xb,                                    //
	    config,                                //
	    xp,                                    //
	    broadcast,                             //
	    out_xsysd,                             //
	    out_xso,                               //
	    ub->open_system_static_roles);         //
}
