// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Simple implementation of ScannerProvider.
 * @author Simon Zeni <simon.zeni@collabora.com>
 */
package org.freedesktop.monado.openxr_runtime

import android.content.Context
import android.content.Intent
import android.graphics.drawable.Drawable
import androidx.core.content.ContextCompat
import dagger.hilt.android.qualifiers.ApplicationContext
import javax.inject.Inject
import org.freedesktop.monado.auxiliary.ScannerProvider
import org.freedesktop.monado.auxiliary.cardboard.QrScannerActivity

class CardboardQRCodeScannerProvider @Inject constructor(@ApplicationContext val context: Context) :
    ScannerProvider {

    override fun getIcon(): Drawable =
        ContextCompat.getDrawable(context, R.drawable.cardboard_oss_qr)!!

    override fun makeScannerIntent(): Intent = Intent(context, QrScannerActivity::class.java)
}
