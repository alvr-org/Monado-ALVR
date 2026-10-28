// Copyright 2020-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Module that binds all the optional Cardboard-related dependencies we inject with Hilt.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

package org.freedesktop.monado.openxr_runtime

import dagger.Binds
import dagger.Module
import dagger.hilt.InstallIn
import dagger.hilt.android.components.ActivityComponent
import org.freedesktop.monado.android_common.AboutMenuProvider
import org.freedesktop.monado.auxiliary.ScannerProvider

@Module
@InstallIn(ActivityComponent::class)
abstract class CardboardModule {
    // Provide the optional menu provider, to access our QR code scanner for Cardboard
    @Binds abstract fun bindAboutMenu(aboutMenuProvider: AboutMenuWithCardboard): AboutMenuProvider

    // which itself needs a scanner
    @Binds
    abstract fun bindScanner(scannerProvider: CardboardQRCodeScannerProvider): ScannerProvider
}
