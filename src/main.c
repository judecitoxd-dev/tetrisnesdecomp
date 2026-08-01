#include "game.h"
#include "rom.h"

#include <SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOGICAL_W 640
#define LOGICAL_H 480
#define CELL 20
#define BOARD_X 220
#define BOARD_Y 40

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
                pixels[(ty + y) * width + tx + x] = value ? (0xff000000u | (shade << 16) | (shade << 8) | shade) : 0;
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

static void draw_text(SDL_Renderer *renderer, SDL_Texture *font, int x, int y, int scale, const char *text) {
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

static void draw_centered(SDL_Renderer *renderer, SDL_Texture *font, int y, int scale, const char *text) {
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
        if (y >= 0) draw_block(renderer, origin_x + x * cell, origin_y + y * cell, cell, piece);
    }
}

static void render_game(SDL_Renderer *renderer, SDL_Texture *font, const TetrisGame *game,
                        bool show_title, bool non_exact_rom) {
    SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
    SDL_RenderClear(renderer);

    if (show_title) {
        draw_centered(renderer, font, 92, 4, "TETRIS");
        draw_centered(renderer, font, 150, 2, "NES PC PORT");
        draw_centered(renderer, font, 250, 2, "PRESS ENTER");
        draw_centered(renderer, font, 300, 1, "ROM ASSETS LOADED AT RUNTIME");
        if (non_exact_rom) draw_centered(renderer, font, 330, 1, "ROM CRC DIFFERS FROM TESTED DUMP");
        SDL_RenderPresent(renderer);
        return;
    }

    SDL_Rect board_bg = {BOARD_X - 4, BOARD_Y - 4, TETRIS_BOARD_W * CELL + 8,
                         TETRIS_BOARD_H * CELL + 8};
    SDL_SetRenderDrawColor(renderer, 185, 185, 205, 255);
    SDL_RenderFillRect(renderer, &board_bg);
    SDL_Rect board_inner = {BOARD_X, BOARD_Y, TETRIS_BOARD_W * CELL, TETRIS_BOARD_H * CELL};
    SDL_SetRenderDrawColor(renderer, 5, 5, 12, 255);
    SDL_RenderFillRect(renderer, &board_inner);

    for (int y = 0; y < TETRIS_BOARD_H; ++y) {
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (game->board[y][x]) draw_block(renderer, BOARD_X + x * CELL, BOARD_Y + y * CELL,
                                               CELL, game->board[y][x] - 1);
        }
    }
    if (!game->game_over) draw_piece(renderer, game->active, game->rotation, game->x, game->y,
                                      BOARD_X, BOARD_Y, CELL);

    char value[32];
    draw_text(renderer, font, 40, 70, 2, "SCORE");
    snprintf(value, sizeof(value), "%06d", game->score > 999999 ? 999999 : game->score);
    draw_text(renderer, font, 40, 100, 2, value);
    draw_text(renderer, font, 40, 165, 2, "LINES");
    snprintf(value, sizeof(value), "%03d", game->lines > 999 ? 999 : game->lines);
    draw_text(renderer, font, 40, 195, 2, value);
    draw_text(renderer, font, 40, 260, 2, "LEVEL");
    snprintf(value, sizeof(value), "%02d", game->level);
    draw_text(renderer, font, 40, 290, 2, value);

    draw_text(renderer, font, 460, 80, 2, "NEXT");
    draw_piece(renderer, game->next, 0, 0, 0, 485, 125, 18);
    draw_text(renderer, font, 455, 260, 1, "ARROWS MOVE");
    draw_text(renderer, font, 455, 280, 1, "Z X ROTATE");
    draw_text(renderer, font, 455, 300, 1, "SPACE DROP");
    draw_text(renderer, font, 455, 320, 1, "P PAUSE");

    if (game->paused || game->game_over) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 185);
        SDL_Rect overlay = {BOARD_X, BOARD_Y + 145, TETRIS_BOARD_W * CELL, 100};
        SDL_RenderFillRect(renderer, &overlay);
        if (game->paused) draw_centered(renderer, font, BOARD_Y + 177, 2, "PAUSED");
        if (game->game_over) {
            draw_centered(renderer, font, BOARD_Y + 165, 2, "GAME OVER");
            draw_centered(renderer, font, BOARD_Y + 205, 1, "PRESS R");
        }
    }
    SDL_RenderPresent(renderer);
}

static bool load_rom_and_font(SDL_Renderer *renderer, const char *path, NesRom *rom,
                              SDL_Texture **font) {
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

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Tetris NES PC Port", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, 960, 720,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, LOGICAL_W, LOGICAL_H);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    NesRom rom = {0};
    SDL_Texture *font = NULL;
    const char *rom_path = argc > 1 ? argv[1] : "Tetris (USA).nes";
    if (!load_rom_and_font(renderer, rom_path, &rom, &font)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 2;
    }

    TetrisGame game;
    tetris_init(&game, (uint32_t)time(NULL), 0);
    bool running = true;
    bool title = true;
    bool left = false, right = false, down = false;
    uint64_t previous = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    const double step = 1.0 / 60.0988;

    while (running) {
        TetrisInput input = {0};
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_DROPFILE) {
                if (load_rom_and_font(renderer, event.drop.file, &rom, &font)) title = true;
                SDL_free(event.drop.file);
            }
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_RETURN:
                        if (title) {
                            title = false;
                            tetris_init(&game, (uint32_t)time(NULL), 0);
                        }
                        break;
                    case SDLK_LEFT: left = true; break;
                    case SDLK_RIGHT: right = true; break;
                    case SDLK_DOWN: down = true; break;
                    case SDLK_UP:
                    case SDLK_x: input.rotate_cw_pressed = true; break;
                    case SDLK_z: input.rotate_ccw_pressed = true; break;
                    case SDLK_SPACE: input.hard_drop_pressed = true; break;
                    case SDLK_p: input.pause_pressed = true; break;
                    case SDLK_r: input.restart_pressed = true; break;
                    default: break;
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_LEFT) left = false;
                if (event.key.keysym.sym == SDLK_RIGHT) right = false;
                if (event.key.keysym.sym == SDLK_DOWN) down = false;
            }
        }

        const uint64_t now = SDL_GetPerformanceCounter();
        accumulator += (double)(now - previous) / (double)SDL_GetPerformanceFrequency();
        previous = now;
        if (accumulator > 0.25) accumulator = 0.25;

        bool one_shot_consumed = false;
        while (accumulator >= step) {
            if (!title) {
                TetrisInput tick_input = input;
                tick_input.left = left;
                tick_input.right = right;
                tick_input.down = down;
                if (one_shot_consumed) {
                    tick_input.rotate_cw_pressed = false;
                    tick_input.rotate_ccw_pressed = false;
                    tick_input.hard_drop_pressed = false;
                    tick_input.pause_pressed = false;
                    tick_input.restart_pressed = false;
                }
                tetris_tick(&game, &tick_input);
                one_shot_consumed = true;
            }
            accumulator -= step;
        }

        render_game(renderer, font, &game, title, !rom.exact_supported_dump);
        SDL_Delay(1);
    }

    SDL_DestroyTexture(font);
    nes_rom_free(&rom);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
