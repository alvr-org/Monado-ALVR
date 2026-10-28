// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Helpers for runtime interaction profile binding data.
 * @ingroup oxr_main
 */

#include "util/u_hashmap.h"
#include "oxr_binding_profile.h"


bool
oxr_binding_profile_find_binding_point(const struct oxr_binding_profile *profile,
                                       XrPath path,
                                       const struct oxr_binding_point **out_binding_point)
{
	void *ptr = NULL;
	if (profile == NULL || u_hashmap_int_find(profile->binding_points_by_path, path, &ptr) != 0) {
		*out_binding_point = NULL;
		return false;
	}

	*out_binding_point = (struct oxr_binding_point *)ptr;
	return true;
}
