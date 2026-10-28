// Copyright 2023-2025, Tobias Frisch
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Xreal Air packet parsing implementation.
 * @author Tobias Frisch <jacki@thejackimonster.de>
 * @ingroup drv_xreal_air
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup drv_xreal_air Xreal Air driver
 * @ingroup drv
 *
 * @brief Driver for the Xreal Air HMD.
 */

/*!
 * Vendor id for Xreal Air.
 *
 * @ingroup drv_xreal_air
 */
#define XREAL_AIR_VID 0x3318

/*!
 * Product id for Xreal Air.
 *
 * @ingroup drv_xreal_air
 */
#define XREAL_AIR_PID 0x0424

/*!
 * Product id for Xreal Air 2.
 *
 * @ingroup drv_xreal_air
 */
#define XREAL_AIR_2_PID 0x0428

/*!
 * Product id for Xreal Air 2 Pro.
 *
 * @ingroup drv_xreal_air
 */
#define XREAL_AIR_2_PRO_PID 0x0432

/*!
 * Product id for Xreal Air 2 Ultra.
 *
 * @ingroup drv_xreal_air
 */
#define XREAL_AIR_2_ULTRA_PID 0x0426

/*!
 * Builder setup for Xreal Air glasses.
 *
 * @ingroup drv_xreal_air
 */
struct xrt_builder *
xreal_air_builder_create(void);

/*!
 * @dir drivers/xreal_air
 *
 * @brief xreal_air files.
 */

#ifdef __cplusplus
}
#endif
