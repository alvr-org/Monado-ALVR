// Copyright 2023, Tobias Frisch
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Nreal Air packet parsing implementation.
 * @author Tobias Frisch <thejackimonster@gmail.com>
 * @ingroup drv_na
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup drv_na Nreal Air driver
 * @ingroup drv
 *
 * @brief Driver for the Nreal Air HMD.
 */

/*!
 * Vendor id for Nreal Air.
 *
 * @ingroup drv_na
 */
#define NA_VID 0x3318

/*!
 * Product id for Nreal Air.
 *
 * @ingroup drv_na
 */
#define NA_PID 0x0424

/*!
 * Builder setup for Nreal Air glasses.
 *
 * @ingroup drv_na
 */
struct xrt_builder *
nreal_air_builder_create(void);

/*!
 * @dir drivers/nreal_air
 *
 * @brief nreal_air files.
 */

#ifdef __cplusplus
}
#endif
