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

bool tetris_cathedral_tables_load(TetrisCathedralTables *tables,
                                  const uint8_t *prg, size_t prg_size) {
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

    for (int level = 0; level < TETRIS_CATHEDRAL_LEVELS; ++level) {
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

void tetris_cathedral_snapshot(const TetrisCathedralTables *tables,
                               int level, int start_height, unsigned frame,
                               TetrisCathedralSnapshot *snapshot) {
    uint8_t positions[TETRIS_CATHEDRAL_MAX_SPRITES];
    unsigned completed_movements;
    unsigned toggles;
    int normalized_level;
    int sprite_count;

    if (!snapshot) return;
    memset(snapshot, 0, sizeof(*snapshot));
    if (!tables || !tables->valid) return;

    normalized_level = level % TETRIS_CATHEDRAL_LEVELS;
    if (normalized_level < 0) normalized_level += TETRIS_CATHEDRAL_LEVELS;
    if (start_height < 0) start_height = 0;
    if (start_height >= TETRIS_CATHEDRAL_MAX_SPRITES)
        start_height = TETRIS_CATHEDRAL_MAX_SPRITES - 1;
    sprite_count = start_height + 1;

    positions[0] = tables->start_x[normalized_level];
    for (int index = 1; index < TETRIS_CATHEDRAL_MAX_SPRITES; ++index)
        positions[index] = tables->sentinel_x[normalized_level];

    /* The 6502 renders the current position, then advances it for next frame. */
    completed_movements = frame / tables->frame_delay[normalized_level];
    for (unsigned event = 0; event < completed_movements; ++event) {
        bool any_visible = false;
        for (int index = 0; index < sprite_count; ++index) {
            uint8_t next;
            if (positions[index] == tables->sentinel_x[normalized_level])
                continue;
            any_visible = true;
            next = (uint8_t)(positions[index] + tables->vector_x[normalized_level]);
            positions[index] = next;
            if (next == tables->trigger_x[
                    normalized_level * TETRIS_CATHEDRAL_MAX_SPRITES + index] &&
                index + 1 < sprite_count) {
                positions[index + 1] = tables->start_x[normalized_level];
            }
        }
        if (!any_visible) break;
    }

    toggles = (frame + 1u) / tables->animation_speed[normalized_level];
    snapshot->sprite_index = tables->sprite_base[normalized_level] +
                             (toggles & 1u);
    snapshot->sprite_count = sprite_count;
    for (int index = 0; index < sprite_count; ++index) {
        snapshot->x[index] = positions[index];
        snapshot->y[index] = tables->position_y[
            normalized_level * TETRIS_CATHEDRAL_MAX_SPRITES + index];
        snapshot->visible[index] =
            positions[index] != tables->sentinel_x[normalized_level];
    }
}
