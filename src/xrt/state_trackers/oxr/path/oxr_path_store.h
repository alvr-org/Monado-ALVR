// Copyright 2019-2020, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Path store structure and functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_extension_support.h"


#ifdef __cplusplus
extern "C" {
#endif

struct u_hashset;
struct oxr_logger;
struct oxr_path;


/*!
 * Path store structure for managing path storage and lookup.
 *
 * @ingroup oxr_main
 */
struct oxr_path_store
{
	//! Path store, for looking up paths.
	struct u_hashset *path_store;
	//! Mapping from ID to path.
	struct oxr_path **path_array;
	//! Total length of path array.
	size_t path_array_length;
	//! Number of paths in the array (0 is always null).
	size_t path_num;
};

/*!
 * Initialize the path store.
 * @public @memberof oxr_path_store
 */
XrResult
oxr_path_store_init(struct oxr_path_store *store);

/*!
 * Destroy the path store and all paths.
 * @public @memberof oxr_path_store
 */
void
oxr_path_store_fini(struct oxr_path_store *store);

/*!
 * Check if a path ID is valid.
 * @public @memberof oxr_path_store
 */
bool
oxr_path_store_is_valid(const struct oxr_path_store *store, XrPath xr_path);

/*!
 * Get the path for the given string if it exists, or create it if it does not.
 * @public @memberof oxr_path_store
 */
XrResult
oxr_path_store_get_or_create(struct oxr_path_store *store, const char *str, size_t length, XrPath *out_path);

/*!
 * Only get the path for the given string if it exists.
 * @public @memberof oxr_path_store
 */
void
oxr_path_store_only_get(const struct oxr_path_store *store, const char *str, size_t length, XrPath *out_path);

/*!
 * Get a pointer and length of the internal string.
 *
 * The pointer has the same life time as the store. The length is the number
 * of valid characters, not including the null termination character (but an
 * extra null byte is always reserved at the end so can strings can be given
 * to functions expecting null terminated strings).
 *
 * @public @memberof oxr_path_store
 */
XrResult
oxr_path_store_get_string(const struct oxr_path_store *store, XrPath path, const char **out_str, size_t *out_length);

#ifdef __cplusplus
}
#endif
