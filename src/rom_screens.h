#ifndef TETRIS_ROM_SCREENS_H
#define TETRIS_ROM_SCREENS_H

#include "rom.h"

#include <SDL.h>
#include <stdbool.h>

typedef enum TetrisRomScreen {
    TETRIS_ROM_SCREEN_TYPE_MENU = 0,
    TETRIS_ROM_SCREEN_LEVEL_A,
    TETRIS_ROM_SCREEN_LEVEL_B,
    TETRIS_ROM_SCREEN_GAME,
    TETRIS_ROM_SCREEN_ENTER_HIGH_SCORE,
    TETRIS_ROM_SCREEN_HIGH_SCORES,
    TETRIS_ROM_SCREEN_B_ENDING_NORMAL,
    TETRIS_ROM_SCREEN_B_ENDING_CASTLE_H0,
    TETRIS_ROM_SCREEN_B_ENDING_CASTLE_H1,
    TETRIS_ROM_SCREEN_B_ENDING_CASTLE_H2,
    TETRIS_ROM_SCREEN_B_ENDING_CASTLE_H3,
    TETRIS_ROM_SCREEN_B_ENDING_CASTLE_H4,
    TETRIS_ROM_SCREEN_B_ENDING_CASTLE_H5,
    TETRIS_ROM_SCREEN_COUNT
} TetrisRomScreen;

/* Builds screen textures from PPU streams and CHR in the user's verified ROM. */
bool tetris_rom_screens_load(SDL_Renderer *renderer, const NesRom *rom);
void tetris_rom_screens_free(void);
bool tetris_rom_screen_available(TetrisRomScreen screen);
void tetris_rom_screen_render(SDL_Renderer *renderer, TetrisRomScreen screen);

#endif
