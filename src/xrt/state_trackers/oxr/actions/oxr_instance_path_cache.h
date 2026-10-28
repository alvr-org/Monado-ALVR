// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Holds per instance action cache.
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_forward_declarations.h"
#include "oxr_extension_support.h"
#include "oxr_subaction.h"
#include "oxr_generated_bindings.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * This holds cached paths for subaction paths.
 */
struct oxr_instance_path_cache
{
#define SUBACTION_PATH_MEMBER(X) XrPath X;
	OXR_FOR_EACH_SUBACTION_PATH(SUBACTION_PATH_MEMBER)
#undef SUBACTION_PATH_MEMBER

	XrPath template_paths[OXR_BINDINGS_PROFILE_TEMPLATE_COUNT];
};


/*
 *
 * Functions
 *
 */

/*!
 * Initialize the action cache for an instance.
 *
 * @param cache Action cache to initialize
 * @param store Path store to create paths in.
 * @public @memberof oxr_instance_path_cache
 */
XrResult
oxr_instance_path_cache_init(struct oxr_instance_path_cache *cache, struct oxr_path_store *store) XRT_NONNULL_ALL;

/*!
 * Finalize and cleanup the action cache for an instance.
 *
 * @param cache Action cache to finalize
 * @public @memberof oxr_instance_path_cache
 */
void
oxr_instance_path_cache_fini(struct oxr_instance_path_cache *cache) XRT_NONNULL_ALL;


#ifdef __cplusplus
}
#endif
