#include "game.h"
#include "highscores.h"
#include "rom.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void tick_empty(TetrisGame *game, int frames) {
    TetrisInput input = {0};
    for (int i = 0; i < frames; ++i) tetris_tick(game, &input);
}

static void prepare_single_line(TetrisGame *game) {
    memset(game->board, 0, sizeof(game->board));
    for (int x = 0; x < 6; ++x) game->board[19][x] = 1;
    game->active = PIECE_I;
    game->rotation = tetris_spawn_rotation(PIECE_I);
    game->x = 8;
    game->y = 19;
    game->phase = TETRIS_PHASE_ACTIVE;
}

static void test_spawn_and_original_orientation(void) {
    TetrisGame game;
    tetris_init(&game, 1234u, 0);
    assert(game.mode == TETRIS_MODE_A);
    assert(game.phase == TETRIS_PHASE_ACTIVE);
    assert(!game.game_over);
    assert(game.x == 5);
    assert(game.y == 0);
    assert(game.rotation == tetris_spawn_rotation(game.active));
    {
        const int old_x = game.x;
        assert(tetris_try_move(&game, -1, 0));
        assert(game.x == old_x - 1);
    }
}

static void test_gravity_table(void) {
    assert(tetris_gravity_frames(0) == 48);
    assert(tetris_gravity_frames(9) == 6);
    assert(tetris_gravity_frames(19) == 2);
    assert(tetris_gravity_frames(29) == 1);
    assert(tetris_gravity_frames(100) == 1);
}

static void test_level_transition_table(void) {
    assert(tetris_level_transition_lines(0) == 10);
    assert(tetris_level_transition_lines(9) == 100);
    assert(tetris_level_transition_lines(10) == 100);
    assert(tetris_level_transition_lines(15) == 100);
    assert(tetris_level_transition_lines(16) == 110);
    assert(tetris_level_transition_lines(18) == 130);
    assert(tetris_level_transition_lines(19) == 140);
}

static void test_entry_delay_table(void) {
    assert(tetris_entry_delay_frames(19) == 10);
    assert(tetris_entry_delay_frames(18) == 10);
    assert(tetris_entry_delay_frames(17) == 12);
    assert(tetris_entry_delay_frames(13) == 14);
    assert(tetris_entry_delay_frames(5) == 18);
}

static void test_line_clear_animation_and_score(void) {
    TetrisGame game;
    tetris_init(&game, 42u, 0);
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    assert(game.phase == TETRIS_PHASE_LINE_CLEAR);
    assert(game.clear_count == 1);
    assert(game.lines == 0);
    assert(tetris_cell_hidden(&game, 4, 19));
    tick_empty(&game, 19);
    assert(game.lines == 0);
    tick_empty(&game, 1);
    assert(game.lines == 1);
    assert(game.total_lines == 1);
    assert(game.score == 40);
    assert(game.phase == TETRIS_PHASE_ENTRY_DELAY);
}

static void test_exact_level_transition_behavior(void) {
    TetrisGame game;
    tetris_init(&game, 99u, 18);
    game.lines = 129;
    game.total_lines = 129;
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    tick_empty(&game, 20);
    assert(game.lines == 130);
    assert(game.level == 19);
    assert(game.score == 800);
}

static void test_entry_delay_spawns_later(void) {
    TetrisGame game;
    tetris_init(&game, 7u, 0);
    memset(game.board, 0, sizeof(game.board));
    game.active = PIECE_O;
    game.rotation = tetris_spawn_rotation(PIECE_O);
    game.x = 5;
    game.y = 18;
    game.phase = TETRIS_PHASE_ACTIVE;
    tetris_hard_drop(&game);
    assert(game.phase == TETRIS_PHASE_ENTRY_DELAY);
    {
        const int delay = game.phase_timer;
        assert(delay == 10);
        tick_empty(&game, delay - 1);
        assert(game.phase == TETRIS_PHASE_ENTRY_DELAY);
        tick_empty(&game, 1);
        assert(game.phase == TETRIS_PHASE_ACTIVE);
    }
}

static void test_pause_and_next_toggle(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 1u, 0);
    input.pause_pressed = true;
    tetris_tick(&game, &input);
    assert(game.paused);
    {
        const int frame = game.frame;
        input.pause_pressed = false;
        input.toggle_next_pressed = true;
        tetris_tick(&game, &input);
        assert(game.frame == frame);
        assert(!game.show_next);
    }
}

static void test_rng_is_deterministic(void) {
    TetrisGame a;
    TetrisGame b;
    tetris_init(&a, 0x12345678u, 0);
    tetris_init(&b, 0x12345678u, 0);
    assert(a.active == b.active);
    assert(a.next == b.next);
    tick_empty(&a, 200);
    tick_empty(&b, 200);
    assert(a.rng_seed == b.rng_seed);
    assert(a.active == b.active);
    assert(a.next == b.next);
}

static void test_type_b_height_table_and_garbage(void) {
    TetrisGame game;
    assert(tetris_type_b_garbage_rows(0) == 0);
    assert(tetris_type_b_garbage_rows(1) == 3);
    assert(tetris_type_b_garbage_rows(2) == 5);
    assert(tetris_type_b_garbage_rows(3) == 8);
    assert(tetris_type_b_garbage_rows(4) == 10);
    assert(tetris_type_b_garbage_rows(5) == 12);

    tetris_init_mode(&game, 0x1989u, 9, TETRIS_MODE_B, 5);
    assert(game.mode == TETRIS_MODE_B);
    assert(game.lines == TETRIS_TYPE_B_GOAL);
    assert(game.level == 9);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < TETRIS_BOARD_W; ++x) assert(game.board[y][x] == 0);
    }
    for (int y = 8; y < TETRIS_BOARD_H; ++y) {
        int blanks = 0;
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (game.board[y][x] == 0) ++blanks;
        }
        assert(blanks >= 1);
    }
}

static void test_type_b_level_fixed_and_completion_bonus(void) {
    TetrisGame game;
    tetris_init_mode(&game, 0x2026u, 9, TETRIS_MODE_B, 5);
    game.lines = 1;
    game.total_lines = 24;
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    tick_empty(&game, 20);
    assert(game.lines == 0);
    assert(game.total_lines == 25);
    assert(game.level == 9);
    assert(game.phase == TETRIS_PHASE_COMPLETE);
    assert(game.completed);
    assert(game.score == 14400);
    assert(tetris_type_b_completion_bonus(19, 5) == 14000);
}

static void test_type_b_restart_preserves_settings(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init_mode(&game, 0x4444u, 7, TETRIS_MODE_B, 4);
    game.phase = TETRIS_PHASE_COMPLETE;
    game.completed = true;
    input.restart_pressed = true;
    tetris_tick(&game, &input);
    assert(game.mode == TETRIS_MODE_B);
    assert(game.start_level == 7);
    assert(game.start_height == 4);
    assert(game.lines == 25);
    assert(game.phase == TETRIS_PHASE_ACTIVE);
}


static void test_rom_level_palette_lookup(void) {
    uint8_t prg[0x184c + 40] = {0};
    uint8_t colors[4] = {0};
    NesRom rom = {0};
    rom.prg = prg;
    rom.prg_size = sizeof(prg);
    prg[0x184c + 3 * 4 + 0] = 0x0f;
    prg[0x184c + 3 * 4 + 1] = 0x30;
    prg[0x184c + 3 * 4 + 2] = 0x21;
    prg[0x184c + 3 * 4 + 3] = 0x12;
    assert(nes_rom_level_palette(&rom, 3, colors));
    assert(colors[0] == 0x0f && colors[1] == 0x30);
    assert(colors[2] == 0x21 && colors[3] == 0x12);
    assert(nes_rom_level_palette(&rom, 13, colors));
    assert(colors[2] == 0x21);
    rom.prg_size = 10;
    assert(!nes_rom_level_palette(&rom, 0, colors));
}

static void test_high_scores_sort_and_round_trip(void) {
    const char *path = "tetris_scores_test.tmp";
    TetrisHighScores scores;
    TetrisHighScores loaded;
    remove(path);
    tetris_high_scores_init(&scores);
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_A, "AAA", 100, 1, 0));
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_A, "BBB", 500, 5, 0));
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_A, "CCC", 300, 3, 0));
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_A, "DDD", 300, 4, 0));
    assert(tetris_high_scores_top(&scores, TETRIS_MODE_A)->score == 500);
    assert(strcmp(tetris_high_scores_top(&scores, TETRIS_MODE_A)->name, "BBB---") == 0);
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_B, "WIN", 14400, 9, 5));
    assert(tetris_high_scores_save(&scores, path));
    tetris_high_scores_init(&loaded);
    assert(tetris_high_scores_load(&loaded, path));
    assert(loaded.entries[0][0].score == 500);
    assert(loaded.entries[0][1].score == 300);
    assert(loaded.entries[0][2].score == 300);
    assert(loaded.entries[0][1].score == 300);
    assert(loaded.entries[1][0].score == 14400);
    assert(loaded.entries[1][0].height == 5);
    remove(path);
}

int main(void) {
    test_spawn_and_original_orientation();
    test_gravity_table();
    test_level_transition_table();
    test_entry_delay_table();
    test_line_clear_animation_and_score();
    test_exact_level_transition_behavior();
    test_entry_delay_spawns_later();
    test_pause_and_next_toggle();
    test_rng_is_deterministic();
    test_type_b_height_table_and_garbage();
    test_type_b_level_fixed_and_completion_bonus();
    test_type_b_restart_preserves_settings();
    test_rom_level_palette_lookup();
    test_high_scores_sort_and_round_trip();
    puts("All gameplay and persistence tests passed.");
    return 0;
}
