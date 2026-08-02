#ifndef TETRIS_APP_H
#define TETRIS_APP_H

#include "audio.h"
#include "game.h"
#include "highscores.h"
#include "name_entry.h"
#include "rom.h"
#include "rom_cathedral_sprites.h"
#include "rom_endings.h"
#include "rom_type_a.h"
#include "settings.h"

#include <SDL.h>
#include <stdbool.h>
#include <stddef.h>

#define LOGICAL_W 640
#define LOGICAL_H 480
#define CELL 20
#define BOARD_X 220
#define BOARD_Y 40

typedef enum AppScreen {
    SCREEN_TITLE = 0,
    SCREEN_TYPE_SELECT,
    SCREEN_LEVEL_SELECT,
    SCREEN_RECORDS,
    SCREEN_OPTIONS,
    SCREEN_NAME_ENTRY,
    SCREEN_ENDING,
    SCREEN_GAME,
    SCREEN_DEMO,
    SCREEN_REPLAY
} AppScreen;

typedef struct AppMenuState {
    TetrisMode mode;
    int level;
    int height;
    bool selecting_height;
} AppMenuState;

typedef struct AppOptionsState {
    int selected;
} AppOptionsState;

typedef struct AppResultState {
    TetrisNameEntry name;
    TetrisMode mode;
    int score;
    int lines;
    int level;
    int height;
    int rank;
    bool completed;
    Uint32 ending_started_ms;
} AppResultState;

typedef struct PendingInput {
    bool rotate_cw;
    bool rotate_ccw;
    bool hard_drop;
    bool pause;
    bool restart;
    bool toggle_next;
} PendingInput;

void render(SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
            const TetrisGame *game, const AppMenuState *menu,
            const AppResultState *result, const AppOptionsState *options,
            const TetrisSettings *settings, const char *replay_status,
            bool non_exact_rom, const TetrisAudio *audio, const NesRom *rom,
            const TetrisHighScores *scores);
bool load_rom_and_font(SDL_Renderer *renderer, const char *path,
                       NesRom *rom, SDL_Texture **font);
bool load_rom_and_font_ex(SDL_Renderer *renderer, const char *path,
                          NesRom *rom, SDL_Texture **font, bool show_error,
                          char *error, size_t error_size);
void free_rom_graphics(void);
SDL_GameController *open_first_controller(void);
void clear_held(bool *left, bool *right, bool *down);
void begin_game(TetrisGame *game, const AppMenuState *menu);
void change_menu_value(AppMenuState *menu, int delta);
void toggle_menu_mode(AppMenuState *menu);
void app_result_begin(AppResultState *result, const TetrisGame *game, int rank);

#endif
