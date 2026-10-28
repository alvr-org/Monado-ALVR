// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Pair of name/localized hashets with atomic erase helper.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_compiler.h"

#include "os/os_threading.h"

#include "oxr_forward_declarations.h"
#include "oxr_extension_support.h"


#ifdef __cplusplus
extern "C" {
#endif


struct u_hashset;
struct u_hashset_item;


/*!
 * A pair of hashets (name_store and loc_store) with an optional mutex
 * so that both members can be removed atomically.
 *
 * @ingroup oxr_main
 */
struct oxr_pair_hashset
{
	struct u_hashset *name_store;
	struct u_hashset *loc_store;
	struct os_mutex mutex;
};

/*!
 * Initialize the pair: create both hashets and the mutex.
 *
 * @param log Logger for error reporting
 * @param pair Pair to initialize
 * @return XR_SUCCESS or XR_ERROR_RUNTIME_FAILURE
 * @public @memberof oxr_pair_hashset
 */
XRT_NONNULL_ALL XRT_CHECK_RESULT XrResult
oxr_pair_hashset_init(struct oxr_logger *log, struct oxr_pair_hashset *pair);

/*!
 * Finalize the pair: destroy mutex and both hashets.
 *
 * @param pair Pair to finalize
 * @public @memberof oxr_pair_hashset
 */
XRT_NONNULL_ALL void
oxr_pair_hashset_fini(struct oxr_pair_hashset *pair);

/*!
 * Insert a name/localized string pair into both hashets under lock.
 * On success, @p out_name_item and @p out_loc_item are set to the
 * inserted items.
 *
 * @param pair Pair to insert into
 * @param name_cstr Name string (null-terminated)
 * @param loc_cstr Localized string (null-terminated)
 * @param out_name_item Output for the name store item
 * @param out_loc_item Output for the loc store item
 * @return 0 on success, non-zero on failure
 * @public @memberof oxr_pair_hashset
 */
XRT_NONNULL_ALL XRT_CHECK_RESULT int
oxr_pair_hashset_insert_str_c(struct oxr_pair_hashset *pair,
                              const char *name_cstr,
                              const char *loc_cstr,
                              struct u_hashset_item **out_name_item,
                              struct u_hashset_item **out_loc_item);

/*!
 * Erase and free both items from the pair under lock, and set the
 * pointed-to pointers to NULL. Either pointer may be NULL to skip
 * that side.
 *
 * @param pair Pair to remove from
 * @param name_item_ptr Pointer to name item pointer (erased, freed, set to NULL)
 * @param loc_item_ptr Pointer to loc item pointer (erased, freed, set to NULL)
 * @public @memberof oxr_pair_hashset
 */
XRT_NONNULL_ALL void
oxr_pair_hashset_erase_and_free(struct oxr_pair_hashset *pair,
                                struct u_hashset_item **name_item_ptr,
                                struct u_hashset_item **loc_item_ptr);

/*!
 * Check both name and loc stores under a single lock (atomic).
 * Writes true to @p out_has_name if the name is present, and to
 * @p out_has_loc if the localized name is present.
 *
 * @param pair Pair whose stores to search
 * @param name Null-terminated name string
 * @param loc Null-terminated localized string
 * @param out_has_name Output: true if name is present (duplicate)
 * @param out_has_loc Output: true if loc is present (duplicate)
 * @return XR_SUCCESS if the check completed successfully
 * @public @memberof oxr_pair_hashset
 */
XRT_NONNULL_ALL XRT_CHECK_RESULT XrResult
oxr_pair_hashset_has_name_and_loc(
    struct oxr_pair_hashset *pair, const char *name, const char *loc, bool *out_has_name, bool *out_has_loc);


#ifdef __cplusplus
}
#endif
