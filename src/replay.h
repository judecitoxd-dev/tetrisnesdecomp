#ifndef TETRIS_REPLAY_H
#define TETRIS_REPLAY_H

#include "game.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TETRIS_REPLAY_MAX_FRAMES 1000000u

typedef struct TetrisReplay {
    uint32_t seed;
    TetrisMode mode;
    int start_level;
    int start_height;
    bool initial_show_next;
    uint32_t frame_count;
    uint32_t capacity;
    uint16_t *frames;
    uint64_t final_hash;
} TetrisReplay;

void tetris_replay_init(TetrisReplay *replay);
void tetris_replay_free(TetrisReplay *replay);
bool tetris_replay_begin(TetrisReplay *replay, const TetrisGame *game);
bool tetris_replay_append(TetrisReplay *replay, const TetrisInput *input);
TetrisInput tetris_replay_input(const TetrisReplay *replay, uint32_t frame);
void tetris_replay_finish(TetrisReplay *replay, const TetrisGame *game);
bool tetris_replay_save(const TetrisReplay *replay, const char *path);
bool tetris_replay_load(TetrisReplay *replay, const char *path,
                        char *error, size_t error_size);
uint64_t tetris_state_hash(const TetrisGame *game);

#endif
