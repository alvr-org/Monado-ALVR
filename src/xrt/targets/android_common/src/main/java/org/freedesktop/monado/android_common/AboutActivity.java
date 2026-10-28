// Copyright 2020-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Simple main activity for Android.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Simon Zeni <simon.zeni@collabora.com>
 */

package org.freedesktop.monado.android_common;

import android.os.Bundle;
import android.text.method.LinkMovementMethod;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;
import dagger.hilt.android.AndroidEntryPoint;
import java.util.Optional;
import javax.inject.Inject;
import org.freedesktop.monado.auxiliary.NameAndLogoProvider;
import org.freedesktop.monado.auxiliary.UiProvider;

@AndroidEntryPoint
public class AboutActivity extends AppCompatActivity {

    @Inject NoticeFragmentProvider noticeFragmentProvider;

    @Inject UiProvider uiProvider;

    @Inject NameAndLogoProvider nameAndLogoProvider;

    /**
     * @noinspection OptionalUsedAsFieldOrParameterType
     */
    @Inject Optional<AboutMenuProvider> aboutMenuProvider;

    private boolean isInProcessBuild() {
        try {
            getClassLoader().loadClass("org/freedesktop/monado/ipc/Client");
            return false;
        } catch (ClassNotFoundException e) {
            // ok, we're in-process.
        }
        return true;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);

        // Default to dark mode universally?
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);

        setSupportActionBar(findViewById(R.id.toolbar));

        // Make our Monado link clickable
        ((TextView) findViewById(R.id.textPowered))
                .setMovementMethod(LinkMovementMethod.getInstance());

        // Branding from the branding provider
        ((TextView) findViewById(R.id.textName))
                .setText(nameAndLogoProvider.getLocalizedRuntimeName());
        ((ImageView) findViewById(R.id.imageView))
                .setImageDrawable(nameAndLogoProvider.getLogoDrawable());

        boolean isInProcess = isInProcessBuild();
        if (!isInProcess) {
            ShutdownProcess.Companion.setupRuntimeShutdownButton(this);
        }

        // Start doing fragments
        FragmentTransaction fragmentTransaction = getSupportFragmentManager().beginTransaction();

        @VrModeStatus.Status
        int status =
                VrModeStatus.detectStatus(
                        this, getApplicationContext().getApplicationInfo().packageName);

        VrModeStatus statusFrag = VrModeStatus.newInstance(status);
        fragmentTransaction.add(R.id.statusFrame, statusFrag, null);

        if (!isInProcess) {
            findViewById(R.id.drawOverOtherAppsFrame).setVisibility(View.VISIBLE);
            DisplayOverOtherAppsStatusFragment drawOverFragment =
                    new DisplayOverOtherAppsStatusFragment();
            fragmentTransaction.replace(R.id.drawOverOtherAppsFrame, drawOverFragment, null);
        }

        if (noticeFragmentProvider != null) {
            Fragment noticeFragment = noticeFragmentProvider.makeNoticeFragment();
            fragmentTransaction.add(R.id.aboutFrame, noticeFragment, null);
        }

        fragmentTransaction.commit();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        aboutMenuProvider.ifPresent(
                menuProvider -> menuProvider.onCreateOptionsMenu(getMenuInflater(), menu));
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        Optional<AboutMenuProvider> aboutMenu = aboutMenuProvider;
        if (aboutMenu.isPresent()) {
            if (aboutMenu.get().onOptionsItemSelected(this, item)) {
                // handled
                return true;
            }
        }

        return super.onOptionsItemSelected(item);
    }
}
