                                SDL_TEXTUREACCESS_STATIC, 256, 240);
    if (texture) {
        SDL_UpdateTexture(texture, NULL, pixels, 256 * (int)sizeof(uint32_t));
        SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    }
    free(pixels);
    return texture;
}

static SDL_Texture *create_chr_texture(SDL_Renderer *renderer, const NesRom *rom) {
    int tile_count = (int)(rom->chr_size / 16u);
    const int width = 16 * 8;
    int height;
    if (tile_count > 1024) tile_count = 1024;
    height = ((tile_count + 15) / 16) * 8;
    uint32_t *pixels = (uint32_t *)calloc((size_t)width * height, sizeof(uint32_t));
    if (!pixels) return NULL;
    for (int tile = 0; tile < tile_count; ++tile) {
        const uint8_t *src = rom->chr + tile * 16;
        const int tx = (tile % 16) * 8;
        const int ty = (tile / 16) * 8;
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                const int bit = 7 - x;
                const int value = ((src[y] >> bit) & 1) |
                                  (((src[y + 8] >> bit) & 1) << 1);
                uint8_t shade = 0;
                if (value == 1) shade = 145;
                if (value == 2) shade = 205;
                if (value == 3) shade = 255;
                pixels[(ty + y) * width + tx + x] = value
                    ? (0xff000000u | ((uint32_t)shade << 16) |
                       ((uint32_t)shade << 8) | shade) : 0;
            }
        }
    }
    {
        SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_STATIC,
                                                 width, height);
        if (texture) {
            SDL_UpdateTexture(texture, NULL, pixels,
                              width * (int)sizeof(uint32_t));
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
        }
        free(pixels);
        return texture;
    }
}

static SDL_Texture *create_colored_chr_tile(SDL_Renderer *renderer,
                                                const NesRom *rom,
                                                int bank, int tile,
                                                const uint8_t palette[4]) {
    uint32_t pixels[64];
    const size_t offset = (size_t)bank * CHR_BANK_SIZE +
                          (size_t)tile * 16u;
    if (!rom || !rom->chr || offset + 16u > rom->chr_size) return NULL;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const int bit = 7 - x;
            const int value = ((rom->chr[offset + (size_t)y] >> bit) & 1) |
                (((rom->chr[offset + 8u + (size_t)y] >> bit) & 1) << 1);
            const SDL_Color color = NES_COLORS[palette[value] & 0x3f];
            pixels[y * 8 + x] = 0xff000000u |
                ((uint32_t)color.r << 16) |
                ((uint32_t)color.g << 8) | color.b;
        }
    }
    {
        SDL_Texture *texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 8, 8);
        if (texture) {
            SDL_UpdateTexture(texture, NULL, pixels,
                              8 * (int)sizeof(uint32_t));
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
        }
        return texture;
    }
}

static bool create_block_textures(SDL_Renderer *renderer, const NesRom *rom,
                                  SDL_Texture *textures[10][3]) {
    const int tile_ids[3] = {
        CHR_GAME_TILE_1, CHR_GAME_TILE_2, CHR_GAME_TILE_3
    };
    memset(textures, 0, sizeof(SDL_Texture *) * 10u * 3u);
    for (int level = 0; level < 10; ++level) {
        uint8_t palette[4] = {0x0f, 0x30, 0x21, 0x12};
        (void)nes_rom_level_palette(rom, level, palette);
        for (int tile = 0; tile < 3; ++tile) {
            textures[level][tile] = create_colored_chr_tile(
                renderer, rom, CHR_BANK_GAME, tile_ids[tile], palette);
            if (!textures[level][tile]) {
                for (int l = 0; l <= level; ++l) {
                    for (int t = 0; t < 3; ++t) {
                        if (textures[l][t]) {
                            SDL_DestroyTexture(textures[l][t]);
                            textures[l][t] = NULL;
                        }
                    }
                }
                return false;
            }
        }
    }
    return true;
}

static int piece_chr_tile(TetrisPiece piece) {
    switch (piece) {
        case PIECE_T:
        case PIECE_O:
        case PIECE_I:
            return 0;
        case PIECE_Z:
        case PIECE_L:
            return 1;
        case PIECE_J:
        case PIECE_S:
            return 2;
    }
    return 0;
}

static int font_tile(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + c - 'A';
    if (c == '-') return 36;
    return -1;
}

static void draw_text(SDL_Renderer *renderer, SDL_Texture *font, int x, int y,
                      int scale, const char *text) {
    for (const char *p = text; *p; ++p) {
        if (*p == ' ') {
            x += 8 * scale;
            continue;
        }
        {
            const int tile = font_tile(*p);
            if (tile >= 0) {
                SDL_Rect src = {(tile % 16) * 8, (tile / 16) * 8, 8, 8};
                SDL_Rect dst = {x, y, 8 * scale, 8 * scale};
                SDL_RenderCopy(renderer, font, &src, &dst);
            }
        }
        x += 8 * scale;
    }
}

static void draw_centered(SDL_Renderer *renderer, SDL_Texture *font, int y,
                          int scale, const char *text) {
    const int width = (int)strlen(text) * 8 * scale;
    draw_text(renderer, font, (LOGICAL_W - width) / 2, y, scale, text);
}

static SDL_Color block_color(const NesRom *rom, int level, int piece) {
    uint8_t palette[4] = {0x0f, 0x30, 0x21, 0x12};
    int slot;
    (void)nes_rom_level_palette(rom, level, palette);
    if (piece == PIECE_O) slot = 1;
    else slot = 2 + ((piece + level) & 1);
    return NES_COLORS[palette[slot] & 0x3f];
}

static void draw_block(SDL_Renderer *renderer, const NesRom *rom, int level,
                       int x, int y, int size, int piece) {
    SDL_Rect destination = {x, y, size, size};
    int palette_level = level % 10;
    if (palette_level < 0) palette_level += 10;
