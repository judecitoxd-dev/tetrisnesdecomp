#ifndef TETRIS_SETTINGS_H
#define TETRIS_SETTINGS_H

#include "game.h"

#include <stdbool.h>
#include <stddef.h>

#define TETRIS_SETTINGS_PATH_LENGTH 1023
#define TETRIS_TOUCH_SETTING_COUNT 11

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
    int touch_opacity;
    int touch_scale;
    int touch_x[TETRIS_TOUCH_SETTING_COUNT];
    int touch_y[TETRIS_TOUCH_SETTING_COUNT];
    char rom_path[TETRIS_SETTINGS_PATH_LENGTH + 1];
} TetrisSettings;

void tetris_settings_init(TetrisSettings *settings);
void tetris_settings_sanitize(TetrisSettings *settings);
void tetris_settings_fit_window_4_3(int *width, int *height);
bool tetris_settings_load(TetrisSettings *settings, const char *path);
bool tetris_settings_save(const TetrisSettings *settings, const char *path);
void tetris_settings_set_rom_path(TetrisSettings *settings, const char *path);

/* Original type/menu behavior: levels 0-9 and music 1,2,3,OFF. */
int tetris_settings_step_level(int current, int delta);
int tetris_settings_step_music(int current, int direction);

/* Compatibility inside settings.c; touch_controls.h undefines it before its enum. */
#define TETRIS_TOUCH_ACTION_COUNT TETRIS_TOUCH_SETTING_COUNT

#endif
