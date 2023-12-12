// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for system objects like @ref xrt_system_devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "util/u_misc.h"
#include "util/u_device.h"
#include "util/u_logging.h"
#include "util/u_system_helpers.h"

#include <assert.h>
#include <limits.h>
#include <inttypes.h>


/*
 *
 * Helper functions.
 *
 */

static int32_t
get_index_for_device(const struct xrt_system_devices *xsysd, const struct xrt_device *xdev)
{
	assert(xsysd->xdev_count <= ARRAY_SIZE(xsysd->xdevs));
	assert(xsysd->xdev_count < INT_MAX);

	if (xdev == NULL) {
		return -1;
	}

	for (int32_t i = 0; i < (int32_t)xsysd->xdev_count; i++) {
		if (xsysd->xdevs[i] == xdev) {
			return i;
		}
	}

	return -1;
}



/*
 *
 * Internal functions.
 *
 */

static void
destroy(struct xrt_system_devices *xsysd)
{
	u_system_devices_close(xsysd);
	free(xsysd);
}

static xrt_result_t
get_roles(struct xrt_system_devices *xsysd, struct xrt_system_roles *out_roles)
{
	struct u_system_devices_static *usysds = u_system_devices_static(xsysd);

	assert(usysds->cached.generation_id == 1);

	*out_roles = usysds->cached;

	return XRT_SUCCESS;
}


/*
 *
 * 'Exported' functions.
 *
 */

struct u_system_devices *
u_system_devices_allocate(void)
{
	struct u_system_devices *usysd = U_TYPED_CALLOC(struct u_system_devices);
	usysd->base.destroy = destroy;

	return usysd;
}

void
u_system_devices_close(struct xrt_system_devices *xsysd)
{
	struct u_system_devices *usysd = u_system_devices(xsysd);

	for (uint32_t i = 0; i < ARRAY_SIZE(usysd->base.xdevs); i++) {
		xrt_device_destroy(&usysd->base.xdevs[i]);
	}

	xrt_frame_context_destroy_nodes(&usysd->xfctx);
}



struct u_system_devices_static *
u_system_devices_static_allocate(void)
{
	struct u_system_devices_static *usysds = U_TYPED_CALLOC(struct u_system_devices_static);
	usysds->base.base.destroy = destroy;
	usysds->base.base.get_roles = get_roles;

	return usysds;
}

void
u_system_devices_static_finalize(struct u_system_devices_static *usysds,
                                 struct xrt_device *left,
                                 struct xrt_device *right)
{
	struct xrt_system_devices *xsysd = &usysds->base.base;
	int32_t left_index = get_index_for_device(xsysd, left);
	int32_t right_index = get_index_for_device(xsysd, right);

	U_LOG_D(
	    "Devices:"
	    "\n\t%i: %p"
	    "\n\t%i: %p",
	    left_index, (void *)left,    //
	    right_index, (void *)right); //

	// Sanity checking.
	assert(usysds->cached.generation_id == 0);
	assert(left_index < 0 || left != NULL);
	assert(left_index >= 0 || left == NULL);
	assert(right_index < 0 || right != NULL);
	assert(right_index >= 0 || right == NULL);

	// Completely clear the struct.
	usysds->cached = (struct xrt_system_roles)XRT_SYSTEM_ROLES_INIT;
	usysds->cached.generation_id = 1;
	usysds->cached.left = left_index;
	usysds->cached.right = right_index;
}


/*
 *
 * Generic system devices helper.
 *
 */

xrt_result_t
u_system_devices_create_from_prober(struct xrt_instance *xinst,
                                    struct xrt_session_event_sink *broadcast,
                                    struct xrt_system_devices **out_xsysd,
                                    struct xrt_space_overseer **out_xso)
{
	xrt_result_t xret;

	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);


	/*
	 * Create the devices.
	 */

	struct xrt_prober *xp = NULL;
	xret = xrt_instance_get_prober(xinst, &xp);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	xret = xrt_prober_probe(xp);
	if (xret < 0) {
		return xret;
	}

	return xrt_prober_create_system(xp, broadcast, out_xsysd, out_xso);
}

struct xrt_device *
u_system_devices_get_ht_device(struct xrt_system_devices *xsysd, enum xrt_input_name name)
{
	for (uint32_t i = 0; i < xsysd->xdev_count; i++) {
		struct xrt_device *xdev = xsysd->xdevs[i];

		if (xdev == NULL || !xdev->hand_tracking_supported) {
			continue;
		}

		for (uint32_t j = 0; j < xdev->input_count; j++) {
			struct xrt_input *input = &xdev->inputs[j];

			if (input->name == name) {
				return xdev;
			}
		}
	}

	return NULL;
}
