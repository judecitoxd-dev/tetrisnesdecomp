#include "game.h"

#include <limits.h>
#include <string.h>

/*
 * Coordinates translated from the orientation table used by the NES release.
 * The game x/y position is the original piece pivot, not a bounding-box corner.
 */
static const int8_t PIECES[TETRIS_PIECE_COUNT][4][4][2] = {
    /* T: up, right, down (spawn), left */ {
        {{-1, 0}, { 0, 0}, { 1, 0}, { 0,-1}},
        {{ 0,-1}, { 0, 0}, { 1, 0}, { 0, 1}},
        {{-1, 0}, { 0, 0}, { 1, 0}, { 0, 1}},
        {{ 0,-1}, {-1, 0}, { 0, 0}, { 0, 1}}
    },
    /* J: left, up, right, down (spawn) */ {
        {{ 0,-1}, { 0, 0}, {-1, 1}, { 0, 1}},
        {{-1,-1}, {-1, 0}, { 0, 0}, { 1, 0}},
        {{ 0,-1}, { 1,-1}, { 0, 0}, { 0, 1}},
        {{-1, 0}, { 0, 0}, { 1, 0}, { 1, 1}}
    },
    /* Z: horizontal (spawn), vertical */ {
        {{-1, 0}, { 0, 0}, { 0, 1}, { 1, 1}},
        {{ 1,-1}, { 0, 0}, { 1, 0}, { 0, 1}},
        {{-1, 0}, { 0, 0}, { 0, 1}, { 1, 1}},
        {{ 1,-1}, { 0, 0}, { 1, 0}, { 0, 1}}
    },
    /* O: fixed (spawn) */ {
        {{-1, 0}, { 0, 0}, {-1, 1}, { 0, 1}},
        {{-1, 0}, { 0, 0}, {-1, 1}, { 0, 1}},
        {{-1, 0}, { 0, 0}, {-1, 1}, { 0, 1}},
        {{-1, 0}, { 0, 0}, {-1, 1}, { 0, 1}}
    },
    /* S: horizontal (spawn), vertical */ {
        {{ 0, 0}, { 1, 0}, {-1, 1}, { 0, 1}},
        {{ 0,-1}, { 0, 0}, { 1, 0}, { 1, 1}},
        {{ 0, 0}, { 1, 0}, {-1, 1}, { 0, 1}},
        {{ 0,-1}, { 0, 0}, { 1, 0}, { 1, 1}}
    },
    /* L: right, down (spawn), left, up */ {
        {{ 0,-1}, { 0, 0}, { 0, 1}, { 1, 1}},
        {{-1, 0}, { 0, 0}, { 1, 0}, {-1, 1}},
        {{-1,-1}, { 0,-1}, { 0, 0}, { 0, 1}},
        {{ 1,-1}, {-1, 0}, { 0, 0}, { 1, 0}}
    },
    /* I: vertical, horizontal (spawn) */ {
        {{ 0,-2}, { 0,-1}, { 0, 0}, { 0, 1}},
        {{-2, 0}, {-1, 0}, { 0, 0}, { 1, 0}},
        {{ 0,-2}, { 0,-1}, { 0, 0}, { 0, 1}},
        {{-2, 0}, {-1, 0}, { 0, 0}, { 1, 0}}
    }
};

static const int SPAWN_ROTATION[TETRIS_PIECE_COUNT] = {2, 3, 0, 0, 0, 1, 1};

/* NTSC frames-per-drop table from the original program. */
static const int GRAVITY_FRAMES[30] = {
    48,43,38,33,28,23,18,13,8,6,
    5,5,5,4,4,4,3,3,3,2,
    2,2,2,2,2,2,2,2,2,1
};

/* Number of initial garbage rows visible for B-Type heights 0 through 5. */
static const int TYPE_B_GARBAGE_ROWS[6] = {0, 3, 5, 8, 10, 12};

/* First board index which the original initializer blanks, inclusive. */
static const int TYPE_B_BLANK_THROUGH[6] = {200, 170, 150, 120, 100, 80};

static void set_event(TetrisGame *game, TetrisEvent event) {
    game->events |= (uint32_t)event;
}

/* Two-byte LFSR used by the original program. */
static void advance_rng(TetrisGame *game) {
    uint8_t lo = (uint8_t)(game->rng_seed & 0xffu);
    uint8_t hi = (uint8_t)(game->rng_seed >> 8);
    const unsigned feedback = (((lo & 0x02u) ^ (hi & 0x02u)) != 0u) ? 1u : 0u;
    const unsigned next_carry = lo & 1u;
    lo = (uint8_t)((lo >> 1) | (feedback << 7));
    hi = (uint8_t)((hi >> 1) | (next_carry << 7));
    game->rng_seed = (uint16_t)(((uint16_t)hi << 8) | lo);
    if (game->rng_seed == 0) game->rng_seed = 0x8988u;
}

/* NES one-reroll selection translated from the disassembly. */
static TetrisPiece choose_piece(TetrisGame *game) {
    game->spawn_count = (uint8_t)(game->spawn_count + 1u);
    int chosen = ((int)(game->rng_seed & 0xffu) + game->spawn_count) & 7;
    if (chosen == 7 || chosen == game->previous_piece) {
        advance_rng(game);
        chosen = (((int)(game->rng_seed & 7u)) + game->previous_piece) % 7;
        if (chosen < 0) chosen += 7;
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

int tetris_spawn_rotation(TetrisPiece piece) {
    return SPAWN_ROTATION[piece];
}

int tetris_gravity_frames(int level) {
    if (level < 0) level = 0;
    if (level > 29) level = 29;
    return GRAVITY_FRAMES[level];
}

int tetris_level_transition_lines(int start_level) {
    if (start_level < 0) start_level = 0;
    if (start_level > 19) start_level = 19;
    {
        const int normal = (start_level + 1) * 10;
        int delayed = start_level * 10 - 50;
        if (delayed < 100) delayed = 100;
        return normal < delayed ? normal : delayed;
    }
}

int tetris_entry_delay_frames(int lock_bottom_row) {
    if (lock_bottom_row >= 18) return 10;
    if (lock_bottom_row >= 14) return 12;
    if (lock_bottom_row >= 10) return 14;
    if (lock_bottom_row >= 6) return 16;
    return 18;
}

int tetris_type_b_garbage_rows(int start_height) {
    if (start_height < 0) start_height = 0;
    if (start_height > 5) start_height = 5;
    return TYPE_B_GARBAGE_ROWS[start_height];
}

int tetris_type_b_completion_bonus(int start_level, int start_height) {
    if (start_level < 0) start_level = 0;
    if (start_level > 19) start_level = 19;
    if (start_height < 0) start_height = 0;
    if (start_height > 5) start_height = 5;
    if (start_level >= 10) start_level -= 10;
    return (start_level + start_height) * 1000;
}

static bool collides(const TetrisGame *game, TetrisPiece piece, int rotation,
                     int px, int py) {
    for (int i = 0; i < 4; ++i) {
        const int x = px + tetris_piece_block_x(piece, rotation, i);
        const int y = py + tetris_piece_block_y(piece, rotation, i);
        if (x < 0 || x >= TETRIS_BOARD_W || y >= TETRIS_BOARD_H) return true;
        if (y >= 0 && game->board[y][x] != 0) return true;
    }
    return false;
}

static void begin_game_over(TetrisGame *game) {
    game->phase = TETRIS_PHASE_GAME_OVER_CURTAIN;
    game->phase_timer = 0;
    game->curtain_rows = 0;
    game->game_over = false;
    game->completed = false;
    game->soft_drop_points = 0;
}

static void spawn_piece(TetrisGame *game) {
    game->active = game->next;
    game->next = choose_piece(game);
    game->rotation = tetris_spawn_rotation(game->active);
    game->x = 5;
    game->y = 0;
    game->fall_counter = 0;
    /* The cartridge preserves horizontal DAS charge through ARE and clears. */
    game->soft_drop_counter = 0;
    game->soft_drop_points = 0;
    game->phase = TETRIS_PHASE_ACTIVE;
    game->phase_timer = 0;
    game->piece_count[game->active] += 1;
    if (collides(game, game->active, game->rotation, game->x, game->y)) {
        begin_game_over(game);
    }
}

static uint8_t type_b_random_cell(TetrisGame *game) {
    static const uint8_t RNG_TABLE[8] = {0, 1, 0, 2, 3, 3, 0, 0};
    advance_rng(game);
    return RNG_TABLE[game->rng_seed & 7u];
}

static int type_b_random_column(TetrisGame *game) {
    for (;;) {
        advance_rng(game);
        {
            const int column = (int)(game->rng_seed & 0x0fu);
            if (column < TETRIS_BOARD_W) return column;
        }
    }
}

static void init_type_b_playfield(TetrisGame *game) {
    /* The cart generates twelve rows, then blanks the portion above the height. */
    for (int y = 8; y < TETRIS_BOARD_H; ++y) {
        for (int x = TETRIS_BOARD_W - 1; x >= 0; --x) {
            game->board[y][x] = type_b_random_cell(game);
        }
        game->board[y][type_b_random_column(game)] = 0;
    }

    {
        int through = TYPE_B_BLANK_THROUGH[game->start_height];
        if (through >= TETRIS_BOARD_W * TETRIS_BOARD_H) {
            through = TETRIS_BOARD_W * TETRIS_BOARD_H - 1;
        }
        for (int index = 0; index <= through; ++index) {
            game->board[index / TETRIS_BOARD_W][index % TETRIS_BOARD_W] = 0;
        }
    }
}

void tetris_init_mode(TetrisGame *game, uint32_t seed, int start_level,
                      TetrisMode mode, int start_height) {
    memset(game, 0, sizeof(*game));
    if (start_level < 0) start_level = 0;
    if (start_level > 19) start_level = 19;
    if (start_height < 0) start_height = 0;
    if (start_height > 5) start_height = 5;
    if (mode != TETRIS_MODE_B) mode = TETRIS_MODE_A;

    game->initial_seed = seed;
    game->rng_seed = (uint16_t)((seed ^ (seed >> 16)) & 0xffffu);
    if (game->rng_seed == 0) game->rng_seed = 0x8988u;
    game->previous_piece = -1;
    game->mode = mode;
    game->start_level = start_level;
    game->level = start_level;
    game->start_height = start_height;
    game->transition_lines = mode == TETRIS_MODE_A
        ? tetris_level_transition_lines(start_level) : 0;
    game->lines = mode == TETRIS_MODE_B ? TETRIS_TYPE_B_GOAL : 0;
    game->show_next = true;

    if (mode == TETRIS_MODE_B) init_type_b_playfield(game);
    game->next = choose_piece(game);
    spawn_piece(game);
}

void tetris_init(TetrisGame *game, uint32_t seed, int start_level) {
    tetris_init_mode(game, seed, start_level, TETRIS_MODE_A, 0);
}

bool tetris_try_move(TetrisGame *game, int dx, int dy) {
    if (game->phase != TETRIS_PHASE_ACTIVE || game->paused) return false;
    if (!collides(game, game->active, game->rotation,
                  game->x + dx, game->y + dy)) {
        game->x += dx;
        game->y += dy;
        if (dx != 0) set_event(game, TETRIS_EVENT_MOVE);
        return true;
    }
    return false;
}

bool tetris_try_rotate(TetrisGame *game, int direction) {
    if (game->phase != TETRIS_PHASE_ACTIVE || game->paused) return false;
    {
        const int new_rotation = (game->rotation + (direction > 0 ? 1 : 3)) & 3;
        if (!collides(game, game->active, new_rotation, game->x, game->y)) {
            game->rotation = new_rotation;
            set_event(game, TETRIS_EVENT_ROTATE);
            return true;
        }
    }
    return false;
}

static int find_completed_rows(TetrisGame *game) {
    int count = 0;
    for (int y = TETRIS_BOARD_H - 1;
         y >= 0 && count < TETRIS_MAX_CLEAR_ROWS; --y) {
        bool full = true;
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (game->board[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) game->clear_rows[count++] = y;
    }
    for (int i = count; i < TETRIS_MAX_CLEAR_ROWS; ++i) game->clear_rows[i] = -1;
    return count;
}

static void collapse_completed_rows(TetrisGame *game) {
    int dst = TETRIS_BOARD_H - 1;
    for (int src = TETRIS_BOARD_H - 1; src >= 0; --src) {
        bool cleared = false;
        for (int i = 0; i < game->clear_count; ++i) {
            if (game->clear_rows[i] == src) {
                cleared = true;
                break;
            }
        }
        if (!cleared) {
            if (dst != src) memcpy(game->board[dst], game->board[src], TETRIS_BOARD_W);
            --dst;
        }
    }
    while (dst >= 0) {
        memset(game->board[dst], 0, TETRIS_BOARD_W);
        --dst;
    }
}

static int bcd_decades_value(int lines) {
    const int hundreds = (lines / 100) % 10;
    const int tens = (lines / 10) % 10;
    return (hundreds << 4) | tens;
}

static void add_lines_and_level(TetrisGame *game, int count) {
    if (game->mode == TETRIS_MODE_B) {
        game->total_lines += count;
        game->lines -= count;
        if (game->lines < 0) game->lines = 0;
        return;
    }

    for (int i = 0; i < count; ++i) {
        if (game->lines < 999) ++game->lines;
        if (game->total_lines < 999) ++game->total_lines;
        if ((game->lines % 10) == 0 &&
            game->level < bcd_decades_value(game->lines)) {
            if (game->level < 255) ++game->level;
            set_event(game, TETRIS_EVENT_LEVEL_UP);
        }
    }
}

static void add_score(TetrisGame *game, int points) {
    if (points <= 0 || game->score >= 999999) return;
    if (points > 999999 - game->score) game->score = 999999;
    else game->score += points;
}

static void begin_entry_delay(TetrisGame *game) {
    game->phase = TETRIS_PHASE_ENTRY_DELAY;
    game->phase_timer = tetris_entry_delay_frames(game->lock_bottom_row);
    game->clear_count = 0;
    game->clear_step = 0;
}

static void finish_line_clear(TetrisGame *game) {
    static const int SCORE_TABLE[5] = {0, 40, 100, 300, 1200};
    const int cleared = game->clear_count;
    collapse_completed_rows(game);
    add_lines_and_level(game, cleared);
    add_score(game, SCORE_TABLE[cleared] * (game->level + 1));
    if (cleared == 4) set_event(game, TETRIS_EVENT_TETRIS);
    else set_event(game, TETRIS_EVENT_LINE);

    if (game->mode == TETRIS_MODE_B && game->lines == 0) {
        add_score(game, tetris_type_b_completion_bonus(game->start_level,
                                                       game->start_height));
        game->phase = TETRIS_PHASE_COMPLETE;
        game->phase_timer = 0;
        game->completed = true;
        game->game_over = false;
        set_event(game, TETRIS_EVENT_COMPLETE);
        return;
    }
    begin_entry_delay(game);
}

static void lock_piece(TetrisGame *game) {
    bool above_top = false;
    int bottom = INT_MIN;
    for (int i = 0; i < 4; ++i) {
        const int x = game->x + tetris_piece_block_x(game->active,
                                                     game->rotation, i);
        const int y = game->y + tetris_piece_block_y(game->active,
                                                     game->rotation, i);
        if (y > bottom) bottom = y;
        if (y < 0) {
            above_top = true;
        } else if (x >= 0 && x < TETRIS_BOARD_W && y < TETRIS_BOARD_H) {
            game->board[y][x] = (uint8_t)game->active + 1u;
        }
    }
    game->lock_bottom_row = bottom;
    set_event(game, TETRIS_EVENT_LOCK);

    if (game->soft_drop_points >= 2) add_score(game, game->soft_drop_points - 1);
    game->soft_drop_points = 0;

    if (above_top) {
        begin_game_over(game);
        return;
    }

    game->clear_count = find_completed_rows(game);
    if (game->clear_count > 0) {
        game->phase = TETRIS_PHASE_LINE_CLEAR;
        game->phase_timer = 0;
        game->clear_step = 0;
    } else {
        begin_entry_delay(game);
    }
}

void tetris_hard_drop(TetrisGame *game) {
    if (game->phase != TETRIS_PHASE_ACTIVE || game->paused) return;
    while (!collides(game, game->active, game->rotation,
                     game->x, game->y + 1)) {
        ++game->y;
    }
    lock_piece(game);
}

static uint8_t horizontal_input_mask(const TetrisInput *input) {
    uint8_t mask = 0u;
    if (!input) return 0u;
    if (input->left) mask |= 1u;
    if (input->right) mask |= 2u;
    return mask;
}

static void update_horizontal(TetrisGame *game, const TetrisInput *input) {
    const uint8_t held = horizontal_input_mask(input);
    const uint8_t newly_pressed = (uint8_t)(held & ~game->horizontal_buttons);
    int direction;

    /* Original shift_tetrimino returns without discharging DAS while DOWN is held. */
    if (input->down) return;

    /* The original checks RIGHT first, so RIGHT wins when both are held. */
    if ((held & 2u) != 0u) direction = 1;
    else if ((held & 1u) != 0u) direction = -1;
    else {
        game->das_direction = 0;
        return;
    }
    game->das_direction = direction;

    if (newly_pressed != 0u) {
        game->das_counter = 0;
    } else {
        ++game->das_counter;
        if (game->das_counter < 16) return;
        game->das_counter = 10;
    }

    if (!tetris_try_move(game, direction, 0)) {
        /* DAS_RESET=$10 on NTSC after a wall collision. */
        game->das_counter = 16;
    }
}

static void tick_active(TetrisGame *game, const TetrisInput *input) {
    update_horizontal(game, input);
    if (input->rotate_cw_pressed) (void)tetris_try_rotate(game, 1);
    if (input->rotate_ccw_pressed) (void)tetris_try_rotate(game, -1);
    if (input->hard_drop_pressed) {
        tetris_hard_drop(game);
        return;
    }

    if (input->down && !input->left && !input->right) {
        ++game->soft_drop_counter;
        if (game->soft_drop_counter >= 2) {
            game->soft_drop_counter = 0;
            game->fall_counter = 0;
            if (!collides(game, game->active, game->rotation,
                          game->x, game->y + 1)) {
                ++game->y;
                ++game->soft_drop_points;
            } else {
                lock_piece(game);
            }
        }
        return;
    }

    game->soft_drop_counter = 0;
    game->soft_drop_points = 0;
    ++game->fall_counter;
    if (game->fall_counter >= tetris_gravity_frames(game->level)) {
        game->fall_counter = 0;
        if (!collides(game, game->active, game->rotation,
                      game->x, game->y + 1)) {
            ++game->y;
        } else {
            lock_piece(game);
        }
    }
}

static void tick_line_clear(TetrisGame *game) {
    ++game->phase_timer;
    /* The PPU animation is keyed to the global NMI frame counter, not phase age. */
    if ((game->frame & 3) != 0) return;
    if (game->clear_step < 4) {
        ++game->clear_step;
        return;
    }
    finish_line_clear(game);
}

static void tick_entry_delay(TetrisGame *game) {
    if (game->phase_timer > 0) --game->phase_timer;
    if (game->phase_timer <= 0) spawn_piece(game);
}

static void tick_game_over_curtain(TetrisGame *game) {
    ++game->phase_timer;
    if ((game->frame & 3) == 0 && game->curtain_rows < TETRIS_BOARD_H) {
        ++game->curtain_rows;
    }
    if (game->curtain_rows >= TETRIS_BOARD_H) {
        game->phase = TETRIS_PHASE_GAME_OVER;
        game->game_over = true;
        set_event(game, TETRIS_EVENT_GAME_OVER);
    }
}

void tetris_tick(TetrisGame *game, const TetrisInput *input) {
    const uint8_t held_horizontal = horizontal_input_mask(input);
    if (input->restart_pressed &&
        (game->phase == TETRIS_PHASE_GAME_OVER ||
         game->phase == TETRIS_PHASE_COMPLETE)) {
        const uint32_t seed = (uint32_t)game->rng_seed ^
                              (uint32_t)game->frame ^ 0xa511e9b3u;
        tetris_init_mode(game, seed, game->start_level,
                         game->mode, game->start_height);
        game->horizontal_buttons = held_horizontal;
        return;
    }
    if (input->toggle_next_pressed &&
        game->phase != TETRIS_PHASE_GAME_OVER &&
        game->phase != TETRIS_PHASE_COMPLETE) {
        game->show_next = !game->show_next;
    }
    if (input->pause_pressed &&
        game->phase != TETRIS_PHASE_GAME_OVER &&
        game->phase != TETRIS_PHASE_GAME_OVER_CURTAIN &&
        game->phase != TETRIS_PHASE_COMPLETE) {
        game->paused = !game->paused;
    }

    /* The original RNG advances from NMI even while play is paused. */
    advance_rng(game);
    if (game->paused || game->phase == TETRIS_PHASE_GAME_OVER ||
        game->phase == TETRIS_PHASE_COMPLETE) {
        game->horizontal_buttons = held_horizontal;
        return;
    }

    ++game->frame;
    switch (game->phase) {
        case TETRIS_PHASE_ACTIVE: tick_active(game, input); break;
        case TETRIS_PHASE_LINE_CLEAR: tick_line_clear(game); break;
        case TETRIS_PHASE_ENTRY_DELAY: tick_entry_delay(game); break;
        case TETRIS_PHASE_GAME_OVER_CURTAIN: tick_game_over_curtain(game); break;
        case TETRIS_PHASE_GAME_OVER:
        case TETRIS_PHASE_COMPLETE:
            break;
    }
    game->horizontal_buttons = held_horizontal;
}

bool tetris_cell_hidden(const TetrisGame *game, int x, int y) {
    if (game->phase != TETRIS_PHASE_LINE_CLEAR) return false;
    {
        bool row_is_clearing = false;
        for (int i = 0; i < game->clear_count; ++i) {
            if (game->clear_rows[i] == y) {
                row_is_clearing = true;
                break;
            }
        }
        if (!row_is_clearing) return false;
    }
    for (int step = 0; step <= game->clear_step && step < 5; ++step) {
        if (x == 4 - step || x == 5 + step) return true;
    }
    return false;
}

uint32_t tetris_consume_events(TetrisGame *game) {
    const uint32_t events = game->events;
    game->events = 0;
    return events;
}
