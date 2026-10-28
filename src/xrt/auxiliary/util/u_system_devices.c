// Copyright 2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Misc helpers for xrt_system_devices implementations
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include "u_system_devices.h"
#include "u_system_devices_ni.h"



void
u_system_devices_populate_function_pointers(struct xrt_system_devices *xsysd,
                                            u_system_devices_get_roles_function_t get_roles_fn,
                                            u_system_devices_destroy_function_t destroy_fn)
{
	assert(xsysd != NULL);
	assert(get_roles_fn != NULL);
	assert(destroy_fn != NULL);

	xsysd->get_roles = get_roles_fn;
	xsysd->create_body_tracker = u_system_devices_ni_create_body_tracker;
	xsysd->create_hand_tracker = u_system_devices_ni_create_hand_tracker;
	xsysd->create_io_cache = u_system_devices_ni_create_io_cache;
	xsysd->feature_inc = u_system_devices_ni_feature_inc;
	xsysd->feature_dec = u_system_devices_ni_feature_dec;
	xsysd->destroy = destroy_fn;
}
