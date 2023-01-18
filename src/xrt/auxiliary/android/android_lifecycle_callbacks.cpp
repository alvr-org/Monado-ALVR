// Copyright 2021-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of a callback collection for Android lifecycle events.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_android
 */

#include "android_lifecycle_callbacks.h"

#include "android_load_class.hpp"
#include "org.freedesktop.monado.auxiliary.hpp"

#include "xrt/xrt_config_android.h"
#include "xrt/xrt_android.h"
#include "util/u_logging.h"
#include "util/u_generic_callbacks.hpp"

#include "wrap/android.app.h"

#include <memory>

using wrap::android::app::Activity;
using wrap::org::freedesktop::monado::auxiliary::ActivityLifecycleListener;
using xrt::auxiliary::android::loadClassFromRuntimeApk;
using xrt::auxiliary::util::GenericCallbacks;

struct android_lifecycle_callbacks
{
	explicit android_lifecycle_callbacks(xrt_instance_android *xinst_android) : instance_android(xinst_android) {}
	xrt_instance_android *instance_android;
	GenericCallbacks<xrt_android_lifecycle_event_handler_t, enum xrt_android_lifecycle_event> callback_collection;
	ActivityLifecycleListener listener{};
};

int
android_lifecycle_callbacks_invoke(struct android_lifecycle_callbacks *alc, enum xrt_android_lifecycle_event event);

/*!
 * JNI functions
 */

static void
on_activity_created(JNIEnv *env, jobject thiz, jlong native_callback_ptr, jobject activity)
{
	auto *alc = reinterpret_cast<android_lifecycle_callbacks *>(native_callback_ptr);
	if (env->IsSameObject(activity, (jobject)xrt_instance_android_get_context(alc->instance_android))) {
		android_lifecycle_callbacks_invoke(alc, XRT_ANDROID_LIVECYCLE_EVENT_ON_CREATE);
	}
}

static void
on_activity_started(JNIEnv *env, jobject thiz, jlong native_callback_ptr, jobject activity)
{
	auto *alc = reinterpret_cast<android_lifecycle_callbacks *>(native_callback_ptr);
	if (env->IsSameObject(activity, (jobject)xrt_instance_android_get_context(alc->instance_android))) {
		android_lifecycle_callbacks_invoke(alc, XRT_ANDROID_LIVECYCLE_EVENT_ON_START);
	}
}

static void
on_activity_resumed(JNIEnv *env, jobject thiz, jlong native_callback_ptr, jobject activity)
{
	auto *alc = reinterpret_cast<android_lifecycle_callbacks *>(native_callback_ptr);
	if (env->IsSameObject(activity, (jobject)xrt_instance_android_get_context(alc->instance_android))) {
		android_lifecycle_callbacks_invoke(alc, XRT_ANDROID_LIVECYCLE_EVENT_ON_RESUME);
	}
}

static void
on_activity_paused(JNIEnv *env, jobject thiz, jlong native_callback_ptr, jobject activity)
{
	auto *alc = reinterpret_cast<android_lifecycle_callbacks *>(native_callback_ptr);
	if (env->IsSameObject(activity, (jobject)xrt_instance_android_get_context(alc->instance_android))) {
		android_lifecycle_callbacks_invoke(alc, XRT_ANDROID_LIVECYCLE_EVENT_ON_PAUSE);
	}
}

static void
on_activity_stopped(JNIEnv *env, jobject thiz, jlong native_callback_ptr, jobject activity)
{
	auto *alc = reinterpret_cast<android_lifecycle_callbacks *>(native_callback_ptr);
	if (env->IsSameObject(activity, (jobject)xrt_instance_android_get_context(alc->instance_android))) {
		android_lifecycle_callbacks_invoke(alc, XRT_ANDROID_LIVECYCLE_EVENT_ON_STOP);
	}
}

static void
on_activity_save_instance_state(JNIEnv *env, jobject thiz, jlong native_callback_ptr, jobject activity)
{}

static void
on_activity_destroyed(JNIEnv *env, jobject thiz, jlong native_callback_ptr, jobject activity)
{
	auto *alc = reinterpret_cast<android_lifecycle_callbacks *>(native_callback_ptr);
	if (env->IsSameObject(activity, (jobject)xrt_instance_android_get_context(alc->instance_android))) {
		android_lifecycle_callbacks_invoke(alc, XRT_ANDROID_LIVECYCLE_EVENT_ON_DESTROY);
	}
}

static JNINativeMethod methods[] = {
    {"nativeOnActivityCreated", "(JLandroid/app/Activity;)V", (void *)&on_activity_created},
    {"nativeOnActivityStarted", "(JLandroid/app/Activity;)V", (void *)&on_activity_started},
    {"nativeOnActivityResumed", "(JLandroid/app/Activity;)V", (void *)&on_activity_resumed},
    {"nativeOnActivityPaused", "(JLandroid/app/Activity;)V", (void *)&on_activity_paused},
    {"nativeOnActivityStopped", "(JLandroid/app/Activity;)V", (void *)&on_activity_stopped},
    {"nativeOnActivitySaveInstanceState", "(JLandroid/app/Activity;)V", (void *)&on_activity_save_instance_state},
    {"nativeOnActivityDestroyed", "(JLandroid/app/Activity;)V", (void *)&on_activity_destroyed},
};

#define CATCH_CLAUSES(ACTION, RET)                                                                                     \
	catch (std::exception const &e)                                                                                \
	{                                                                                                              \
		U_LOG_E("Exception while " ACTION "! %s", e.what());                                                   \
		return RET;                                                                                            \
	}                                                                                                              \
	catch (...)                                                                                                    \
	{                                                                                                              \
		U_LOG_E("Unknown exception while " ACTION "!");                                                        \
		return RET;                                                                                            \
	}

int
android_lifecycle_callbacks_register_callback(struct android_lifecycle_callbacks *alc,
                                              xrt_android_lifecycle_event_handler_t callback,
                                              enum xrt_android_lifecycle_event event_mask,
                                              void *userdata)
{
	try {
		alc->callback_collection.addCallback(callback, event_mask, userdata);
		return 0;
	}
	CATCH_CLAUSES("adding callback to collection", -1)
}

int
android_lifecycle_callbacks_remove_callback(struct android_lifecycle_callbacks *alc,
                                            xrt_android_lifecycle_event_handler_t callback,
                                            enum xrt_android_lifecycle_event event_mask,
                                            void *userdata)
{
	try {
		return alc->callback_collection.removeCallback(callback, event_mask, userdata);
	}
	CATCH_CLAUSES("removing callback", -1)
}

int
android_lifecycle_callbacks_invoke(struct android_lifecycle_callbacks *alc, enum xrt_android_lifecycle_event event)
{
	try {
		return alc->callback_collection.invokeCallbacks(
		    event, [=](enum xrt_android_lifecycle_event event, xrt_android_lifecycle_event_handler_t callback,
		               void *userdata) { return callback(alc->instance_android, event, userdata); });
	}
	CATCH_CLAUSES("invoking callbacks", -1)
}

struct android_lifecycle_callbacks *
android_lifecycle_callbacks_create(struct xrt_instance_android *xinst_android)
{
	try {
		jni::init(xrt_instance_android_get_vm(xinst_android));
		jobject context = (jobject)xrt_instance_android_get_context(xinst_android);
		if (!jni::env()->IsInstanceOf(context, jni::Class(Activity::getTypeName()).getHandle())) {
			// skip if context is not android.app.Activity
			U_LOG_W("Context is not Activity, skip");
			return nullptr;
		}

		auto clazz = loadClassFromRuntimeApk(context, ActivityLifecycleListener::getFullyQualifiedTypeName());
		if (clazz.isNull()) {
			U_LOG_E("Could not load class '%s' from package '%s'",
			        ActivityLifecycleListener::getFullyQualifiedTypeName(), XRT_ANDROID_PACKAGE);
			return nullptr;
		}

		auto ret = std::make_unique<android_lifecycle_callbacks>(xinst_android);
		// Teach the wrapper our class before we start to use it.
		ActivityLifecycleListener::staticInitClass((jclass)clazz.object().getHandle());
		jni::env()->RegisterNatives((jclass)clazz.object().getHandle(), methods,
		                            sizeof(methods) / sizeof(methods[0]));

		ret->listener = ActivityLifecycleListener::construct(ret.get());
		ret->listener.registerCallback(Activity(context));
		return ret.release();
	}
	CATCH_CLAUSES("creating callbacks structure", nullptr)
}

void
android_lifecycle_callbacks_destroy(struct android_lifecycle_callbacks **ptr_callbacks)
{
	if (ptr_callbacks == nullptr) {
		return;
	}
	struct android_lifecycle_callbacks *alc = *ptr_callbacks;
	if (alc == nullptr) {
		return;
	}

	alc->listener.unregisterCallback(Activity((jobject)xrt_instance_android_get_context(alc->instance_android)));
	delete alc;
	*ptr_callbacks = nullptr;
}
