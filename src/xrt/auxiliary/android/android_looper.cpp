// Copyright 2021, Qualcomm Innovation Center, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of android looper functions.
 * @author Jarvis Huang
 * @ingroup aux_android
 */

#include "android_looper.h"

#include "util/u_logging.h"
#include "wrap/android.app.h"

#include <android_native_app_glue.h>
#include <android/looper.h>

void
android_looper_poll_until_activity_resumed(struct _JavaVM *vm, void *activity)
{
	jni::init(vm);
	wrap::android::app::Activity activityObject{(jobject)activity};
	if (!jni::env()->IsInstanceOf(activityObject.object().getHandle(),
	                              jni::Class("android/app/NativeActivity").getHandle())) {
		// skip if given activity is not android.app.NativeActivity
		U_LOG_I("Activity is not NativeActivity, skip");
		return;
	}

	// Activity is in resumed state if window is active. Check Activity#onPostResume for detail.
	if (!activityObject.getWindow().isNull() && activityObject.getWindow().call<bool>("isActive()Z")) {
		// Already in resume state, skip
		U_LOG_I("Activity is NativeActivity and already in resume state with window available, skip");
		return;
	}

	struct android_poll_source *source;
	while (ALooper_pollOnce(1000, NULL, NULL, (void **)&source) >= 0) {
		if (source) {
			// Let callback owner handle the event
			source->process(source->app, source);
			if (source->app->destroyRequested != 0)
				break;
			if (source->app->activityState == APP_CMD_RESUME && source->app->window) {
				U_LOG_I("Activity is in resume state with window available now");
				break;
			}
		}
	}
}
