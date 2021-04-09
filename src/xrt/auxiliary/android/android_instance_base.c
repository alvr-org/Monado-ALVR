// Copyright 2023, Qualcomm Innovation Center, Inc.
// Copyright 2021-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Basic xrt_instance_base implementation.
 * @author Jarvis Huang
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_android
 */

#include "android_instance_base.h"

#include "android/android_globals.h"
#include "android/android_lifecycle_callbacks.h"
#include "util/u_logging.h"
#include "xrt/xrt_instance.h"
#include "xrt/xrt_android.h"
#include "xrt/xrt_results.h"

#include <jni.h>
#include <assert.h>


static inline struct android_instance_base *
android_instance_base(struct xrt_instance_android *xinst_android)
{
	return (struct android_instance_base *)(xinst_android);
}

static inline const struct android_instance_base *
android_instance_base_const(const struct xrt_instance_android *xinst_android)
{
	return (const struct android_instance_base *)(xinst_android);
}

static struct _JavaVM *
base_get_vm(const struct xrt_instance_android *xinst_android)
{

	const struct android_instance_base *aib = android_instance_base_const(xinst_android);
	return aib->vm;
}

static void *
base_get_context(const struct xrt_instance_android *xinst_android)
{

	const struct android_instance_base *aib = android_instance_base_const(xinst_android);
	return aib->context;
}

static xrt_result_t
base_register_activity_lifecycle_callback(struct xrt_instance_android *xinst_android,
                                          xrt_android_lifecycle_event_handler_t callback,
                                          enum xrt_android_lifecycle_event event_mask,
                                          void *userdata)
{
	struct android_instance_base *aib = android_instance_base(xinst_android);
	int ret = 0;
	if (aib->lifecycle_callbacks == NULL) {
		U_LOG_I("No lifecycle callback container, instance is likely Service");
	} else {
		ret = android_lifecycle_callbacks_register_callback(aib->lifecycle_callbacks, callback, event_mask,
		                                                    userdata);
	}
	// If it fails, the inner callback container threw on emplace_back. Should basically never happen,
	// but technically an allocation error.
	return ret == 0 ? XRT_SUCCESS : XRT_ERROR_ALLOCATION;
}


static xrt_result_t
base_remove_activity_lifecycle_callback(struct xrt_instance_android *xinst_android,
                                        xrt_android_lifecycle_event_handler_t callback,
                                        enum xrt_android_lifecycle_event event_mask,
                                        void *userdata)
{
	struct android_instance_base *aib = android_instance_base(xinst_android);
	int ret = 0;
	if (aib->lifecycle_callbacks != NULL) {
		// We expect 1 to be returned, to remove the callback we previously added
		ret = android_lifecycle_callbacks_remove_callback(aib->lifecycle_callbacks, callback, event_mask,
		                                                  userdata);
	}

	return ret > 0 ? XRT_SUCCESS : XRT_ERROR_ANDROID;
}

xrt_result_t
android_instance_base_init(struct android_instance_base *aib,
                           struct xrt_instance *xinst,
                           const struct xrt_instance_info *ii)
{
	struct _JavaVM *vm = ii->platform_info.vm;
	void *context = ii->platform_info.context;

	if (vm == NULL) {
		U_LOG_E("Invalid Java VM - trying globals");
		vm = android_globals_get_vm();
	}

	if (context == NULL) {
		U_LOG_E("Invalid Context - trying globals");
		context = android_globals_get_context();
	}

	if (vm == NULL) {
		U_LOG_E("Invalid Java VM");
		return XRT_ERROR_ANDROID;
	}

	if (context == NULL) {
		U_LOG_E("Invalid context");
		return XRT_ERROR_ANDROID;
	}

	JNIEnv *env = NULL;
	if (vm->functions->AttachCurrentThread(&vm->functions, &env, NULL) != JNI_OK) {
		U_LOG_E("Failed to attach thread");
		return XRT_ERROR_ANDROID;
	}

	jobject global_context = (*env)->NewGlobalRef(env, context);
	if (global_context == NULL) {
		U_LOG_E("Failed to create global ref");
		return XRT_ERROR_ANDROID;
	}

	xinst->android_instance = &aib->base;
	aib->vm = vm;
	aib->context = global_context;
	aib->base.get_vm = base_get_vm;
	aib->base.get_context = base_get_context;
	aib->base.register_activity_lifecycle_callback = base_register_activity_lifecycle_callback;
	aib->base.remove_activity_lifecycle_callback = base_remove_activity_lifecycle_callback;

	// aib->base.register_surface_callback = base_register_surface_callback;
	// aib->base.remove_surface_callback = base_remove_surface_callback;

	aib->lifecycle_callbacks = android_lifecycle_callbacks_create(&aib->base);

	if (aib->lifecycle_callbacks == NULL) {
		return XRT_ERROR_ALLOCATION;
	}
	return XRT_SUCCESS;
}

void
android_instance_base_cleanup(struct android_instance_base *aib, struct xrt_instance *xinst)
{
	assert(&(aib->base) == xinst->android_instance);
	android_lifecycle_callbacks_destroy(&aib->lifecycle_callbacks);

	if (aib->vm != NULL) {
		JNIEnv *env = NULL;
		if (aib->vm->functions->AttachCurrentThread(&aib->vm->functions, &env, NULL) == JNI_OK) {
			(*env)->DeleteGlobalRef(env, aib->context);
		}
	}

	xinst->android_instance = NULL;
}
