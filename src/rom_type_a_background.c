#include "rom_type_a.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NES_VIEW_X 64
#define NES_SCALE 2
#define NES_SCREEN_W 256
#define NES_SCREEN_H 240
#define CHR_BANK_SIZE 4096u
#define CHR_TYPEA_ENDING 2u
#define PRG_ENDING_PALETTE 0x2d43u
#define PRG_TYPE_A_OVER120K_PATCH 0x28ccu
#define PRG_TYPE_A_ENDING 0x5268u

static SDL_Texture *g_type_a_backgrounds[2];

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

static bool apply_patch(const uint8_t *stream, size_t stream_size,
                        uint8_t nametable[1024]) {
    size_t position = 2u;
    uint16_t address;
    if (!stream || stream_size < 3u) return false;
    address = (uint16_t)(((uint16_t)stream[0] << 8) | stream[1]);
    while (position < stream_size) {
        const uint8_t value = stream[position++];
        if (value == 0xfdu) return true;
        if (value == 0xfeu) {
            if (position + 2u > stream_size) return false;
            address = (uint16_t)(((uint16_t)stream[position] << 8) |
                                 stream[position + 1u]);
            position += 2u;
        } else {
            if (address >= 0x2000u && address < 0x2400u)
                nametable[address - 0x2000u] = value;
            address = (uint16_t)(address + 1u);
        }
    }
    return false;
}

static SDL_Texture *build_background(SDL_Renderer *renderer, const NesRom *rom,
                                     bool over_120k) {
    uint8_t nametable[1024];
    uint8_t palette[32];
    uint32_t *pixels;
    SDL_Texture *texture;
    const size_t chr_base = (size_t)CHR_TYPEA_ENDING * CHR_BANK_SIZE;
    if (!renderer || !rom || !rom->prg || !rom->chr ||
        PRG_TYPE_A_ENDING >= rom->prg_size ||
        chr_base + CHR_BANK_SIZE > rom->chr_size) return NULL;
    memset(nametable, 0xff, sizeof(nametable));
    memset(palette, 0x0f, sizeof(palette));
    if (!apply_ppu_stream(rom->prg + PRG_ENDING_PALETTE,
                          rom->prg_size - PRG_ENDING_PALETTE,
                          nametable, palette)) return NULL;
    if (!apply_ppu_stream(rom->prg + PRG_TYPE_A_ENDING,
                          rom->prg_size - PRG_TYPE_A_ENDING,
                          nametable, palette)) return NULL;
    if (over_120k &&
        !apply_patch(rom->prg + PRG_TYPE_A_OVER120K_PATCH,
                     rom->prg_size - PRG_TYPE_A_OVER120K_PATCH,
                     nametable)) return NULL;
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

void tetris_rom_type_a_backgrounds_free(void) {
    for (int index = 0; index < 2; ++index) {
        if (g_type_a_backgrounds[index])
            SDL_DestroyTexture(g_type_a_backgrounds[index]);
        g_type_a_backgrounds[index] = NULL;
    }
}

bool tetris_rom_type_a_backgrounds_load(SDL_Renderer *renderer,
                                        const NesRom *rom) {
    SDL_Texture *loaded[2] = {NULL, NULL};
    bool any;
    if (!renderer || !rom || !rom->exact_supported_dump) {
        tetris_rom_type_a_backgrounds_free();
        return false;
    }
    loaded[0] = build_background(renderer, rom, false);
    loaded[1] = build_background(renderer, rom, true);
    any = loaded[0] != NULL || loaded[1] != NULL;
    tetris_rom_type_a_backgrounds_free();
    memcpy(g_type_a_backgrounds, loaded, sizeof(g_type_a_backgrounds));
    return any;
}

bool tetris_rom_type_a_background_available(bool over_120k) {
    return g_type_a_backgrounds[over_120k ? 1 : 0] != NULL;
}

void tetris_rom_type_a_background_render(SDL_Renderer *renderer,
                                         bool over_120k) {
    SDL_Rect destination = {NES_VIEW_X, 0, NES_SCREEN_W * NES_SCALE,
                            NES_SCREEN_H * NES_SCALE};
    SDL_Texture *texture = g_type_a_backgrounds[over_120k ? 1 : 0];
    if (!renderer || !texture) return;
    SDL_RenderCopy(renderer, texture, NULL, &destination);
}
