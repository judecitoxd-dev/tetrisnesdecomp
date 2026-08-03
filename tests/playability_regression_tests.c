#include "exact_layout.h"
#include "game.h"
#include "settings.h"

#include <assert.h>
#include <stdio.h>

static void test_exact_layout_does_not_erase_board(void) {
    assert(TETRIS_EXACT_STATS_CLEAN_X + TETRIS_EXACT_STATS_CLEAN_W <=
           TETRIS_EXACT_BOARD_TILE_X);
    assert(TETRIS_EXACT_BOARD_TILE_W == TETRIS_BOARD_W);
    assert(TETRIS_EXACT_BOARD_TILE_H == TETRIS_BOARD_H);
    assert(TETRIS_PPU_LINES == 0x2073);
    assert(TETRIS_PPU_TOP_SCORE == 0x20b8);
    assert(TETRIS_PPU_SCORE == 0x2118);
    assert(TETRIS_PPU_LEVEL == 0x22ba);
}

static void test_level_score_table_layout(void) {
    assert(TETRIS_PPU_RECORD_HEADER_NAME == 0x224a);
    assert(TETRIS_PPU_RECORD_HEADER_SCORE == 0x2250);
    assert(TETRIS_PPU_RECORD_HEADER_LEVEL == 0x2257);
    assert(TETRIS_PPU_RECORD_ROW_1 == 0x2289);
    assert(TETRIS_PPU_RECORD_ROW_2 == 0x22c9);
    assert(TETRIS_PPU_RECORD_ROW_3 == 0x2309);
    assert((TETRIS_PPU_RECORD_HEADER_LEVEL & 31) + 2 <= 32);
    assert((TETRIS_PPU_RECORD_ROW_1 & 31) +
           TETRIS_EXACT_RECORD_FIELD_W <= 32);
}

static void test_saved_level_migration(void) {
    TetrisSettings settings;
    tetris_settings_init(&settings);
    settings.last_level = 10;
    tetris_settings_sanitize(&settings);
    assert(settings.last_level == 0);
    settings.last_level = 19;
    tetris_settings_sanitize(&settings);
    assert(settings.last_level == 9);
}

static void test_level_menu_range(void) {
    assert(tetris_settings_step_level(0, -1) == 0);
    assert(tetris_settings_step_level(0, 5) == 5);
    assert(tetris_settings_step_level(5, 5) == 9);
    assert(tetris_settings_step_level(9, 1) == 9);
    assert(tetris_settings_step_level(10, 0) == 0);
}

static void test_original_music_menu_order(void) {
    int music = 0;
    music = tetris_settings_step_music(music, 1);
    assert(music == 1);
    music = tetris_settings_step_music(music, 1);
    assert(music == 2);
    music = tetris_settings_step_music(music, 1);
    assert(music == -1);
    assert(tetris_settings_step_music(music, 1) == -1);
    music = tetris_settings_step_music(music, -1);
    assert(music == 2);
    music = tetris_settings_step_music(music, -1);
    assert(music == 1);
    music = tetris_settings_step_music(music, -1);
    assert(music == 0);
    assert(tetris_settings_step_music(music, -1) == 0);
}

static void test_level_zero_game_timing(void) {
    TetrisGame game;
    tetris_init_mode(&game, 0x8988u, 0, TETRIS_MODE_A, 0);
    assert(game.start_level == 0);
    assert(game.level == 0);
    assert(tetris_gravity_frames(game.level) == 48);
    assert(game.phase == TETRIS_PHASE_ACTIVE);
}

int main(void) {
    test_exact_layout_does_not_erase_board();
    test_level_score_table_layout();
    test_saved_level_migration();
    test_level_menu_range();
    test_original_music_menu_order();
    test_level_zero_game_timing();
    puts("playability regression tests: OK");
    return 0;
}
