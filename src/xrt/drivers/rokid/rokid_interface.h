// Copyright 2020-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface to rokid driver.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup drv_rokid
 */

#pragma once

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

// NOTE: this is 1234:5679
#define ROKID_VID 0x04d2
#define ROKID_PID 0x162f

typedef struct cJSON cJSON;
struct xrt_prober;
struct xrt_prober_device;
struct xrt_device;

/*!
 * Probing function for Rokid devices.
 *
 * @ingroup drv_hdk
 * @see xrt_prober_found_func_t
 */
int
rokid_found(struct xrt_prober *xp,
            struct xrt_prober_device **devices,
            size_t device_count,
            size_t index,
            cJSON *attached_data,
            struct xrt_device **out_xdev);

#ifdef __cplusplus
}
#endif
