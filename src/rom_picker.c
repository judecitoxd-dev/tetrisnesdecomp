#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include "rom_picker.h"

#include <stdio.h>
#include <string.h>

#ifdef __ANDROID__
#include "android_bridge.h"

bool tetris_pick_rom(char *path, size_t path_size) {
    if (path && path_size > 0) path[0] = '\0';
    (void)tetris_android_request_rom();
    return false;
}
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

bool tetris_pick_rom(char *path, size_t path_size) {
    OPENFILENAMEA dialog;
    char selected[1024] = {0};
    if (!path || path_size == 0) return false;
    memset(&dialog, 0, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = "NES ROM (*.nes)\0*.nes\0All files\0*.*\0";
    dialog.lpstrFile = selected;
    dialog.nMaxFile = (DWORD)sizeof(selected);
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrDefExt = "nes";
    if (!GetOpenFileNameA(&dialog)) return false;
    snprintf(path, path_size, "%s", selected);
    return true;
}
#else
#include <stdlib.h>

static bool run_picker_command(const char *command, char *path, size_t path_size) {
    FILE *pipe;
    char selected[2048];
    size_t length;
    pipe = popen(command, "r");
    if (!pipe) return false;
    if (!fgets(selected, sizeof(selected), pipe)) {
        (void)pclose(pipe);
        return false;
    }
    if (pclose(pipe) != 0) return false;
    length = strlen(selected);
    while (length > 0 && (selected[length - 1] == '\n' || selected[length - 1] == '\r'))
        selected[--length] = '\0';
    if (length == 0) return false;
    snprintf(path, path_size, "%s", selected);
    return true;
}

bool tetris_pick_rom(char *path, size_t path_size) {
    if (!path || path_size == 0) return false;
#ifdef __APPLE__
    return run_picker_command(
        "osascript -e 'POSIX path of (choose file with prompt \"Choose Tetris NES ROM\")' 2>/dev/null",
        path, path_size);
#else
    if (run_picker_command(
            "zenity --file-selection --title='Choose Tetris NES ROM' --file-filter='NES ROM (*.nes) | *.nes' 2>/dev/null",
            path, path_size)) return true;
    return run_picker_command(
        "kdialog --getopenfilename . '*.nes|NES ROM' 2>/dev/null", path, path_size);
#endif
}
#endif
