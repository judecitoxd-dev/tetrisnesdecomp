#include "demo.h"
#include "game.h"
#include "replay.h"
#include "rom.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEMO_FRAMES 200000u

static void print_usage(const char *program) {
    fprintf(stderr,
            "Usage: %s <Tetris (USA).nes> [trace.csv]\n"
            "Runs the NTSC demo commands and piece sequence directly from "
            "the user's verified ROM.\n",
            program);
}

static uint64_t mix_trace_hash(uint64_t accumulated, uint64_t state_hash,
                               uint32_t frame) {
    const uint8_t *bytes = (const uint8_t *)&state_hash;
    accumulated ^= (uint64_t)frame;
    accumulated *= UINT64_C(1099511628211);
    for (size_t index = 0; index < sizeof(state_hash); ++index) {
        accumulated ^= bytes[index];
        accumulated *= UINT64_C(1099511628211);
    }
    return accumulated;
}

static void write_trace_header(FILE *trace) {
    fputs("frame,state_hash,phase,active,next,x,y,rotation,score,lines,level,"
          "spawn_count,button_offset,piece_offset\n", trace);
}

static void write_trace_row(FILE *trace, uint32_t frame, uint64_t state_hash,
                            const TetrisGame *game,
                            const TetrisDemoController *demo) {
    fprintf(trace,
            "%" PRIu32 ",%016" PRIx64 ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%zu,%zu\n",
            frame, state_hash, (int)game->phase, (int)game->active,
            (int)game->next, game->x, game->y, game->rotation,
            game->score, game->lines, game->level,
            (unsigned)game->spawn_count, demo->button_index,
            demo->piece_index);
}

int main(int argc, char **argv) {
    NesRom rom;
    TetrisGame game;
    TetrisDemoController demo;
    FILE *trace = NULL;
    char error[256];
    uint32_t frames = 0;
    uint64_t trace_hash = UINT64_C(1469598103934665603);

    memset(&rom, 0, sizeof(rom));
    if (argc < 2 || argc > 3) {
        print_usage(argv[0]);
        return 2;
    }
    if (!nes_rom_load(argv[1], &rom, error, sizeof(error))) {
        fprintf(stderr, "ROM error: %s\n", error);
        return 1;
    }
    if (!rom.exact_supported_dump) {
        fprintf(stderr,
                "The demo offsets are verified only for CRC32 D16EA396; "
                "this ROM reports %08X.\n",
                rom.crc32);
        nes_rom_free(&rom);
        return 1;
    }
    if (argc == 3) {
        trace = fopen(argv[2], "wb");
        if (!trace) {
            fprintf(stderr, "Could not create trace: %s\n", argv[2]);
            nes_rom_free(&rom);
            return 1;
        }
        write_trace_header(trace);
    }

    tetris_init_mode(&game, UINT32_C(0x19891101), 0, TETRIS_MODE_A, 0);
    if (!tetris_demo_reset_from_rom(&demo, &game, &rom)) {
        fputs("Could not initialize the ROM demo.\n", stderr);
        if (trace) fclose(trace);
        nes_rom_free(&rom);
        return 1;
    }

    while (!tetris_demo_is_finished(&demo) &&
           game.phase != TETRIS_PHASE_GAME_OVER &&
           frames < MAX_DEMO_FRAMES) {
        const TetrisInput input = tetris_demo_next_input(&demo, &game);
        uint64_t state_hash;
        tetris_tick(&game, &input);
        (void)tetris_consume_events(&game);
        state_hash = tetris_state_hash(&game);
        trace_hash = mix_trace_hash(trace_hash, state_hash, frames);
        if (trace) write_trace_row(trace, frames, state_hash, &game, &demo);
        ++frames;
    }

    if (trace && fclose(trace) != 0) {
        fputs("Could not finish writing the trace.\n", stderr);
        nes_rom_free(&rom);
        return 1;
    }

    printf("ROM_CRC32=%08X\n", rom.crc32);
    printf("FRAMES=%" PRIu32 "\n", frames);
    printf("BUTTON_BYTES=%zu\n", demo.button_index);
    printf("PIECE_BYTES=%zu\n", demo.piece_index);
    printf("SPAWNS=%u\n", (unsigned)game.spawn_count);
    printf("PHASE=%d\n", (int)game.phase);
    printf("SCORE=%d\n", game.score);
    printf("LINES=%d\n", game.lines);
    printf("LEVEL=%d\n", game.level);
    printf("FINAL_STATE_HASH=%016" PRIx64 "\n", tetris_state_hash(&game));
    printf("TRACE_HASH=%016" PRIx64 "\n", trace_hash);

    nes_rom_free(&rom);
    if (!tetris_demo_is_finished(&demo) ||
        demo.button_index != 0x0200u ||
        frames >= MAX_DEMO_FRAMES) {
        fputs("Demo verification did not reach the expected table boundary.\n",
              stderr);
        return 1;
    }
    return 0;
}
