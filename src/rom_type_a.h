#ifndef TETRIS_ROM_TYPE_A_H
#define TETRIS_ROM_TYPE_A_H

#include "rom.h"

#include <SDL.h>
#include <stdbool.h>

bool tetris_rom_type_a_load(SDL_Renderer *renderer, const NesRom *rom);
void tetris_rom_type_a_free(void);
bool tetris_rom_type_a_backgrounds_load(SDL_Renderer *renderer,
                                        const NesRom *rom);
void tetris_rom_type_a_backgrounds_free(void);
bool tetris_rom_type_a_background_available(bool over_120k);
void tetris_rom_type_a_background_render(SDL_Renderer *renderer,
                                         bool over_120k);
bool tetris_rom_type_a_sprite_available(unsigned sprite_index);
void tetris_rom_type_a_sprite_render(SDL_Renderer *renderer,
                                     unsigned sprite_index,
                                     int nes_x, int nes_y);

#endif
