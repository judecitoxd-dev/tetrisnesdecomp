#include "level_score_table_fix.h"
#include "exact_layout.h"
#include "rom_screens.h"

#include <stdio.h>
#include <string.h>

static int score_font_tile(char character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'A' && character <= 'Z')
        return 10 + character - 'A';
    if (character == '-') return 36;
    return -1;
}

static int ppu_x(uint16_t address) {
    return 64 + (int)((address - 0x2000u) & 31u) * 16;
}

static int ppu_y(uint16_t address) {
    return (int)((address - 0x2000u) >> 5) * 16;
}

static void clear_ppu_span(SDL_Renderer *renderer, uint16_t address,
                           int tile_count) {
    SDL_Rect rectangle;
    rectangle.x = ppu_x(address);
    rectangle.y = ppu_y(address);
    rectangle.w = tile_count * 16;
    rectangle.h = 16;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &rectangle);
}

static void draw_ppu_text(SDL_Renderer *renderer, SDL_Texture *font,
                          uint16_t address, const char *text) {
    int x = ppu_x(address);
    const int y = ppu_y(address);
    const char *cursor;
    if (!renderer || !font || !text) return;
    for (cursor = text; *cursor; ++cursor) {
        const int tile = score_font_tile(*cursor);
        if (tile >= 0) {
            SDL_Rect source = {(tile % 16) * 8, (tile / 16) * 8, 8, 8};
            SDL_Rect destination = {x, y, 16, 16};
            SDL_RenderCopy(renderer, font, &source, &destination);
        }
        x += 16;
    }
}

static void format_name(char output[7], const char *name) {
    size_t index;
    for (index = 0; index < 6u; ++index) {
        char character = name && name[index] ? name[index] : ' ';
        if (character >= 'a' && character <= 'z')
            character = (char)(character - 'a' + 'A');
        if (score_font_tile(character) < 0 && character != ' ')
            character = ' ';
        output[index] = character;
    }
    output[6] = '\0';
}

void render_level_score_table_fix(
    SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
    const AppMenuState *menu, const TetrisHighScores *scores) {
    static const uint16_t row_address[TETRIS_HIGH_SCORE_COUNT] = {
        TETRIS_PPU_RECORD_ROW_1,
        TETRIS_PPU_RECORD_ROW_2,
        TETRIS_PPU_RECORD_ROW_3
    };
    int mode;
    int rank;

    if (!renderer || !font || !menu || !scores ||
        screen != SCREEN_LEVEL_SELECT) return;
    if (!tetris_rom_screen_available(
            menu->mode == TETRIS_MODE_B
                ? TETRIS_ROM_SCREEN_LEVEL_B
                : TETRIS_ROM_SCREEN_LEVEL_A)) return;

    /*
     * The ROM background already owns the yellow frame. Only clear the text
     * cells. v0.18-v0.20 left the static header partially covered, which is why
     * the screenshot showed "NAME SCO" instead of "NAME SCORE LV".
     */
    clear_ppu_span(renderer,
                   (uint16_t)(0x2000u +
                       TETRIS_EXACT_RECORD_HEADER_Y * 32u +
                       TETRIS_EXACT_RECORD_NAME_X),
                   TETRIS_EXACT_RECORD_FIELD_W);
    draw_ppu_text(renderer, font, TETRIS_PPU_RECORD_HEADER_NAME, "NAME");
    draw_ppu_text(renderer, font, TETRIS_PPU_RECORD_HEADER_SCORE, "SCORE");
    draw_ppu_text(renderer, font, TETRIS_PPU_RECORD_HEADER_LEVEL, "LV");

    mode = menu->mode == TETRIS_MODE_B ? 1 : 0;
    for (rank = 0; rank < TETRIS_HIGH_SCORE_COUNT; ++rank) {
        const TetrisHighScoreEntry *entry = &scores->entries[mode][rank];
        char name[7];
        char value[16];
        const uint16_t base = row_address[rank];

        clear_ppu_span(renderer, base, TETRIS_EXACT_RECORD_FIELD_W);
        if (entry->score <= 0) continue;

        format_name(name, entry->name);
        draw_ppu_text(renderer, font, base, name);
        snprintf(value, sizeof(value), "%06d",
                 entry->score > 999999 ? 999999 : entry->score);
        draw_ppu_text(renderer, font, (uint16_t)(base + 7u), value);
        snprintf(value, sizeof(value), "%02d",
                 entry->level < 0 ? 0 :
                 (entry->level > 99 ? 99 : entry->level));
        draw_ppu_text(renderer, font, (uint16_t)(base + 14u), value);
    }
}
