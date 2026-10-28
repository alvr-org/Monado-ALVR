// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface for target-specific scanner-related things on Android.
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @ingroup aux_android
 */

package org.freedesktop.monado.auxiliary

import android.content.Intent
import android.graphics.drawable.Drawable

/**
 * Scanner activity. This interface must be provided by any Android "XRT Target".
 *
 * Intended for use in dependency injection.
 */
interface ScannerProvider {
    /** Get a drawable icon for use the UI, for the runtime/Monado-incorporating target. */
    fun getIcon(): Drawable

    /** Make a {@code Intent} to launch a scanner activity, if provided by the target. */
    fun makeScannerIntent(): Intent
}
