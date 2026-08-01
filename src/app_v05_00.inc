#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Common RGB approximation of the 64-color NES palette. */
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

#define CHR_BANK_SIZE 4096u
#define CHR_BANK_GAME 3
#define CHR_GAME_TILE_1 0x7b
#define CHR_GAME_TILE_2 0x7c
#define CHR_GAME_TILE_3 0x7d

static SDL_Texture *g_block_textures[10][3];
static SDL_Texture *g_title_texture;

static void destroy_block_textures(void) {
    for (int level = 0; level < 10; ++level) {
        for (int tile = 0; tile < 3; ++tile) {
            if (g_block_textures[level][tile]) {
                SDL_DestroyTexture(g_block_textures[level][tile]);
                g_block_textures[level][tile] = NULL;
            }
        }
    }
}

static const uint8_t *find_bytes(const uint8_t *data, size_t data_size,
                                 const uint8_t *needle, size_t needle_size) {
    if (!data || !needle || needle_size == 0 || data_size < needle_size) return NULL;
    for (size_t offset = 0; offset + needle_size <= data_size; ++offset) {
        if (memcmp(data + offset, needle, needle_size) == 0) return data + offset;
    }
    return NULL;
}

static bool decode_ppu_stream(const uint8_t *stream, size_t stream_size,
                              uint8_t nametable[1024]) {
    size_t position = 0;
    memset(nametable, 0xff, 1024u);
    while (position < stream_size) {
        uint16_t address;
        uint8_t control;
        int count;
        int increment;
        bool repeat;
        if (stream[position] & 0x80u) return true;
        if (position + 3u > stream_size) return false;
        address = (uint16_t)((uint16_t)stream[position] << 8) |
                  stream[position + 1u];
        control = stream[position + 2u];
        position += 3u;
        increment = (control & 0x80u) ? 32 : 1;
        repeat = (control & 0x40u) != 0;
        count = control & 0x3f;
        if (count == 0) count = 64;
        if (repeat) {
            uint8_t value;
            if (position >= stream_size) return false;
            value = stream[position++];
            for (int index = 0; index < count; ++index) {
                if (address >= 0x2000u && address < 0x2400u)
                    nametable[address - 0x2000u] = value;
                address = (uint16_t)(address + increment);
            }
        } else {
            if (position + (size_t)count > stream_size) return false;
            for (int index = 0; index < count; ++index) {
                if (address >= 0x2000u && address < 0x2400u)
                    nametable[address - 0x2000u] = stream[position + (size_t)index];
                address = (uint16_t)(address + increment);
            }
            position += (size_t)count;
        }
    }
    return false;
}

static SDL_Texture *create_original_title_texture(SDL_Renderer *renderer,
                                                   const NesRom *rom) {
    static const uint8_t title_signature[] = {
        0x20,0x00,0x20,0x83,0x90,0x83,0x90,0x81,
        0x83,0x62,0x72,0x80,0x81,0x91,0x82,0x90
    };
    static const uint8_t palette_signature[] = {
        0x3f,0x00,0x14,0x0f,0x30,0x38,0x00,0x0f,
        0x30,0x16,0x00,0x0f,0x30,0x21,0x00,0x0f,
        0x16,0x2a,0x28,0x0f,0x30,0x29,0x27,0xff
    };
    const uint8_t *stream;
    const uint8_t *palette_stream;
    uint8_t nametable[1024];
    uint8_t palette[32];
    uint32_t *pixels;
    SDL_Texture *texture;
    if (!rom || !rom->exact_supported_dump || rom->chr_size < CHR_BANK_SIZE)
        return NULL;
    stream = find_bytes(rom->prg, rom->prg_size,
                        title_signature, sizeof(title_signature));
    palette_stream = find_bytes(rom->prg, rom->prg_size,
                                palette_signature, sizeof(palette_signature));
    if (!stream || !palette_stream) return NULL;
    if (!decode_ppu_stream(stream, (size_t)(rom->prg + rom->prg_size - stream),
                           nametable)) return NULL;
    memset(palette, 0x0f, sizeof(palette));
    memcpy(palette, palette_stream + 3, 20u);
    pixels = (uint32_t *)malloc(256u * 240u * sizeof(uint32_t));
    if (!pixels) return NULL;
    for (int py = 0; py < 240; ++py) {
        const int tile_y = py / 8;
        const int fine_y = py & 7;
        for (int px = 0; px < 256; ++px) {
            const int tile_x = px / 8;
            const int fine_x = px & 7;
            const uint8_t tile = nametable[tile_y * 32 + tile_x];
            const uint8_t attribute = nametable[0x3c0 +
                (tile_y / 4) * 8 + tile_x / 4];
            const int shift = ((tile_y & 2) ? 4 : 0) +
                              ((tile_x & 2) ? 2 : 0);
            const int palette_number = (attribute >> shift) & 3;
            const size_t chr_offset = (size_t)tile * 16u;
            const int bit = 7 - fine_x;
            const int value = ((rom->chr[chr_offset + (size_t)fine_y] >> bit) & 1) |
                (((rom->chr[chr_offset + 8u + (size_t)fine_y] >> bit) & 1) << 1);
            const int palette_index = value == 0 ? 0 : palette_number * 4 + value;
            const SDL_Color color = NES_COLORS[palette[palette_index] & 0x3f];
            pixels[py * 256 + px] = 0xff000000u |
                ((uint32_t)color.r << 16) |
                ((uint32_t)color.g << 8) | color.b;
        }
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
