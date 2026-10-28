// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Cache of per-session priority group state.
 * @ingroup oxr_main
 */

#include "oxr_priority_group_cache.h"

#include "util/u_hashmap.h"
#include "util/u_misc.h"

#include <stdlib.h>


/*
 *
 * Helpers.
 *
 */

static void
free_state(void *item, void *priv)
{
	(void)priv;
	free(item);
}

static void
zero_priority(uint64_t key, const void *value, void *priv_ctx)
{
	(void)key;
	(void)priv_ctx;

	// Safe to do a const cast, we own the memory.
	struct oxr_priority_group_state *state = (struct oxr_priority_group_state *)value;
	state->highest_priority = 0;
}


/*
 *
 * 'Exported' functions.
 *
 */

XrResult
oxr_priority_group_cache_init(struct oxr_priority_group_cache *cache)
{
	U_ZERO(cache);

	int h_ret = u_hashmap_int_create(&cache->states_by_path);
	if (h_ret != 0) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return XR_SUCCESS;
}

void
oxr_priority_group_cache_clear(struct oxr_priority_group_cache *cache)
{
	if (cache->states_by_path != NULL) {
		u_hashmap_int_clear_and_call_for_each(cache->states_by_path, free_state, NULL);
	}
}

void
oxr_priority_group_cache_fini(struct oxr_priority_group_cache *cache)
{
	oxr_priority_group_cache_clear(cache);
	u_hashmap_int_destroy(&cache->states_by_path);
}

void
oxr_priority_group_cache_zero_priorities(struct oxr_priority_group_cache *cache)
{
	u_hashmap_int_for_each(cache->states_by_path, zero_priority, NULL);
}

XrResult
oxr_priority_group_cache_get_or_create(struct oxr_priority_group_cache *cache,
                                       XrPath path,
                                       struct oxr_priority_group_state **out_state)
{
	void *existing = NULL;
	if (u_hashmap_int_find(cache->states_by_path, path, &existing) == 0) {
		*out_state = (struct oxr_priority_group_state *)existing;
		return XR_SUCCESS;
	}

	struct oxr_priority_group_state *state = U_TYPED_CALLOC(struct oxr_priority_group_state);
	if (state == NULL) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	state->path = path;

	int h_ret = u_hashmap_int_insert(cache->states_by_path, path, state);
	if (h_ret < 0) {
		free(state);
		return XR_ERROR_RUNTIME_FAILURE;
	}

	*out_state = state;
	return XR_SUCCESS;
}
