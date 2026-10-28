// Copyright 2020-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Dep injection module for when you do not want a menu in the about activity
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Simon Zeni <simon.zeni@collabora.com>
 */
package org.freedesktop.monado.android_common;

import dagger.BindsOptionalOf;
import dagger.Module;
import dagger.hilt.InstallIn;
import dagger.hilt.android.components.ActivityComponent;

/**
 * Lets you optionally **not** provide a menu.
 *
 * @noinspection unused
 */
@Module
@InstallIn(ActivityComponent.class)
public interface OptionalMenuModule {
    @BindsOptionalOf
    AboutMenuProvider bindOptionalAboutMenu();
}
