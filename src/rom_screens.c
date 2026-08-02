#include "rom_screens.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NES_SCREEN_W 256
#define NES_SCREEN_H 240
#define CHR_BANK_SIZE 4096u

/* PRG-relative offsets in the verified Tetris (USA) dump, CRC32 D16EA396. */
#define PRG_GAME_PALETTE       0x2cf3u
#define PRG_MENU_PALETTE       0x2d2bu
#define PRG_ENDING_PALETTE     0x2d43u
#define PRG_TYPE_MENU          0x367au
#define PRG_LEVEL_MENU         0x3adbu
#define PRG_GAME_NAMETABLE     0x3f3cu
#define PRG_ENTER_HIGH_SCORE   0x439du
#define PRG_HIGH_SCORES_PATCH  0x47feu
#define PRG_HEIGHT_MENU_PATCH  0x495du
#define PRG_B_ENDING_CASTLE    0x49a6u
#define PRG_B_ENDING_NORMAL    0x4e07u

/* patchToPpu tables applied cumulatively for concert heights 0 through 4. */
#define PRG_CONCERT_PATCH_H0   0x2834u
#define PRG_CONCERT_PATCH_H1   0x284au
#define PRG_CONCERT_PATCH_H2   0x2862u
#define PRG_CONCERT_PATCH_H3   0x287au
#define PRG_CONCERT_PATCH_H4   0x2896u

#define CHR_TITLE_MENU   0u
#define CHR_TYPEB_ENDING 1u
#define CHR_TYPEA_ENDING 2u
#define CHR_GAME         3u

static SDL_Texture *g_screens[TETRIS_ROM_SCREEN_COUNT];

/* Common RGB approximation of the 64-color 2C02 palette. */
static const SDL_Color NES_COLORS[64] = {
    {84,84,84,255},{0,30,116,255},{8,16,144,255},{48,0,136,255},
    {68,0,100,255},{92,0,48,255},{84,4,0,255},{60,24,0,255},
    {32,42,0,255},{8,58,0,255},{0,64,0,255},{0,60,0,255},
    {0,50,60,255},{0,0,0,255},{0,0,0,255},{0,0,0,255},
    {152,150,152,255},{8,76,196,255},{48,50,236,255},{92,30,228,255},
    {136,20,176,255},{160,20,100,255},{152,34,32,255},{120,60,0,255},
    {84,90,0,255},{40,114,0,255},{8,124,0,255},{0,118,40,255},
    {0,102,120,255},{0,0,0,255},{0,0,0,255},{0,0,0,255},
    {236,238,236,255},{76,154,236,255},{120,124,236,255},{176,98,236,255},
    {228,84,236,255},{236,88,180,255},{236,106,100,255},{212,136,32,255},
    {160,170,0,255},{116,196,0,255},{76,208,32,255},{56,204,108,255},
    {56,180,204,255},{60,60,60,255},{0,0,0,255},{0,0,0,255},
    {236,238,236,255},{168,204,236,255},{188,188,236,255},{212,178,236,255},
    {236,174,236,255},{236,174,212,255},{236,180,176,255},{228,196,144,255},
    {204,210,120,255},{180,222,120,255},{168,226,144,255},{152,226,180,255},
    {160,214,228,255},{160,162,160,255},{0,0,0,255},{0,0,0,255}
};

static bool apply_ppu_stream(const uint8_t *stream, size_t stream_size,
                             uint8_t nametable[1024], uint8_t palette[32]) {
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
        increment = (control & 0x80u) ? 32 : 1;
        repeat = (control & 0x40u) != 0u;
        count = control & 0x3f;
        if (count == 0) count = 64;
        if (repeat) {
            uint8_t value;
            if (position >= stream_size) return false;
            value = stream[position++];
            for (int index = 0; index < count; ++index) {
                if (address >= 0x2000u && address < 0x2400u)
                    nametable[address - 0x2000u] = value;
                else if (address >= 0x3f00u && address < 0x3f20u)
                    palette[address - 0x3f00u] = value;
                address = (uint16_t)(address + increment);
            }
        } else {
            if (position + (size_t)count > stream_size) return false;
            for (int index = 0; index < count; ++index) {
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

/* Direct patch format used by patchToPpu: address, values, FE/new address, FD/end. */
static bool apply_patch_stream(const uint8_t *stream, size_t stream_size,
                               uint8_t nametable[1024]) {
    size_t position = 0;
    uint16_t address;
    if (!stream || stream_size < 3u) return false;
    address = (uint16_t)(((uint16_t)stream[position] << 8) |
                         stream[position + 1u]);
    position += 2u;
    while (position < stream_size) {
        const uint8_t value = stream[position++];
        if (value == 0xfdu) return true;
        if (value == 0xfeu) {
            if (position + 2u > stream_size) return false;
            address = (uint16_t)(((uint16_t)stream[position] << 8) |
                                 stream[position + 1u]);
            position += 2u;
            continue;
        }
        if (address >= 0x2000u && address < 0x2400u)
            nametable[address - 0x2000u] = value;
        address = (uint16_t)(address + 1u);
    }
    return false;
}

static SDL_Texture *create_screen_texture(SDL_Renderer *renderer,
                                          const NesRom *rom,
                                          const uint8_t nametable[1024],
                                          const uint8_t palette[32],
                                          unsigned chr_bank) {
    uint32_t *pixels;
    SDL_Texture *texture;
    const size_t chr_base = (size_t)chr_bank * CHR_BANK_SIZE;
    if (!renderer || !rom || !rom->chr ||
        chr_base + CHR_BANK_SIZE > rom->chr_size) return NULL;
    pixels = (uint32_t *)malloc((size_t)NES_SCREEN_W * NES_SCREEN_H *
                                sizeof(uint32_t));
    if (!pixels) return NULL;
    for (int py = 0; py < NES_SCREEN_H; ++py) {
        const int tile_y = py / 8;
        const int fine_y = py & 7;
        for (int px = 0; px < NES_SCREEN_W; ++px) {
            const int tile_x = px / 8;
            const int fine_x = px & 7;
            const uint8_t tile = nametable[tile_y * 32 + tile_x];
            const uint8_t attribute = nametable[0x3c0 +
                (tile_y / 4) * 8 + tile_x / 4];
            const int shift = ((tile_y & 2) ? 4 : 0) +
                              ((tile_x & 2) ? 2 : 0);
            const int palette_number = (attribute >> shift) & 3;
            const size_t tile_offset = chr_base + (size_t)tile * 16u;
            const int bit = 7 - fine_x;
            const int value =
                ((rom->chr[tile_offset + (size_t)fine_y] >> bit) & 1) |
                (((rom->chr[tile_offset + 8u + (size_t)fine_y] >> bit) & 1) << 1);
            const int palette_index = value == 0 ? 0 : palette_number * 4 + value;
            const SDL_Color color = NES_COLORS[palette[palette_index] & 0x3f];
            pixels[py * NES_SCREEN_W + px] = 0xff000000u |
                ((uint32_t)color.r << 16) |
                ((uint32_t)color.g << 8) | color.b;
        }
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC,
                                NES_SCREEN_W, NES_SCREEN_H);
    if (texture) {
        SDL_UpdateTexture(texture, NULL, pixels,
                          NES_SCREEN_W * (int)sizeof(uint32_t));
        SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    }
    free(pixels);
    return texture;
}

static bool load_screen_state(const NesRom *rom, size_t nametable_offset,
                              size_t palette_offset, size_t patch_offset,
                              uint8_t nametable[1024], uint8_t palette[32]) {
    if (!rom || !rom->exact_supported_dump || !rom->prg ||
        nametable_offset >= rom->prg_size || palette_offset >= rom->prg_size)
        return false;
    memset(nametable, 0xff, 1024u);
    memset(palette, 0x0f, 32u);
    if (!apply_ppu_stream(rom->prg + palette_offset,
                          rom->prg_size - palette_offset,
                          nametable, palette)) return false;
    if (!apply_ppu_stream(rom->prg + nametable_offset,
                          rom->prg_size - nametable_offset,
                          nametable, palette)) return false;
    if (patch_offset != SIZE_MAX) {
        if (patch_offset >= rom->prg_size ||
            !apply_ppu_stream(rom->prg + patch_offset,
                              rom->prg_size - patch_offset,
                              nametable, palette)) return false;
    }
    return true;
}

static SDL_Texture *build_screen(SDL_Renderer *renderer, const NesRom *rom,
                                 size_t nametable_offset,
                                 size_t palette_offset, unsigned chr_bank,
                                 size_t patch_offset) {
    uint8_t nametable[1024];
    uint8_t palette[32];
    if (!load_screen_state(rom, nametable_offset, palette_offset, patch_offset,
                           nametable, palette)) return NULL;
    return create_screen_texture(renderer, rom, nametable, palette, chr_bank);
}

static SDL_Texture *build_castle_screen(SDL_Renderer *renderer,
                                        const NesRom *rom, int height) {
    static const size_t patches[5] = {
        PRG_CONCERT_PATCH_H0, PRG_CONCERT_PATCH_H1,
        PRG_CONCERT_PATCH_H2, PRG_CONCERT_PATCH_H3,
        PRG_CONCERT_PATCH_H4
    };
    uint8_t nametable[1024];
    uint8_t palette[32];
    if (height < 0) height = 0;
    if (height > 5) height = 5;
    if (!load_screen_state(rom, PRG_B_ENDING_CASTLE, PRG_ENDING_PALETTE,
                           SIZE_MAX, nametable, palette)) return NULL;
    for (int patch = height; patch < 5; ++patch) {
        const size_t offset = patches[patch];
        if (offset >= rom->prg_size ||
            !apply_patch_stream(rom->prg + offset,
                                rom->prg_size - offset, nametable)) return NULL;
    }
    return create_screen_texture(renderer, rom, nametable, palette,
                                 CHR_TYPEB_ENDING);
}

void tetris_rom_screens_free(void) {
    for (int index = 0; index < TETRIS_ROM_SCREEN_COUNT; ++index) {
        if (g_screens[index]) {
            SDL_DestroyTexture(g_screens[index]);
            g_screens[index] = NULL;
        }
    }
}

bool tetris_rom_screens_load(SDL_Renderer *renderer, const NesRom *rom) {
    SDL_Texture *screens[TETRIS_ROM_SCREEN_COUNT] = {0};
    bool any = false;
    if (!renderer || !rom || !rom->exact_supported_dump) {
        tetris_rom_screens_free();
        return false;
    }
    screens[TETRIS_ROM_SCREEN_TYPE_MENU] =
        build_screen(renderer, rom, PRG_TYPE_MENU, PRG_MENU_PALETTE,
                     CHR_TITLE_MENU, SIZE_MAX);
    screens[TETRIS_ROM_SCREEN_LEVEL_B] =
        build_screen(renderer, rom, PRG_LEVEL_MENU, PRG_MENU_PALETTE,
                     CHR_TITLE_MENU, SIZE_MAX);
    screens[TETRIS_ROM_SCREEN_LEVEL_A] =
        build_screen(renderer, rom, PRG_LEVEL_MENU, PRG_MENU_PALETTE,
                     CHR_TITLE_MENU, PRG_HEIGHT_MENU_PATCH);
    screens[TETRIS_ROM_SCREEN_GAME] =
        build_screen(renderer, rom, PRG_GAME_NAMETABLE, PRG_GAME_PALETTE,
                     CHR_GAME, SIZE_MAX);
    screens[TETRIS_ROM_SCREEN_ENTER_HIGH_SCORE] =
        build_screen(renderer, rom, PRG_ENTER_HIGH_SCORE, PRG_MENU_PALETTE,
                     CHR_TITLE_MENU, SIZE_MAX);
    screens[TETRIS_ROM_SCREEN_HIGH_SCORES] =
        build_screen(renderer, rom, PRG_ENTER_HIGH_SCORE, PRG_MENU_PALETTE,
                     CHR_TITLE_MENU, PRG_HIGH_SCORES_PATCH);
    screens[TETRIS_ROM_SCREEN_B_ENDING_NORMAL] =
        build_screen(renderer, rom, PRG_B_ENDING_NORMAL, PRG_ENDING_PALETTE,
                     CHR_TYPEA_ENDING, SIZE_MAX);
    for (int height = 0; height <= 5; ++height) {
        screens[TETRIS_ROM_SCREEN_B_ENDING_CASTLE_H0 + height] =
            build_castle_screen(renderer, rom, height);
    }
    for (int index = 0; index < TETRIS_ROM_SCREEN_COUNT; ++index)
        if (screens[index]) any = true;
    tetris_rom_screens_free();
    memcpy(g_screens, screens, sizeof(g_screens));
    return any;
}

bool tetris_rom_screen_available(TetrisRomScreen screen) {
    return screen >= 0 && screen < TETRIS_ROM_SCREEN_COUNT &&
           g_screens[screen] != NULL;
}

void tetris_rom_screen_render(SDL_Renderer *renderer, TetrisRomScreen screen) {
    SDL_Rect destination = {64, 0, 512, 480};
    if (!renderer || !tetris_rom_screen_available(screen)) return;
    SDL_RenderCopy(renderer, g_screens[screen], NULL, &destination);
}
