package com.github.Keriew.augustus;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.view.OrientationEventListener;

import org.libsdl.app.SDLActivity;

public class AugustusMainActivity extends SDLActivity {
    private static final int GET_FOLDER_RESULT = 500;
    private static final String PREFS_NAME = "AugustusPrefs";
    private static final String KEY_BACKGROUND_ORIENTATION = "background_orientation";

    private OrientationEventListener orientationEventListener;
    private boolean isRestoringOrientation = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initOrientationListener();
    }

    @SuppressWarnings("SourceLockedOrientationActivity")
    private void initOrientationListener() {
        orientationEventListener = new OrientationEventListener(this) {
            @Override
            public void onOrientationChanged(int orientationDeg) {
                if (orientationDeg == OrientationEventListener.ORIENTATION_UNKNOWN || isAutoRotateEnabled()) {
                    return;
                }

                int currentAppOrientation = getResources().getConfiguration().orientation;
                boolean isPhysicalPortrait = (orientationDeg >= 315 || orientationDeg < 45) || (orientationDeg >= 135 && orientationDeg < 225);

                if (currentAppOrientation == Configuration.ORIENTATION_LANDSCAPE) {
                    if (isPhysicalPortrait) {
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_USER);
                    } else {
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE);
                    }
                } else if (currentAppOrientation == Configuration.ORIENTATION_PORTRAIT) {
                    if (!isPhysicalPortrait) {
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_USER);
                    } else {
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT);
                    }
                }
            }
        };
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (orientationEventListener != null) {
            orientationEventListener.disable();
        }
        saveBackgroundOrientation();
    }

    @Override
    protected void onResume() {
        super.onResume();
        restoreBackgroundOrientation();
        if (orientationEventListener != null && orientationEventListener.canDetectOrientation()) {
            orientationEventListener.enable();
        }
    }

    @Override
    @SuppressWarnings("SourceLockedOrientationActivity")
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (isRestoringOrientation) {
            isRestoringOrientation = false;
            return;
        }
        if (!isAutoRotateEnabled()) {
            if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE);
            } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT);
            }
        }
    }

    private boolean isAutoRotateEnabled() {
        try {
            return Settings.System.getInt(
                    getContentResolver(),
                    Settings.System.ACCELEROMETER_ROTATION
            ) == 1;
        } catch (Settings.SettingNotFoundException e) {
            return false;
        }
    }

    private void saveBackgroundOrientation() {
        int orientation = getResources().getConfiguration().orientation;
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        prefs.edit().putInt(KEY_BACKGROUND_ORIENTATION, orientation).apply();
    }

    @SuppressWarnings("SourceLockedOrientationActivity")
    private void restoreBackgroundOrientation() {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        int savedOrientation = prefs.getInt(KEY_BACKGROUND_ORIENTATION, Configuration.ORIENTATION_UNDEFINED);
        int currentOrientation = getResources().getConfiguration().orientation;

        if (savedOrientation == Configuration.ORIENTATION_LANDSCAPE) {
            if (currentOrientation != Configuration.ORIENTATION_LANDSCAPE) {
                isRestoringOrientation = true;
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE);
            } else {
                isRestoringOrientation = false;
                if (!isAutoRotateEnabled()) {
                    setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE);
                } else {
                    setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_USER);
                }
            }
        } else if (savedOrientation == Configuration.ORIENTATION_PORTRAIT) {
            if (currentOrientation != Configuration.ORIENTATION_PORTRAIT) {
                isRestoringOrientation = true;
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT);
            } else {
                isRestoringOrientation = false;
                if (!isAutoRotateEnabled()) {
                    setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT);
                } else {
                    setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_USER);
                }
            }
        } else {
            isRestoringOrientation = false;
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_USER);
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        releaseAssetManager();
        FileManager.clearCache();
    }

    @Override
    protected String[] getLibraries() {
        String[] parentLibs = super.getLibraries();
        if (parentLibs.length > 0 && parentLibs[0].equals("SDL3")) {
             return new String[]{
                "SDL3",
                "SDL3_mixer",
                "augustus"
            };
        } else {
            return new String[]{
                "SDL2",
                "SDL2_mixer",
                "augustus"
            };
        }
    }

    @SuppressWarnings("unused")
    public void showDirectorySelection(boolean again) {
        startActivityForResult(DirectorySelectionActivity.newIntent(this, again), GET_FOLDER_RESULT);
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == GET_FOLDER_RESULT) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null) {
                FileManager.setBaseUri(data.getData());
            } else {
                FileManager.setBaseUri(Uri.EMPTY);
            }
            gotDirectory();
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    @SuppressWarnings("unused")
    public float getScreenDensity() {
        return getResources().getDisplayMetrics().density;
    }

    private native void gotDirectory();
    private native void releaseAssetManager();
}
