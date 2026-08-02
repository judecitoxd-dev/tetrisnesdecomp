#include "menu_music_cursor.h"
#include "rom_screens.h"

/*
 * The 6502 type-menu renderer places sprite53MusicTypeCursor at NES X=$67 and
 * Y=$8F + musicType*$10. v0.19 changed the setting but never drew that second
 * cursor, making Up/Down look broken. This provisional outline uses the exact
 * position and extent until sprite53 itself is decoded by rom_sprites.c.
 */
static bool type_music_cursor_visible(void) {
    const unsigned frame = (unsigned)((SDL_GetTicks() * 60u) / 1000u);
    return (frame & 3u) != 0u;
}

void render_type_music_cursor_overlay(SDL_Renderer *renderer,
                                      AppScreen screen,
                                      const TetrisSettings *settings) {
    int music_type;
    int nes_x;
    int nes_y;
    SDL_Rect outer;
    SDL_Rect inner;

    if (!renderer || !settings || screen != SCREEN_TYPE_SELECT ||
        !tetris_rom_screen_available(TETRIS_ROM_SCREEN_TYPE_MENU) ||
        !type_music_cursor_visible()) return;

    music_type = settings->music_track < 0 ? 3 : settings->music_track;
    if (music_type < 0) music_type = 0;
    if (music_type > 3) music_type = 3;

    nes_x = 0x67;
    nes_y = 0x8f + music_type * 0x10;
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
