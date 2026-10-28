// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds subaction related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @ingroup oxr_main
 */

#include "util/u_misc.h"

#include "oxr_subaction.h"
#include "oxr_instance_path_cache.h"


bool
oxr_classify_subaction_paths(const struct oxr_instance_path_cache *cache,
                             uint32_t subaction_path_count,
                             const XrPath *subaction_paths,
                             struct oxr_subaction_paths *out_subaction_paths)
{
	// Reset the subaction_paths completely.
	U_ZERO(out_subaction_paths);

	if (subaction_path_count == 0) {
		out_subaction_paths->any = true;
		return true;
	}

	for (uint32_t i = 0; i < subaction_path_count; i++) {
		XrPath path = subaction_paths[i];

#define IDENTIFY_PATH(X)                                                                                               \
	else if (path == cache->X)                                                                                     \
	{                                                                                                              \
		out_subaction_paths->X = true;                                                                         \
	}

		if (path == XR_NULL_PATH) {
			out_subaction_paths->any = true;
		}
		OXR_FOR_EACH_VALID_SUBACTION_PATH(IDENTIFY_PATH) else
		{
			// Calling functions must error/warn.
			return false;
		}

#undef IDENTIFY_PATH
	}

	return true;
}
