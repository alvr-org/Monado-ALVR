// Copyright 2020-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Dependency injection interface for about activity menu.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Simon Zeni <simon.zeni@collabora.com>
 */
package org.freedesktop.monado.android_common

import android.content.Context
import android.view.Menu
import android.view.MenuItem

/**
 * Menu handler for the about activity. This interface may be provided by any Android final target,
 * optionally.
 *
 * Intended for use in dependency injection, so you can add your own menu items
 */
interface AboutMenuProvider {
    fun onCreateOptionsMenu(menuInflater: android.view.MenuInflater, menu: Menu?)

    fun onOptionsItemSelected(context: Context, item: MenuItem): Boolean
}
