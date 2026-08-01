#ifndef TETRIS_DEMO_H
#define TETRIS_DEMO_H

#include "game.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct TetrisDemoController {
    uint8_t observed_spawn_count;
    int target_rotation;
    int target_x;
    bool has_plan;
} TetrisDemoController;

void tetris_demo_reset(TetrisDemoController *demo);
TetrisInput tetris_demo_next_input(TetrisDemoController *demo,
                                   const TetrisGame *game);

#endif
