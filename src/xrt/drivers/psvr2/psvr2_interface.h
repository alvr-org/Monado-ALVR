// Copyright 2020-2021, Collabora, Ltd.
// Copyright 2023, Jan Schmidt
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  PSVR2 HMD device
 *
 * @author Jan Schmidt <jan@centricular.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup drv_psvr2
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup drv_psvr2 PS VR2 HMD driver
 * @ingroup drv
 *
 * @brief Driver for the Playstation VR2 headset
 *
 */

#define PSVR2_VID 0x054C
#define PSVR2_PID 0x0CDE

/*!
 * Create the PS VR2 HMD device
 *
 * @ingroup drv_psvr2
 */
struct xrt_device *
psvr2_hmd_create(struct xrt_prober_device *xpdev);

/*!
 * Probing function for PlayStation VR2 devices.
 *
 * @ingroup drv_psvr2
 * @see xrt_prober_found_func_t
 */
int
psvr2_found(struct xrt_prober *xp,
            struct xrt_prober_device **devices,
            size_t device_count,
            size_t index,
            cJSON *attached_data,
            struct xrt_device **out_xdevs);

/*!
 * @dir drivers/psvr2
 *
 * @brief @ref drv_psvr2 files.
 */


#ifdef __cplusplus
}
#endif
