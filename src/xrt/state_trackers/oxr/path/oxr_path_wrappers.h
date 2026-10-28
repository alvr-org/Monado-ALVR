// Copyright 2019-2020, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Wrapper functions that take oxr_instance and call into path/cache functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_openxr_includes.h"

#include "oxr_forward_declarations.h"
#include "oxr_path_store.h"

#include "../oxr_objects.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @public @memberof oxr_instance
 */
static inline bool
oxr_path_is_valid(struct oxr_logger *log, struct oxr_instance *inst, XrPath path)
{
	return oxr_path_store_is_valid(&inst->path_store, path);
}

/*!
 * Get the path for the given string if it exists, or create it if it does not.
 *
 * @public @memberof oxr_instance
 */
static inline XrResult
oxr_path_get_or_create(
    struct oxr_logger *log, struct oxr_instance *inst, const char *str, size_t length, XrPath *out_path)
{
	return oxr_path_store_get_or_create(&inst->path_store, str, length, out_path);
}

/*!
 * Only get the path for the given string if it exists.
 *
 * @public @memberof oxr_instance
 */
static inline void
oxr_path_only_get(struct oxr_logger *log, struct oxr_instance *inst, const char *str, size_t length, XrPath *out_path)
{
	oxr_path_store_only_get(&inst->path_store, str, length, out_path);
}

/*!
 * Get a pointer and length of the internal string.
 *
 * The pointer has the same life time as the instance. The length is the number
 * of valid characters, not including the null termination character (but an
 * extra null byte is always reserved at the end so can strings can be given
 * to functions expecting null terminated strings).
 *
 * @public @memberof oxr_instance
 */
static inline XrResult
oxr_path_get_string(
    struct oxr_logger *log, const struct oxr_instance *inst, XrPath path, const char **out_str, size_t *out_length)
{
	return oxr_path_store_get_string(&inst->path_store, path, out_str, out_length);
}


#ifdef __cplusplus
}
#endif
