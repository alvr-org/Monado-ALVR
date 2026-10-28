// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Runtime OpenXR-level rebinding profile data.
 * @ingroup oxr_main
 */

#include "oxr_rebinding_profile.h"


bool
oxr_rebinding_profile_find_target(const struct oxr_rebinding_profile *profile,
                                  XrPath source_path,
                                  XrPath *out_target_path)
{
	if (profile == NULL) {
		*out_target_path = XR_NULL_PATH;
		return false;
	}

	for (size_t i = 0; i < profile->binding_count; i++) {
		if (profile->bindings[i].source_path == source_path) {
			*out_target_path = profile->bindings[i].target_path;
			return true;
		}
	}

	*out_target_path = XR_NULL_PATH;
	return false;
}
