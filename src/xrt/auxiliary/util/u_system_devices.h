// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Misc helpers for xrt_system_devices implementations
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_system.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Function pointer type for the system devices' get_roles function.
 *
 * @ingroup aux_util
 */
typedef xrt_result_t (*u_system_devices_get_roles_function_t)(struct xrt_system_devices *xsysd,
                                                              struct xrt_system_roles *out_roles);

/*!
 * Function pointer type for the system devices' destroy function.
 *
 * @ingroup aux_util
 */
typedef void (*u_system_devices_destroy_function_t)(struct xrt_system_devices *xsysd);

void
u_system_devices_populate_function_pointers(struct xrt_system_devices *xsysd,
                                            u_system_devices_get_roles_function_t get_roles_fn,
                                            u_system_devices_destroy_function_t destroy_fn);


#ifdef __cplusplus
}
#endif
