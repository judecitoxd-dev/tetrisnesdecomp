#include "rom_cathedral_sprites.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NES_VIEW_X 64
#define NES_SCALE 2
#define CHR_BANK_SIZE 4096u
#define CHR_TYPEB_ENDING 1u
#define PRG_ENDING_PALETTE 0x2d43u
#define PRG_OAM_LOOKUP 0x0c6cu
#define OAM_LOOKUP_COUNT 90u
#define MAX_OAM_ENTRIES 64

typedef struct CathedralSprite {
    SDL_Texture *texture;
    int width;
    int height;
    int min_x;
    int min_y;
} CathedralSprite;

static const unsigned CATHEDRAL_INDICES[] = {
    0x2cu,0x2du,0x2eu,0x2fu,0x54u,0x55u,
    0x32u,0x33u,0x34u,0x35u,0x36u,0x37u,
    0x4bu,0x4cu,0x38u,0x39u,0x3au,0x3bu
};
#define CATHEDRAL_COUNT \
    (sizeof(CATHEDRAL_INDICES) / sizeof(CATHEDRAL_INDICES[0]))

static CathedralSprite g_cathedral[CATHEDRAL_COUNT];
static TetrisCathedralTables g_tables;

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

static int slot_for(unsigned sprite_index) {
    for (size_t slot = 0; slot < CATHEDRAL_COUNT; ++slot) {
        if (CATHEDRAL_INDICES[slot] == sprite_index) return (int)slot;
    }
    return -1;
}

static bool load_ending_palette(const NesRom *rom, uint8_t palette[32]) {
    size_t position = PRG_ENDING_PALETTE;
    memset(palette, 0x0f, 32u);
    if (!rom || !rom->prg || position >= rom->prg_size) return false;
    while (position < rom->prg_size) {
        uint16_t address;
        uint8_t control;
        int count;
        int increment;
        bool repeat;
        if ((rom->prg[position] & 0x80u) != 0u) return true;
        if (position + 3u > rom->prg_size) return false;
        address = (uint16_t)(((uint16_t)rom->prg[position] << 8) |
                             rom->prg[position + 1u]);
        control = rom->prg[position + 2u];
        position += 3u;
        increment = (control & 0x80u) ? 32 : 1;
        repeat = (control & 0x40u) != 0u;
        count = control & 0x3f;
        if (count == 0) count = 64;
        if (repeat) {
            uint8_t value;
            if (position >= rom->prg_size) return false;
            value = rom->prg[position++];
            for (int index = 0; index < count; ++index) {
                if (address >= 0x3f00u && address < 0x3f20u)
                    palette[address - 0x3f00u] = value;
                address = (uint16_t)(address + increment);
            }
        } else {
            if (position + (size_t)count > rom->prg_size) return false;
            for (int index = 0; index < count; ++index) {
                if (address >= 0x3f00u && address < 0x3f20u)
                    palette[address - 0x3f00u] =
                        rom->prg[position + (size_t)index];
                address = (uint16_t)(address + increment);
            }
            position += (size_t)count;
        }
    }
    return false;
}

static bool lookup_data_offset(const NesRom *rom, unsigned sprite_index,
                               size_t *data_offset) {
    size_t entry;
    uint16_t address;
    if (!rom || !rom->prg || !data_offset ||
        sprite_index >= OAM_LOOKUP_COUNT) return false;
    entry = PRG_OAM_LOOKUP + (size_t)sprite_index * 2u;
    if (entry + 2u > rom->prg_size) return false;
    address = (uint16_t)(rom->prg[entry] |
                         ((uint16_t)rom->prg[entry + 1u] << 8));
    if (address < 0x8000u) return false;
    *data_offset = (size_t)(address - 0x8000u);
    return *data_offset < rom->prg_size;
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

static CathedralSprite build_sprite(SDL_Renderer *renderer, const NesRom *rom,
                                     size_t data_offset,
                                     const uint8_t palette[32]) {
    CathedralSprite result;
    const uint8_t *data;
    uint32_t *pixels;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    int entries;
    const size_t chr_base = (size_t)CHR_TYPEB_ENDING * CHR_BANK_SIZE;
    memset(&result, 0, sizeof(result));
    if (!renderer || !rom || !rom->prg || !rom->chr ||
        data_offset >= rom->prg_size ||
        chr_base + CHR_BANK_SIZE > rom->chr_size) return result;
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
        const size_t tile_offset = chr_base + (size_t)tile * 16u;
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

void tetris_rom_cathedral_sprites_free(void) {
    memset(&g_tables, 0, sizeof(g_tables));
    for (size_t slot = 0; slot < CATHEDRAL_COUNT; ++slot) {
        if (g_cathedral[slot].texture)
            SDL_DestroyTexture(g_cathedral[slot].texture);
        memset(&g_cathedral[slot], 0, sizeof(g_cathedral[slot]));
    }
}

bool tetris_rom_cathedral_sprites_load(SDL_Renderer *renderer,
                                       const NesRom *rom) {
    CathedralSprite loaded[CATHEDRAL_COUNT];
    TetrisCathedralTables tables;
    uint8_t palette[32];
    bool any = false;
    memset(loaded, 0, sizeof(loaded));
    if (!renderer || !rom || !rom->exact_supported_dump ||
        !tetris_cathedral_tables_load(&tables, rom->prg, rom->prg_size) ||
        !load_ending_palette(rom, palette)) {
        tetris_rom_cathedral_sprites_free();
        return false;
    }
    for (size_t slot = 0; slot < CATHEDRAL_COUNT; ++slot) {
        size_t data_offset;
        if (lookup_data_offset(rom, CATHEDRAL_INDICES[slot], &data_offset)) {
            loaded[slot] = build_sprite(renderer, rom, data_offset, palette);
            if (loaded[slot].texture) any = true;
        }
    }
    tetris_rom_cathedral_sprites_free();
    memcpy(g_cathedral, loaded, sizeof(g_cathedral));
    if (any) g_tables = tables;
    return any;
}

bool tetris_rom_cathedral_sprite_available(unsigned sprite_index) {
    const int slot = slot_for(sprite_index);
    return slot >= 0 && g_cathedral[slot].texture != NULL;
}

void tetris_rom_cathedral_sprite_render(SDL_Renderer *renderer,
                                        unsigned sprite_index,
                                        int nes_x, int nes_y) {
    SDL_Rect destination;
    const CathedralSprite *sprite;
    const int slot = slot_for(sprite_index);
    if (!renderer || slot < 0 || !g_cathedral[slot].texture) return;
    sprite = &g_cathedral[slot];
    destination.x = NES_VIEW_X + (nes_x + sprite->min_x) * NES_SCALE;
    destination.y = (nes_y + sprite->min_y) * NES_SCALE;
    destination.w = sprite->width * NES_SCALE;
    destination.h = sprite->height * NES_SCALE;
    SDL_RenderCopy(renderer, sprite->texture, NULL, &destination);
}

bool tetris_rom_cathedral_snapshot(int level, int start_height,
                                    unsigned frame,
                                    TetrisCathedralSnapshot *snapshot) {
    if (!snapshot || !g_tables.valid) return false;
    tetris_cathedral_snapshot(&g_tables, level, start_height, frame, snapshot);
    return true;
}
