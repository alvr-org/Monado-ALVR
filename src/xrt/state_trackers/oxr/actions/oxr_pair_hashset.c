// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Pair of name/localized hashets with atomic erase helper.
 * @ingroup oxr_main
 */

#include "util/u_hashset.h"

#include "oxr_pair_hashset.h"
#include "../oxr_logger.h"

#include <stdlib.h>


XrResult
oxr_pair_hashset_init(struct oxr_logger *log, struct oxr_pair_hashset *pair)
{
	int h_ret;
	int m_ret;

	pair->name_store = NULL;
	pair->loc_store = NULL;

	h_ret = u_hashset_create(&pair->name_store);
	if (h_ret != 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to create name_store hashset");
	}

	h_ret = u_hashset_create(&pair->loc_store);
	if (h_ret != 0) {
		u_hashset_destroy(&pair->name_store);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to create loc_store hashset");
	}

	m_ret = os_mutex_init(&pair->mutex);
	if (m_ret != 0) {
		u_hashset_destroy(&pair->loc_store);
		u_hashset_destroy(&pair->name_store);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to create pair hashset mutex");
	}

	return XR_SUCCESS;
}

void
oxr_pair_hashset_fini(struct oxr_pair_hashset *pair)
{
	os_mutex_destroy(&pair->mutex);
	u_hashset_destroy(&pair->name_store);
	u_hashset_destroy(&pair->loc_store);
}

int
oxr_pair_hashset_insert_str_c(struct oxr_pair_hashset *pair,
                              const char *name_cstr,
                              const char *loc_cstr,
                              struct u_hashset_item **out_name_item,
                              struct u_hashset_item **out_loc_item)
{
	int h_ret;

	os_mutex_lock(&pair->mutex);

	h_ret = u_hashset_create_and_insert_str_c(pair->name_store, name_cstr, out_name_item);
	if (h_ret != 0) {
		os_mutex_unlock(&pair->mutex);
		return h_ret;
	}

	h_ret = u_hashset_create_and_insert_str_c(pair->loc_store, loc_cstr, out_loc_item);
	if (h_ret != 0) {
		u_hashset_erase_item(pair->name_store, *out_name_item);
		free(*out_name_item);
		*out_name_item = NULL;
		os_mutex_unlock(&pair->mutex);
		return h_ret;
	}

	os_mutex_unlock(&pair->mutex);

	return 0;
}

void
oxr_pair_hashset_erase_and_free(struct oxr_pair_hashset *pair,
                                struct u_hashset_item **name_item_ptr,
                                struct u_hashset_item **loc_item_ptr)
{
	os_mutex_lock(&pair->mutex);

	// Is enforced to be not NULL.
	if (*name_item_ptr != NULL) {
		u_hashset_erase_item(pair->name_store, *name_item_ptr);
		free(*name_item_ptr);
		*name_item_ptr = NULL;
	}

	// Is enforced to be not NULL.
	if (*loc_item_ptr != NULL) {
		u_hashset_erase_item(pair->loc_store, *loc_item_ptr);
		free(*loc_item_ptr);
		*loc_item_ptr = NULL;
	}

	os_mutex_unlock(&pair->mutex);
}

XrResult
oxr_pair_hashset_has_name_and_loc(
    struct oxr_pair_hashset *pair, const char *name, const char *loc, bool *out_has_name, bool *out_has_loc)
{
	int h_ret;
	struct u_hashset_item *unused = NULL;

	os_mutex_lock(&pair->mutex);

	h_ret = u_hashset_find_c_str(pair->name_store, name, &unused);
	*out_has_name = (h_ret >= 0); // Is enforced to be not NULL.

	h_ret = u_hashset_find_c_str(pair->loc_store, loc, &unused);
	*out_has_loc = (h_ret >= 0); // Is enforced to be not NULL.

	os_mutex_unlock(&pair->mutex);

	return XR_SUCCESS;
}
