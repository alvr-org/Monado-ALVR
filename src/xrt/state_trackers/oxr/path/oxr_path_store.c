// Copyright 2019-2020, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds path related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */

#include "math/m_api.h"
#include "util/u_misc.h"
#include "util/u_hashset.h"

#include "oxr_path_store.h"
#include "oxr_defines.h"


/*
 *
 * Internal structure.
 *
 */

/*!
 * Internal representation of a path, item follows this struct in memory and
 * that in turn is followed by the string.
 *
 * @ingroup oxr_main
 */
struct oxr_path
{
	uint64_t debug;
	XrPath id;
};


/*
 *
 * Converter helpers.
 *
 */

static inline XrPath
to_xr_path(struct oxr_path *path)
{
	return path->id;
}

static inline struct u_hashset_item *
get_item(struct oxr_path *path)
{
	return (struct u_hashset_item *)&path[1];
}

static inline struct oxr_path *
from_item(struct u_hashset_item *item)
{
	return &((struct oxr_path *)item)[-1];
}


/*
 *
 * General helpers.
 *
 */

static struct oxr_path *
get_or_null(const struct oxr_path_store *store, XrPath xr_path)
{
	if (xr_path >= store->path_array_length) {
		return NULL;
	}

	return store->path_array[xr_path];
}

static XrResult
ensure_array_length(struct oxr_path_store *store, XrPath *out_id)
{
	size_t num = store->path_num + 1;

	if (num < store->path_array_length) {
		*out_id = store->path_num++;
		return XR_SUCCESS;
	}

	size_t new_size = store->path_array_length;
	while (new_size < num) {
		new_size += 64;
	}

	U_ARRAY_REALLOC_OR_FREE(store->path_array, struct oxr_path *, new_size);
	store->path_array_length = new_size;

	*out_id = store->path_num++;

	return XR_SUCCESS;
}

static XrResult
insert_path(struct oxr_path_store *store, struct oxr_path *path, XrPath *out_id)
{
	struct u_hashset_item *item = get_item(path);
	int ret;

	// Insert into hashset.
	ret = u_hashset_insert_item(store->path_store, item);
	if (ret) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	// Ensure array length and assign ID.
	XrResult xr_ret = ensure_array_length(store, out_id);
	if (xr_ret != XR_SUCCESS) {
		u_hashset_erase_item(store->path_store, item);
		return xr_ret;
	}

	path->id = *out_id;
	store->path_array[*out_id] = path;

	return XR_SUCCESS;
}

static XrResult
allocate_and_insert(struct oxr_path_store *store, const char *str, size_t length, XrPath *out_id)
{
	struct u_hashset_item *item = NULL;
	struct oxr_path *path = NULL;
	size_t size = 0;
	XrResult ret;

	size += sizeof(struct oxr_path);       // Main path object.
	size += sizeof(struct u_hashset_item); // Embedded hashset item.
	size += length;                        // String.
	size += 1;                             // Null terminate it.

	// Now allocate and setup the path.
	path = U_CALLOC_WITH_CAST(struct oxr_path, size);
	if (path == NULL) {
		return XR_ERROR_RUNTIME_FAILURE;
	}
	path->debug = OXR_XR_DEBUG_PATH;

	// Setup the item.
	item = get_item(path);
	item->hash = math_hash_string(str, length);
	item->length = length;

	// Yes a const cast! D:
	char *store_str = (char *)item->c_str;
	for (size_t i = 0; i < length; i++) {
		store_str[i] = str[i];
	}
	store_str[length] = '\0';

	// Insert into store, and get the ID to set on the path.
	ret = insert_path(store, path, &path->id);
	if (ret != XR_SUCCESS) {
		free(path);
		return ret;
	}

	// Finally we are done and can return the ID.
	*out_id = to_xr_path(path);

	return XR_SUCCESS;
}

static void
destroy_callback(struct u_hashset_item *item, void *priv)
{
	struct oxr_path *path = from_item(item);

	free(path);
}


/*
 *
 * 'Exported' functions.
 *
 */

XrResult
oxr_path_store_init(struct oxr_path_store *store)
{
	int h_ret = u_hashset_create(&store->path_store);
	if (h_ret != 0) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	size_t new_size = 64;
	U_ARRAY_REALLOC_OR_FREE(store->path_array, struct oxr_path *, new_size);
	store->path_array_length = new_size;
	store->path_num = 1; // Reserve space for XR_NULL_PATH

	return XR_SUCCESS;
}

void
oxr_path_store_fini(struct oxr_path_store *store)
{
	if (store->path_array != NULL) {
		free(store->path_array);
	}

	store->path_array = NULL;
	store->path_num = 0;
	store->path_array_length = 0;

	if (store->path_store == NULL) {
		return;
	}

	u_hashset_clear_and_call_for_each(store->path_store, destroy_callback, store);
	u_hashset_destroy(&store->path_store);
}

bool
oxr_path_store_is_valid(const struct oxr_path_store *store, XrPath xr_path)
{
	return get_or_null(store, xr_path) != NULL;
}

XrResult
oxr_path_store_get_or_create(struct oxr_path_store *store, const char *str, size_t length, XrPath *out_path)
{
	struct u_hashset_item *item;
	XrResult ret;
	int h_ret;

	// Look it up the store path store.
	h_ret = u_hashset_find_str(store->path_store, str, length, &item);
	if (h_ret == 0) {
		*out_path = to_xr_path(from_item(item));
		return XR_SUCCESS;
	}

	// Create the path since it was not found.
	ret = allocate_and_insert(store, str, length, out_path);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	return XR_SUCCESS;
}

void
oxr_path_store_only_get(const struct oxr_path_store *store, const char *str, size_t length, XrPath *out_path)
{
	struct u_hashset_item *item;
	int h_ret;

	// Look it up the store path store.
	h_ret = u_hashset_find_str(store->path_store, str, length, &item);
	if (h_ret == 0) {
		*out_path = to_xr_path(from_item(item));
		return;
	}

	*out_path = XR_NULL_PATH;
}

XrResult
oxr_path_store_get_string(const struct oxr_path_store *store, XrPath xr_path, const char **out_str, size_t *out_length)
{
	struct oxr_path *path = get_or_null(store, xr_path);
	if (path == NULL) {
		return XR_ERROR_PATH_INVALID;
	}

	*out_str = get_item(path)->c_str;
	*out_length = get_item(path)->length;

	return XR_SUCCESS;
}
