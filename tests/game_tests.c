#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_spawn_and_move(void) {
    TetrisGame game;
    tetris_init(&game, 1234u, 0);
    assert(!game.game_over);
    const int old_x = game.x;
    assert(tetris_try_move(&game, -1, 0));
    assert(game.x == old_x - 1);
}

static void test_gravity_table(void) {
    assert(tetris_gravity_frames(0) == 48);
    assert(tetris_gravity_frames(9) == 6);
    assert(tetris_gravity_frames(29) == 1);
    assert(tetris_gravity_frames(100) == 1);
}

static void test_line_clear_and_score(void) {
    TetrisGame game;
    tetris_init(&game, 42u, 0);
    memset(game.board, 0, sizeof(game.board));
    for (int x = 0; x < 6; ++x) game.board[19][x] = 1;
    game.active = PIECE_I;
    game.rotation = 0;
    game.x = 6;
    game.y = 18;
    tetris_hard_drop(&game);
    assert(game.lines == 1);
    assert(game.score >= 40);
}

static void test_pause(void) {
    TetrisGame game;
    tetris_init(&game, 1u, 0);
    TetrisInput input = {0};
    input.pause_pressed = true;
    tetris_tick(&game, &input);
    assert(game.paused);
    const int frame = game.frame;
    input.pause_pressed = false;
    tetris_tick(&game, &input);
    assert(game.frame == frame);
}

int main(void) {
    test_spawn_and_move();
    test_gravity_table();
    test_line_clear_and_score();
    test_pause();
    puts("All gameplay tests passed.");
    return 0;
}
