#ifndef TETRIS_EXACT_LAYOUT_H
#define TETRIS_EXACT_LAYOUT_H

/* 256x240 NES nametable coordinates used by the exact SDL renderer. */
enum {
    TETRIS_EXACT_BOARD_TILE_X = 12,
    TETRIS_EXACT_BOARD_TILE_Y = 6,
    TETRIS_EXACT_BOARD_TILE_W = 10,
    TETRIS_EXACT_BOARD_TILE_H = 20,

    /* Cleanup region for the obsolete generic statistics text only. */
    TETRIS_EXACT_STATS_CLEAN_X = 6,
    TETRIS_EXACT_STATS_CLEAN_Y = 11,
    TETRIS_EXACT_STATS_CLEAN_W = 4,
    TETRIS_EXACT_STATS_CLEAN_H = 16,

    /* Level-select score table. The yellow frame itself is left untouched. */
    TETRIS_EXACT_RECORD_HEADER_Y = 18,
    TETRIS_EXACT_RECORD_NAME_X = 9,
    TETRIS_EXACT_RECORD_SCORE_X = 16,
    TETRIS_EXACT_RECORD_LEVEL_X = 23,
    TETRIS_EXACT_RECORD_FIELD_W = 16
};

/* PPU addresses copied from the original 6502 render routine. */
enum {
    TETRIS_PPU_LINES = 0x2073,
    TETRIS_PPU_TOP_SCORE = 0x20b8,
    TETRIS_PPU_SCORE = 0x2118,
    TETRIS_PPU_LEVEL = 0x22ba,

    TETRIS_PPU_RECORD_HEADER_NAME = 0x224a,
    TETRIS_PPU_RECORD_HEADER_SCORE = 0x2250,
    TETRIS_PPU_RECORD_HEADER_LEVEL = 0x2257,
    TETRIS_PPU_RECORD_ROW_1 = 0x2289,
    TETRIS_PPU_RECORD_ROW_2 = 0x22c9,
    TETRIS_PPU_RECORD_ROW_3 = 0x2309
};

#endif
