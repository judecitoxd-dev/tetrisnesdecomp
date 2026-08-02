#ifndef TETRIS_CATHEDRAL_H
#define TETRIS_CATHEDRAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TETRIS_CATHEDRAL_LEVELS 10
#define TETRIS_CATHEDRAL_MAX_SPRITES 6

typedef struct TetrisCathedralTables {
    uint8_t animation_speed[TETRIS_CATHEDRAL_LEVELS];
    uint8_t frame_delay[TETRIS_CATHEDRAL_LEVELS];
    uint8_t start_x[TETRIS_CATHEDRAL_LEVELS];
    uint8_t sentinel_x[TETRIS_CATHEDRAL_LEVELS];
    int8_t vector_x[TETRIS_CATHEDRAL_LEVELS];
    uint8_t trigger_x[TETRIS_CATHEDRAL_LEVELS * TETRIS_CATHEDRAL_MAX_SPRITES];
    uint8_t position_y[TETRIS_CATHEDRAL_LEVELS * TETRIS_CATHEDRAL_MAX_SPRITES];
    uint8_t sprite_base[TETRIS_CATHEDRAL_LEVELS];
    bool valid;
} TetrisCathedralTables;

typedef struct TetrisCathedralSnapshot {
    unsigned sprite_index;
    int sprite_count;
    bool visible[TETRIS_CATHEDRAL_MAX_SPRITES];
    uint8_t x[TETRIS_CATHEDRAL_MAX_SPRITES];
    uint8_t y[TETRIS_CATHEDRAL_MAX_SPRITES];
} TetrisCathedralSnapshot;

/* Reads the eight movement tables from the verified PRG layout. */
bool tetris_cathedral_tables_load(TetrisCathedralTables *tables,
                                  const uint8_t *prg, size_t prg_size);

/* Reconstructs the state rendered at one B-Type cathedral frame. */
void tetris_cathedral_snapshot(const TetrisCathedralTables *tables,
                               int level, int start_height, unsigned frame,
                               TetrisCathedralSnapshot *snapshot);

#endif
