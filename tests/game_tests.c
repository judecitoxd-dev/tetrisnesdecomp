#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void tick_empty(TetrisGame *game, int frames) {
    TetrisInput input = {0};
    for (int i = 0; i < frames; ++i) tetris_tick(game, &input);
}

static void test_spawn_and_original_orientation(void) {
    TetrisGame game;
    tetris_init(&game, 1234u, 0);
    assert(game.phase == TETRIS_PHASE_ACTIVE);
    assert(!game.game_over);
    assert(game.x == 5);
    assert(game.y == 0);
    assert(game.rotation == tetris_spawn_rotation(game.active));
    const int old_x = game.x;
    assert(tetris_try_move(&game, -1, 0));
    assert(game.x == old_x - 1);
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
    memset(game.board, 0, sizeof(game.board));
    for (int x = 0; x < 6; ++x) game.board[19][x] = 1;
    game.active = PIECE_I;
    game.rotation = tetris_spawn_rotation(PIECE_I);
    game.x = 8;
    game.y = 19;
    game.phase = TETRIS_PHASE_ACTIVE;
    tetris_hard_drop(&game);
    assert(game.phase == TETRIS_PHASE_LINE_CLEAR);
    assert(game.clear_count == 1);
    assert(game.lines == 0);
    assert(tetris_cell_hidden(&game, 4, 19));
    tick_empty(&game, 19);
    assert(game.lines == 0);
    tick_empty(&game, 1);
    assert(game.lines == 1);
    assert(game.score == 40);
    assert(game.phase == TETRIS_PHASE_ENTRY_DELAY);
}

static void test_exact_level_transition_behavior(void) {
    TetrisGame game;
    tetris_init(&game, 99u, 18);
    game.lines = 129;
    memset(game.board, 0, sizeof(game.board));
    for (int x = 0; x < 6; ++x) game.board[19][x] = 1;
    game.active = PIECE_I;
    game.rotation = tetris_spawn_rotation(PIECE_I);
    game.x = 8;
    game.y = 19;
    game.phase = TETRIS_PHASE_ACTIVE;
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
    const int delay = game.phase_timer;
    assert(delay == 10);
    tick_empty(&game, delay - 1);
    assert(game.phase == TETRIS_PHASE_ENTRY_DELAY);
    tick_empty(&game, 1);
    assert(game.phase == TETRIS_PHASE_ACTIVE);
}

static void test_pause_and_next_toggle(void) {
    TetrisGame game;
    tetris_init(&game, 1u, 0);
    TetrisInput input = {0};
    input.pause_pressed = true;
    tetris_tick(&game, &input);
    assert(game.paused);
    const int frame = game.frame;
    input.pause_pressed = false;
    input.toggle_next_pressed = true;
    tetris_tick(&game, &input);
    assert(game.frame == frame);
    assert(!game.show_next);
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
    puts("All gameplay tests passed.");
    return 0;
}
