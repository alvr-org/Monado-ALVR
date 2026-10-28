// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for probing and querying @ref xrt_system_devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_instance.h"
#include "xrt/xrt_results.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_tracking.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_device;
struct xrt_session_event_sink;
struct xrt_space_overseer;

/*!
 * Takes a @ref xrt_instance, gets the prober from it and then uses the prober
 * to create a populated @ref xrt_system_devices struct from it.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_system_devices_create_from_prober(struct xrt_instance *xinst,
                                    struct xrt_session_event_sink *broadcast,
                                    struct xrt_system_devices **out_xsysd,
                                    struct xrt_space_overseer **out_xso);

/*!
 * Looks through @ref xrt_system_devices's devices and returns the first device
 * that supports hand tracking and the supplied input name.
 *
 * Used by target_builder_lighthouse to find Knuckles controllers in the list of
 * devices returned, the legacy builder to find hand tracking devices, etc.
 *
 * @ingroup aux_util
 */
struct xrt_device *
u_system_devices_get_ht_device(struct xrt_system_devices *xsysd, enum xrt_input_name name);

/*!
 * Helper to get the first left (unobstructed) hand-tracking device,
 * uses @ref u_system_devices_get_ht_device.
 *
 * @ingroup aux_util
 */
static inline struct xrt_device *
u_system_devices_get_ht_device_unobstructed_left(struct xrt_system_devices *xsysd)
{
	return u_system_devices_get_ht_device(xsysd, XRT_INPUT_HT_UNOBSTRUCTED_LEFT);
}

/*!
 * Helper to get the first (unobstructed) right hand-tracking device,
 * uses @ref u_system_devices_get_ht_device.
 *
 * @ingroup aux_util
 */
static inline struct xrt_device *
u_system_devices_get_ht_device_unobstructed_right(struct xrt_system_devices *xsysd)
{
	return u_system_devices_get_ht_device(xsysd, XRT_INPUT_HT_UNOBSTRUCTED_RIGHT);
}

/*!
 * Helper to get the first left (conforming) hand-tracking device,
 * uses @ref u_system_devices_get_ht_device.
 *
 * @ingroup aux_util
 */
static inline struct xrt_device *
u_system_devices_get_ht_device_conforming_left(struct xrt_system_devices *xsysd)
{
	return u_system_devices_get_ht_device(xsysd, XRT_INPUT_HT_CONFORMING_LEFT);
}

/*!
 * Helper to get the first (conforming) right hand-tracking device,
 * uses @ref u_system_devices_get_ht_device.
 *
 * @ingroup aux_util
 */
static inline struct xrt_device *
u_system_devices_get_ht_device_conforming_right(struct xrt_system_devices *xsysd)
{
	return u_system_devices_get_ht_device(xsysd, XRT_INPUT_HT_CONFORMING_RIGHT);
}

#ifdef __cplusplus
}
#endif
