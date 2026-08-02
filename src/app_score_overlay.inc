/* Dynamic fields drawn over the static nametables reconstructed from the ROM. */
enum {
    RECORD_SCORE_X = 208,
    RECORD_LINES_X = 320,
    RECORD_NAME_X = 400,
    RECORD_FIRST_Y = 320,
    RECORD_ROW_STEP = 32
};

static void clear_rom_field(SDL_Renderer *renderer, int x, int y,
                            int width, int height) {
    SDL_Rect field = {x, y, width, height};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &field);
}

static void clear_record_row(SDL_Renderer *renderer, int y) {
    clear_rom_field(renderer, RECORD_SCORE_X - 8, y - 3, 112, 22);
    clear_rom_field(renderer, RECORD_LINES_X - 8, y - 3, 64, 22);
    clear_rom_field(renderer, RECORD_NAME_X - 8, y - 3, 112, 22);
}

static void draw_record_values(SDL_Renderer *renderer, SDL_Texture *font,
                               int y, const char *name,
                               int score, int lines) {
    char value[16];
    if (score < 0) score = 0;
    if (score > 999999) score = 999999;
    if (lines < 0) lines = 0;
    if (lines > 999) lines = 999;

    snprintf(value, sizeof(value), "%06d", score);
    draw_text(renderer, font, RECORD_SCORE_X, y, 2, value);
    snprintf(value, sizeof(value), "%03d", lines);
    draw_text(renderer, font, RECORD_LINES_X, y, 2, value);
    draw_text(renderer, font, RECORD_NAME_X, y, 2,
              name && name[0] ? name : "------");
}

static void overlay_exact_title_score(SDL_Renderer *renderer,
                                      SDL_Texture *font,
                                      const TetrisHighScores *scores) {
    char value[16];
    const TetrisHighScoreEntry *top;
    if (!g_title_texture || !scores) return;
    top = tetris_high_scores_top(scores, TETRIS_MODE_A);
    clear_rom_field(renderer, 264, 348, 112, 24);
    snprintf(value, sizeof(value), "%06d",
             top->score > 999999 ? 999999 : top->score);
    draw_text(renderer, font, 272, 352, 2, value);
}

static void overlay_exact_game_scores(SDL_Renderer *renderer,
                                      SDL_Texture *font,
                                      const TetrisGame *game,
                                      const TetrisHighScores *scores) {
    char value[16];
    const TetrisHighScoreEntry *top;
    if (!game || !scores ||
        !tetris_rom_screen_available(TETRIS_ROM_SCREEN_GAME)) return;
    top = tetris_high_scores_top(scores, game->mode);

    clear_rom_field(renderer, 440, 84, 112, 22);
    snprintf(value, sizeof(value), "%06d",
             top->score > 999999 ? 999999 : top->score);
    draw_text(renderer, font, 448, 88, 2, value);

    clear_rom_field(renderer, 440, 138, 112, 22);
    snprintf(value, sizeof(value), "%06d",
             game->score > 999999 ? 999999 : game->score);
    draw_text(renderer, font, 448, 142, 2, value);
}

static void overlay_exact_records(SDL_Renderer *renderer, SDL_Texture *font,
                                  const TetrisHighScores *scores) {
    int mode;
    int rank;
    if (!scores ||
        !tetris_rom_screen_available(TETRIS_ROM_SCREEN_HIGH_SCORES)) return;
    mode = (int)((SDL_GetTicks() / 3000u) & 1u);
    for (rank = 0; rank < TETRIS_HIGH_SCORE_COUNT; ++rank) {
        const int y = RECORD_FIRST_Y + rank * RECORD_ROW_STEP;
        const TetrisHighScoreEntry *entry = &scores->entries[mode][rank];
        clear_record_row(renderer, y);
        draw_record_values(renderer, font, y, entry->name,
                           entry->score, entry->lines);
    }
}

static void overlay_exact_name_entry(SDL_Renderer *renderer,
                                     SDL_Texture *font,
                                     const AppResultState *result) {
    static const int cursor_x[TETRIS_NAME_ENTRY_LENGTH] = {
        0xa8, 0xb0, 0xb8, 0xc0, 0xc8, 0xd0
    };
    static const int cursor_y[TETRIS_HIGH_SCORE_COUNT] = {
        0x9f, 0xaf, 0xbf
    };
    int rank;
    int y;
    if (!result ||
        !tetris_rom_screen_available(TETRIS_ROM_SCREEN_ENTER_HIGH_SCORE)) return;
    rank = result->rank;
    if (rank < 0) rank = 0;
    if (rank >= TETRIS_HIGH_SCORE_COUNT) rank = TETRIS_HIGH_SCORE_COUNT - 1;
    y = RECORD_FIRST_Y + rank * RECORD_ROW_STEP;
    clear_record_row(renderer, y);

    if (nes_cursor_visible()) {
        if (tetris_rom_sprite_available(TETRIS_ROM_SPRITE_HIGH_SCORE_CURSOR)) {
            tetris_rom_sprite_render(renderer,
                TETRIS_ROM_SPRITE_HIGH_SCORE_CURSOR,
                cursor_x[result->name.cursor], cursor_y[rank]);
        } else {
            SDL_Rect cursor = {RECORD_NAME_X + result->name.cursor * 16,
                               y, 16, 16};
            SDL_SetRenderDrawColor(renderer, 212, 136, 32, 255);
            SDL_RenderFillRect(renderer, &cursor);
        }
    }
    draw_record_values(renderer, font, y, result->name.text,
                       result->score, result->lines);
}

void tetris_present_with_score_overlay(
    SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
    const TetrisGame *game, const AppResultState *result,
    const TetrisHighScores *scores) {
    switch (screen) {
        case SCREEN_TITLE:
            overlay_exact_title_score(renderer, font, scores);
            break;
        case SCREEN_RECORDS:
            overlay_exact_records(renderer, font, scores);
            break;
        case SCREEN_NAME_ENTRY:
            overlay_exact_name_entry(renderer, font, result);
            break;
        case SCREEN_GAME:
        case SCREEN_DEMO:
        case SCREEN_REPLAY:
            overlay_exact_game_scores(renderer, font, game, scores);
            break;
        default:
            break;
    }
    SDL_RenderPresent(renderer);
}
