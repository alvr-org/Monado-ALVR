// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Store of registry-wide binding subaction payloads.
 * @ingroup oxr_main
 */

#include "oxr_subaction_store.h"

#include "util/u_hashmap.h"
#include "util/u_misc.h"

#include <stdlib.h>


/*
 *
 * Helpers
 *
 */

static void
free_subaction(void *item, void *priv)
{
	(void)priv;
	free(item);
}


/*
 *
 * Exported functions
 *
 */

XrResult
oxr_subaction_store_init(struct oxr_subaction_store *store)
{
	U_ZERO(store);

	int h_ret = u_hashmap_int_create(&store->subactions_by_path);
	if (h_ret != 0) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return XR_SUCCESS;
}

void
oxr_subaction_store_fini(struct oxr_subaction_store *store)
{
	if (store->subactions_by_path != NULL) {
		u_hashmap_int_clear_and_call_for_each(store->subactions_by_path, free_subaction, NULL);
	}
	u_hashmap_int_destroy(&store->subactions_by_path);
}

XrResult
oxr_subaction_store_add(struct oxr_subaction_store *store, XrPath path, const char *localized_name)
{
	void *existing = NULL;
	if (u_hashmap_int_find(store->subactions_by_path, path, &existing) == 0) {
		return XR_SUCCESS;
	}

	struct oxr_binding_subaction *subaction = U_TYPED_CALLOC(struct oxr_binding_subaction);
	if (subaction == NULL) {
		return XR_ERROR_RUNTIME_FAILURE;
	}
	U_ZERO(subaction);
	subaction->path = path;
	subaction->localized_name = localized_name;

	int h_ret = u_hashmap_int_insert(store->subactions_by_path, path, subaction);
	if (h_ret < 0) {
		free(subaction);
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return XR_SUCCESS;
}

bool
oxr_subaction_store_get(const struct oxr_subaction_store *store,
                        XrPath path,
                        const struct oxr_binding_subaction **out_subaction)
{
	void *existing = NULL;
	if (u_hashmap_int_find(store->subactions_by_path, path, &existing) != 0) {
		*out_subaction = NULL;
		return false;
	}

	*out_subaction = (const struct oxr_binding_subaction *)existing;
	return true;
}
