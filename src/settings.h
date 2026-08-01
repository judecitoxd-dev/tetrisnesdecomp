#ifndef TETRIS_SETTINGS_H
#define TETRIS_SETTINGS_H

#include "game.h"

#include <stdbool.h>
#include <stddef.h>

#define TETRIS_SETTINGS_PATH_LENGTH 1023

typedef struct TetrisSettings {
    bool audio_enabled;
    int music_track;
    bool fullscreen;
    bool integer_scale;
    bool demo_enabled;
    bool hard_drop_enabled;
    bool show_next;
    TetrisMode last_mode;
    int last_level;
    int last_height;
    int window_width;
    int window_height;
    char rom_path[TETRIS_SETTINGS_PATH_LENGTH + 1];
} TetrisSettings;

void tetris_settings_init(TetrisSettings *settings);
void tetris_settings_sanitize(TetrisSettings *settings);
bool tetris_settings_load(TetrisSettings *settings, const char *path);
bool tetris_settings_save(const TetrisSettings *settings, const char *path);
void tetris_settings_set_rom_path(TetrisSettings *settings, const char *path);

#endif
