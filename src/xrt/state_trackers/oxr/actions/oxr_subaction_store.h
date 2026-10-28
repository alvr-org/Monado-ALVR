// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Store of registry-wide OpenXR subaction path payloads.
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
 * Runtime payload for one OpenXR subaction path.
 *
 * Subaction paths let applications use the same action handle on multiple
 * devices and query state per device (e.g. `/user/hand/left` vs
 * `/user/hand/right`). Binding points and interaction profiles share these
 * singleton payloads from the registry subaction store.
 *
 * @ingroup oxr_main
 * @memberof oxr_binding_subaction
 */
struct oxr_binding_subaction
{
	//! User-facing localized name for this subaction (e.g. "Left").
	const char *localized_name;

	//! Runtime XrPath for this subaction (e.g. `/user/hand/left`).
	XrPath path;
};

/*!
 * Registry-wide store of subaction payloads keyed by XrPath.
 *
 * Populated once when the binding registry is created with the known
 * top-level subaction paths. Binding points look up shared payloads here
 * instead of owning their own copies.
 *
 * @ingroup oxr_main
 * @memberof oxr_subaction_store
 */
struct oxr_subaction_store
{
	//! Maps subaction XrPath to @ref oxr_binding_subaction.
	struct u_hashmap_int *subactions_by_path;
};

/*!
 * Initialize an empty subaction store.
 *
 * @public @memberof oxr_subaction_store
 */
XRT_CHECK_RESULT XrResult
oxr_subaction_store_init(struct oxr_subaction_store *store);

/*!
 * Release all stored subactions and destroy the store.
 *
 * @public @memberof oxr_subaction_store
 */
void
oxr_subaction_store_fini(struct oxr_subaction_store *store);

/*!
 * Insert a subaction path if not already present.
 *
 * Duplicate paths are ignored and return success.
 *
 * @public @memberof oxr_subaction_store
 */
XRT_CHECK_RESULT XrResult
oxr_subaction_store_add(struct oxr_subaction_store *store, XrPath path, const char *localized_name);

/*!
 * Look up a subaction payload by path.
 *
 * @return true if @p path was found, with @p out_subaction set
 * @return false if @p path is unknown, with @p out_subaction set to NULL
 *
 * @public @memberof oxr_subaction_store
 */
bool
oxr_subaction_store_get(const struct oxr_subaction_store *store,
                        XrPath path,
                        const struct oxr_binding_subaction **out_subaction);


#ifdef __cplusplus
}
#endif
