#include "cathedral.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

#define TEST_PRG_SIZE 0x27fdu

static TetrisCathedralTables fake_tables(void) {
    TetrisCathedralTables tables;
    memset(&tables, 0, sizeof(tables));
    for (int level = 0; level < TETRIS_CATHEDRAL_LEVELS; ++level) {
        tables.animation_speed[level] = 1;
        tables.frame_delay[level] = 1;
        tables.start_x[level] = 2;
        tables.sentinel_x[level] = 0;
        tables.vector_x[level] = 1;
        tables.sprite_base[level] = 1;
    }
    tables.animation_speed[0] = 2;
    tables.frame_delay[0] = 3;
    tables.sprite_base[0] = 0x2c;
    tables.trigger_x[0] = 0x3a;
    tables.trigger_x[1] = 0x24;
    tables.position_y[0] = 0x98;
    tables.position_y[1] = 0xa8;

    tables.animation_speed[1] = 4;
    tables.frame_delay[1] = 1;
    tables.sprite_base[1] = 0x2e;
    tables.position_y[6] = 0xb0;

    tables.animation_speed[2] = 6;
    tables.frame_delay[2] = 1;
    tables.start_x[2] = 0xfe;
    tables.vector_x[2] = -1;
    tables.sprite_base[2] = 0x54;
    tables.position_y[12] = 0xc8;
    tables.valid = true;
    return tables;
}

static int test_loader(void) {
    uint8_t prg[TEST_PRG_SIZE];
    TetrisCathedralTables tables;
    memset(prg, 0, sizeof(prg));
    for (int level = 0; level < TETRIS_CATHEDRAL_LEVELS; ++level) {
        prg[0x2749u + (size_t)level] = (uint8_t)(level + 1);
        prg[0x2753u + (size_t)level] = 1;
        prg[0x275du + (size_t)level] = 2;
        prg[0x2767u + (size_t)level] = 0;
        prg[0x2771u + (size_t)level] = 1;
        prg[0x27f3u + (size_t)level] = (uint8_t)(10 + level);
    }
    for (size_t index = 0; index < 60u; ++index) {
        prg[0x277bu + index] = (uint8_t)index;
        prg[0x27b7u + index] = (uint8_t)(200u - index);
    }
    CHECK(!tetris_cathedral_tables_load(&tables, prg, sizeof(prg) - 1u));
    CHECK(tetris_cathedral_tables_load(&tables, prg, sizeof(prg)));
    CHECK(tables.valid);
    CHECK(tables.animation_speed[9] == 10u);
    CHECK(tables.trigger_x[59] == 59u);
    CHECK(tables.position_y[59] == 141u);
    CHECK(tables.sprite_base[0] == 10u);
    return 0;
}

static int test_initial_state(void) {
    TetrisCathedralTables tables = fake_tables();
    TetrisCathedralSnapshot snapshot;
    tetris_cathedral_snapshot(&tables, 0, 0, 0, &snapshot);
    CHECK(snapshot.sprite_index == 0x2cu);
    CHECK(snapshot.sprite_count == 1);
    CHECK(snapshot.visible[0]);
    CHECK(snapshot.x[0] == 0x02u);
    CHECK(snapshot.y[0] == 0x98u);
    return 0;
}

static int test_animation_and_render_before_move(void) {
    TetrisCathedralTables tables = fake_tables();
    TetrisCathedralSnapshot snapshot;
    tetris_cathedral_snapshot(&tables, 0, 0, 1, &snapshot);
    CHECK(snapshot.sprite_index == 0x2du);
    CHECK(snapshot.x[0] == 0x02u);

    tetris_cathedral_snapshot(&tables, 0, 0, 2, &snapshot);
    CHECK(snapshot.x[0] == 0x02u);
    tetris_cathedral_snapshot(&tables, 0, 0, 3, &snapshot);
    CHECK(snapshot.x[0] == 0x03u);

    tetris_cathedral_snapshot(&tables, 1, 0, 0, &snapshot);
    CHECK(snapshot.x[0] == 0x02u);
    tetris_cathedral_snapshot(&tables, 1, 0, 1, &snapshot);
    CHECK(snapshot.x[0] == 0x03u);
    CHECK(snapshot.y[0] == 0xb0u);

    tetris_cathedral_snapshot(&tables, 2, 0, 0, &snapshot);
    CHECK(snapshot.x[0] == 0xfeu);
    tetris_cathedral_snapshot(&tables, 2, 0, 1, &snapshot);
    CHECK(snapshot.x[0] == 0xfdu);
    CHECK(snapshot.y[0] == 0xc8u);
    return 0;
}

static int test_spawn_chain(void) {
    TetrisCathedralTables tables = fake_tables();
    TetrisCathedralSnapshot snapshot;
    tetris_cathedral_snapshot(&tables, 0, 1, 168, &snapshot);
    CHECK(snapshot.sprite_count == 2);
    CHECK(snapshot.visible[0]);
    CHECK(snapshot.visible[1]);
    CHECK(snapshot.x[0] == 0x3au);
    CHECK(snapshot.x[1] == 0x03u);
    CHECK(snapshot.y[1] == 0xa8u);
    return 0;
}

static int test_normalization_and_clamps(void) {
    TetrisCathedralTables tables = fake_tables();
    TetrisCathedralSnapshot level0;
    TetrisCathedralSnapshot level10;
    TetrisCathedralSnapshot low_height;
    TetrisCathedralSnapshot high_height;

    tetris_cathedral_snapshot(&tables, 0, 2, 33, &level0);
    tetris_cathedral_snapshot(&tables, 10, 2, 33, &level10);
    CHECK(level0.sprite_index == level10.sprite_index);
    CHECK(level0.sprite_count == level10.sprite_count);
    for (int index = 0; index < level0.sprite_count; ++index) {
        CHECK(level0.visible[index] == level10.visible[index]);
        CHECK(level0.x[index] == level10.x[index]);
        CHECK(level0.y[index] == level10.y[index]);
    }

    tetris_cathedral_snapshot(&tables, 4, -3, 0, &low_height);
    CHECK(low_height.sprite_count == 1);
    tetris_cathedral_snapshot(&tables, 4, 99, 0, &high_height);
    CHECK(high_height.sprite_count == TETRIS_CATHEDRAL_MAX_SPRITES);
    return 0;
}

int main(void) {
    if (test_loader() != 0) return 1;
    if (test_initial_state() != 0) return 1;
    if (test_animation_and_render_before_move() != 0) return 1;
    if (test_spawn_chain() != 0) return 1;
    if (test_normalization_and_clamps() != 0) return 1;
    puts("Cathedral movement tests passed.");
    return 0;
}
