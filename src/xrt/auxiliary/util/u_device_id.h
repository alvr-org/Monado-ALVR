// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Generator for per-process unique @ref xrt_device_id values.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_device.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Generate a new per-process unique device ID.
 *
 * @ingroup aux_util
 */
struct xrt_device_id
u_device_id_generate(void);

/*!
 * Assign a newly generated ID to the given device.
 *
 * @ingroup aux_util
 */
void
u_device_id_assign(struct xrt_device *xdev);


#ifdef __cplusplus
}
#endif
