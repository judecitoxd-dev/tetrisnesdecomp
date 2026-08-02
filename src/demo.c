#include "demo.h"

#include <limits.h>
#include <string.h>

#define DEMO_BUTTONS_PRG_OFFSET 0x5d00u
#define DEMO_BUTTONS_SIZE       0x0200u
#define DEMO_PIECES_PRG_OFFSET  0x5f00u
#define DEMO_PIECES_SIZE        0x0100u

#define NES_BUTTON_A     0x80u
#define NES_BUTTON_B     0x40u
#define NES_BUTTON_DOWN  0x04u
#define NES_BUTTON_LEFT  0x02u
#define NES_BUTTON_RIGHT 0x01u

static bool placement_collides(const uint8_t *board, TetrisPiece piece,
                               int rotation, int px, int py) {
    int block;
    for (block = 0; block < 4; ++block) {
        const int x = px + tetris_piece_block_x(piece, rotation, block);
        const int y = py + tetris_piece_block_y(piece, rotation, block);
        if (x < 0 || x >= TETRIS_BOARD_W || y >= TETRIS_BOARD_H) return true;
        if (y >= 0 && board[y * TETRIS_BOARD_W + x] != 0) return true;
    }
    return false;
}

static int unique_rotation_count(TetrisPiece piece) {
    if (piece == PIECE_O) return 1;
    if (piece == PIECE_I || piece == PIECE_S || piece == PIECE_Z) return 2;
    return 4;
}

static int evaluate_placement(const TetrisGame *game, int rotation, int px) {
    uint8_t board[TETRIS_BOARD_H][TETRIS_BOARD_W];
    int py = -4;
    int block;
    int lines = 0;
    int aggregate_height = 0;
    int holes = 0;
    int bumpiness = 0;
    int maximum_height = 0;
    int previous_height = -1;

    memcpy(board, game->board, sizeof(board));
    if (placement_collides(&board[0][0], game->active, rotation, px, py)) return INT_MIN;
    while (!placement_collides(&board[0][0], game->active, rotation, px, py + 1)) ++py;

    for (block = 0; block < 4; ++block) {
        const int x = px + tetris_piece_block_x(game->active, rotation, block);
        const int y = py + tetris_piece_block_y(game->active, rotation, block);
        if (y < 0 || x < 0 || x >= TETRIS_BOARD_W || y >= TETRIS_BOARD_H) {
            return INT_MIN;
        }
        board[y][x] = (uint8_t)game->active + 1u;
    }

    for (int y = TETRIS_BOARD_H - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (board[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            ++lines;
            for (int pull = y; pull > 0; --pull) {
                memcpy(board[pull], board[pull - 1], TETRIS_BOARD_W);
            }
            memset(board[0], 0, TETRIS_BOARD_W);
            ++y;
        }
    }

    for (int x = 0; x < TETRIS_BOARD_W; ++x) {
        int first = TETRIS_BOARD_H;
        bool seen_block = false;
        for (int y = 0; y < TETRIS_BOARD_H; ++y) {
            if (board[y][x] != 0) {
                if (!seen_block) first = y;
                seen_block = true;
            } else if (seen_block) {
                ++holes;
            }
        }
        {
            const int height = TETRIS_BOARD_H - first;
            aggregate_height += height;
            if (height > maximum_height) maximum_height = height;
            if (previous_height >= 0) {
                int difference = height - previous_height;
                if (difference < 0) difference = -difference;
                bumpiness += difference;
            }
            previous_height = height;
        }
    }

    return lines * 1200 - holes * 180 - aggregate_height * 8 -
           bumpiness * 5 - maximum_height * 12;
}

static void plan_piece(TetrisDemoController *demo, const TetrisGame *game) {
    int best_score = INT_MIN;
    int best_rotation = game->rotation;
    int best_x = game->x;
    const int rotations = unique_rotation_count(game->active);

    for (int rotation_index = 0; rotation_index < rotations; ++rotation_index) {
        const int rotation = rotation_index;
        for (int x = -2; x < TETRIS_BOARD_W + 2; ++x) {
            const int score = evaluate_placement(game, rotation, x);
            if (score > best_score ||
                (score == best_score && x < best_x)) {
                best_score = score;
                best_rotation = rotation;
                best_x = x;
            }
        }
    }
    demo->target_rotation = best_rotation;
    demo->target_x = best_x;
    demo->observed_spawn_count = game->spawn_count;
    demo->has_plan = best_score != INT_MIN;
}

static TetrisPiece demo_piece_from_byte(uint8_t value) {
    const unsigned index = (unsigned)((value >> 4) & 7u);
    return index == 7u ? PIECE_T : (TetrisPiece)index;
}

static void apply_initial_rom_pieces(TetrisDemoController *demo,
                                     TetrisGame *game) {
    if (!demo || !game || !demo->piece_data) return;
    game->active = demo_piece_from_byte(demo->piece_data[0]);
    game->next = demo_piece_from_byte(demo->piece_data[1]);
    game->rotation = tetris_spawn_rotation(game->active);
    game->x = 5;
    game->y = 0;
    game->fall_counter = 0;
    game->das_counter = 0;
    game->das_direction = 0;
    game->soft_drop_counter = 0;
    game->soft_drop_points = 0;
    memset(game->piece_count, 0, sizeof(game->piece_count));
    game->piece_count[game->active] = 1;
    demo->piece_index = 2;
    demo->observed_spawn_count = game->spawn_count;
}

void tetris_demo_reset(TetrisDemoController *demo) {
    if (!demo) return;
    memset(demo, 0, sizeof(*demo));
    demo->observed_spawn_count = 0xffu;
}

bool tetris_demo_reset_from_rom(TetrisDemoController *demo,
                                TetrisGame *game, const NesRom *rom) {
    tetris_demo_reset(demo);
    if (!demo || !game || !rom || !rom->exact_supported_dump ||
        !rom->prg || rom->prg_size < DEMO_PIECES_PRG_OFFSET + DEMO_PIECES_SIZE) {
        return false;
    }
    demo->button_data = rom->prg + DEMO_BUTTONS_PRG_OFFSET;
    demo->piece_data = rom->prg + DEMO_PIECES_PRG_OFFSET;
    demo->rom_script = true;
    apply_initial_rom_pieces(demo, game);
    return true;
}

void tetris_demo_sync_after_tick(TetrisDemoController *demo,
                                 TetrisGame *game) {
    if (!demo || !game || !demo->rom_script || demo->finished) return;
    if (demo->observed_spawn_count == game->spawn_count) return;
    demo->observed_spawn_count = game->spawn_count;
    if (demo->piece_index >= DEMO_PIECES_SIZE) {
        demo->finished = true;
        return;
    }
    game->next = demo_piece_from_byte(demo->piece_data[demo->piece_index++]);
}

bool tetris_demo_is_finished(const TetrisDemoController *demo) {
    return demo && demo->finished;
}

bool tetris_demo_uses_rom_script(const TetrisDemoController *demo) {
    return demo && demo->rom_script;
}

static TetrisInput next_rom_input(TetrisDemoController *demo,
                                  TetrisGame *game) {
    TetrisInput input;
    uint8_t newly_pressed = 0;
    memset(&input, 0, sizeof(input));

    if (demo->finished) return input;
    if (demo->repeats != 0u) {
        demo->repeats = (uint8_t)(demo->repeats - 1u);
    } else {
        uint8_t next_buttons;
        if (demo->button_index + 1u >= DEMO_BUTTONS_SIZE) {
            demo->finished = true;
            game->phase = TETRIS_PHASE_GAME_OVER;
            game->game_over = true;
            return input;
        }
        next_buttons = demo->button_data[demo->button_index++];
        newly_pressed = (uint8_t)((demo->held_buttons ^ next_buttons) &
                                  next_buttons);
        demo->held_buttons = next_buttons;
        demo->repeats = demo->button_data[demo->button_index++];
    }

    input.left = (demo->held_buttons & NES_BUTTON_LEFT) != 0u;
    input.right = (demo->held_buttons & NES_BUTTON_RIGHT) != 0u;
    input.down = (demo->held_buttons & NES_BUTTON_DOWN) != 0u;
    input.rotate_cw_pressed = (newly_pressed & NES_BUTTON_A) != 0u;
    input.rotate_ccw_pressed = (newly_pressed & NES_BUTTON_B) != 0u;
    return input;
}

TetrisInput tetris_demo_next_input(TetrisDemoController *demo,
                                   TetrisGame *game) {
    TetrisInput input;
    memset(&input, 0, sizeof(input));
    if (!demo || !game) return input;
    if (demo->rom_script) {
        tetris_demo_sync_after_tick(demo, game);
        return next_rom_input(demo, game);
    }
    if (game->paused || game->phase != TETRIS_PHASE_ACTIVE) return input;

    if (!demo->has_plan || demo->observed_spawn_count != game->spawn_count) {
        plan_piece(demo, game);
    }
    if (!demo->has_plan) return input;

    if ((game->rotation & 3) != (demo->target_rotation & 3)) {
        input.rotate_cw_pressed = true;
    } else if (game->x < demo->target_x) {
        input.right = true;
    } else if (game->x > demo->target_x) {
        input.left = true;
    } else {
        input.hard_drop_pressed = true;
        demo->has_plan = false;
    }
    return input;
}
