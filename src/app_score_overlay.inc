/*
 * ROM-native dynamic fields.
 *
 * The exact backgrounds contain placeholder digits.  Earlier versions drew a
 * second, grayscale font over those placeholders.  This module reconstructs
 * the relevant nametable/palette state and replaces complete 8x8 NES tiles,
 * preserving the original CHR bank, palette and pixel alignment.
 */
#define OVERLAY_NES_W 256
#define OVERLAY_NES_H 240
#define OVERLAY_CHR_BANK_SIZE 4096u
#define OVERLAY_ATLAS_W 128
#define OVERLAY_ATLAS_H 512

#define OVERLAY_PRG_GAME_PALETTE       0x2cf3u
#define OVERLAY_PRG_MENU_PALETTE       0x2d2bu
#define OVERLAY_PRG_LEVEL_MENU         0x3adbu
#define OVERLAY_PRG_GAME_NAMETABLE     0x3f3cu
#define OVERLAY_PRG_ENTER_HIGH_SCORE   0x439du
#define OVERLAY_PRG_HIGH_SCORES_PATCH  0x47feu
#define OVERLAY_PRG_HEIGHT_MENU_PATCH  0x495du

enum OverlayExactScreen {
    OVERLAY_EXACT_GAME = 0,
    OVERLAY_EXACT_LEVEL_A,
    OVERLAY_EXACT_LEVEL_B,
    OVERLAY_EXACT_HIGH_SCORES,
    OVERLAY_EXACT_COUNT
};

typedef struct OverlayScreenState {
    uint8_t nametable[1024];
    uint8_t palette[32];
    unsigned chr_bank;
    SDL_Texture *atlas;
    bool attempted;
    bool available;
} OverlayScreenState;

static OverlayScreenState g_overlay_screen[OVERLAY_EXACT_COUNT];
static SDL_Renderer *g_overlay_renderer;
static uint32_t g_overlay_crc;

static bool overlay_apply_ppu_stream(const uint8_t *stream, size_t stream_size,
                                     uint8_t nametable[1024],
                                     uint8_t palette[32]) {
    size_t position = 0;
    if (!stream) return false;
    while (position < stream_size) {
        uint16_t address;
        uint8_t control;
        int count;
        int increment;
        bool repeat;
        if ((stream[position] & 0x80u) != 0u) return true;
        if (position + 3u > stream_size) return false;
        address = (uint16_t)(((uint16_t)stream[position] << 8) |
                             stream[position + 1u]);
        control = stream[position + 2u];
        position += 3u;
        increment = (control & 0x80u) != 0u ? 32 : 1;
        repeat = (control & 0x40u) != 0u;
        count = control & 0x3f;
        if (count == 0) count = 64;
        if (repeat) {
            uint8_t value;
            int index;
            if (position >= stream_size) return false;
            value = stream[position++];
            for (index = 0; index < count; ++index) {
                if (address >= 0x2000u && address < 0x2400u)
                    nametable[address - 0x2000u] = value;
                else if (address >= 0x3f00u && address < 0x3f20u)
                    palette[address - 0x3f00u] = value;
                address = (uint16_t)(address + increment);
            }
        } else {
            int index;
            if (position + (size_t)count > stream_size) return false;
            for (index = 0; index < count; ++index) {
                const uint8_t value = stream[position + (size_t)index];
                if (address >= 0x2000u && address < 0x2400u)
                    nametable[address - 0x2000u] = value;
                else if (address >= 0x3f00u && address < 0x3f20u)
                    palette[address - 0x3f00u] = value;
                address = (uint16_t)(address + increment);
            }
            position += (size_t)count;
        }
    }
    return false;
}

static SDL_Texture *overlay_create_atlas(SDL_Renderer *renderer,
                                         const NesRom *rom,
                                         const uint8_t palette[32],
                                         unsigned chr_bank) {
    uint32_t *pixels;
    SDL_Texture *texture;
    const size_t chr_base = (size_t)chr_bank * OVERLAY_CHR_BANK_SIZE;
    int palette_number;
    int tile;
    if (!renderer || !rom || !rom->chr ||
        chr_base + OVERLAY_CHR_BANK_SIZE > rom->chr_size) return NULL;
    pixels = (uint32_t *)malloc((size_t)OVERLAY_ATLAS_W *
                                OVERLAY_ATLAS_H * sizeof(uint32_t));
    if (!pixels) return NULL;
    for (palette_number = 0; palette_number < 4; ++palette_number) {
        for (tile = 0; tile < 256; ++tile) {
            const size_t tile_offset = chr_base + (size_t)tile * 16u;
            const int tile_x = (tile & 15) * 8;
            const int tile_y = palette_number * 128 + (tile >> 4) * 8;
            int y;
            for (y = 0; y < 8; ++y) {
                int x;
                for (x = 0; x < 8; ++x) {
                    const int bit = 7 - x;
                    const int value =
                        ((rom->chr[tile_offset + (size_t)y] >> bit) & 1) |
                        (((rom->chr[tile_offset + 8u + (size_t)y] >> bit) & 1) << 1);
                    const int palette_index = value == 0
                        ? 0 : palette_number * 4 + value;
                    const SDL_Color color = NES_COLORS[palette[palette_index] & 0x3f];
                    pixels[(tile_y + y) * OVERLAY_ATLAS_W + tile_x + x] =
                        0xff000000u | ((uint32_t)color.r << 16) |
                        ((uint32_t)color.g << 8) | color.b;
                }
            }
        }
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC,
                                OVERLAY_ATLAS_W, OVERLAY_ATLAS_H);
    if (texture) {
        SDL_UpdateTexture(texture, NULL, pixels,
                          OVERLAY_ATLAS_W * (int)sizeof(uint32_t));
        SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    }
    free(pixels);
    return texture;
}

void tetris_score_overlay_reset(void) {
    int index;
    for (index = 0; index < OVERLAY_EXACT_COUNT; ++index) {
        if (g_overlay_screen[index].atlas)
            SDL_DestroyTexture(g_overlay_screen[index].atlas);
    }
    memset(g_overlay_screen, 0, sizeof(g_overlay_screen));
    g_overlay_renderer = NULL;
    g_overlay_crc = 0;
}

static bool overlay_screen_spec(enum OverlayExactScreen screen,
                                size_t *nametable_offset,
                                size_t *palette_offset,
                                size_t *patch_offset,
                                unsigned *chr_bank) {
    *patch_offset = (size_t)-1;
    switch (screen) {
        case OVERLAY_EXACT_GAME:
            *nametable_offset = OVERLAY_PRG_GAME_NAMETABLE;
            *palette_offset = OVERLAY_PRG_GAME_PALETTE;
            *chr_bank = 3u;
            return true;
        case OVERLAY_EXACT_LEVEL_A:
            *nametable_offset = OVERLAY_PRG_LEVEL_MENU;
            *palette_offset = OVERLAY_PRG_MENU_PALETTE;
            *patch_offset = OVERLAY_PRG_HEIGHT_MENU_PATCH;
            *chr_bank = 0u;
            return true;
        case OVERLAY_EXACT_LEVEL_B:
            *nametable_offset = OVERLAY_PRG_LEVEL_MENU;
            *palette_offset = OVERLAY_PRG_MENU_PALETTE;
            *chr_bank = 0u;
            return true;
        case OVERLAY_EXACT_HIGH_SCORES:
            *nametable_offset = OVERLAY_PRG_ENTER_HIGH_SCORE;
            *palette_offset = OVERLAY_PRG_MENU_PALETTE;
            *patch_offset = OVERLAY_PRG_HIGH_SCORES_PATCH;
            *chr_bank = 0u;
            return true;
        default:
            return false;
    }
}

static OverlayScreenState *overlay_get_screen(SDL_Renderer *renderer,
                                               const NesRom *rom,
                                               enum OverlayExactScreen screen) {
    OverlayScreenState *state;
    size_t nametable_offset;
    size_t palette_offset;
    size_t patch_offset;
    unsigned chr_bank;
    if (!renderer || !rom || !rom->exact_supported_dump || !rom->prg)
        return NULL;
    if (g_overlay_renderer != renderer || g_overlay_crc != rom->crc32) {
        tetris_score_overlay_reset();
        g_overlay_renderer = renderer;
        g_overlay_crc = rom->crc32;
    }
    if (screen < 0 || screen >= OVERLAY_EXACT_COUNT) return NULL;
    state = &g_overlay_screen[screen];
    if (state->attempted) return state->available ? state : NULL;
    state->attempted = true;
    if (!overlay_screen_spec(screen, &nametable_offset, &palette_offset,
                             &patch_offset, &chr_bank)) return NULL;
    if (nametable_offset >= rom->prg_size || palette_offset >= rom->prg_size)
        return NULL;
    memset(state->nametable, 0xff, sizeof(state->nametable));
    memset(state->palette, 0x0f, sizeof(state->palette));
    if (!overlay_apply_ppu_stream(rom->prg + palette_offset,
                                  rom->prg_size - palette_offset,
                                  state->nametable, state->palette) ||
        !overlay_apply_ppu_stream(rom->prg + nametable_offset,
                                  rom->prg_size - nametable_offset,
                                  state->nametable, state->palette)) return NULL;
    if (patch_offset != (size_t)-1) {
        if (patch_offset >= rom->prg_size ||
            !overlay_apply_ppu_stream(rom->prg + patch_offset,
                                      rom->prg_size - patch_offset,
                                      state->nametable, state->palette)) return NULL;
    }
    state->chr_bank = chr_bank;
    state->atlas = overlay_create_atlas(renderer, rom, state->palette, chr_bank);
    if (!state->atlas) return NULL;
    state->available = true;
    return state;
}

static int overlay_palette_number(const OverlayScreenState *state,
                                  int tile_x, int tile_y) {
    const uint8_t attribute = state->nametable[0x3c0 +
        (tile_y / 4) * 8 + tile_x / 4];
    const int shift = ((tile_y & 2) != 0 ? 4 : 0) +
                      ((tile_x & 2) != 0 ? 2 : 0);
    return (attribute >> shift) & 3;
}

static void overlay_draw_tile(SDL_Renderer *renderer,
                              const OverlayScreenState *state,
                              uint16_t ppu_address, uint8_t tile) {
    int offset;
    int tile_x;
    int tile_y;
    int palette_number;
    SDL_Rect source;
    SDL_Rect destination;
    if (!renderer || !state || !state->atlas ||
        ppu_address < 0x2000u || ppu_address >= 0x23c0u) return;
    offset = (int)(ppu_address - 0x2000u);
    tile_x = offset & 31;
    tile_y = offset >> 5;
    if (tile_y < 0 || tile_y >= 30) return;
    palette_number = overlay_palette_number(state, tile_x, tile_y);
    source.x = (tile & 15) * 8;
    source.y = palette_number * 128 + (tile >> 4) * 8;
    source.w = 8;
    source.h = 8;
    destination.x = 64 + tile_x * 16;
    destination.y = tile_y * 16;
    destination.w = 16;
    destination.h = 16;
    SDL_RenderCopy(renderer, state->atlas, &source, &destination);
}

static void overlay_restore_span(SDL_Renderer *renderer,
                                 const OverlayScreenState *state,
                                 uint16_t ppu_address, int count) {
    int index;
    for (index = 0; index < count; ++index) {
        const uint16_t address = (uint16_t)(ppu_address + index);
        if (address >= 0x2000u && address < 0x23c0u)
            overlay_draw_tile(renderer, state, address,
                              state->nametable[address - 0x2000u]);
    }
}

static void overlay_restore_rectangle(SDL_Renderer *renderer,
                                      const OverlayScreenState *state,
                                      int tile_x, int tile_y,
                                      int width, int height) {
    int y;
    for (y = 0; y < height; ++y) {
        int x;
        for (x = 0; x < width; ++x) {
            const int px = tile_x + x;
            const int py = tile_y + y;
            if (px >= 0 && px < 32 && py >= 0 && py < 30) {
                const uint16_t address = (uint16_t)(0x2000 + py * 32 + px);
                overlay_draw_tile(renderer, state, address,
                                  state->nametable[address - 0x2000u]);
            }
        }
    }
}

static void overlay_draw_text(SDL_Renderer *renderer,
                              const OverlayScreenState *state,
                              uint16_t ppu_address, const char *text) {
    int index = 0;
    if (!text) return;
    while (text[index] != '\0') {
        const int tile = font_tile(text[index]);
        const uint16_t address = (uint16_t)(ppu_address + index);
        if (tile >= 0)
            overlay_draw_tile(renderer, state, address, (uint8_t)tile);
        else if (address >= 0x2000u && address < 0x23c0u)
            overlay_draw_tile(renderer, state, address,
                              state->nametable[address - 0x2000u]);
        ++index;
    }
}

static void overlay_draw_game(SDL_Renderer *renderer, const NesRom *rom,
                              const TetrisGame *game,
                              const TetrisHighScores *scores) {
    static const uint16_t stat_address[7] = {
        0x2186u, 0x21c6u, 0x2206u, 0x2246u,
        0x2286u, 0x22c6u, 0x2306u
    };
    OverlayScreenState *state;
    const TetrisHighScoreEntry *top;
    char value[16];
    int index;
    if (!game || !scores ||
        !tetris_rom_screen_available(TETRIS_ROM_SCREEN_GAME)) return;
    state = overlay_get_screen(renderer, rom, OVERLAY_EXACT_GAME);
    if (!state) return;

    /* Remove every misplaced half-size value drawn by render_game_exact. */
    overlay_restore_rectangle(renderer, state, 17, 2, 7, 4);
    overlay_restore_rectangle(renderer, state, 23, 4, 9, 7);
    overlay_restore_rectangle(renderer, state, 25, 20, 7, 4);
    overlay_restore_rectangle(renderer, state, 4, 10, 9, 17);

    snprintf(value, sizeof(value), "%03d",
             game->lines < 0 ? 0 : (game->lines > 999 ? 999 : game->lines));
    overlay_draw_text(renderer, state, 0x2073u, value);

    top = tetris_high_scores_top(scores, game->mode);
    snprintf(value, sizeof(value), "%06d",
             top->score < 0 ? 0 : (top->score > 999999 ? 999999 : top->score));
    overlay_draw_text(renderer, state, 0x20b8u, value);

    snprintf(value, sizeof(value), "%06d",
             game->score < 0 ? 0 : (game->score > 999999 ? 999999 : game->score));
    overlay_draw_text(renderer, state, 0x2118u, value);

    snprintf(value, sizeof(value), "%02d",
             game->level < 0 ? 0 : (game->level > 99 ? 99 : game->level));
    overlay_draw_text(renderer, state, 0x22bau, value);

    for (index = 0; index < 7; ++index) {
        const int count = game->piece_count[index] < 0 ? 0 :
            (game->piece_count[index] > 999 ? 999 : game->piece_count[index]);
        snprintf(value, sizeof(value), "%03d", count);
        overlay_draw_text(renderer, state, stat_address[index], value);
    }
}

static void overlay_draw_record_rows(SDL_Renderer *renderer,
                                     const OverlayScreenState *state,
                                     const TetrisHighScores *scores,
                                     int mode) {
    static const uint16_t row_address[TETRIS_HIGH_SCORE_COUNT] = {
        0x2289u, 0x22c9u, 0x2309u
    };
    int rank;
    if (!state || !scores) return;
    if (mode < 0) mode = 0;
    if (mode > 1) mode = 1;
    for (rank = 0; rank < TETRIS_HIGH_SCORE_COUNT; ++rank) {
        const TetrisHighScoreEntry *entry = &scores->entries[mode][rank];
        const uint16_t base = row_address[rank];
        char value[16];
        overlay_restore_span(renderer, state, base, 16);
        if (entry->score <= 0) continue;
        overlay_draw_text(renderer, state, base, entry->name);
        snprintf(value, sizeof(value), "%06d",
                 entry->score > 999999 ? 999999 : entry->score);
        overlay_draw_text(renderer, state, (uint16_t)(base + 7u), value);
        snprintf(value, sizeof(value), "%02d",
                 entry->level < 0 ? 0 : (entry->level > 99 ? 99 : entry->level));
        overlay_draw_text(renderer, state, (uint16_t)(base + 14u), value);
    }
}

static void overlay_draw_level_records(SDL_Renderer *renderer,
                                       const NesRom *rom,
                                       const AppMenuState *menu,
                                       const TetrisHighScores *scores) {
    OverlayScreenState *state;
    enum OverlayExactScreen exact;
    if (!menu || !scores) return;
    exact = menu->mode == TETRIS_MODE_B
        ? OVERLAY_EXACT_LEVEL_B : OVERLAY_EXACT_LEVEL_A;
    state = overlay_get_screen(renderer, rom, exact);
    if (!state) return;
    overlay_draw_record_rows(renderer, state, scores,
                             menu->mode == TETRIS_MODE_B ? 1 : 0);
}

static void overlay_draw_records_screen(SDL_Renderer *renderer,
                                        const NesRom *rom,
                                        const TetrisHighScores *scores) {
    OverlayScreenState *state;
    const int mode = (int)((SDL_GetTicks() / 3000u) & 1u);
    if (!scores ||
        !tetris_rom_screen_available(TETRIS_ROM_SCREEN_HIGH_SCORES)) return;
    state = overlay_get_screen(renderer, rom, OVERLAY_EXACT_HIGH_SCORES);
    if (!state) return;
    overlay_draw_record_rows(renderer, state, scores, mode);
}

void tetris_present_with_score_overlay(
    SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
    const TetrisGame *game, const AppMenuState *menu,
    const AppResultState *result, const NesRom *rom,
    const TetrisHighScores *scores) {
    (void)font;
    (void)result;
    switch (screen) {
        case SCREEN_LEVEL_SELECT:
            overlay_draw_level_records(renderer, rom, menu, scores);
            break;
        case SCREEN_RECORDS:
            overlay_draw_records_screen(renderer, rom, scores);
            break;
        case SCREEN_GAME:
        case SCREEN_DEMO:
        case SCREEN_REPLAY:
            overlay_draw_game(renderer, rom, game, scores);
            break;
        default:
            /* Title and CONGRATULATIONS remain exactly as rendered by the ROM. */
            break;
    }
    SDL_RenderPresent(renderer);
}
