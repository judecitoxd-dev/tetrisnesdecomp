#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool parse_bool(const char *value, bool fallback) {
    if (!value) return fallback;
    if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
        strcmp(value, "on") == 0 || strcmp(value, "yes") == 0) return true;
    if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 ||
        strcmp(value, "off") == 0 || strcmp(value, "no") == 0) return false;
    return fallback;
}

static int parse_int(const char *value, int fallback) {
    char *end = NULL;
    long parsed;
    if (!value || !*value) return fallback;
    parsed = strtol(value, &end, 10);
    if (!end || *end != '\0') return fallback;
    if (parsed < -2147483647L || parsed > 2147483647L) return fallback;
    return (int)parsed;
}

static void trim_line(char *line) {
    size_t length;
    if (!line) return;
    length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[--length] = '\0';
    }
}

static int normalize_menu_level(int level) {
    if (level < 0) return 0;
    /* v0.18 could persist 10-19 while drawing only the low decimal digit. */
    if (level > 9) return level % 10;
    return level;
}

int tetris_settings_step_level(int current, int delta) {
    long value = (long)normalize_menu_level(current) + (long)delta;
    if (value < 0) value = 0;
    if (value > 9) value = 9;
    return (int)value;
}

int tetris_settings_step_music(int current, int direction) {
    if (current < -1 || current > 2) current = 0;
    if (direction > 0) {
        if (current < 0) return -1;
        if (current < 2) return current + 1;
        return -1;
    }
    if (direction < 0) {
        if (current < 0) return 2;
        if (current > 0) return current - 1;
        return 0;
    }
    return current;
}

void tetris_settings_init(TetrisSettings *settings) {
    int index;
    if (!settings) return;
    memset(settings, 0, sizeof(*settings));
    settings->audio_enabled = true;
    settings->music_track = 0;
    settings->integer_scale = true;
    settings->demo_enabled = true;
    settings->hard_drop_enabled = true;
    settings->show_next = true;
    settings->last_mode = TETRIS_MODE_A;
    settings->window_width = 960;
    settings->window_height = 720;
    settings->touch_opacity = 58;
    settings->touch_scale = 100;
    for (index = 0; index < TETRIS_TOUCH_ACTION_COUNT; ++index) {
        settings->touch_x[index] = -1;
        settings->touch_y[index] = -1;
    }
}

void tetris_settings_sanitize(TetrisSettings *settings) {
    int index;
    if (!settings) return;
    if (settings->music_track < -1) settings->music_track = -1;
    if (settings->music_track > 2) settings->music_track = 2;
    if (settings->last_mode != TETRIS_MODE_B) settings->last_mode = TETRIS_MODE_A;
    settings->last_level = normalize_menu_level(settings->last_level);
    if (settings->last_height < 0) settings->last_height = 0;
    if (settings->last_height > 5) settings->last_height = 5;
    if (settings->window_width < 640) settings->window_width = 640;
    if (settings->window_width > 7680) settings->window_width = 7680;
    if (settings->window_height < 480) settings->window_height = 480;
    if (settings->window_height > 4320) settings->window_height = 4320;
    if (settings->touch_opacity < 20) settings->touch_opacity = 20;
    if (settings->touch_opacity > 100) settings->touch_opacity = 100;
    if (settings->touch_scale < 60) settings->touch_scale = 60;
    if (settings->touch_scale > 160) settings->touch_scale = 160;
    for (index = 0; index < TETRIS_TOUCH_ACTION_COUNT; ++index) {
        if (settings->touch_x[index] < -1 || settings->touch_x[index] > 1000)
            settings->touch_x[index] = -1;
        if (settings->touch_y[index] < -1 || settings->touch_y[index] > 1000)
            settings->touch_y[index] = -1;
    }
    settings->rom_path[TETRIS_SETTINGS_PATH_LENGTH] = '\0';
}

void tetris_settings_set_rom_path(TetrisSettings *settings, const char *path) {
    size_t index;
    if (!settings) return;
    if (!path) path = "";
    snprintf(settings->rom_path, sizeof(settings->rom_path), "%s", path);
    for (index = 0; settings->rom_path[index] != '\0'; ++index) {
        if (settings->rom_path[index] == '\n' || settings->rom_path[index] == '\r')
            settings->rom_path[index] = ' ';
    }
}

bool tetris_settings_load(TetrisSettings *settings, const char *path) {
    FILE *file;
    char line[2048];
    if (!settings || !path) return false;
    file = fopen(path, "rb");
    if (!file) return false;
    while (fgets(line, sizeof(line), file)) {
        char *equals;
        char *key;
        char *value;
        trim_line(line);
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') continue;
        equals = strchr(line, '=');
        if (!equals) continue;
        *equals = '\0';
        key = line;
        value = equals + 1;
        if (strcmp(key, "audio_enabled") == 0)
            settings->audio_enabled = parse_bool(value, settings->audio_enabled);
        else if (strcmp(key, "music_track") == 0)
            settings->music_track = parse_int(value, settings->music_track);
        else if (strcmp(key, "fullscreen") == 0)
            settings->fullscreen = parse_bool(value, settings->fullscreen);
        else if (strcmp(key, "integer_scale") == 0)
            settings->integer_scale = parse_bool(value, settings->integer_scale);
        else if (strcmp(key, "demo_enabled") == 0)
            settings->demo_enabled = parse_bool(value, settings->demo_enabled);
        else if (strcmp(key, "hard_drop_enabled") == 0)
            settings->hard_drop_enabled = parse_bool(value, settings->hard_drop_enabled);
        else if (strcmp(key, "show_next") == 0)
            settings->show_next = parse_bool(value, settings->show_next);
        else if (strcmp(key, "last_mode") == 0)
            settings->last_mode = parse_int(value, settings->last_mode) == 1
                ? TETRIS_MODE_B : TETRIS_MODE_A;
        else if (strcmp(key, "last_level") == 0)
            settings->last_level = parse_int(value, settings->last_level);
        else if (strcmp(key, "last_height") == 0)
            settings->last_height = parse_int(value, settings->last_height);
        else if (strcmp(key, "window_width") == 0)
            settings->window_width = parse_int(value, settings->window_width);
        else if (strcmp(key, "window_height") == 0)
            settings->window_height = parse_int(value, settings->window_height);
        else if (strcmp(key, "touch_opacity") == 0)
            settings->touch_opacity = parse_int(value, settings->touch_opacity);
        else if (strcmp(key, "touch_scale") == 0)
            settings->touch_scale = parse_int(value, settings->touch_scale);
        else if (strncmp(key, "touch_x_", 8) == 0) {
            int index = parse_int(key + 8, -1);
            if (index >= 0 && index < TETRIS_TOUCH_ACTION_COUNT)
                settings->touch_x[index] = parse_int(value, settings->touch_x[index]);
        } else if (strncmp(key, "touch_y_", 8) == 0) {
            int index = parse_int(key + 8, -1);
            if (index >= 0 && index < TETRIS_TOUCH_ACTION_COUNT)
                settings->touch_y[index] = parse_int(value, settings->touch_y[index]);
        } else if (strcmp(key, "rom_path") == 0)
            tetris_settings_set_rom_path(settings, value);
    }
    fclose(file);
    tetris_settings_sanitize(settings);
    return true;
}

bool tetris_settings_save(const TetrisSettings *settings, const char *path) {
    FILE *file;
    int index;
    char temporary[1400];
    if (!settings || !path || !*path) return false;
    if (snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= (int)sizeof(temporary))
        return false;
    file = fopen(temporary, "wb");
    if (!file) return false;
    fprintf(file, "# Tetris NES Port settings v3\n");
    fprintf(file, "audio_enabled=%d\n", settings->audio_enabled ? 1 : 0);
    fprintf(file, "music_track=%d\n", settings->music_track);
    fprintf(file, "fullscreen=%d\n", settings->fullscreen ? 1 : 0);
    fprintf(file, "integer_scale=%d\n", settings->integer_scale ? 1 : 0);
    fprintf(file, "demo_enabled=%d\n", settings->demo_enabled ? 1 : 0);
    fprintf(file, "hard_drop_enabled=%d\n", settings->hard_drop_enabled ? 1 : 0);
    fprintf(file, "show_next=%d\n", settings->show_next ? 1 : 0);
    fprintf(file, "last_mode=%d\n", settings->last_mode == TETRIS_MODE_B ? 1 : 0);
    fprintf(file, "last_level=%d\n", normalize_menu_level(settings->last_level));
    fprintf(file, "last_height=%d\n", settings->last_height);
    fprintf(file, "window_width=%d\n", settings->window_width);
    fprintf(file, "window_height=%d\n", settings->window_height);
    fprintf(file, "touch_opacity=%d\n", settings->touch_opacity);
    fprintf(file, "touch_scale=%d\n", settings->touch_scale);
    for (index = 0; index < TETRIS_TOUCH_ACTION_COUNT; ++index) {
        fprintf(file, "touch_x_%d=%d\n", index, settings->touch_x[index]);
        fprintf(file, "touch_y_%d=%d\n", index, settings->touch_y[index]);
    }
    fprintf(file, "rom_path=%s\n", settings->rom_path);
    if (fclose(file) != 0) {
        remove(temporary);
        return false;
    }
    if (rename(temporary, path) != 0) {
        remove(path);
        if (rename(temporary, path) != 0) {
            remove(temporary);
            return false;
        }
    }
    return true;
}
