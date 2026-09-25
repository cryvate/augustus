package com.github.Keriew.augustus;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.view.OrientationEventListener;

import org.libsdl.app.SDLActivity;

public class AugustusMainActivity extends SDLActivity {
    private static final int GET_FOLDER_RESULT = 500;
    private int lastOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
    private OrientationEventListener orientationEventListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        orientationEventListener = new OrientationEventListener(this) {
            @Override
            public void onOrientationChanged(int orientation) {
                if (orientation == ORIENTATION_UNKNOWN) return;

                if ((orientation >= 60 && orientation <= 120) || (orientation >= 240 && orientation <= 300)) {
                    if (getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE) {
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
                        lastOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
                    }
                } else if ((orientation >= 330 || orientation <= 30) || (orientation >= 150 && orientation <= 210)) {
                    if (getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT) {
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
                        lastOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
                    }
                }
            }
        };
    }

    @Override
    protected void onPause() {
        if (orientationEventListener != null) {
            orientationEventListener.disable();
        }
        int currentOrientation = getResources().getConfiguration().orientation;
        if (currentOrientation == Configuration.ORIENTATION_LANDSCAPE) {
            lastOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
        } else if (currentOrientation == Configuration.ORIENTATION_PORTRAIT) {
            lastOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        if (lastOrientation != ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED) {
            setRequestedOrientation(lastOrientation);
        }
        super.onResume();
        if (orientationEventListener != null && orientationEventListener.canDetectOrientation()) {
            orientationEventListener.enable();
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
