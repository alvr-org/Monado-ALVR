// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Base implementation factory for @ref xrt_io_cache.
 * @ingroup base
 */

#pragma once

#include "xrt/xrt_io_cache.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_space.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Create a base input/output cache from system devices.
 *
 * @ingroup base
 */
xrt_result_t
b_io_cache_create(struct xrt_system_devices *xsysd,
                  struct xrt_space_overseer *xso,
                  struct xrt_io_cache **out_xic);

#ifdef __cplusplus
}
#endif
