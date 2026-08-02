package com.ylports.tetrisnes;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;

import org.libsdl.app.SDLActivity;

import java.io.File;

public final class TetrisActivity extends SDLActivity {
    @Override
    protected String[] getArguments() {
        File rom = new File(getFilesDir(), LauncherActivity.ROM_FILE_NAME);
        return new String[] {"--rom", rom.getAbsolutePath()};
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideSystemUi();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemUi();
    }

    private void hideSystemUi() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }

    /** Called from the shared C frontend through SDL's JNI bridge. */
    public void requestRomSelection() {
        runOnUiThread(() -> {
            Intent picker = new Intent(this, LauncherActivity.class);
            picker.putExtra(LauncherActivity.EXTRA_FORCE_PICKER, true);
            picker.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(picker);
            finish();
        });
    }
}
