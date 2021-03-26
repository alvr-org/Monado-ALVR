// Copyright 2021-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  An implementation of a callback collection for the Android lifecycle.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_android
 */

#pragma once

#include <xrt/xrt_config_os.h>
#include <xrt/xrt_android.h>

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_instance_android;

/*!
 * @class android_lifecycle_callbacks
 * @brief An object handling a collection of callbacks for the Android lifecycle.
 */
struct android_lifecycle_callbacks;

/*!
 * Create an @ref android_lifecycle_callbacks object.
 *
 * @param xinst_android The instance that will be passed to all callbacks.
 *
 * @public @memberof android_lifecycle_callbacks
 */
struct android_lifecycle_callbacks *
android_lifecycle_callbacks_create(struct xrt_instance_android *xinst_android);

/*!
 * Destroy an @ref android_lifecycle_callbacks object.
 * @public @memberof android_lifecycle_callbacks
 */
void
android_lifecycle_callbacks_destroy(struct android_lifecycle_callbacks **ptr_callbacks);

/*!
 * Register a lifecycle event callback.
 *
 * @param alc Pointer to self
 * @param callback Function pointer for callback
 * @param event_mask bitwise-OR of one or more values from @ref xrt_android_lifecycle_event
 * @param userdata An opaque pointer for use by the callback. Whatever you pass here will be passed to the
 * callback when invoked.
 *
 * @return 0 on success, <0 on error.
 * @public @memberof android_lifecycle_callbacks
 */
int
android_lifecycle_callbacks_register_callback(struct android_lifecycle_callbacks *alc,
                                              xrt_android_lifecycle_event_handler_t callback,
                                              enum xrt_android_lifecycle_event event_mask,
                                              void *userdata);

/*!
 * Remove a lifecycle event callback that matches the supplied parameters.
 *
 * @param alc Pointer to self
 * @param callback Function pointer for callback
 * @param event_mask bitwise-OR of one or more values from @ref xrt_android_lifecycle_event
 * @param userdata An opaque pointer for use by the callback, must match the one originally supplied
 *
 * @return number of callbacks removed (typically 1) on success, <0 on error.
 * @public @memberof android_lifecycle_callbacks
 */
int
android_lifecycle_callbacks_remove_callback(struct android_lifecycle_callbacks *alc,
                                            xrt_android_lifecycle_event_handler_t callback,
                                            enum xrt_android_lifecycle_event event_mask,
                                            void *userdata);


/*!
 * Invoke all lifecycle event callbacks that match a given event.
 *
 * @param alc Pointer to self
 * @param event The event from @ref xrt_android_lifecycle_event
 *
 * @return the number of invoked callbacks on success, <0 on error.
 * @public @memberof android_lifecycle_callbacks
 */
int
android_lifecycle_callbacks_invoke(struct android_lifecycle_callbacks *alc, enum xrt_android_lifecycle_event event);


#ifdef __cplusplus
} // extern "C"
#endif
