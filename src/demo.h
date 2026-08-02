#ifndef TETRIS_DEMO_H
#define TETRIS_DEMO_H

#include "game.h"
#include "rom.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct TetrisDemoController {
    uint8_t observed_spawn_count;
    int target_rotation;
    int target_x;
    bool has_plan;

    const uint8_t *button_data;
    const uint8_t *piece_data;
    size_t button_index;
    size_t piece_index;
    uint8_t held_buttons;
    uint8_t repeats;
    bool rom_script;
    bool finished;
} TetrisDemoController;

void tetris_demo_reset(TetrisDemoController *demo);
bool tetris_demo_reset_from_rom(TetrisDemoController *demo,
                                TetrisGame *game, const NesRom *rom);
void tetris_demo_sync_after_tick(TetrisDemoController *demo,
                                 TetrisGame *game);
bool tetris_demo_is_finished(const TetrisDemoController *demo);
bool tetris_demo_uses_rom_script(const TetrisDemoController *demo);
TetrisInput tetris_demo_next_input(TetrisDemoController *demo,
                                   TetrisGame *game);

#endif
