#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include <stdbool.h>
#include <stdint.h>

#define TETRIS_BOARD_W 10
#define TETRIS_BOARD_H 20
#define TETRIS_PIECE_COUNT 7
#define TETRIS_MAX_CLEAR_ROWS 4
#define TETRIS_TYPE_B_GOAL 25

typedef enum TetrisPiece {
    PIECE_T = 0,
    PIECE_J,
    PIECE_Z,
    PIECE_O,
    PIECE_S,
    PIECE_L,
    PIECE_I
} TetrisPiece;

typedef enum TetrisMode {
    TETRIS_MODE_A = 0,
    TETRIS_MODE_B = 1
} TetrisMode;

typedef enum TetrisPhase {
    TETRIS_PHASE_ACTIVE = 0,
    TETRIS_PHASE_LINE_CLEAR,
    TETRIS_PHASE_ENTRY_DELAY,
    TETRIS_PHASE_GAME_OVER_CURTAIN,
    TETRIS_PHASE_GAME_OVER,
    TETRIS_PHASE_COMPLETE
} TetrisPhase;

typedef enum TetrisEvent {
    TETRIS_EVENT_NONE      = 0,
    TETRIS_EVENT_MOVE      = 1u << 0,
    TETRIS_EVENT_ROTATE    = 1u << 1,
    TETRIS_EVENT_LOCK      = 1u << 2,
    TETRIS_EVENT_LINE      = 1u << 3,
    TETRIS_EVENT_TETRIS    = 1u << 4,
    TETRIS_EVENT_LEVEL_UP  = 1u << 5,
    TETRIS_EVENT_GAME_OVER = 1u << 6,
    TETRIS_EVENT_COMPLETE  = 1u << 7
} TetrisEvent;

typedef struct TetrisInput {
    bool left;
    bool right;
    bool down;
    bool rotate_cw_pressed;
    bool rotate_ccw_pressed;
    bool hard_drop_pressed;
    bool pause_pressed;
    bool restart_pressed;
    bool toggle_next_pressed;
} TetrisInput;

typedef struct TetrisGame {
    uint8_t board[TETRIS_BOARD_H][TETRIS_BOARD_W];
    TetrisPiece active;
    TetrisPiece next;
    int rotation;
    int x;
    int y;

    TetrisMode mode;
    int start_height;
    int score;
    int lines;
    int total_lines;
    int level;
    int start_level;
    int transition_lines;
    int piece_count[TETRIS_PIECE_COUNT];

    int frame;
    int fall_counter;
    int das_counter;
    int das_direction;
    int soft_drop_counter;
    int soft_drop_points;

    uint16_t rng_seed;
    uint8_t spawn_count;
    int previous_piece;

    TetrisPhase phase;
    int phase_timer;
    int clear_rows[TETRIS_MAX_CLEAR_ROWS];
    int clear_count;
    int clear_step;
    int lock_bottom_row;
    int curtain_rows;

    uint32_t events;
    bool show_next;
    bool paused;
    bool game_over;
    bool completed;
} TetrisGame;

void tetris_init(TetrisGame *game, uint32_t seed, int start_level);
void tetris_init_mode(TetrisGame *game, uint32_t seed, int start_level,
                      TetrisMode mode, int start_height);
void tetris_tick(TetrisGame *game, const TetrisInput *input);
bool tetris_try_move(TetrisGame *game, int dx, int dy);
bool tetris_try_rotate(TetrisGame *game, int direction);
void tetris_hard_drop(TetrisGame *game);
int tetris_piece_block_x(TetrisPiece piece, int rotation, int block);
int tetris_piece_block_y(TetrisPiece piece, int rotation, int block);
int tetris_spawn_rotation(TetrisPiece piece);
int tetris_gravity_frames(int level);
int tetris_level_transition_lines(int start_level);
int tetris_entry_delay_frames(int lock_bottom_row);
int tetris_type_b_garbage_rows(int start_height);
int tetris_type_b_completion_bonus(int start_level, int start_height);
bool tetris_cell_hidden(const TetrisGame *game, int x, int y);
uint32_t tetris_consume_events(TetrisGame *game);

#endif
