// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Cache of per-session priority group state.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_defines.h"

#include "oxr_defines.h"
#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Per-session accumulated priority state for one binding priority group,
 * more detailed comments about the purpose of this struct are in the
 * comments for the @ref oxr_priority_group_cache struct.
 */
struct oxr_priority_group_state
{
	XrPath path;
	uint32_t highest_priority;
};

/*!
 * Per-session cache of priority group states.
 *
 * Implements the OpenXR action-set priority rules described for
 * XrActionSetCreateInfo::priority: when several active action sets bind
 * actions to the same input source, higher priority wins and lower-priority
 * bindings are suppressed (treated as inactive, with no feedback). Priority
 * groups here are the runtime's notion of that shared input source, filled
 * while resolving suggested bindings and re-evaluated each action sync.
 *
 * States are allocated individually so pointers stored on action inputs remain
 * stable until the cache is cleared (for example when interaction profiles
 * change and bindings are rebuilt).
 */
struct oxr_priority_group_cache
{
	struct u_hashmap_int *states_by_path;
};

/*!
 * Initialize a priority group cache.
 *
 * @memberof oxr_priority_group_cache
 */
XRT_CHECK_RESULT XrResult
oxr_priority_group_cache_init(struct oxr_priority_group_cache *cache);

/*!
 * Finalize a priority group cache.
 *
 * @memberof oxr_priority_group_cache
 */
void
oxr_priority_group_cache_fini(struct oxr_priority_group_cache *cache);

/*!
 * Clear a priority group cache.
 *
 * @memberof oxr_priority_group_cache
 */
void
oxr_priority_group_cache_clear(struct oxr_priority_group_cache *cache);

/*!
 * Reset accumulated action-set priorities for every priority group.
 *
 * Called when syncing actions so the runtime can recompute, from the currently
 * active action sets, which priority wins on each shared control surface. The
 * group entries themselves stay alive so resolved inputs keep stable pointers.
 *
 * @memberof oxr_priority_group_cache
 */
void
oxr_priority_group_cache_zero_priorities(struct oxr_priority_group_cache *cache);

/*!
 * Look up or allocate shared priority state for a binding priority group path.
 *
 * Used while resolving suggested bindings: inputs that share a priority group
 * (typically the same physical control) all reference one accumulator so later
 * action sync can suppress lower-priority action sets on that surface.
 *
 * @memberof oxr_priority_group_cache
 */
XRT_CHECK_RESULT XrResult
oxr_priority_group_cache_get_or_create(struct oxr_priority_group_cache *cache,
                                       XrPath path,
                                       struct oxr_priority_group_state **out_state);


#ifdef __cplusplus
}
#endif
