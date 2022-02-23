// Copyright 2022, Qualcomm Innovation Center, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Class that listens to activity lifecycle change event.
 * @author Jarvis Huang
 * @ingroup aux_android_java
 */
package org.freedesktop.monado.auxiliary

import android.app.Activity
import android.app.Application
import android.os.Bundle
import android.util.Log
import androidx.annotation.Keep

/** Monitor activity lifecycle of application. */
@Keep
class ActivityLifecycleListener(private val nativePtr: Long) :
    Application.ActivityLifecycleCallbacks {

    /** Register callback with [Application] from given [activity] object. */
    fun registerCallback(activity: Activity) {
        activity.application.registerActivityLifecycleCallbacks(this)
    }

    /** Unregister callback with [Application] from given [activity] object. */
    fun unregisterCallback(activity: Activity) {
        activity.application.unregisterActivityLifecycleCallbacks(this)
    }

    override fun onActivityCreated(activity: Activity, savedInstanceState: Bundle?) {
        Log.i(TAG, "$activity onActivityCreated")
        nativeOnActivityCreated(nativePtr, activity)
    }

    override fun onActivityStarted(activity: Activity) {
        Log.i(TAG, "$activity onActivityStarted")
        nativeOnActivityStarted(nativePtr, activity)
    }

    override fun onActivityResumed(activity: Activity) {
        Log.i(TAG, "$activity onActivityResumed")
        nativeOnActivityResumed(nativePtr, activity)
    }

    override fun onActivityPaused(activity: Activity) {
        Log.i(TAG, "$activity onActivityPaused")
        nativeOnActivityPaused(nativePtr, activity)
    }

    override fun onActivityStopped(activity: Activity) {
        Log.i(TAG, "$activity onActivityStopped")
        nativeOnActivityStopped(nativePtr, activity)
    }

    override fun onActivitySaveInstanceState(activity: Activity, outState: Bundle) {
        Log.i(TAG, "$activity onActivitySaveInstanceState")
        nativeOnActivitySaveInstanceState(nativePtr, activity)
    }

    override fun onActivityDestroyed(activity: Activity) {
        Log.i(TAG, "$activity onActivityDestroyed")
        nativeOnActivityDestroyed(nativePtr, activity)
    }

    private external fun nativeOnActivityCreated(nativePtr: Long, activity: Activity)

    private external fun nativeOnActivityStarted(nativePtr: Long, activity: Activity)

    private external fun nativeOnActivityResumed(nativePtr: Long, activity: Activity)

    private external fun nativeOnActivityPaused(nativePtr: Long, activity: Activity)

    private external fun nativeOnActivityStopped(nativePtr: Long, activity: Activity)

    private external fun nativeOnActivitySaveInstanceState(nativePtr: Long, activity: Activity)

    private external fun nativeOnActivityDestroyed(nativePtr: Long, activity: Activity)

    companion object {
        const val TAG = "ActivityLifecycleListener"
    }
}
