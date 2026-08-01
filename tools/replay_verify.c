#include "replay.h"

#include <inttypes.h>
#include <stdio.h>

int main(int argc, char **argv) {
    TetrisReplay replay;
    TetrisGame game;
    char error[256];
    uint32_t frame;
    uint64_t actual;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s replay.ttr\n", argv[0]);
        return 2;
    }
    tetris_replay_init(&replay);
    if (!tetris_replay_load(&replay, argv[1], error, sizeof(error))) {
        fprintf(stderr, "Replay error: %s\n", error);
        return 2;
    }
    tetris_init_mode(&game, replay.seed, replay.start_level,
                     replay.mode, replay.start_height);
    game.show_next = replay.initial_show_next;
    for (frame = 0; frame < replay.frame_count; ++frame) {
        TetrisInput input = tetris_replay_input(&replay, frame);
        tetris_tick(&game, &input);
        (void)tetris_consume_events(&game);
    }
    actual = tetris_state_hash(&game);
    {
        const uint64_t expected = replay.final_hash;
        printf("frames=%" PRIu32 " expected=%016" PRIx64 " actual=%016" PRIx64 "\n",
               replay.frame_count, expected, actual);
        tetris_replay_free(&replay);
        return actual == expected ? 0 : 1;
    }
}
