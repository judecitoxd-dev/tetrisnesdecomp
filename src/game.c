#include "game.h"

#include <string.h>

/*
 * Four block coordinates for every tetromino and rotation.
 * The layout intentionally uses the compact, no-wall-kick behavior associated
 * with the NES game rather than modern SRS rotation.
 */
static const int8_t PIECES[TETRIS_PIECE_COUNT][4][4][2] = {
    /* T */ {
        {{1,0},{0,1},{1,1},{2,1}},
        {{1,0},{1,1},{2,1},{1,2}},
        {{0,1},{1,1},{2,1},{1,2}},
        {{1,0},{0,1},{1,1},{1,2}}
    },
    /* J */ {
        {{0,0},{0,1},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{1,2}},
        {{0,1},{1,1},{2,1},{2,2}},
        {{1,0},{1,1},{0,2},{1,2}}
    },
    /* Z */ {
        {{0,0},{1,0},{1,1},{2,1}},
        {{2,0},{1,1},{2,1},{1,2}},
        {{0,1},{1,1},{1,2},{2,2}},
        {{1,0},{0,1},{1,1},{0,2}}
    },
    /* O */ {
        {{1,0},{2,0},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{2,1}}
    },
    /* S */ {
        {{1,0},{2,0},{0,1},{1,1}},
        {{1,0},{1,1},{2,1},{2,2}},
        {{1,1},{2,1},{0,2},{1,2}},
        {{0,0},{0,1},{1,1},{1,2}}
    },
    /* L */ {
        {{2,0},{0,1},{1,1},{2,1}},
        {{1,0},{1,1},{1,2},{2,2}},
        {{0,1},{1,1},{2,1},{0,2}},
        {{0,0},{1,0},{1,1},{1,2}}
    },
    /* I */ {
        {{0,1},{1,1},{2,1},{3,1}},
        {{2,0},{2,1},{2,2},{2,3}},
        {{0,2},{1,2},{2,2},{3,2}},
        {{1,0},{1,1},{1,2},{1,3}}
    }
};

static const int GRAVITY_FRAMES[30] = {
    48,43,38,33,28,23,18,13,8,6,
    5,5,5,4,4,4,3,3,3,2,
    2,2,2,2,2,2,2,2,2,1
};

static uint32_t next_random(TetrisGame *game) {
    uint32_t x = game->rng_state;
    if (x == 0) x = 0x6d2b79f5u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    game->rng_state = x;
    return x;
}

/* Approximation of the NES one-reroll randomizer. */
static TetrisPiece choose_piece(TetrisGame *game) {
    int first = (int)(next_random(game) % 8u);
    int chosen;
    if (first == 7 || first == game->previous_piece) {
        chosen = (int)(next_random(game) % 7u);
    } else {
        chosen = first;
    }
    game->previous_piece = chosen;
    return (TetrisPiece)chosen;
}

int tetris_piece_block_x(TetrisPiece piece, int rotation, int block) {
    return PIECES[piece][rotation & 3][block & 3][0];
}

int tetris_piece_block_y(TetrisPiece piece, int rotation, int block) {
    return PIECES[piece][rotation & 3][block & 3][1];
}

int tetris_gravity_frames(int level) {
    if (level < 0) level = 0;
    if (level > 29) level = 29;
    return GRAVITY_FRAMES[level];
}

static bool collides(const TetrisGame *game, TetrisPiece piece, int rotation, int px, int py) {
    for (int i = 0; i < 4; ++i) {
        const int x = px + tetris_piece_block_x(piece, rotation, i);
        const int y = py + tetris_piece_block_y(piece, rotation, i);
        if (x < 0 || x >= TETRIS_BOARD_W || y >= TETRIS_BOARD_H) return true;
        if (y >= 0 && game->board[y][x] != 0) return true;
    }
    return false;
}

static void spawn_piece(TetrisGame *game) {
    game->active = game->next;
    game->next = choose_piece(game);
    game->rotation = 0;
    game->x = 3;
    game->y = -1;
    game->fall_counter = 0;
    game->das_counter = 0;
    game->das_direction = 0;
    if (collides(game, game->active, game->rotation, game->x, game->y)) {
        game->game_over = true;
    }
}

void tetris_init(TetrisGame *game, uint32_t seed, int start_level) {
    memset(game, 0, sizeof(*game));
    if (start_level < 0) start_level = 0;
    if (start_level > 19) start_level = 19;
    game->rng_state = seed ? seed : 0x19891101u;
    game->previous_piece = -1;
    game->start_level = start_level;
    game->level = start_level;
    game->next = choose_piece(game);
    spawn_piece(game);
}

bool tetris_try_move(TetrisGame *game, int dx, int dy) {
    if (!collides(game, game->active, game->rotation, game->x + dx, game->y + dy)) {
        game->x += dx;
        game->y += dy;
        return true;
    }
    return false;
}

bool tetris_try_rotate(TetrisGame *game, int direction) {
    const int new_rotation = (game->rotation + (direction > 0 ? 1 : 3)) & 3;
    if (!collides(game, game->active, new_rotation, game->x, game->y)) {
        game->rotation = new_rotation;
        return true;
    }
    return false;
}

static int clear_lines(TetrisGame *game) {
    int cleared = 0;
    for (int y = TETRIS_BOARD_H - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (game->board[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (!full) continue;
        ++cleared;
        for (int pull = y; pull > 0; --pull) {
            memcpy(game->board[pull], game->board[pull - 1], TETRIS_BOARD_W);
        }
        memset(game->board[0], 0, TETRIS_BOARD_W);
        ++y;
    }
    return cleared;
}

static void lock_piece(TetrisGame *game) {
    bool above_top = false;
    for (int i = 0; i < 4; ++i) {
        const int x = game->x + tetris_piece_block_x(game->active, game->rotation, i);
        const int y = game->y + tetris_piece_block_y(game->active, game->rotation, i);
        if (y < 0) {
            above_top = true;
        } else if (x >= 0 && x < TETRIS_BOARD_W && y < TETRIS_BOARD_H) {
            game->board[y][x] = (uint8_t)game->active + 1;
        }
    }
    if (above_top) {
        game->game_over = true;
        return;
    }

    const int count = clear_lines(game);
    static const int SCORE_TABLE[5] = {0, 40, 100, 300, 1200};
    game->score += SCORE_TABLE[count] * (game->level + 1);
    game->lines += count;
    game->level = game->start_level + game->lines / 10;
    if (game->level > 99) game->level = 99;
    spawn_piece(game);
}

void tetris_hard_drop(TetrisGame *game) {
    int distance = 0;
    while (tetris_try_move(game, 0, 1)) ++distance;
    game->score += distance * 2;
    lock_piece(game);
}

static void update_horizontal(TetrisGame *game, const TetrisInput *input) {
    int direction = 0;
    if (input->left != input->right) direction = input->left ? -1 : 1;
    if (direction == 0) {
        game->das_direction = 0;
        game->das_counter = 0;
        return;
    }
    if (game->das_direction != direction) {
        game->das_direction = direction;
        game->das_counter = 0;
        (void)tetris_try_move(game, direction, 0);
        return;
    }

    ++game->das_counter;
    if (game->das_counter == 16 || (game->das_counter > 16 && ((game->das_counter - 16) % 6) == 0)) {
        (void)tetris_try_move(game, direction, 0);
    }
}

void tetris_tick(TetrisGame *game, const TetrisInput *input) {
    if (input->restart_pressed && game->game_over) {
        const uint32_t seed = game->rng_state ^ (uint32_t)game->frame ^ 0xa511e9b3u;
        tetris_init(game, seed, game->start_level);
        return;
    }
    if (input->pause_pressed && !game->game_over) game->paused = !game->paused;
    if (game->paused || game->game_over) return;

    ++game->frame;
    update_horizontal(game, input);
    if (input->rotate_cw_pressed) (void)tetris_try_rotate(game, 1);
    if (input->rotate_ccw_pressed) (void)tetris_try_rotate(game, -1);
    if (input->hard_drop_pressed) {
        tetris_hard_drop(game);
        return;
    }

    if (input->down) {
        if ((game->frame & 1) == 0) {
            if (tetris_try_move(game, 0, 1)) {
                game->score += 1;
            } else {
                lock_piece(game);
            }
        }
        game->fall_counter = 0;
        return;
    }

    ++game->fall_counter;
    if (game->fall_counter >= tetris_gravity_frames(game->level)) {
        game->fall_counter = 0;
        if (!tetris_try_move(game, 0, 1)) lock_piece(game);
    }
}
