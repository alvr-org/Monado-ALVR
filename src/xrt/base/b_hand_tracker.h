// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Base implementation factory for @ref xrt_hand_tracker.
 * @ingroup base
 */

#pragma once

#include "xrt/xrt_hand_tracker.h"
#include "xrt/xrt_system.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Callback used by @ref b_hand_tracker_create_with_cb to provide an
 * ordered list of devices to try for @p input_name.
 *
 * @param input_name Hand-tracking input being resolved.
 * @param xsysd System devices object that owns the static device list.
 * @param data Private callback data.
 * @param in_xdev_count Maximum number of devices that fit in @p out_xdevs.
 * @param out_xdevs Caller-provided array to fill with devices in priority order.
 * @param out_xdev_count Number of devices written to @p out_xdevs.
 *
 * @ingroup base
 */
typedef xrt_result_t (*b_hand_tracker_get_devices_func_t)(enum xrt_input_name input_name,
                                                          struct xrt_system_devices *xsysd,
                                                          void *data,
                                                          uint32_t in_xdev_count,
                                                          struct xrt_device **out_xdevs,
                                                          uint32_t *out_xdev_count);

/*!
 * Create a base hand tracker from system devices.
 *
 * @ingroup base
 */
xrt_result_t
b_hand_tracker_create(struct xrt_system_devices *xsysd,
                      const struct xrt_hand_tracker_create_info *info,
                      struct xrt_hand_tracker **out_xht);

/*!
 * Create a base hand tracker that uses a callback to provide other devices to
 * try first.
 *
 * If a locked device is supplied in @p info, locked mode takes precedence.
 * Otherwise this selects @ref HandTrackerMode::Callback, defined and described
 * in b_hand_tracker.hpp, and uses @p get_devices to resolve its inputs.
 *
 * @ingroup base
 */
xrt_result_t
b_hand_tracker_create_with_cb(struct xrt_system_devices *xsysd,
                              const struct xrt_hand_tracker_create_info *info,
                              b_hand_tracker_get_devices_func_t get_devices,
                              void *data,
                              struct xrt_hand_tracker **out_xht);

#ifdef __cplusplus
}
#endif
