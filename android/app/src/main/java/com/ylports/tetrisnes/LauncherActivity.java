package com.ylports.tetrisnes;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.CRC32;

public final class LauncherActivity extends Activity {
    public static final String ROM_FILE_NAME = "Tetris (USA).nes";
    public static final String EXTRA_FORCE_PICKER = "force_picker";
    private static final int REQUEST_ROM = 1001;
    private static final int EXPECTED_SIZE = 49168;
    private static final long EXPECTED_CRC32 = 0xD16EA396L;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        hideSystemUi();

        Uri incoming = getIntent() != null ? getIntent().getData() : null;
        if (incoming != null) {
            importRom(incoming);
            return;
        }

        File rom = romFile();
        boolean force = getIntent().getBooleanExtra(EXTRA_FORCE_PICKER, false);
        if (!force && rom.isFile() && rom.length() == EXPECTED_SIZE) {
            startGame();
        } else {
            openPicker();
        }
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

    private File romFile() {
        return new File(getFilesDir(), ROM_FILE_NAME);
    }

    private void openPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {
                "application/octet-stream", "application/x-nes-rom", "*/*"
        });
        startActivityForResult(intent, REQUEST_ROM);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_ROM) return;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            if (romFile().isFile()) startGame();
            else finish();
            return;
        }
        importRom(data.getData());
    }

    private void importRom(Uri uri) {
        try {
            byte[] bytes = readAll(uri);
            String validationError = validateRom(bytes);
            if (validationError != null) {
                showInvalidRom(validationError);
                return;
            }
            File target = romFile();
            File temporary = new File(getFilesDir(), ROM_FILE_NAME + ".tmp");
            try (FileOutputStream output = new FileOutputStream(temporary)) {
                output.write(bytes);
                output.getFD().sync();
            }
            if (target.exists() && !target.delete()) {
                throw new IOException("No se pudo reemplazar la ROM anterior.");
            }
            if (!temporary.renameTo(target)) {
                throw new IOException("No se pudo guardar la ROM importada.");
            }
            CRC32 crc = new CRC32();
            crc.update(bytes);
            if (crc.getValue() != EXPECTED_CRC32) {
                Toast.makeText(this,
                        "ROM compatible, pero no coincide con el volcado probado D16EA396.",
                        Toast.LENGTH_LONG).show();
            }
            startGame();
        } catch (Exception exception) {
            showInvalidRom("No se pudo leer el archivo: " + exception.getMessage());
        }
    }

    private byte[] readAll(Uri uri) throws IOException {
        try (InputStream input = getContentResolver().openInputStream(uri);
             ByteArrayOutputStream output = new ByteArrayOutputStream(EXPECTED_SIZE)) {
            if (input == null) throw new IOException("El proveedor no abrió el archivo.");
            byte[] buffer = new byte[8192];
            int read;
            while ((read = input.read(buffer)) >= 0) {
                output.write(buffer, 0, read);
                if (output.size() > 1024 * 1024) {
                    throw new IOException("El archivo es demasiado grande para esta ROM.");
                }
            }
            return output.toByteArray();
        }
    }

    private String validateRom(byte[] data) {
        if (data.length != EXPECTED_SIZE) {
            return "Tamaño inesperado: " + data.length + " bytes; se esperaban "
                    + EXPECTED_SIZE + ".";
        }
        if (data[0] != 'N' || data[1] != 'E' || data[2] != 'S' || data[3] != 0x1A) {
            return "El archivo no tiene una cabecera iNES válida.";
        }
        int prgBanks = data[4] & 0xFF;
        int chrBanks = data[5] & 0xFF;
        int mapper = ((data[6] & 0xF0) >>> 4) | (data[7] & 0xF0);
        if (prgBanks != 2 || chrBanks != 2 || mapper != 1) {
            return "La ROM debe usar 32 KiB PRG, 16 KiB CHR y mapper MMC1.";
        }
        return null;
    }

    private void showInvalidRom(String message) {
        new AlertDialog.Builder(this)
                .setTitle("ROM no compatible")
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("Elegir otra", (dialog, which) -> openPicker())
                .setNegativeButton("Salir", (dialog, which) -> finish())
                .show();
    }

    private void startGame() {
        Intent game = new Intent(this, TetrisActivity.class);
        game.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(game);
        finish();
    }
}
