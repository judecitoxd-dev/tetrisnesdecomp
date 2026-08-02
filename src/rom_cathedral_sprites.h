#ifndef TETRIS_ROM_CATHEDRAL_SPRITES_H
#define TETRIS_ROM_CATHEDRAL_SPRITES_H

#include "cathedral.h"
#include "rom.h"

#include <SDL.h>
#include <stdbool.h>

bool tetris_rom_cathedral_sprites_load(SDL_Renderer *renderer,
                                       const NesRom *rom);
void tetris_rom_cathedral_sprites_free(void);
bool tetris_rom_cathedral_sprite_available(unsigned sprite_index);
void tetris_rom_cathedral_sprite_render(SDL_Renderer *renderer,
                                        unsigned sprite_index,
                                        int nes_x, int nes_y);
bool tetris_rom_cathedral_snapshot(int level, int start_height,
                                    unsigned frame,
                                    TetrisCathedralSnapshot *snapshot);

#endif
