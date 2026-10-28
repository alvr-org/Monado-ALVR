// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Misc helpers for @ref xrt_fs implementations.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_frameserver.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Populate the frameserver's function pointers with default implementations.
 *
 * Fills in all frameserver function pointers with not-implemented versions,
 * allowing implementations to override only the functions they implement.
 *
 * @param[in,out] xfs Pointer to the frameserver to populate.
 * @ingroup aux_util
 */
void
u_frameserver_populate_function_pointers(struct xrt_fs *xfs);


#ifdef __cplusplus
}
#endif
