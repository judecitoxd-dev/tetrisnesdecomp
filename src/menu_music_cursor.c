#include "menu_music_cursor.h"
#include "rom_screens.h"
#include "rom_sprites.h"
#include "songs.h"

#include <string.h>

/*
 * The 6502 type-menu renderer places sprite53MusicTypeCursor at NES X=$67 and
 * Y=$8F + musicType*$10. Custom songs reuse the same three visible rows and
 * scroll their labels inside the existing frame; OFF remains the fourth row.
 */
static bool type_music_cursor_visible(void) {
    const unsigned frame = (unsigned)((SDL_GetTicks() * 60u) / 1000u);
    return (frame & 3u) != 0u;
}

static int menu_font_tile(char character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'A' && character <= 'Z') return 10 + character - 'A';
    if (character == '-') return 36;
    return -1;
}

static void draw_menu_text(SDL_Renderer *renderer, SDL_Texture *font,
                           int x, int y, const char *text) {
    const int scale = 2;
    const char *cursor;
    if (!renderer || !font || !text) return;
    for (cursor = text; *cursor; ++cursor) {
        if (*cursor == ' ') {
            x += 8 * scale;
            continue;
        }
        {
            const int tile = menu_font_tile(*cursor);
            if (tile >= 0) {
                SDL_Rect source = {
                    (tile % 16) * 8, (tile / 16) * 8, 8, 8
                };
                SDL_Rect destination = {x, y, 8 * scale, 8 * scale};
                SDL_RenderCopy(renderer, font, &source, &destination);
            }
        }
        x += 8 * scale;
    }
}

static void render_song_labels(SDL_Renderer *renderer, SDL_Texture *font,
                               const TetrisSettings *settings) {
    const int total = tetris_songs_total_music_count();
    const int start = tetris_songs_menu_window_start(settings->music_track);
    int row;
    if (!font || tetris_songs_count() <= 0) return;

    for (row = 0; row < 3; ++row) {
        const int selection = start + row;
        const int nes_y = 0x8f + row * 0x10;
        SDL_Rect clean = {
            64 + 0x6c * 2,
            (nes_y - 2) * 2,
            0x4a * 2,
            10 * 2
        };
        char visible[10];
        const char *label = selection < total
            ? tetris_songs_label(selection) : "";
        size_t length = strlen(label);
        if (length > 9u) length = 9u;
        memcpy(visible, label, length);
        visible[length] = '\0';

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &clean);
        draw_menu_text(renderer, font, 64 + 0x70 * 2,
                       nes_y * 2, visible);
    }
}

void render_type_music_cursor_overlay(SDL_Renderer *renderer,
                                      SDL_Texture *font,
                                      AppScreen screen,
                                      const TetrisSettings *settings) {
    int music_row;
    int nes_x;
    int nes_y;
    SDL_Rect outer;
    SDL_Rect inner;

    if (!renderer || !settings || screen != SCREEN_TYPE_SELECT ||
        !tetris_rom_screen_available(TETRIS_ROM_SCREEN_TYPE_MENU)) return;

    render_song_labels(renderer, font, settings);
    if (!type_music_cursor_visible()) return;

    music_row = tetris_songs_count() > 0
        ? tetris_songs_menu_row(settings->music_track)
        : (settings->music_track < 0 ? 3 : settings->music_track);
    if (music_row < 0) music_row = 0;
    if (music_row > 3) music_row = 3;

    nes_x = 0x67;
    nes_y = 0x8f + music_row * 0x10;
    if (tetris_rom_sprite_available(TETRIS_ROM_SPRITE_MUSIC_CURSOR)) {
        tetris_rom_sprite_render(renderer, TETRIS_ROM_SPRITE_MUSIC_CURSOR,
                                 nes_x, nes_y);
        return;
    }

    outer.x = 64 + nes_x * 2;
    outer.y = nes_y * 2;
    outer.w = (0x4a + 8) * 2;
    outer.h = 8 * 2;
    inner.x = outer.x + 2;
    inner.y = outer.y + 2;
    inner.w = outer.w - 4;
    inner.h = outer.h - 4;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 236, 238, 236, 235);
    SDL_RenderDrawRect(renderer, &outer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderDrawRect(renderer, &inner);
}
