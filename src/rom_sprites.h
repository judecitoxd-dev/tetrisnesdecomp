#ifndef TETRIS_ROM_SPRITES_H
#define TETRIS_ROM_SPRITES_H

#include "rom.h"

#include <SDL.h>
#include <stdbool.h>

typedef enum TetrisRomSprite {
    TETRIS_ROM_SPRITE_LEVEL_CURSOR = 0,
    TETRIS_ROM_SPRITE_TYPE_CURSOR,
    TETRIS_ROM_SPRITE_HIGH_SCORE_CURSOR,
    TETRIS_ROM_SPRITE_COUNT
} TetrisRomSprite;

bool tetris_rom_sprites_load(SDL_Renderer *renderer, const NesRom *rom);
void tetris_rom_sprites_free(void);
bool tetris_rom_sprite_available(TetrisRomSprite sprite);
void tetris_rom_sprite_render(SDL_Renderer *renderer, TetrisRomSprite sprite,
                              int nes_x, int nes_y);

#endif
