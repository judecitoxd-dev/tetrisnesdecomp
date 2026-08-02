        draw_text(renderer, font, 500, 240 + i * 18, 1, value);
    }
}

static void render_game_exact(SDL_Renderer *renderer, SDL_Texture *font,
                              const TetrisGame *game, const NesRom *rom,
                              const TetrisHighScores *scores) {
    enum { EXACT_BOARD_X = 256, EXACT_BOARD_Y = 96, EXACT_CELL = 16 };
    const TetrisHighScoreEntry *top = tetris_high_scores_top(scores, game->mode);
    char value[32];

    tetris_rom_screen_render(renderer, TETRIS_ROM_SCREEN_GAME);

    for (int y = 0; y < TETRIS_BOARD_H; ++y) {
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (game->board[y][x] && !tetris_cell_hidden(game, x, y)) {
                draw_block(renderer, rom, game->level,
                           EXACT_BOARD_X + x * EXACT_CELL,
                           EXACT_BOARD_Y + y * EXACT_CELL,
                           EXACT_CELL, game->board[y][x] - 1);
            }
        }
    }
    if (game->phase == TETRIS_PHASE_ACTIVE) {
        draw_piece(renderer, rom, game->level, game->active, game->rotation,
                   game->x, game->y, EXACT_BOARD_X, EXACT_BOARD_Y, EXACT_CELL);
    }

    if (game->phase == TETRIS_PHASE_GAME_OVER_CURTAIN ||
        game->phase == TETRIS_PHASE_GAME_OVER) {
        const int rows = game->phase == TETRIS_PHASE_GAME_OVER
            ? TETRIS_BOARD_H : game->curtain_rows;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < TETRIS_BOARD_W; ++x) {
                SDL_Rect tile = {EXACT_BOARD_X + x * EXACT_CELL,
                                 EXACT_BOARD_Y + y * EXACT_CELL,
                                 EXACT_CELL, EXACT_CELL};
                SDL_SetRenderDrawColor(renderer, 92, 92, 104, 255);
                SDL_RenderFillRect(renderer, &tile);
            }
        }
    }

    /* Dynamic values are placed over the original zero-filled PPU fields. */
    draw_text(renderer, font, 112, 68, 2,
              game->mode == TETRIS_MODE_A ? "A" : "B");
    snprintf(value, sizeof(value), "%03d", game->lines > 999 ? 999 : game->lines);
    draw_text(renderer, font, 344, 52, 2, value);
    snprintf(value, sizeof(value), "%06d", top->score > 999999 ? 999999 : top->score);
    draw_text(renderer, font, 448, 88, 1, value);
    snprintf(value, sizeof(value), "%06d", game->score > 999999 ? 999999 : game->score);
    draw_text(renderer, font, 448, 142, 1, value);
    snprintf(value, sizeof(value), "%02d", game->level > 99 ? 99 : game->level);
    draw_text(renderer, font, 486, 356, 2, value);

    if (game->show_next) {
        draw_piece(renderer, rom, game->level, game->next,
                   tetris_spawn_rotation(game->next),
                   2, 2, 456, 218, 14);
    }

    for (int index = 0; index < 7; ++index) {
        snprintf(value, sizeof(value), "%03d",
                 game->piece_count[index] > 999 ? 999 : game->piece_count[index]);
        draw_text(renderer, font, 174, 188 + index * 36, 1, value);
    }

    if (game->paused || game->phase == TETRIS_PHASE_GAME_OVER ||
        game->phase == TETRIS_PHASE_COMPLETE) {
        SDL_Rect overlay = {EXACT_BOARD_X, EXACT_BOARD_Y + 112,
                            TETRIS_BOARD_W * EXACT_CELL, 96};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 215);
        SDL_RenderFillRect(renderer, &overlay);
        if (game->paused)
            draw_text(renderer, font, EXACT_BOARD_X + 32,
                      EXACT_BOARD_Y + 148, 2, "PAUSED");
        else if (game->phase == TETRIS_PHASE_GAME_OVER)
            draw_text(renderer, font, EXACT_BOARD_X + 8,
                      EXACT_BOARD_Y + 148, 2, "GAME OVER");
        else
            draw_text(renderer, font, EXACT_BOARD_X + 24,
                      EXACT_BOARD_Y + 148, 2, "SUCCESS");
    }
}

static void render_game(SDL_Renderer *renderer, SDL_Texture *font,
                        const TetrisGame *game, const TetrisAudio *audio,
                        const NesRom *rom, const TetrisHighScores *scores) {
    if (tetris_rom_screen_available(TETRIS_ROM_SCREEN_GAME)) {
        render_game_exact(renderer, font, game, rom, scores);
        return;
    }
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
            const AppResultState *result, const AppOptionsState *options,
            const TetrisSettings *settings, const char *replay_status,
            bool non_exact_rom, const TetrisAudio *audio, const NesRom *rom,
            const TetrisHighScores *scores) {
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
        case SCREEN_OPTIONS:
            render_options(renderer, font, options, settings);
            break;
        case SCREEN_NAME_ENTRY:
            render_name_entry(renderer, font, result);
            break;
        case SCREEN_ENDING:
            render_ending(renderer, font, result);
            break;
        case SCREEN_GAME:
            render_game(renderer, font, game, audio, rom, scores);
            break;
        case SCREEN_DEMO:
            render_game(renderer, font, game, audio, rom, scores);
