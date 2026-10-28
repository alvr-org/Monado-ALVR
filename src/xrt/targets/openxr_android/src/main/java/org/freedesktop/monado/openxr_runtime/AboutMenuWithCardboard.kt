// Copyright 2020-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Menu with QR code scanner for Cardboard.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Simon Zeni <simon.zeni@collabora.com>
 */

package org.freedesktop.monado.openxr_runtime

import android.content.Context
import android.content.Intent
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import javax.inject.Inject
import org.freedesktop.monado.android_common.AboutMenuProvider
import org.freedesktop.monado.auxiliary.ScannerProvider

/** An about menu that has a QR code scanner to set up Cardboard parameters. */
class AboutMenuWithCardboard @Inject constructor(private val scanner: ScannerProvider) :
    AboutMenuProvider {

    override fun onCreateOptionsMenu(menuInflater: MenuInflater, menu: Menu?) {
        menuInflater.inflate(R.menu.menu_runtime, menu)
        menu ?: return
        menu.findItem(R.id.qrscan).icon = scanner.getIcon()
    }

    override fun onOptionsItemSelected(context: Context, item: MenuItem): Boolean {
        if (item.itemId == R.id.qrscan) {
            val scannerIntent: Intent = scanner.makeScannerIntent()
            context.startActivity(scannerIntent)
            return true
        }
        return false
    }
}
