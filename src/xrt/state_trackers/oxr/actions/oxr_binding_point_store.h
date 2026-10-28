// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Store of generated binding point source paths.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_defines.h"

#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Store mapping interaction-profile/binding-path pairs to unique source paths.
 *
 * OpenXR returns source paths from xrEnumerateBoundSourcesForAction, and later
 * accepts those same source paths in xrGetInputSourceLocalizedName. The binding
 * registry may contain the same binding path under multiple interaction
 * profiles, so this store assigns a generated unique path to each
 * (interaction profile path, binding path) pair.
 *
 * entries_by_pair maps the packed pair to the generated source path and binding
 * point, while points_by_unique_path maps that generated source path back to
 * the binding point for fast localized-name lookup.
 */
struct oxr_binding_point_store
{
	struct u_hashmap_int *entries_by_pair;
	struct u_hashmap_int *points_by_unique_path;
	uint32_t next_unique_index;
};

XRT_CHECK_RESULT XrResult
oxr_binding_point_store_init(struct oxr_binding_point_store *store);

void
oxr_binding_point_store_fini(struct oxr_binding_point_store *store);

XRT_CHECK_RESULT XrResult
oxr_binding_point_store_add(struct oxr_binding_point_store *store,
                            XrPath interaction_profile_path,
                            XrPath binding_path,
                            struct oxr_path_store *path_store,
                            struct oxr_binding_point *binding_point,
                            XrPath *out_unique_path);

bool
oxr_binding_point_store_get(const struct oxr_binding_point_store *store,
                            XrPath unique_path,
                            const struct oxr_binding_point **out_binding_point);


#ifdef __cplusplus
}
#endif
