// Copyright 2023, Qualcomm Innovation Center, Inc.
// Copyright 2020-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Base implementation of the @ref xrt_instance_android interface.
 * @author Jarvis Huang
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_android
 */
#pragma once

#include "xrt/xrt_instance.h"
#include "xrt/xrt_android.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _JavaVM;
struct android_lifecycle_callbacks;

/*!
 * @brief A basic implementation of the @ref xrt_instance_android interface,
 * a platform-specific "aspect" of @ref xrt_instance.
 *
 * Store nested in your @ref xrt_instance implementation (by value, not separately allocated),
 * and call @ref android_instance_base_init in your instance creation and
 * @ref android_instance_base_cleanup in instance destruction.
 *
 */
struct android_instance_base
{
	struct xrt_instance_android base;
	struct _JavaVM *vm;
	void *context;
	struct android_lifecycle_callbacks *lifecycle_callbacks;
};

/*!
 * @brief Initialize resources owned by @p android_instance_base and
 * sets the @ref xrt_instance::android_instance pointer.
 *
 * @param aib The object to initialize.
 * @param xinst The xrt_instance to update.
 * @param vm The JavaVM pointer.
 * @param activity The activity jobject, cast to a void pointer.
 *
 * @returns @ref XRT_SUCCESS on success, @ref XRT_ERROR_ALLOCATION if we could not allocate
 * our required objects, and @ref XRT_ERROR_ANDROID if something goes very wrong with Java/JNI
 * that should be impossible and likely indicates a logic error in the code.
 *
 * @public @memberof android_instance_base
 */
xrt_result_t
android_instance_base_init(struct android_instance_base *aib,
                           struct xrt_instance *xinst,
                           const struct xrt_instance_info *ii);

/*!
 * @brief Release resources owned by @ref android_instance_base and unsets the aspect pointer
 * - but does not free @p aib itself, since it is intended to be held by value.
 *
 * @param aib The object to de-initialize.
 * @param xinst The xrt_instance to update.
 *
 * @public @memberof android_instance_base
 */
void
android_instance_base_cleanup(struct android_instance_base *aib, struct xrt_instance *xinst);

#ifdef __cplusplus
};
#endif
