// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Runtime registry built from generated OpenXR binding templates.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_defines.h"

#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"
#include "oxr_binding_point.h"
#include "oxr_binding_point_store.h"
#include "oxr_refcounted.h"
#include "oxr_subaction_store.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Complete runtime binding registry for all action contexts of an XrInstance.
 *
 * The registry owns all generated interaction profiles, all binding points, and
 * all rebinding profiles. It deliberately includes generated data even when an
 * interaction profile or path is not currently enabled for API suggestions. It
 * is constant through the lifetime of the XrInstance, and is shared between all
 * sessions and action contexts, and their actions.
 */
struct oxr_binding_registry
{
	struct oxr_refcounted base;

	//! All generated interaction profiles.
	struct oxr_binding_profile *profiles;
	size_t profile_count;

	//! Maps interaction profile XrPath to @ref profiles entries.
	struct u_hashmap_int *profiles_by_path;

	struct oxr_binding_point_store binding_points;

	/*!
	 * Maps subaction paths to subaction payloads, used to singleton
	 * subactions payloads so they are shared between all the profiles.
	 */
	struct oxr_subaction_store subactions;

	//! All generated OpenXR-level rebinding profiles.
	struct oxr_rebinding_profile *rebindings;
	size_t rebinding_count;
};

/*!
 * Create a complete runtime registry from generated templates.
 *
 * The registry does not filter by extension support or OpenXR version.
 * Suggestion-time validation rejects paths that are unavailable to the app.
 *
 * @memberof oxr_binding_registry
 */
XRT_CHECK_RESULT XrResult
oxr_binding_registry_create(struct oxr_logger *log,
                            struct oxr_path_store *store,
                            struct oxr_binding_registry **out_registry);

/*!
 * Add a reference to a registry.
 *
 * @memberof oxr_binding_registry
 */
void
oxr_binding_registry_ref(struct oxr_binding_registry *registry);

/*!
 * Drop a reference to a registry and destroy it when the count reaches zero.
 *
 * @memberof oxr_binding_registry
 */
void
oxr_binding_registry_unref(struct oxr_binding_registry *registry);

/*!
 * Find a binding profile by interaction profile path.
 *
 * @memberof oxr_binding_registry
 */
bool
oxr_binding_registry_find_profile(const struct oxr_binding_registry *registry,
                                  XrPath path,
                                  const struct oxr_binding_profile **out_profile);

/*!
 * Resolve the generated unique source path returned by
 * xrEnumerateBoundSourcesForAction.
 *
 * @memberof oxr_binding_registry
 */
bool
oxr_binding_registry_get_for_source_path(const struct oxr_binding_registry *registry,
                                         XrPath source_path,
                                         const struct oxr_binding_point **out_binding_point);

/*!
 * Find a rebinding profile from a source profile to a target device.
 *
 * @memberof oxr_binding_registry
 */
bool
oxr_binding_registry_find_rebinding_from_source_for_device(const struct oxr_binding_registry *registry,
                                                           XrPath source_profile_path,
                                                           enum xrt_device_name target_device_name,
                                                           const struct oxr_rebinding_profile **out_rebinding);


#ifdef __cplusplus
}
#endif
