// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Base implementation factory for @ref xrt_body_tracker.
 * @ingroup base
 */

#pragma once

#include "xrt/xrt_body_tracker.h"
#include "xrt/xrt_system.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Create a base body tracker from system devices.
 *
 * @ingroup base
 */
xrt_result_t
b_body_tracker_create(struct xrt_system_devices *xsysd,
                      const struct xrt_body_tracker_create_info *info,
                      struct xrt_body_tracker **out_xbt);

#ifdef __cplusplus
}
#endif
