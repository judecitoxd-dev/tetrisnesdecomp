#include "rom_sprites.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NES_VIEW_X 64
#define NES_SCALE 2
#define CHR_BANK_SIZE 4096u
#define CHR_TITLE_MENU 0u

#define PRG_LEVEL_CURSOR      0x0d20u
#define PRG_TYPE_CURSOR       0x0d31u
#define PRG_HIGH_SCORE_CURSOR 0x0de0u
#define PRG_MENU_PALETTE      0x2d2bu
#define PRG_TITLE_PALETTE     0x2d43u

#define MAX_OAM_ENTRIES 32

typedef struct RomSpriteTexture {
    SDL_Texture *texture;
    int width;
    int height;
    int min_x;
    int min_y;
} RomSpriteTexture;

static RomSpriteTexture g_sprites[TETRIS_ROM_SPRITE_COUNT];

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

static bool apply_palette_stream(const uint8_t *stream, size_t stream_size,
                                 uint8_t palette[32]) {
    size_t position = 0;
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
                if (address >= 0x3f00u && address < 0x3f20u)
                    palette[address - 0x3f00u] = value;
                address = (uint16_t)(address + increment);
            }
        } else {
            if (position + (size_t)count > stream_size) return false;
            for (int index = 0; index < count; ++index) {
                if (address >= 0x3f00u && address < 0x3f20u)
                    palette[address - 0x3f00u] = stream[position + (size_t)index];
                address = (uint16_t)(address + increment);
            }
            position += (size_t)count;
        }
    }
    return false;
}

static size_t find_sprite_data(const uint8_t *data, size_t data_size,
                               const uint8_t *signature,
                               size_t signature_size) {
    size_t offset;
    if (!data || !signature || signature_size == 0 ||
        data_size < signature_size) return SIZE_MAX;
    for (offset = 0; offset + signature_size <= data_size; ++offset) {
        if (memcmp(data + offset, signature, signature_size) == 0)
            return offset;
    }
    return SIZE_MAX;
}

static bool sprite_bounds(const uint8_t *data, size_t available,
                          int *min_x, int *min_y, int *max_x, int *max_y,
                          int *entry_count) {
    size_t position = 0;
    int count = 0;
    *min_x = 127;
    *min_y = 127;
    *max_x = -128;
    *max_y = -128;
    while (position < available && count < MAX_OAM_ENTRIES) {
        int x;
        int y;
        if (data[position] == 0xffu) break;
        if (position + 4u > available) return false;
        y = (int)(int8_t)data[position];
        x = (int)(int8_t)data[position + 3u];
        if (x < *min_x) *min_x = x;
        if (y < *min_y) *min_y = y;
        if (x + 8 > *max_x) *max_x = x + 8;
        if (y + 8 > *max_y) *max_y = y + 8;
        position += 4u;
        ++count;
    }
    *entry_count = count;
    return count > 0 && position < available && data[position] == 0xffu;
}

static RomSpriteTexture build_sprite(SDL_Renderer *renderer, const NesRom *rom,
                                     size_t data_offset,
                                     const uint8_t palette[32]) {
    RomSpriteTexture result;
    const uint8_t *data;
    uint32_t *pixels;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    int entries;
    memset(&result, 0, sizeof(result));
    if (!renderer || !rom || data_offset >= rom->prg_size ||
        rom->chr_size < CHR_BANK_SIZE) return result;
    data = rom->prg + data_offset;
    if (!sprite_bounds(data, rom->prg_size - data_offset,
                       &min_x, &min_y, &max_x, &max_y, &entries)) return result;
    result.width = max_x - min_x;
    result.height = max_y - min_y;
    result.min_x = min_x;
    result.min_y = min_y;
    pixels = (uint32_t *)calloc((size_t)result.width * (size_t)result.height,
                                sizeof(uint32_t));
    if (!pixels) return result;

    for (int entry = 0; entry < entries; ++entry) {
        const size_t position = (size_t)entry * 4u;
        const int y_offset = (int)(int8_t)data[position] - min_y;
        const uint8_t tile = data[position + 1u];
        const uint8_t attributes = data[position + 2u];
        const int x_offset = (int)(int8_t)data[position + 3u] - min_x;
        const bool flip_x = (attributes & 0x40u) != 0u;
        const bool flip_y = (attributes & 0x80u) != 0u;
        const int sprite_palette = attributes & 3u;
        const size_t tile_offset = (size_t)CHR_TITLE_MENU * CHR_BANK_SIZE +
                                   (size_t)tile * 16u;
        for (int py = 0; py < 8; ++py) {
            const int source_y = flip_y ? 7 - py : py;
            for (int px = 0; px < 8; ++px) {
                const int source_x = flip_x ? 7 - px : px;
                const int bit = 7 - source_x;
                const int value =
                    ((rom->chr[tile_offset + (size_t)source_y] >> bit) & 1) |
                    (((rom->chr[tile_offset + 8u + (size_t)source_y] >> bit) & 1) << 1);
                if (value != 0) {
                    const int palette_index = 0x10 + sprite_palette * 4 + value;
                    const SDL_Color color = NES_COLORS[palette[palette_index] & 0x3f];
                    pixels[(y_offset + py) * result.width + x_offset + px] =
                        0xff000000u | ((uint32_t)color.r << 16) |
                        ((uint32_t)color.g << 8) | color.b;
                }
            }
        }
    }

    result.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STATIC,
                                       result.width, result.height);
    if (result.texture) {
        SDL_UpdateTexture(result.texture, NULL, pixels,
                          result.width * (int)sizeof(uint32_t));
        SDL_SetTextureBlendMode(result.texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(result.texture, SDL_ScaleModeNearest);
    }
    free(pixels);
    return result;
}

void tetris_rom_sprites_free(void) {
    for (int index = 0; index < TETRIS_ROM_SPRITE_COUNT; ++index) {
        if (g_sprites[index].texture) SDL_DestroyTexture(g_sprites[index].texture);
        memset(&g_sprites[index], 0, sizeof(g_sprites[index]));
    }
}

bool tetris_rom_sprites_load(SDL_Renderer *renderer, const NesRom *rom) {
    static const uint8_t music_cursor_signature[] = {
        0x00,0x27,0x00,0x00,0x00,0x27,0x40,0x4a,0xff
    };
    uint8_t palette[32];
    RomSpriteTexture sprites[TETRIS_ROM_SPRITE_COUNT];
    size_t music_cursor_offset;
    bool any = false;
    memset(sprites, 0, sizeof(sprites));
    if (!renderer || !rom || !rom->exact_supported_dump || !rom->prg ||
        !rom->chr || rom->prg_size <= PRG_TITLE_PALETTE ||
        rom->chr_size < CHR_BANK_SIZE) {
        tetris_rom_sprites_free();
        return false;
    }
    memset(palette, 0x0f, sizeof(palette));
    if (!apply_palette_stream(rom->prg + PRG_TITLE_PALETTE,
                              rom->prg_size - PRG_TITLE_PALETTE, palette) ||
        !apply_palette_stream(rom->prg + PRG_MENU_PALETTE,
                              rom->prg_size - PRG_MENU_PALETTE, palette)) {
        tetris_rom_sprites_free();
        return false;
    }
    sprites[TETRIS_ROM_SPRITE_LEVEL_CURSOR] =
        build_sprite(renderer, rom, PRG_LEVEL_CURSOR, palette);
    sprites[TETRIS_ROM_SPRITE_TYPE_CURSOR] =
        build_sprite(renderer, rom, PRG_TYPE_CURSOR, palette);
    music_cursor_offset = find_sprite_data(rom->prg, rom->prg_size,
                                           music_cursor_signature,
                                           sizeof(music_cursor_signature));
    if (music_cursor_offset != SIZE_MAX) {
        sprites[TETRIS_ROM_SPRITE_MUSIC_CURSOR] =
            build_sprite(renderer, rom, music_cursor_offset, palette);
    }
    sprites[TETRIS_ROM_SPRITE_HIGH_SCORE_CURSOR] =
        build_sprite(renderer, rom, PRG_HIGH_SCORE_CURSOR, palette);
    for (int index = 0; index < TETRIS_ROM_SPRITE_COUNT; ++index)
        if (sprites[index].texture) any = true;
    tetris_rom_sprites_free();
    memcpy(g_sprites, sprites, sizeof(g_sprites));
    return any;
}

bool tetris_rom_sprite_available(TetrisRomSprite sprite) {
    return sprite >= 0 && sprite < TETRIS_ROM_SPRITE_COUNT &&
           g_sprites[sprite].texture != NULL;
}

void tetris_rom_sprite_render(SDL_Renderer *renderer, TetrisRomSprite sprite,
                              int nes_x, int nes_y) {
    SDL_Rect destination;
    const RomSpriteTexture *item;
    if (!renderer || !tetris_rom_sprite_available(sprite)) return;
    item = &g_sprites[sprite];
    destination.x = NES_VIEW_X + (nes_x + item->min_x) * NES_SCALE;
    destination.y = (nes_y + item->min_y) * NES_SCALE;
    destination.w = item->width * NES_SCALE;
    destination.h = item->height * NES_SCALE;
    SDL_RenderCopy(renderer, item->texture, NULL, &destination);
}
