#include "demo.h"
#include "game.h"
#include "rom.h"

#include <stdio.h>
#include <string.h>

#define DEMO_BUTTONS_PRG_OFFSET 0x5d00u
#define DEMO_PIECES_PRG_OFFSET  0x5f00u
#define FAKE_PRG_SIZE            0x6000u

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static void test_rom_demo_commands_and_pieces(void) {
    uint8_t prg[FAKE_PRG_SIZE];
    NesRom rom;
    TetrisGame game;
    TetrisDemoController demo;
    TetrisInput input;

    memset(prg, 0, sizeof(prg));
    memset(&rom, 0, sizeof(rom));
    rom.prg = prg;
    rom.prg_size = sizeof(prg);
    rom.exact_supported_dump = true;

    prg[DEMO_BUTTONS_PRG_OFFSET + 0] = 0x81u; /* A + right */
    prg[DEMO_BUTTONS_PRG_OFFSET + 1] = 2u;
    prg[DEMO_BUTTONS_PRG_OFFSET + 2] = 0x42u; /* B + left */
    prg[DEMO_BUTTONS_PRG_OFFSET + 3] = 0u;

    prg[DEMO_PIECES_PRG_OFFSET + 0] = 0x00u; /* T */
    prg[DEMO_PIECES_PRG_OFFSET + 1] = 0x10u; /* J */
    prg[DEMO_PIECES_PRG_OFFSET + 2] = 0x20u; /* Z */
    prg[DEMO_PIECES_PRG_OFFSET + 3] = 0x70u; /* table index 7 maps to T */

    tetris_init_mode(&game, 0x19891101u, 5, TETRIS_MODE_A, 0);
    CHECK(tetris_demo_reset_from_rom(&demo, &game, &rom));
    CHECK(tetris_demo_uses_rom_script(&demo));
    CHECK(game.active == PIECE_T);
    CHECK(game.next == PIECE_J);
    CHECK(game.rotation == tetris_spawn_rotation(PIECE_T));

    input = tetris_demo_next_input(&demo, &game);
    CHECK(input.right);
    CHECK(input.rotate_cw_pressed);
    CHECK(!input.rotate_ccw_pressed);

    input = tetris_demo_next_input(&demo, &game);
    CHECK(input.right);
    CHECK(!input.rotate_cw_pressed);
    input = tetris_demo_next_input(&demo, &game);
    CHECK(input.right);
    CHECK(!input.rotate_cw_pressed);

    input = tetris_demo_next_input(&demo, &game);
    CHECK(input.left);
    CHECK(input.rotate_ccw_pressed);
    CHECK(!input.rotate_cw_pressed);

    game.spawn_count = (uint8_t)(game.spawn_count + 1u);
    tetris_demo_sync_after_tick(&demo, &game);
    CHECK(game.next == PIECE_Z);
    game.spawn_count = (uint8_t)(game.spawn_count + 1u);
    tetris_demo_sync_after_tick(&demo, &game);
    CHECK(game.next == PIECE_T);

    demo.button_index = 0x0200u;
    demo.repeats = 0;
    (void)tetris_demo_next_input(&demo, &game);
    CHECK(tetris_demo_is_finished(&demo));
}

static void test_unverified_rom_uses_ai_fallback(void) {
    uint8_t prg[FAKE_PRG_SIZE];
    NesRom rom;
    TetrisGame game;
    TetrisDemoController demo;

    memset(prg, 0, sizeof(prg));
    memset(&rom, 0, sizeof(rom));
    rom.prg = prg;
    rom.prg_size = sizeof(prg);
    rom.exact_supported_dump = false;
    tetris_init_mode(&game, 1u, 5, TETRIS_MODE_A, 0);

    CHECK(!tetris_demo_reset_from_rom(&demo, &game, &rom));
    CHECK(!tetris_demo_uses_rom_script(&demo));
    CHECK(!tetris_demo_is_finished(&demo));
}

int main(void) {
    test_rom_demo_commands_and_pieces();
    test_unverified_rom_uses_ai_fallback();
    if (failures != 0) {
        fprintf(stderr, "%d demo test(s) failed.\n", failures);
        return 1;
    }
    puts("ROM demo tests passed.");
    return 0;
}
