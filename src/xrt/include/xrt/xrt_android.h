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

#ifdef XRT_OS_ANDROID

/*!
 * @interface xrt_instance_android
 *
 * This is the interface to the Android-specific "aspect" of @ref xrt_instance.
 *
 * It is expected that your implementation of this interface will be nested in your
 * implementation of @ref xrt_instance. It does not have a separate create or
 * destroy function as it is an (optional) aspect of the instance.
 */
struct xrt_instance_android
{

	/*!
	 * @name Interface Methods
	 *
	 * All Android-based implementations of the xrt_instance interface must additionally populate all these function
	 * pointers with their implementation methods. To use this interface, see the helper functions.
	 * @{
	 */
	/*!
	 * Retrieve the stored Java VM instance pointer.
	 *
	 * @note Code consuming this interface should use xrt_instance_android_get_vm()
	 *
	 * @param xinst_android Pointer to self
	 *
	 * @return The VM pointer.
	 */
	struct _JavaVM *(*get_vm)(const struct xrt_instance_android *xinst_android);

	/*!
	 * Retrieve the stored activity android.content.Context jobject.
	 *
	 * For usage, cast the return value to jobject - a typedef whose definition
	 * differs between C (a void *) and C++ (a pointer to an empty class)
	 *
	 * @note Code consuming this interface should use xrt_instance_android_get_context()
	 *
	 * @param xinst_android Pointer to self
	 *
	 * @return The activity context.
	 */
	void *(*get_context)(const struct xrt_instance_android *xinst_android);

	/*!
	 * Register a activity lifecycle event callback.
	 *
	 * @note Code consuming this interface should use xrt_instance_android_register_activity_lifecycle_callback()
	 *
	 * @param xinst_android Pointer to self
	 * @param callback Function pointer for callback
	 * @param event_mask bitwise-OR of one or more values from @ref xrt_android_lifecycle_event
	 * @param userdata An opaque pointer for use by the callback. Whatever you pass here will be passed to the
	 * callback when invoked.
	 *
	 * @return XRT_SUCCESS on success, other error code on error.
	 */
	xrt_result_t (*register_activity_lifecycle_callback)(struct xrt_instance_android *xinst_android,
	                                                     xrt_android_lifecycle_event_handler_t callback,
	                                                     enum xrt_android_lifecycle_event event_mask,
	                                                     void *userdata);

	/*!
	 * Remove a activity lifecycle event callback that matches the supplied parameters.
	 *
	 * @note Code consuming this interface should use xrt_instance_android_remove_activity_lifecycle_callback()
	 *
	 * @param xinst_android Pointer to self
	 * @param callback Function pointer for callback
	 * @param event_mask bitwise-OR of one or more values from @ref xrt_android_lifecycle_event
	 * @param userdata An opaque pointer for use by the callback. Whatever you pass here will be passed to the
	 * callback when invoked.
	 *
	 * @return XRT_SUCCESS on success (at least one callback was removed), @ref XRT_ERROR_ANDROID on error.
	 */
	xrt_result_t (*remove_activity_lifecycle_callback)(struct xrt_instance_android *xinst_android,
	                                                   xrt_android_lifecycle_event_handler_t callback,
	                                                   enum xrt_android_lifecycle_event event_mask,
	                                                   void *userdata);

	/*!
	 * @}
	 */
};

/*!
 * @copydoc xrt_instance_android::get_vm
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_instance_android
 */
static inline struct _JavaVM *
xrt_instance_android_get_vm(struct xrt_instance_android *xinst_android)
{
	return xinst_android->get_vm(xinst_android);
}

/*!
 * @copydoc xrt_instance_android::get_context
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_instance_android
 */
static inline void *
xrt_instance_android_get_context(struct xrt_instance_android *xinst_android)
{
	return xinst_android->get_context(xinst_android);
}

/*!
 * @copydoc xrt_instance_android::register_activity_lifecycle_callback
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_instance_android
 */
static inline xrt_result_t
xrt_instance_android_register_activity_lifecycle_callback(struct xrt_instance_android *xinst_android,
                                                          xrt_android_lifecycle_event_handler_t callback,
                                                          enum xrt_android_lifecycle_event event_mask,
                                                          void *userdata)
{
	return xinst_android->register_activity_lifecycle_callback(xinst_android, callback, event_mask, userdata);
}

/*!
 * @copydoc xrt_instance_android::remove_activity_lifecycle_callback
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_instance_android
 */
static inline xrt_result_t
xrt_instance_android_remove_activity_lifecycle_callback(struct xrt_instance_android *xinst_android,
                                                        xrt_android_lifecycle_event_handler_t callback,
                                                        enum xrt_android_lifecycle_event event_mask,
                                                        void *userdata)
{
	return xinst_android->remove_activity_lifecycle_callback(xinst_android, callback, event_mask, userdata);
}

#endif // XRT_OS_ANDROID

#ifdef __cplusplus
}
#endif
