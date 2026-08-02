    if (piece >= PIECE_T && piece <= PIECE_I) {
        SDL_Texture *texture =
            g_block_textures[palette_level][piece_chr_tile((TetrisPiece)piece)];
        if (texture) {
            SDL_RenderCopy(renderer, texture, NULL, &destination);
            return;
        }
    }
    {
        const SDL_Color c = block_color(rom, level, piece);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        SDL_RenderFillRect(renderer, &destination);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 145);
        SDL_RenderDrawLine(renderer, x + 1, y + 1, x + size - 2, y + 1);
        SDL_RenderDrawLine(renderer, x + 1, y + 1, x + 1, y + size - 2);
        SDL_SetRenderDrawColor(renderer, 20, 20, 28, 255);
        SDL_RenderDrawLine(renderer, x + size - 1, y,
                           x + size - 1, y + size - 1);
        SDL_RenderDrawLine(renderer, x, y + size - 1,
                           x + size - 1, y + size - 1);
    }
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
    if (g_title_texture && !non_exact_rom) {
        SDL_Rect destination = {64, 0, 512, 480};
        SDL_RenderCopy(renderer, g_title_texture, NULL, &destination);
        return;
    }
    draw_centered(renderer, font, 44, 4, "TETRIS");
    draw_centered(renderer, font, 102, 2, "NES PC PORT V05");
    draw_centered(renderer, font, 164, 2, "PRESS ENTER");
    draw_centered(renderer, font, 202, 1, "H RECORDS  O OPTIONS");
    draw_centered(renderer, font, 224, 1, "L LOAD ROM  F8 REPLAY");
    draw_centered(renderer, font, 264, 1, "TOP A");
    snprintf(value, sizeof(value), "%06d",
             tetris_high_scores_top(scores, TETRIS_MODE_A)->score);
    draw_centered(renderer, font, 284, 1, value);
    draw_centered(renderer, font, 314, 1, "TOP B");
    snprintf(value, sizeof(value), "%06d",
             tetris_high_scores_top(scores, TETRIS_MODE_B)->score);
    draw_centered(renderer, font, 334, 1, value);
    draw_centered(renderer, font, 380, 1, "MODE B  REPLAYS  SAVED SETTINGS");
    if (non_exact_rom) draw_centered(renderer, font, 414, 1,
                                     "ROM CRC DIFFERS FROM TESTED DUMP");
}

static bool nes_cursor_visible(void) {
    const unsigned frame = (unsigned)((SDL_GetTicks() * 60u) / 1000u);
    return (frame & 3u) != 0u;
}

static void draw_nes_menu_cursor(SDL_Renderer *renderer, int x, int y,
                                 int width, int height) {
    SDL_Rect outer = {x, y, width, height};
    SDL_Rect inner = {x + 2, y + 2, width - 4, height - 4};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 236, 238, 236, 230);
    SDL_RenderDrawRect(renderer, &outer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 165);
    SDL_RenderDrawRect(renderer, &inner);
}

static void render_type_select(SDL_Renderer *renderer, SDL_Texture *font,
                               const AppMenuState *menu) {
    if (tetris_rom_screen_available(TETRIS_ROM_SCREEN_TYPE_MENU)) {
        const int nes_x = 0x3f + (menu->mode == TETRIS_MODE_B ? 0x60 : 0);
        tetris_rom_screen_render(renderer, TETRIS_ROM_SCREEN_TYPE_MENU);
        if (nes_cursor_visible()) {
            if (tetris_rom_sprite_available(TETRIS_ROM_SPRITE_TYPE_CURSOR))
                tetris_rom_sprite_render(renderer, TETRIS_ROM_SPRITE_TYPE_CURSOR,
                                          nes_x, 0x3f);
            else
                draw_nes_menu_cursor(renderer, 64 + nes_x * 2, 0x3f * 2,
                                     120, 16);
        }
        return;
    }
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
    const TetrisRomScreen exact_screen = menu->mode == TETRIS_MODE_A
        ? TETRIS_ROM_SCREEN_LEVEL_A : TETRIS_ROM_SCREEN_LEVEL_B;
    if (tetris_rom_screen_available(exact_screen)) {
        int nes_x;
        int nes_y;
        int selected_digit;
        tetris_rom_screen_render(renderer, exact_screen);
        if (menu->mode == TETRIS_MODE_B && menu->selecting_height) {
            selected_digit = menu->height;
            nes_x = 0x9c + (menu->height % 3) * 0x10;
            nes_y = 0x53 + (menu->height / 3) * 0x10;
        } else {
            selected_digit = menu->level % 10;
            nes_x = 0x34 + (selected_digit % 5) * 0x10;
            nes_y = 0x53 + (selected_digit / 5) * 0x10;
        }
        if (nes_cursor_visible()) {
            char digit[2] = {(char)('0' + selected_digit), '\0'};
            if (tetris_rom_sprite_available(TETRIS_ROM_SPRITE_LEVEL_CURSOR))
                tetris_rom_sprite_render(renderer, TETRIS_ROM_SPRITE_LEVEL_CURSOR,
                                          nes_x, nes_y);
            else
                draw_nes_menu_cursor(renderer, 64 + nes_x * 2, nes_y * 2,
                                     16, 16);
            /* The original sprite has priority behind the background digit. */
            draw_text(renderer, font, 64 + (nes_x + 4) * 2,
                      (nes_y + 5) * 2, 2, digit);
        }
        if (menu->level >= 10) {
            snprintf(value, sizeof(value), "START LEVEL %02d", menu->level);
            draw_text(renderer, font, 232, 452, 1, value);
        }
        return;
    }
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
    if (tetris_rom_screen_available(TETRIS_ROM_SCREEN_HIGH_SCORES)) {
        const int mode = (int)((SDL_GetTicks() / 3000u) & 1u);
        tetris_rom_screen_render(renderer, TETRIS_ROM_SCREEN_HIGH_SCORES);
        draw_text(renderer, font, 296, 48, 2, mode == 0 ? "A" : "B");
        for (int rank = 0; rank < TETRIS_HIGH_SCORE_COUNT; ++rank) {
            const TetrisHighScoreEntry *entry = &scores->entries[mode][rank];
            const int y = 320 + rank * 32;
            draw_text(renderer, font, 208, y, 2, entry->name);
            snprintf(value, sizeof(value), "%06d",
                     entry->score > 999999 ? 999999 : entry->score);
            draw_text(renderer, font, 320, y, 2, value);
            snprintf(value, sizeof(value), "%02d",
                     entry->level > 99 ? 99 : entry->level);
            draw_text(renderer, font, 448, y, 2, value);
        }
        draw_centered(renderer, font, 438, 1,
                      "A/B RECORDS SWITCH EVERY 3 SECONDS");
        return;
    }
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

static void render_option_row(SDL_Renderer *renderer, SDL_Texture *font,
                              int y, bool selected, const char *label,
                              const char *value) {
    if (selected) {
        SDL_Rect cursor = {92, y - 3, 12, 18};
        SDL_SetRenderDrawColor(renderer, 236, 238, 236, 255);
        SDL_RenderFillRect(renderer, &cursor);
    }
    draw_text(renderer, font, 120, y, 1, label);
    draw_text(renderer, font, 390, y, 1, value);
}

static void render_options(SDL_Renderer *renderer, SDL_Texture *font,
