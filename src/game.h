#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include <stdbool.h>
#include <stdint.h>

#define TETRIS_BOARD_W 10
#define TETRIS_BOARD_H 20
#define TETRIS_PIECE_COUNT 7

typedef enum TetrisPiece {
    PIECE_T = 0,
    PIECE_J,
    PIECE_Z,
    PIECE_O,
    PIECE_S,
    PIECE_L,
    PIECE_I
} TetrisPiece;

typedef struct TetrisInput {
    bool left;
    bool right;
    bool down;
    bool rotate_cw_pressed;
    bool rotate_ccw_pressed;
    bool hard_drop_pressed;
    bool pause_pressed;
    bool restart_pressed;
} TetrisInput;

typedef struct TetrisGame {
    uint8_t board[TETRIS_BOARD_H][TETRIS_BOARD_W];
    TetrisPiece active;
    TetrisPiece next;
    int rotation;
    int x;
    int y;
    int score;
    int lines;
    int level;
    int start_level;
    int frame;
    int fall_counter;
    int das_counter;
    int das_direction;
    int previous_piece;
    uint32_t rng_state;
    bool paused;
    bool game_over;
} TetrisGame;

void tetris_init(TetrisGame *game, uint32_t seed, int start_level);
void tetris_tick(TetrisGame *game, const TetrisInput *input);
bool tetris_try_move(TetrisGame *game, int dx, int dy);
bool tetris_try_rotate(TetrisGame *game, int direction);
void tetris_hard_drop(TetrisGame *game);
int tetris_piece_block_x(TetrisPiece piece, int rotation, int block);
int tetris_piece_block_y(TetrisPiece piece, int rotation, int block);
int tetris_gravity_frames(int level);

#endif
