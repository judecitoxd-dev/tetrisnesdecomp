#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const SDL_Color PIECE_COLORS[7] = {
    {178, 95, 255, 255}, {70, 100, 255, 255}, {255, 75, 75, 255},
    {245, 215, 60, 255}, {80, 220, 110, 255}, {255, 150, 55, 255},
    {70, 220, 235, 255}
};

static SDL_Texture *create_chr_texture(SDL_Renderer *renderer, const NesRom *rom) {
    const int tile_count = rom->chr_size >= 4096 ? 256 : (int)(rom->chr_size / 16);
    const int width = 16 * 8;
    const int height = 16 * 8;
    uint32_t *pixels = (uint32_t *)calloc((size_t)width * height, sizeof(uint32_t));
    if (!pixels) return NULL;
    for (int tile = 0; tile < tile_count; ++tile) {
        const uint8_t *src = rom->chr + tile * 16;
        const int tx = (tile % 16) * 8;
        const int ty = (tile / 16) * 8;
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                const int bit = 7 - x;
                const int value = ((src[y] >> bit) & 1) | (((src[y + 8] >> bit) & 1) << 1);
                uint8_t shade = 0;
                if (value == 1) shade = 145;
                if (value == 2) shade = 205;
                if (value == 3) shade = 255;
                pixels[(ty + y) * width + tx + x] =
                    value ? (0xff000000u | ((uint32_t)shade << 16) |
                             ((uint32_t)shade << 8) | shade) : 0;
            }
        }
    }
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STATIC, width, height);
    if (texture) {
        SDL_UpdateTexture(texture, NULL, pixels, width * (int)sizeof(uint32_t));
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    }
    free(pixels);
    return texture;
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
        const int tile = font_tile(*p);
        if (tile >= 0) {
            SDL_Rect src = {(tile % 16) * 8, (tile / 16) * 8, 8, 8};
            SDL_Rect dst = {x, y, 8 * scale, 8 * scale};
            SDL_RenderCopy(renderer, font, &src, &dst);
        }
        x += 8 * scale;
    }
}

static void draw_centered(SDL_Renderer *renderer, SDL_Texture *font, int y,
                          int scale, const char *text) {
    const int width = (int)strlen(text) * 8 * scale;
    draw_text(renderer, font, (LOGICAL_W - width) / 2, y, scale, text);
}

static void draw_block(SDL_Renderer *renderer, int x, int y, int size, int piece) {
    SDL_Color c = PIECE_COLORS[piece % 7];
    SDL_Rect r = {x, y, size, size};
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 130);
    SDL_RenderDrawLine(renderer, x + 1, y + 1, x + size - 2, y + 1);
    SDL_RenderDrawLine(renderer, x + 1, y + 1, x + 1, y + size - 2);
    SDL_SetRenderDrawColor(renderer, 25, 25, 35, 255);
    SDL_RenderDrawLine(renderer, x + size - 1, y, x + size - 1, y + size - 1);
    SDL_RenderDrawLine(renderer, x, y + size - 1, x + size - 1, y + size - 1);
}

static void draw_piece(SDL_Renderer *renderer, TetrisPiece piece, int rotation,
                       int px, int py, int origin_x, int origin_y, int cell) {
    for (int i = 0; i < 4; ++i) {
        const int x = px + tetris_piece_block_x(piece, rotation, i);
        const int y = py + tetris_piece_block_y(piece, rotation, i);
        if (y >= 0) draw_block(renderer, origin_x + x * cell,
                               origin_y + y * cell, cell, piece);
    }
}

static void render_title(SDL_Renderer *renderer, SDL_Texture *font, bool non_exact_rom) {
    draw_centered(renderer, font, 78, 4, "TETRIS");
    draw_centered(renderer, font, 138, 2, "NES PC PORT V02");
    draw_centered(renderer, font, 238, 2, "PRESS ENTER");
    draw_centered(renderer, font, 292, 1, "NATIVE C PORT WITH ROM ASSETS");
    draw_centered(renderer, font, 318, 1, "GAMEPAD SUPPORTED");
    if (non_exact_rom) draw_centered(renderer, font, 350, 1,
                                     "ROM CRC DIFFERS FROM TESTED DUMP");
}

static void render_level_select(SDL_Renderer *renderer, SDL_Texture *font,
                                int selected_level) {
    char value[16];
    draw_centered(renderer, font, 70, 3, "LEVEL SELECT");
    snprintf(value, sizeof(value), "%02d", selected_level);
    draw_centered(renderer, font, 170, 6, value);
    draw_centered(renderer, font, 285, 1, "ARROWS CHANGE LEVEL");
    draw_centered(renderer, font, 310, 1, "ENTER STARTS GAME");
    draw_centered(renderer, font, 342, 1, "LEVELS 10 TO 19 ARE PC SHORTCUTS");
}

static void draw_piece_stats(SDL_Renderer *renderer, SDL_Texture *font,
                             const TetrisGame *game) {
    static const char *names[7] = {"T", "J", "Z", "O", "S", "L", "I"};
    char value[16];
    draw_text(renderer, font, 470, 218, 1, "STATS");
    for (int i = 0; i < 7; ++i) {
        draw_text(renderer, font, 472, 240 + i * 18, 1, names[i]);
        snprintf(value, sizeof(value), "%03d", game->piece_count[i] > 999 ? 999 : game->piece_count[i]);
        draw_text(renderer, font, 500, 240 + i * 18, 1, value);
    }
}

static void render_game(SDL_Renderer *renderer, SDL_Texture *font,
                        const TetrisGame *game, const TetrisAudio *audio) {
    SDL_Rect board_bg = {BOARD_X - 4, BOARD_Y - 4,
                         TETRIS_BOARD_W * CELL + 8,
                         TETRIS_BOARD_H * CELL + 8};
    SDL_SetRenderDrawColor(renderer, 185, 185, 205, 255);
    SDL_RenderFillRect(renderer, &board_bg);
    SDL_Rect board_inner = {BOARD_X, BOARD_Y,
                            TETRIS_BOARD_W * CELL,
                            TETRIS_BOARD_H * CELL};
    SDL_SetRenderDrawColor(renderer, 5, 5, 12, 255);
    SDL_RenderFillRect(renderer, &board_inner);

    for (int y = 0; y < TETRIS_BOARD_H; ++y) {
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (game->board[y][x] && !tetris_cell_hidden(game, x, y)) {
                draw_block(renderer, BOARD_X + x * CELL, BOARD_Y + y * CELL,
                           CELL, game->board[y][x] - 1);
            }
        }
    }
    if (game->phase == TETRIS_PHASE_ACTIVE) {
        draw_piece(renderer, game->active, game->rotation, game->x, game->y,
                   BOARD_X, BOARD_Y, CELL);
    }

    if (game->phase == TETRIS_PHASE_GAME_OVER_CURTAIN ||
        game->phase == TETRIS_PHASE_GAME_OVER) {
        const int rows = game->phase == TETRIS_PHASE_GAME_OVER ? TETRIS_BOARD_H : game->curtain_rows;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < TETRIS_BOARD_W; ++x) {
                SDL_Rect r = {BOARD_X + x * CELL + 1, BOARD_Y + y * CELL + 1,
                              CELL - 2, CELL - 2};
                SDL_SetRenderDrawColor(renderer, 90, 90, 105, 255);
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }

    char value[32];
    draw_text(renderer, font, 36, 58, 2, "SCORE");
    snprintf(value, sizeof(value), "%06d", game->score > 999999 ? 999999 : game->score);
    draw_text(renderer, font, 36, 88, 2, value);
    draw_text(renderer, font, 36, 152, 2, "LINES");
    snprintf(value, sizeof(value), "%03d", game->lines > 999 ? 999 : game->lines);
    draw_text(renderer, font, 36, 182, 2, value);
    draw_text(renderer, font, 36, 246, 2, "LEVEL");
    snprintf(value, sizeof(value), "%02d", game->level > 99 ? 99 : game->level);
    draw_text(renderer, font, 36, 276, 2, value);
    draw_text(renderer, font, 36, 340, 1, "TRANSITION");
    snprintf(value, sizeof(value), "%03d", game->transition_lines);
    draw_text(renderer, font, 60, 362, 1, value);

    draw_text(renderer, font, 470, 52, 2, "NEXT");
    if (game->show_next) {
        draw_piece(renderer, game->next, tetris_spawn_rotation(game->next),
                   2, 2, 478, 94, 18);
    } else {
        draw_text(renderer, font, 482, 120, 1, "HIDDEN");
    }
    draw_piece_stats(renderer, font, game);

    draw_text(renderer, font, 28, 410, 1, "Z X ROTATE  SPACE DROP  TAB NEXT");
    draw_text(renderer, font, 28, 432, 1, "P PAUSE  M AUDIO  F11 FULLSCREEN");
    draw_text(renderer, font, 480, 410, 1, audio->enabled ? "AUDIO ON" : "AUDIO OFF");

    if (game->paused || game->phase == TETRIS_PHASE_GAME_OVER) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
        SDL_Rect overlay = {BOARD_X, BOARD_Y + 145,
                            TETRIS_BOARD_W * CELL, 100};
        SDL_RenderFillRect(renderer, &overlay);
        if (game->paused) draw_centered(renderer, font, BOARD_Y + 177, 2, "PAUSED");
        if (game->phase == TETRIS_PHASE_GAME_OVER) {
            draw_centered(renderer, font, BOARD_Y + 162, 2, "GAME OVER");
            draw_centered(renderer, font, BOARD_Y + 203, 1, "PRESS R");
        }
    }
}

void render(SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
            const TetrisGame *game, int selected_level,
            bool non_exact_rom, const TetrisAudio *audio) {
    SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
    SDL_RenderClear(renderer);
    if (screen == SCREEN_TITLE) render_title(renderer, font, non_exact_rom);
    else if (screen == SCREEN_LEVEL_SELECT) render_level_select(renderer, font, selected_level);
    else render_game(renderer, font, game, audio);
    SDL_RenderPresent(renderer);
}

bool load_rom_and_font(SDL_Renderer *renderer, const char *path,
                       NesRom *rom, SDL_Texture **font) {
    char error[256];
    NesRom loaded;
    if (!nes_rom_load(path, &loaded, error, sizeof(error))) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Tetris NES PC Port", error, NULL);
        return false;
    }
    SDL_Texture *new_font = create_chr_texture(renderer, &loaded);
    if (!new_font) {
        nes_rom_free(&loaded);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Tetris NES PC Port",
                                 "Could not create the CHR texture.", NULL);
        return false;
    }
    if (*font) SDL_DestroyTexture(*font);
    nes_rom_free(rom);
    *rom = loaded;
    *font = new_font;
    fprintf(stdout, "Loaded ROM: %s\nCRC32: %08X%s\n", path, rom->crc32,
            rom->exact_supported_dump ? " (tested dump)" : " (compatible, unverified dump)");
    return true;
}

SDL_GameController *open_first_controller(void) {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController *controller = SDL_GameControllerOpen(i);
            if (controller) return controller;
        }
    }
    return NULL;
}

void clear_held(bool *left, bool *right, bool *down) {
    *left = false;
    *right = false;
    *down = false;
}

void begin_game(TetrisGame *game, int level) {
    tetris_init(game, (uint32_t)time(NULL) ^ SDL_GetTicks(), level);
}

void change_level(int *level, int delta) {
    *level += delta;
    if (*level < 0) *level = 0;
    if (*level > 19) *level = 19;
}
