#ifndef TETRIS_APP_H
#define TETRIS_APP_H

#include "audio.h"
#include "game.h"
#include "rom.h"

#include <SDL.h>
#include <stdbool.h>

#define LOGICAL_W 640
#define LOGICAL_H 480
#define CELL 20
#define BOARD_X 220
#define BOARD_Y 40

typedef enum AppScreen {
    SCREEN_TITLE = 0,
    SCREEN_LEVEL_SELECT,
    SCREEN_GAME
} AppScreen;

typedef struct PendingInput {
    bool rotate_cw;
    bool rotate_ccw;
    bool hard_drop;
    bool pause;
    bool restart;
    bool toggle_next;
} PendingInput;

void render(SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
            const TetrisGame *game, int selected_level, bool non_exact_rom,
            const TetrisAudio *audio);
bool load_rom_and_font(SDL_Renderer *renderer, const char *path,
                       NesRom *rom, SDL_Texture **font);
SDL_GameController *open_first_controller(void);
void clear_held(bool *left, bool *right, bool *down);
void begin_game(TetrisGame *game, int level);
void change_level(int *level, int delta);

#endif
