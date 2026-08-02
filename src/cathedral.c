#include "cathedral.h"

#include <string.h>

#define PRG_ANIMATION_SPEED 0x2749u
#define PRG_FRAME_DELAY    0x2753u
#define PRG_START_X        0x275du
#define PRG_SENTINEL_X     0x2767u
#define PRG_VECTOR_X       0x2771u
#define PRG_TRIGGER_X      0x277bu
#define PRG_POSITION_Y     0x27b7u
#define PRG_SPRITE_BASE    0x27f3u
#define PRG_TABLES_END     0x27fdu

static int normalize_level(int level) {
    level %= TETRIS_CATHEDRAL_LEVELS;
    if (level < 0) level += TETRIS_CATHEDRAL_LEVELS;
    return level;
}

static int clamp_height(int start_height) {
    if (start_height < 0) return 0;
    if (start_height >= TETRIS_CATHEDRAL_MAX_SPRITES)
        return TETRIS_CATHEDRAL_MAX_SPRITES - 1;
    return start_height;
}

bool tetris_cathedral_tables_load(TetrisCathedralTables *tables,
                                  const uint8_t *prg, size_t prg_size) {
    int level;
    if (!tables) return false;
    memset(tables, 0, sizeof(*tables));
    if (!prg || prg_size < PRG_TABLES_END) return false;

    memcpy(tables->animation_speed, prg + PRG_ANIMATION_SPEED,
           sizeof(tables->animation_speed));
    memcpy(tables->frame_delay, prg + PRG_FRAME_DELAY,
           sizeof(tables->frame_delay));
    memcpy(tables->start_x, prg + PRG_START_X,
           sizeof(tables->start_x));
    memcpy(tables->sentinel_x, prg + PRG_SENTINEL_X,
           sizeof(tables->sentinel_x));
    memcpy(tables->vector_x, prg + PRG_VECTOR_X,
           sizeof(tables->vector_x));
    memcpy(tables->trigger_x, prg + PRG_TRIGGER_X,
           sizeof(tables->trigger_x));
    memcpy(tables->position_y, prg + PRG_POSITION_Y,
           sizeof(tables->position_y));
    memcpy(tables->sprite_base, prg + PRG_SPRITE_BASE,
           sizeof(tables->sprite_base));

    for (level = 0; level < TETRIS_CATHEDRAL_LEVELS; ++level) {
        if (tables->animation_speed[level] == 0 ||
            tables->frame_delay[level] == 0 ||
            tables->sprite_base[level] >= 90u) {
            memset(tables, 0, sizeof(*tables));
            return false;
        }
    }
    tables->valid = true;
    return true;
}

void tetris_cathedral_state_init(TetrisCathedralState *state,
                                 const TetrisCathedralTables *tables,
                                 int level, int start_height) {
    int index;
    if (!state) return;
    memset(state, 0, sizeof(*state));
    if (!tables || !tables->valid) return;
    state->tables = tables;
    state->level = normalize_level(level);
    state->sprite_count = clamp_height(start_height) + 1;
    state->x[0] = tables->start_x[state->level];
    for (index = 1; index < TETRIS_CATHEDRAL_MAX_SPRITES; ++index)
        state->x[index] = tables->sentinel_x[state->level];
    state->valid = true;
}

bool tetris_cathedral_state_step(TetrisCathedralState *state,
                                 TetrisCathedralSnapshot *snapshot) {
    const TetrisCathedralTables *tables;
    int index;
    bool move_frame;
    if (snapshot) memset(snapshot, 0, sizeof(*snapshot));
    if (!state || !state->valid || !state->tables || !snapshot) return false;
    tables = state->tables;

    /* ending_typeBCathedralSetSprite executes first on each rendered frame. */
    ++state->animation_counter;
    if (state->animation_counter == tables->animation_speed[state->level]) {
        state->animation_phase ^= 1u;
        state->animation_counter = 0;
    }
    snapshot->sprite_index =
        tables->sprite_base[state->level] + state->animation_phase;

    ++state->frame_delay_counter;
    move_frame = state->frame_delay_counter ==
                 tables->frame_delay[state->level];
    snapshot->sprite_count = state->sprite_count;

    for (index = 0; index < state->sprite_count; ++index) {
        const size_t table_index =
            (size_t)state->level * TETRIS_CATHEDRAL_MAX_SPRITES +
            (size_t)index;
        const uint8_t current_x = state->x[index];
        snapshot->x[index] = current_x;
        snapshot->y[index] = tables->position_y[table_index];
        snapshot->visible[index] =
            current_x != tables->sentinel_x[state->level];

        /* The original routine renders the old coordinate, then advances it. */
        if (move_frame && snapshot->visible[index]) {
            const uint8_t next_x = (uint8_t)(
                current_x + tables->vector_x[state->level]);
            state->x[index] = next_x;
            if (next_x == tables->trigger_x[table_index] &&
                index + 1 < state->sprite_count) {
                state->x[index + 1] = tables->start_x[state->level];
            }
        }
    }
    if (move_frame) state->frame_delay_counter = 0;
    ++state->frames;
    return true;
}

void tetris_cathedral_snapshot(const TetrisCathedralTables *tables,
                               int level, int start_height, unsigned frame,
                               TetrisCathedralSnapshot *snapshot) {
    TetrisCathedralState state;
    unsigned current;
    if (!snapshot) return;
    memset(snapshot, 0, sizeof(*snapshot));
    tetris_cathedral_state_init(&state, tables, level, start_height);
    if (!state.valid) return;
    for (current = 0; current <= frame; ++current) {
        if (!tetris_cathedral_state_step(&state, snapshot)) {
            memset(snapshot, 0, sizeof(*snapshot));
            return;
        }
    }
}
