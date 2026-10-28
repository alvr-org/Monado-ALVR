// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Store of generated binding point source paths.
 * @ingroup oxr_main
 */

#include "oxr_binding_point_store.h"

#include "util/u_hashmap.h"
#include "util/u_misc.h"

#include "../path/oxr_path_store.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*!
 * Maximum unique index for a binding point store entry, this just needs to be
 * large enough to fit all of the unique paths for the binding points covering
 * all interaction profiles. But also small enough to avoid collisions with the
 * interaction profile path and binding path.
 */
#define OXR_BINDING_POINT_STORE_MAX_UNIQUE_INDEX 10000

/*!
 * Pointer to the binding point for a given unique path.
 */
struct oxr_binding_point_store_entry
{
	XrPath unique_path;
	struct oxr_binding_point *binding_point;
};


/*
 *
 * Helper functions
 *
 */

static void
free_entry(void *item, void *priv)
{
	(void)priv;
	free(item);
}

static bool
make_pair_key(XrPath interaction_profile_path, XrPath binding_path, uint64_t *out_key)
{
	if (interaction_profile_path >= UINT32_MAX || binding_path >= UINT32_MAX) {
		assert(!"Unreachable");
		return false;
	}

	*out_key = (interaction_profile_path << 32u) | binding_path;
	return true;
}

static XrResult
make_unique_path(struct oxr_binding_point_store *store, struct oxr_path_store *path_store, XrPath *out_path)
{
	if (store->next_unique_index > OXR_BINDING_POINT_STORE_MAX_UNIQUE_INDEX) {
		assert(!"Unreachable");
		return XR_ERROR_RUNTIME_FAILURE;
	}

	char path[16] = {0};
	snprintf(path, sizeof(path), "/mnd/%04u", store->next_unique_index++);

	return oxr_path_store_get_or_create(path_store, path, strlen(path), out_path);
}


/*
 *
 * Exported functions
 *
 */

XrResult
oxr_binding_point_store_init(struct oxr_binding_point_store *store)
{
	U_ZERO(store);

	int h_ret = u_hashmap_int_create(&store->entries_by_pair);
	if (h_ret != 0) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	h_ret = u_hashmap_int_create(&store->points_by_unique_path);
	if (h_ret != 0) {
		u_hashmap_int_destroy(&store->entries_by_pair);
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return XR_SUCCESS;
}

void
oxr_binding_point_store_fini(struct oxr_binding_point_store *store)
{
	if (store->entries_by_pair != NULL) {
		u_hashmap_int_clear_and_call_for_each(store->entries_by_pair, free_entry, NULL);
	}
	u_hashmap_int_destroy(&store->entries_by_pair);
	u_hashmap_int_destroy(&store->points_by_unique_path);
}

XrResult
oxr_binding_point_store_add(struct oxr_binding_point_store *store,
                            XrPath interaction_profile_path,
                            XrPath binding_path,
                            struct oxr_path_store *path_store,
                            struct oxr_binding_point *binding_point,
                            XrPath *out_unique_path)
{
	uint64_t key = 0;
	if (!make_pair_key(interaction_profile_path, binding_path, &key)) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	void *existing = NULL;
	if (u_hashmap_int_find(store->entries_by_pair, key, &existing) == 0) {
		struct oxr_binding_point_store_entry *entry = (struct oxr_binding_point_store_entry *)existing;
		*out_unique_path = entry->unique_path;
		return XR_SUCCESS;
	}

	struct oxr_binding_point_store_entry *entry = U_TYPED_CALLOC(struct oxr_binding_point_store_entry);
	if (entry == NULL) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	XrResult ret = make_unique_path(store, path_store, &entry->unique_path);
	if (ret != XR_SUCCESS) {
		free(entry);
		return ret;
	}

	entry->binding_point = binding_point;

	int h_ret = u_hashmap_int_insert(store->entries_by_pair, key, entry);
	if (h_ret < 0) {
		free(entry);
		return XR_ERROR_RUNTIME_FAILURE;
	}

	h_ret = u_hashmap_int_insert(store->points_by_unique_path, entry->unique_path, binding_point);
	if (h_ret < 0) {
		u_hashmap_int_erase(store->entries_by_pair, key);
		free(entry);
		return XR_ERROR_RUNTIME_FAILURE;
	}

	*out_unique_path = entry->unique_path;
	return XR_SUCCESS;
}

bool
oxr_binding_point_store_get(const struct oxr_binding_point_store *store,
                            XrPath unique_path,
                            const struct oxr_binding_point **out_binding_point)
{
	void *existing = NULL;
	if (u_hashmap_int_find(store->points_by_unique_path, unique_path, &existing) != 0) {
		*out_binding_point = NULL;
		return false;
	}

	*out_binding_point = (const struct oxr_binding_point *)existing;
	return true;
}
