// Copyright 2025, rcelyte
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  SolarXR protocol bridge device
 * @ingroup drv_solarxr
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
struct xrt_device;
struct xrt_tracking_origin;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t
solarxr_device_create_xdevs(struct xrt_tracking_origin *tracking_origin,
                            struct xrt_device *out_xdevs[],
                            uint32_t out_xdevs_cap);

bool
solarxr_device_add_feeder_device(struct xrt_device *solarxr, struct xrt_device *xdev);

void
solarxr_device_remove_feeder_device(struct xrt_device *solarxr, struct xrt_device *xdev);

void
solarxr_device_clear_feeder_devices(struct xrt_device *solarxr);

static inline struct xrt_device *
solarxr_device_create(struct xrt_tracking_origin *const tracking_origin)
{
	struct xrt_device *out = NULL;
	solarxr_device_create_xdevs(tracking_origin, &out, 1);
	return out;
}

static inline void
solarxr_device_set_feeder_devices(struct xrt_device *const solarxr,
                                  struct xrt_device *const xdevs[],
                                  const uint32_t xdev_count)
{
	solarxr_device_clear_feeder_devices(solarxr);
	for (uint32_t i = 0; i < xdev_count; ++i) {
		solarxr_device_add_feeder_device(solarxr, xdevs[i]);
	}
}


#ifdef __cplusplus
}
#endif
