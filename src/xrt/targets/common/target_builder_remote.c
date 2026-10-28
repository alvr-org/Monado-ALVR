// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Remote driver builder.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_tracking.h"

#include "util/u_misc.h"
#include "util/u_config_json.h"
#include "b_space_overseer.h"

#include "target_builder_interface.h"

#include "remote/r_interface.h"

#include <assert.h>


#ifndef XRT_BUILD_DRIVER_REMOTE
#error "Must only be built with XRT_BUILD_DRIVER_REMOTE set"
#endif


/*
 *
 * Helper functions.
 *
 */

/*!
 * Wire the remote @ref xrt_system_devices into the space graph (offset, stage,
 * local, view, device links). Matches the former setup in @c r_hub.c.
 */
static void
remote_builder_setup_space_overseer(struct b_space_overseer *bso, struct xrt_system_devices *xsysd)
{
	struct xrt_space_overseer *xso = (struct xrt_space_overseer *)bso; // Convenience
	struct xrt_device *head = xsysd->static_roles.head;
	struct xrt_space *root = xso->semantic.root; // Convenience

	assert(head != NULL);
	assert(head->tracking_origin != NULL);

	struct xrt_space *offset = NULL;
	b_space_overseer_create_offset_space(bso, root, &head->tracking_origin->initial_offset, &offset);

	for (uint32_t i = 0; i < xsysd->static_xdev_count; i++) {
		b_space_overseer_link_space_to_device(bso, offset, xsysd->static_xdevs[i]);
	}

	// Unreference now
	xrt_space_reference(&offset, NULL);

	// Set root as stage space.
	xrt_space_reference(&xso->semantic.stage, root);

	// Local 1.6 meters up.
	struct xrt_pose local_offset = {XRT_QUAT_IDENTITY, {0.0f, 1.6f, 0.0f}};
	b_space_overseer_create_offset_space(bso, root, &local_offset, &xso->semantic.local);

	// Local floor at the same place as local except at floor height.
	struct xrt_pose local_floor_offset = local_offset;
	local_floor_offset.position.y = 0.0f;
	b_space_overseer_create_offset_space(bso, root, &local_floor_offset, &xso->semantic.local_floor);

	// Make view space be the head pose.
	b_space_overseer_create_pose_space(bso, head, XRT_INPUT_GENERIC_HEAD_POSE, &xso->semantic.view);
}

static bool
get_settings(cJSON *json, struct u_config_remote_settings *out_settings)
{
	struct u_config_json config_json = {0};
	u_config_json_open_or_create_main_file(&config_json);

	bool bret = u_config_json_get_remote_settings(&config_json, out_settings);

	u_config_json_close(&config_json);

	return bret;
}

static const char *driver_list[] = {
    "remote",
};


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
remote_estimate_system(struct xrt_builder *xb,
                       cJSON *config,
                       struct xrt_prober *xp,
                       struct xrt_builder_estimate *estimate)
{
	estimate->certain.head = true;
	estimate->certain.left = true;
	estimate->certain.right = true;
	estimate->priority = -50;

	return XRT_SUCCESS;
}

static xrt_result_t
remote_open_system(struct xrt_builder *xb,
                   cJSON *config,
                   struct xrt_prober *xp,
                   struct xrt_session_event_sink *broadcast,
                   struct xrt_system_devices **out_xsysd,
                   struct xrt_space_overseer **out_xso)
{
	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);
	assert(out_xso != NULL);
	assert(*out_xso == NULL);


	struct u_config_remote_settings settings = {
	    .port = 4242,
	    .view_count = 2,
	    .w_pixels = 1920,
	    .h_pixels = 1080,
	    .w_meters = 0.13f,
	    .h_meters = 0.07f,
	};
	get_settings(config, &settings); // Leaves the defaults on failure.

	struct b_space_overseer *uso = b_space_overseer_create(broadcast);
	struct xrt_space_overseer *xso = (struct xrt_space_overseer *)uso;

	xrt_result_t xret = r_create_devices(settings.port, settings.view_count, settings.w_pixels, settings.h_pixels,
	                                     settings.w_meters, settings.h_meters, out_xsysd);
	if (xret != XRT_SUCCESS) {
		xrt_space_overseer_destroy(&xso);
		return xret;
	}

	remote_builder_setup_space_overseer(uso, *out_xsysd);

	*out_xso = xso;
	return XRT_SUCCESS;
}

static void
remote_destroy(struct xrt_builder *xb)
{
	free(xb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
t_builder_remote_create(void)
{
	struct xrt_builder *xb = U_TYPED_CALLOC(struct xrt_builder);
	xb->estimate_system = remote_estimate_system;
	xb->open_system = remote_open_system;
	xb->destroy = remote_destroy;
	xb->identifier = "remote";
	xb->name = "Remote simulation devices builder";
	xb->driver_identifiers = driver_list;
	xb->driver_identifier_count = ARRAY_SIZE(driver_list);
	xb->exclude_from_automatic_discovery = true;

	return xb;
}
