// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface to Blubur S1 driver code.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_blubur_s1
 */

#pragma once

#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#ifdef __cplusplus
extern "C" {
#endif


#define BLUBUR_S1_VID 0x2b1c
#define BLUBUR_S1_PID 0x0001

struct blubur_s1_hmd;

/*!
 * Probing function for Blubur S1 devices.
 *
 * @ingroup drv_blubur_s1
 * @see xrt_prober_found_func_t
 */
int
blubur_s1_found(struct xrt_prober *xp,
                struct xrt_prober_device **devices,
                size_t device_count,
                size_t index,
                cJSON *attached_data,
                struct xrt_device **out_xdev);

/*!
 * Creates a Blubur S1 HMD device from an opened HID handle.
 *
 * @ingroup drv_blubur_s1
 */
struct blubur_s1_hmd *
blubur_s1_hmd_create(struct os_hid_device *dev, const char *serial);


/*!
 * @dir drivers/blubur_s1
 *
 * @brief @ref drv_blubur_s1 files.
 */

#ifdef __cplusplus
}
#endif
