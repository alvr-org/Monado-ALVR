// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Resolve suggested bindings into action cache IO.
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"
#include "oxr_resolver_for_profiles.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Resolve suggested input bindings for one action/subaction path into @p cache.
 *
 * This is intentionally lower-level than the existing action binding path: it
 * consumes the attached suggested-profile snapshot and resolves binding points
 * at cache population time.
 *
 * Assumes @p dev is valid and @p cache has already been cleared by the caller.
 */
void
oxr_resolver_populate_input(struct oxr_logger *log,
                            struct oxr_sink_logger *slog,
                            const struct oxr_io_cache *io_cache,
                            const struct oxr_profile_resolver_info *info,
                            const struct xrt_io_cache_device *dev,
                            XrPath subaction_path,
                            struct oxr_action_attachment *act_attached,
                            struct oxr_action_cache *cache);

/*!
 * Resolve suggested output bindings for one action/subaction path into @p cache.
 *
 * Assumes @p dev is valid and @p cache has already been cleared by the caller.
 */
void
oxr_resolver_populate_output(struct oxr_logger *log,
                             struct oxr_sink_logger *slog,
                             const struct oxr_io_cache *io_cache,
                             const struct oxr_profile_resolver_info *info,
                             const struct xrt_io_cache_device *dev,
                             XrPath subaction_path,
                             struct oxr_action_attachment *act_attached,
                             struct oxr_action_cache *cache);


#ifdef __cplusplus
}
#endif
