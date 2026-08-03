#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "timing fidelity assertion failed: %s at %s:%d\n", \
                #expression, __FILE__, __LINE__); \
        exit(1); \
    } \
} while (0)

static void tick_with(TetrisGame *game, TetrisInput input, int count) {
    int frame;
    for (frame = 0; frame < count; ++frame) tetris_tick(game, &input);
}

static void prepare_active_o(TetrisGame *game, int x, int y) {
    memset(game->board, 0, sizeof(game->board));
    game->active = PIECE_O;
    game->rotation = tetris_spawn_rotation(PIECE_O);
    game->x = x;
    game->y = y;
    game->phase = TETRIS_PHASE_ACTIVE;
    game->paused = false;
    game->fall_counter = 0;
}

static void prepare_single_line(TetrisGame *game) {
    int x;
    memset(game->board, 0, sizeof(game->board));
    for (x = 0; x < 6; ++x) game->board[19][x] = 1;
    game->active = PIECE_I;
    game->rotation = tetris_spawn_rotation(PIECE_I);
    game->x = 8;
    game->y = 19;
    game->phase = TETRIS_PHASE_ACTIVE;
}

static int occupied_cells(const TetrisGame *game) {
    int total = 0;
    int y;
    int x;
    for (y = 0; y < TETRIS_BOARD_H; ++y)
        for (x = 0; x < TETRIS_BOARD_W; ++x)
            if (game->board[y][x] != 0) ++total;
    return total;
}

static void test_down_preserves_horizontal_das(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_active_o(&game, 5, 5);
    game.das_counter = 15;
    game.das_direction = 1;
    game.horizontal_buttons = 2u;
    input.right = true;
    input.down = true;
    tetris_tick(&game, &input);
    CHECK(game.das_counter == 15);
    CHECK(game.das_direction == 1);
    CHECK(game.horizontal_buttons == 2u);
}

static void test_blocked_press_charges_das_to_reset(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_active_o(&game, 9, 5);
    game.das_counter = 0;
    game.horizontal_buttons = 0u;
    input.right = true;
    tetris_tick(&game, &input);
    CHECK(game.x == 9);
    CHECK(game.das_direction == 1);
    CHECK(game.das_counter == 16);
}

static void test_right_has_priority_when_both_are_held(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_active_o(&game, 5, 5);
    game.horizontal_buttons = 0u;
    input.left = true;
    input.right = true;
    tetris_tick(&game, &input);
    CHECK(game.x == 6);
    CHECK(game.das_direction == 1);
}

static void test_das_survives_entry_delay_and_spawn(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 0x1989u, 0);
    game.phase = TETRIS_PHASE_ENTRY_DELAY;
    game.phase_timer = 1;
    game.das_counter = 12;
    game.das_direction = 1;
    game.horizontal_buttons = 2u;
    input.right = true;
    tetris_tick(&game, &input);
    CHECK(game.phase == TETRIS_PHASE_ACTIVE);
    CHECK(game.das_counter == 12);
    CHECK(game.horizontal_buttons == 2u);
}

static void test_collision_defers_lock_one_update(void) {
    TetrisGame game;
    TetrisInput empty = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_active_o(&game, 5, 18);
    game.fall_counter = tetris_gravity_frames(game.level) - 1;
    CHECK(occupied_cells(&game) == 0);
    tetris_tick(&game, &empty);
    CHECK(game.phase == TETRIS_PHASE_LOCK_PENDING);
    CHECK(occupied_cells(&game) == 0);
    tetris_tick(&game, &empty);
    CHECK(game.phase == TETRIS_PHASE_ROW_CHECK);
    CHECK(occupied_cells(&game) == 4);
    CHECK(game.clear_step == 0);
}

static void test_four_row_checks_are_separate_updates(void) {
    TetrisGame game;
    TetrisInput empty = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    CHECK(game.phase == TETRIS_PHASE_ROW_CHECK);
    CHECK(game.clear_count == 0);
    tetris_tick(&game, &empty);
    CHECK(game.phase == TETRIS_PHASE_ROW_CHECK);
    CHECK(game.clear_step == 1 && game.clear_count == 0);
    tetris_tick(&game, &empty);
    CHECK(game.clear_step == 2 && game.clear_count == 0);
    tetris_tick(&game, &empty);
    CHECK(game.clear_step == 3 && game.clear_count == 1);
    CHECK(game.lines == 0 && game.score == 0);
    tetris_tick(&game, &empty);
    CHECK(game.phase == TETRIS_PHASE_LINE_CLEAR);
    CHECK(game.clear_step == 0 && game.clear_count == 1);
}

static void test_line_clear_uses_global_frame_alignment(void) {
    TetrisGame game;
    TetrisInput empty = {0};
    tetris_init(&game, 0x1989u, 0);
    game.frame = 1;
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    CHECK(game.phase == TETRIS_PHASE_ROW_CHECK);
    tick_with(&game, empty, 4);
    CHECK(game.frame == 5);
    CHECK(game.phase == TETRIS_PHASE_LINE_CLEAR);
    tick_with(&game, empty, 18);
    CHECK(game.frame == 23);
    CHECK(game.phase == TETRIS_PHASE_LINE_CLEAR);
    CHECK(game.lines == 0 && game.score == 0);
    tick_with(&game, empty, 1);
    CHECK(game.frame == 24);
    CHECK(game.phase == TETRIS_PHASE_ENTRY_DELAY);
    CHECK(game.lines == 1 && game.score == 40);
}

static void test_curtain_uses_global_frame_alignment(void) {
    TetrisGame game;
    TetrisInput empty = {0};
    tetris_init(&game, 0x1989u, 0);
    game.phase = TETRIS_PHASE_GAME_OVER_CURTAIN;
    game.frame = 1;
    game.phase_timer = 0;
    game.curtain_rows = 0;
    tick_with(&game, empty, 2);
    CHECK(game.frame == 3);
    CHECK(game.curtain_rows == 0);
    tick_with(&game, empty, 1);
    CHECK(game.frame == 4);
    CHECK(game.curtain_rows == 1);
}

int main(void) {
    test_down_preserves_horizontal_das();
    test_blocked_press_charges_das_to_reset();
    test_right_has_priority_when_both_are_held();
    test_das_survives_entry_delay_and_spawn();
    test_collision_defers_lock_one_update();
    test_four_row_checks_are_separate_updates();
    test_line_clear_uses_global_frame_alignment();
    test_curtain_uses_global_frame_alignment();
    puts("NES lock, row-check, DAS and global-frame timing tests passed.");
    return 0;
}
