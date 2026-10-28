// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Misc helpers for @ref xrt_body_tracker implementations.
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_body_tracker.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Function pointer type for the body tracker's destroy function.
 *
 * @ingroup aux_util
 */
typedef void (*u_body_tracker_destroy_function_t)(struct xrt_body_tracker *xbt);

/*!
 * Populate the body tracker's function pointers with default implementations.
 *
 * Fills in all body tracker function pointers with not-implemented versions,
 * allowing implementations to override only the functions they implement.
 * @p destroy_fn must be implemented and must not be NULL.
 *
 * @param[in,out] xbt Pointer to the body tracker to populate.
 * @param[in] destroy_fn The body tracker's destroy function.
 * @ingroup aux_util
 */
void
u_body_tracker_populate_function_pointers(struct xrt_body_tracker *xbt, u_body_tracker_destroy_function_t destroy_fn);


#ifdef __cplusplus
}
#endif
