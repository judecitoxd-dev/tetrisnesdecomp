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
    const SDL_Color c = block_color(rom, level, piece);
    SDL_Rect r = {x, y, size, size};
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 145);
    SDL_RenderDrawLine(renderer, x + 1, y + 1, x + size - 2, y + 1);
    SDL_RenderDrawLine(renderer, x + 1, y + 1, x + 1, y + size - 2);
    SDL_SetRenderDrawColor(renderer, 20, 20, 28, 255);
    SDL_RenderDrawLine(renderer, x + size - 1, y,
                       x + size - 1, y + size - 1);
    SDL_RenderDrawLine(renderer, x, y + size - 1,
                       x + size - 1, y + size - 1);
}

static void draw_piece(SDL_Renderer *renderer, const NesRom *rom, int level,
                       TetrisPiece piece, int rotation, int px, int py,
                       int origin_x, int origin_y, int cell) {
    for (int i = 0; i < 4; ++i) {
        const int x = px + tetris_piece_block_x(piece, rotation, i);
        const int y = py + tetris_piece_block_y(piece, rotation, i);
        if (y >= 0) draw_block(renderer, rom, level,
                               origin_x + x * cell, origin_y + y * cell,
                               cell, piece);
    }
}

static void draw_score_value(SDL_Renderer *renderer, SDL_Texture *font,
                             int x, int y, int score) {
    char value[16];
    if (score < 0) score = 0;
    if (score > 999999) score = 999999;
    snprintf(value, sizeof(value), "%06d", score);
    draw_text(renderer, font, x, y, 1, value);
}

static void render_title(SDL_Renderer *renderer, SDL_Texture *font,
                         bool non_exact_rom, const TetrisHighScores *scores) {
    char value[32];
    draw_centered(renderer, font, 58, 4, "TETRIS");
    draw_centered(renderer, font, 116, 2, "NES PC PORT V03");
    draw_centered(renderer, font, 198, 2, "PRESS ENTER");
    draw_centered(renderer, font, 236, 1, "H SHOWS RECORDS");
    draw_centered(renderer, font, 276, 1, "TOP A");
    snprintf(value, sizeof(value), "%06d",
             tetris_high_scores_top(scores, TETRIS_MODE_A)->score);
    draw_centered(renderer, font, 296, 1, value);
    draw_centered(renderer, font, 326, 1, "TOP B");
    snprintf(value, sizeof(value), "%06d",
             tetris_high_scores_top(scores, TETRIS_MODE_B)->score);
    draw_centered(renderer, font, 346, 1, value);
    draw_centered(renderer, font, 390, 1, "MODE B  PALETTES  LOCAL SCORES");
    if (non_exact_rom) draw_centered(renderer, font, 420, 1,
                                     "ROM CRC DIFFERS FROM TESTED DUMP");
}

static void render_type_select(SDL_Renderer *renderer, SDL_Texture *font,
                               const AppMenuState *menu) {
    draw_centered(renderer, font, 70, 3, "GAME TYPE");
    draw_centered(renderer, font, 170, 3,
                  menu->mode == TETRIS_MODE_A ? "A TYPE" : "B TYPE");
    draw_centered(renderer, font, 265, 1, "ARROWS CHANGE TYPE");
    draw_centered(renderer, font, 295, 1, "ENTER CONTINUES");
    draw_centered(renderer, font, 335, 1,
                  menu->mode == TETRIS_MODE_A
                    ? "ENDURANCE MODE" : "CLEAR 25 LINES");
}

static void render_level_select(SDL_Renderer *renderer, SDL_Texture *font,
                                const AppMenuState *menu) {
    char value[32];
    draw_centered(renderer, font, 54, 3,
                  menu->mode == TETRIS_MODE_A ? "A TYPE SETUP" : "B TYPE SETUP");
    snprintf(value, sizeof(value), "LEVEL %02d", menu->level);
    draw_centered(renderer, font, 145, 3, value);
    if (menu->mode == TETRIS_MODE_B) {
        snprintf(value, sizeof(value), "HEIGHT %d", menu->height);
        draw_centered(renderer, font, 220, 3, value);
        draw_centered(renderer, font, 285, 1,
                      menu->selecting_height ? "SELECT HEIGHT" : "SELECT LEVEL");
        draw_centered(renderer, font, 315, 1, "UP DOWN CHANGES FIELD");
        draw_centered(renderer, font, 340, 1, "LEFT RIGHT CHANGES VALUE");
    } else {
        draw_centered(renderer, font, 260, 1, "ARROWS CHANGE LEVEL");
    }
    draw_centered(renderer, font, 390, 1, "ENTER STARTS GAME");
    draw_centered(renderer, font, 415, 1, "BACKSPACE GOES BACK");
}

static void render_records(SDL_Renderer *renderer, SDL_Texture *font,
                           const TetrisHighScores *scores) {
    char value[64];
    draw_centered(renderer, font, 38, 3, "LOCAL RECORDS");
    draw_text(renderer, font, 95, 105, 2, "A TYPE");
    draw_text(renderer, font, 385, 105, 2, "B TYPE");
    for (int rank = 0; rank < TETRIS_HIGH_SCORE_COUNT; ++rank) {
        const TetrisHighScoreEntry *a = &scores->entries[0][rank];
        const TetrisHighScoreEntry *b = &scores->entries[1][rank];
        snprintf(value, sizeof(value), "%d %s %06d", rank + 1, a->name, a->score);
        draw_text(renderer, font, 55, 155 + rank * 48, 1, value);
        snprintf(value, sizeof(value), "LEVEL %02d", a->level);
        draw_text(renderer, font, 90, 175 + rank * 48, 1, value);
        snprintf(value, sizeof(value), "%d %s %06d", rank + 1, b->name, b->score);
        draw_text(renderer, font, 345, 155 + rank * 48, 1, value);
        snprintf(value, sizeof(value), "L%02d H%d", b->level, b->height);
        draw_text(renderer, font, 390, 175 + rank * 48, 1, value);
    }
    draw_centered(renderer, font, 390, 1, "ENTER OR BACKSPACE RETURNS");
}

static void draw_piece_stats(SDL_Renderer *renderer, SDL_Texture *font,
                             const TetrisGame *game) {
    static const char *names[7] = {"T", "J", "Z", "O", "S", "L", "I"};
    char value[16];
    draw_text(renderer, font, 470, 218, 1, "STATS");
    for (int i = 0; i < 7; ++i) {
        draw_text(renderer, font, 472, 240 + i * 18, 1, names[i]);
        snprintf(value, sizeof(value), "%03d",
                 game->piece_count[i] > 999 ? 999 : game->piece_count[i]);
        draw_text(renderer, font, 500, 240 + i * 18, 1, value);
    }
}

static void render_game(SDL_Renderer *renderer, SDL_Texture *font,
                        const TetrisGame *game, const TetrisAudio *audio,
                        const NesRom *rom, const TetrisHighScores *scores) {
    SDL_Rect board_bg = {BOARD_X - 4, BOARD_Y - 4,
                         TETRIS_BOARD_W * CELL + 8,
                         TETRIS_BOARD_H * CELL + 8};
    SDL_Rect board_inner = {BOARD_X, BOARD_Y,
                            TETRIS_BOARD_W * CELL,
                            TETRIS_BOARD_H * CELL};
    const TetrisHighScoreEntry *top = tetris_high_scores_top(scores, game->mode);
    char value[32];

    SDL_SetRenderDrawColor(renderer, 185, 185, 205, 255);
    SDL_RenderFillRect(renderer, &board_bg);
    SDL_SetRenderDrawColor(renderer, 5, 5, 12, 255);
    SDL_RenderFillRect(renderer, &board_inner);

    for (int y = 0; y < TETRIS_BOARD_H; ++y) {
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (game->board[y][x] && !tetris_cell_hidden(game, x, y)) {
                draw_block(renderer, rom, game->level,
                           BOARD_X + x * CELL, BOARD_Y + y * CELL,
                           CELL, game->board[y][x] - 1);
            }
        }
    }
    if (game->phase == TETRIS_PHASE_ACTIVE) {
        draw_piece(renderer, rom, game->level, game->active, game->rotation,
                   game->x, game->y, BOARD_X, BOARD_Y, CELL);
    }

    if (game->phase == TETRIS_PHASE_GAME_OVER_CURTAIN ||
        game->phase == TETRIS_PHASE_GAME_OVER) {
        const int rows = game->phase == TETRIS_PHASE_GAME_OVER
            ? TETRIS_BOARD_H : game->curtain_rows;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < TETRIS_BOARD_W; ++x) {
                SDL_Rect r = {BOARD_X + x * CELL + 1,
                              BOARD_Y + y * CELL + 1,
                              CELL - 2, CELL - 2};
                SDL_SetRenderDrawColor(renderer, 90, 90, 105, 255);
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }

    draw_text(renderer, font, 36, 42, 1,
              game->mode == TETRIS_MODE_A ? "A TYPE" : "B TYPE");
    draw_text(renderer, font, 36, 70, 2, "SCORE");
    snprintf(value, sizeof(value), "%06d", game->score);
    draw_text(renderer, font, 36, 100, 2, value);
    draw_text(renderer, font, 36, 160, 2,
              game->mode == TETRIS_MODE_A ? "LINES" : "LEFT");
    snprintf(value, sizeof(value), "%03d", game->lines > 999 ? 999 : game->lines);
    draw_text(renderer, font, 36, 190, 2, value);
    draw_text(renderer, font, 36, 250, 2, "LEVEL");
    snprintf(value, sizeof(value), "%02d", game->level > 99 ? 99 : game->level);
    draw_text(renderer, font, 36, 280, 2, value);
    if (game->mode == TETRIS_MODE_B) {
        draw_text(renderer, font, 36, 334, 1, "HEIGHT");
        snprintf(value, sizeof(value), "%d", game->start_height);
        draw_text(renderer, font, 84, 354, 1, value);
    } else {
        draw_text(renderer, font, 36, 334, 1, "TRANSITION");
        snprintf(value, sizeof(value), "%03d", game->transition_lines);
        draw_text(renderer, font, 60, 354, 1, value);
    }
    draw_text(renderer, font, 36, 382, 1, "TOP");
    draw_score_value(renderer, font, 68, 382, top->score);

    draw_text(renderer, font, 470, 52, 2, "NEXT");
    if (game->show_next) {
        draw_piece(renderer, rom, game->level, game->next,
                   tetris_spawn_rotation(game->next),
                   2, 2, 478, 94, 18);
    } else {
        draw_text(renderer, font, 482, 120, 1, "HIDDEN");
    }
    draw_piece_stats(renderer, font, game);

    draw_text(renderer, font, 24, 410, 1, "Z X ROTATE  SPACE DROP  TAB NEXT");
    draw_text(renderer, font, 24, 432, 1, "P PAUSE  M AUDIO  N MUSIC  F11 FULL");
    draw_text(renderer, font, 470, 390, 1,
              audio->enabled ? "AUDIO ON" : "AUDIO OFF");
    draw_text(renderer, font, 470, 410, 1, tetris_audio_music_label(audio));

    if (game->paused || game->phase == TETRIS_PHASE_GAME_OVER ||
        game->phase == TETRIS_PHASE_COMPLETE) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 205);
        {
            SDL_Rect overlay = {BOARD_X, BOARD_Y + 140,
                                TETRIS_BOARD_W * CELL, 120};
            SDL_RenderFillRect(renderer, &overlay);
        }
        if (game->paused) draw_centered(renderer, font, BOARD_Y + 188, 2, "PAUSED");
        if (game->phase == TETRIS_PHASE_GAME_OVER) {
            draw_centered(renderer, font, BOARD_Y + 168, 2, "GAME OVER");
            draw_centered(renderer, font, BOARD_Y + 215, 1, "PRESS R");
        }
        if (game->phase == TETRIS_PHASE_COMPLETE) {
            draw_centered(renderer, font, BOARD_Y + 158, 2, "SUCCESS");
            draw_centered(renderer, font, BOARD_Y + 202, 1, "25 LINES CLEARED");
            draw_centered(renderer, font, BOARD_Y + 230, 1, "PRESS R");
        }
    }
}

void render(SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
            const TetrisGame *game, const AppMenuState *menu,
            bool non_exact_rom, const TetrisAudio *audio,
            const NesRom *rom, const TetrisHighScores *scores) {
    SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
    SDL_RenderClear(renderer);
    switch (screen) {
        case SCREEN_TITLE:
            render_title(renderer, font, non_exact_rom, scores);
            break;
        case SCREEN_TYPE_SELECT:
            render_type_select(renderer, font, menu);
            break;
        case SCREEN_LEVEL_SELECT:
            render_level_select(renderer, font, menu);
            break;
        case SCREEN_RECORDS:
            render_records(renderer, font, scores);
            break;
        case SCREEN_GAME:
            render_game(renderer, font, game, audio, rom, scores);
            break;
    }
    SDL_RenderPresent(renderer);
}

bool load_rom_and_font(SDL_Renderer *renderer, const char *path,
                       NesRom *rom, SDL_Texture **font) {
    char error[256];
    NesRom loaded;
    if (!nes_rom_load(path, &loaded, error, sizeof(error))) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Tetris NES PC Port", error, NULL);
        return false;
    }
    {
        SDL_Texture *new_font = create_chr_texture(renderer, &loaded);
        if (!new_font) {
            nes_rom_free(&loaded);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                     "Tetris NES PC Port",
                                     "Could not create the CHR texture.", NULL);
            return false;
        }
        if (*font) SDL_DestroyTexture(*font);
        nes_rom_free(rom);
        *rom = loaded;
        *font = new_font;
    }
    fprintf(stdout, "Loaded ROM: %s\nCRC32: %08X%s\n", path, rom->crc32,
            rom->exact_supported_dump
                ? " (tested dump)" : " (compatible, unverified dump)");
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

void begin_game(TetrisGame *game, const AppMenuState *menu) {
    tetris_init_mode(game, (uint32_t)time(NULL) ^ SDL_GetTicks(),
                     menu->level, menu->mode, menu->height);
}

void change_menu_value(AppMenuState *menu, int delta) {
    if (menu->mode == TETRIS_MODE_B && menu->selecting_height) {
        menu->height += delta;
        if (menu->height < 0) menu->height = 0;
        if (menu->height > 5) menu->height = 5;
    } else {
        menu->level += delta;
        if (menu->level < 0) menu->level = 0;
        if (menu->level > 19) menu->level = 19;
    }
}

void toggle_menu_mode(AppMenuState *menu) {
    menu->mode = menu->mode == TETRIS_MODE_A ? TETRIS_MODE_B : TETRIS_MODE_A;
    menu->selecting_height = false;
}
