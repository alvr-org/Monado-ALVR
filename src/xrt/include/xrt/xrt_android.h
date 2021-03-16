// Copyright 2021-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header holding Android-specific details.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_config_os.h"
#include "xrt/xrt_compiler.h"
#include "xrt/xrt_results.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _JavaVM;
struct xrt_instance_android;
struct xrt_instance_info;

/*!
 * Distinguishes the possible Android lifecycle events from each other.
 *
 * Used as a bitmask when registering for callbacks.
 */
enum xrt_android_lifecycle_event
{
	XRT_ANDROID_LIVECYCLE_EVENT_ON_CREATE = 1 << 0,
	XRT_ANDROID_LIVECYCLE_EVENT_ON_DESTROY = 1 << 1,
	XRT_ANDROID_LIVECYCLE_EVENT_ON_PAUSE = 1 << 2,
	XRT_ANDROID_LIVECYCLE_EVENT_ON_RESUME = 1 << 3,
	XRT_ANDROID_LIVECYCLE_EVENT_ON_START = 1 << 4,
	XRT_ANDROID_LIVECYCLE_EVENT_ON_STOP = 1 << 5,
};

/*!
 * A callback type for a handler of Android lifecycle events.
 *
 * Return true to be removed from the callback list.
 */
typedef bool (*xrt_android_lifecycle_event_handler_t)(struct xrt_instance_android *xinst_android,
                                                      enum xrt_android_lifecycle_event event,
                                                      void *userdata);

#ifdef __cplusplus
}
#endif
