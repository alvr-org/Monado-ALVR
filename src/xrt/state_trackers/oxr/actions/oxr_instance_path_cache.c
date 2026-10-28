// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Holds per instance action cache.
 * @ingroup oxr_main
 */

#include "oxr_instance_path_cache.h"
#include "../path/oxr_path_store.h"

#include <string.h>


/*
 *
 * 'Exported' functions.
 *
 */

XrResult
oxr_instance_path_cache_init(struct oxr_instance_path_cache *cache, struct oxr_path_store *store)
{
	// Cache certain often looked up paths.
#define CACHE_SUBACTION_PATHS(NAME, NAME_CAPS, PATH, LOCALIZED_NAME)                                                   \
	do {                                                                                                           \
		XrResult ret = oxr_path_store_get_or_create(store, PATH, strlen(PATH), &cache->NAME);                  \
		if (ret != XR_SUCCESS) {                                                                               \
			return ret;                                                                                    \
		}                                                                                                      \
	} while (false);

	OXR_FOR_EACH_SUBACTION_PATH_DETAILED(CACHE_SUBACTION_PATHS)
#undef CACHE_SUBACTION_PATHS

	static_assert(OXR_BINDINGS_PROFILE_TEMPLATE_COUNT == ARRAY_SIZE(oxr_profile_templates), "Must match");
	static_assert(OXR_BINDINGS_PROFILE_TEMPLATE_COUNT == ARRAY_SIZE(cache->template_paths), "Must match");

	for (size_t i = 0; i < OXR_BINDINGS_PROFILE_TEMPLATE_COUNT; i++) {
		const char *str = oxr_profile_templates[i].path;
		size_t length = strlen(str);
		XrResult ret = oxr_path_store_get_or_create(store, str, length, &cache->template_paths[i]);
		if (ret != XR_SUCCESS) {
			return ret;
		}
	}

	return XR_SUCCESS;
}

void
oxr_instance_path_cache_fini(struct oxr_instance_path_cache *cache)
{
	// Nothing to clean up - paths are managed by the instance's path system
	(void)cache;
}
