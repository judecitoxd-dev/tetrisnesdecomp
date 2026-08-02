#ifndef TETRIS_ROM_ENDINGS_H
#define TETRIS_ROM_ENDINGS_H

#include "rom.h"

#include <SDL.h>
#include <stdbool.h>

bool tetris_rom_endings_load(SDL_Renderer *renderer, const NesRom *rom);
void tetris_rom_endings_free(void);
bool tetris_rom_concert_sprite_available(unsigned sprite_index);
void tetris_rom_concert_sprite_render(SDL_Renderer *renderer,
                                      unsigned sprite_index,
                                      int nes_x, int nes_y);
void tetris_rom_concert_render(SDL_Renderer *renderer,
                               int start_height, unsigned frame);
void tetris_rom_cathedral_render(SDL_Renderer *renderer,
                                 int level, int start_height,
                                 unsigned frame);

#endif
