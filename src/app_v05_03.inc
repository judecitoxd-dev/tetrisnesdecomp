                           const AppOptionsState *options,
                           const TetrisSettings *settings) {
    char music[32];
    if (settings->music_track < 0) snprintf(music, sizeof(music), "OFF");
    else snprintf(music, sizeof(music), "TRACK %d", settings->music_track + 1);
    draw_centered(renderer, font, 38, 3, "OPTIONS");
    render_option_row(renderer, font, 105, options->selected == 0,
                      "AUDIO", settings->audio_enabled ? "ON" : "OFF");
    render_option_row(renderer, font, 145, options->selected == 1,
                      "MUSIC", music);
    render_option_row(renderer, font, 185, options->selected == 2,
                      "FULLSCREEN", settings->fullscreen ? "ON" : "OFF");
    render_option_row(renderer, font, 225, options->selected == 3,
                      "INTEGER SCALE", settings->integer_scale ? "ON" : "OFF");
    render_option_row(renderer, font, 265, options->selected == 4,
                      "TITLE DEMO", settings->demo_enabled ? "ON" : "OFF");
    render_option_row(renderer, font, 305, options->selected == 5,
                      "HARD DROP", settings->hard_drop_enabled ? "ON" : "OFF");
    render_option_row(renderer, font, 345, options->selected == 6,
                      "NEXT PIECE", settings->show_next ? "ON" : "OFF");
    draw_centered(renderer, font, 405, 1, "UP DOWN SELECT  LEFT RIGHT CHANGE");
    draw_centered(renderer, font, 430, 1, "ENTER CHANGES  BACKSPACE SAVES");
}


static void render_name_entry(SDL_Renderer *renderer, SDL_Texture *font,
                              const AppResultState *result) {
    char value[64];
    int start_x;
    draw_centered(renderer, font, 52, 3, "NEW RECORD");
    snprintf(value, sizeof(value), "%s RANK %d",
             result->mode == TETRIS_MODE_A ? "A TYPE" : "B TYPE",
             result->rank + 1);
    draw_centered(renderer, font, 112, 1, value);
    snprintf(value, sizeof(value), "SCORE %06d", result->score);
    draw_centered(renderer, font, 145, 2, value);
    draw_centered(renderer, font, 214, 1, "ENTER YOUR NAME");
    start_x = (LOGICAL_W - TETRIS_NAME_ENTRY_LENGTH * 32) / 2;
    for (int index = 0; index < TETRIS_NAME_ENTRY_LENGTH; ++index) {
        char character[2] = {result->name.text[index], '\0'};
        draw_text(renderer, font, start_x + index * 32, 258, 3, character);
        if (index == result->name.cursor && ((SDL_GetTicks() / 250u) & 1u) == 0u) {
            SDL_Rect cursor = {start_x + index * 32, 286, 24, 3};
            SDL_SetRenderDrawColor(renderer, 236, 238, 236, 255);
            SDL_RenderFillRect(renderer, &cursor);
        }
    }
    draw_centered(renderer, font, 330, 1, "TYPE OR USE ARROWS");
    draw_centered(renderer, font, 355, 1, "ENTER SAVES RECORD");
    draw_centered(renderer, font, 395, 1, "LEFT RIGHT MOVE  UP DOWN CHANGE");
}

static void draw_star_field(SDL_Renderer *renderer, unsigned tick) {
    for (int index = 0; index < 36; ++index) {
        const int x = (index * 97 + (int)(tick / 9u)) % LOGICAL_W;
        const int y = (index * 53 + (int)(tick / 5u)) % LOGICAL_H;
        const int brightness = 120 + ((index * 37) & 127);
        SDL_SetRenderDrawColor(renderer, (Uint8)brightness,
                               (Uint8)brightness, (Uint8)brightness, 255);
        SDL_RenderDrawPoint(renderer, x, y);
    }
}

static void render_ending(SDL_Renderer *renderer, SDL_Texture *font,
                          const AppResultState *result) {
    const unsigned tick = SDL_GetTicks();
    const int travel = (int)((tick / 8u) % 520u);
    const int rocket_y = LOGICAL_H + 40 - travel;
    char value[64];
    draw_star_field(renderer, tick);

    SDL_SetRenderDrawColor(renderer, 235, 235, 245, 255);
    {
        SDL_Rect body = {LOGICAL_W / 2 - 10, rocket_y, 20, 56};
        SDL_RenderFillRect(renderer, &body);
    }
    SDL_SetRenderDrawColor(renderer, 220, 50, 55, 255);
    {
        SDL_Point nose[4] = {
            {LOGICAL_W / 2 - 10, rocket_y},
            {LOGICAL_W / 2, rocket_y - 22},
            {LOGICAL_W / 2 + 10, rocket_y},
            {LOGICAL_W / 2 - 10, rocket_y}
        };
        SDL_RenderDrawLines(renderer, nose, 4);
        SDL_Rect left_fin = {LOGICAL_W / 2 - 20, rocket_y + 34, 10, 20};
        SDL_Rect right_fin = {LOGICAL_W / 2 + 10, rocket_y + 34, 10, 20};
        SDL_RenderFillRect(renderer, &left_fin);
        SDL_RenderFillRect(renderer, &right_fin);
    }
    SDL_SetRenderDrawColor(renderer, 255, 180, 40, 255);
    {
        SDL_Rect flame = {LOGICAL_W / 2 - 5, rocket_y + 56,
                          10, 12 + (int)((tick / 80u) & 7u)};
        SDL_RenderFillRect(renderer, &flame);
    }

    draw_centered(renderer, font, 38, 3, "B TYPE COMPLETE");
    snprintf(value, sizeof(value), "SCORE %06d", result->score);
    draw_centered(renderer, font, 90, 2, value);
    snprintf(value, sizeof(value), "LEVEL %02d  HEIGHT %d",
             result->level, result->height);
    draw_centered(renderer, font, 126, 1, value);
    draw_centered(renderer, font, 410, 1, "ENTER RETURNS TO TITLE");
    draw_centered(renderer, font, 435, 1, "H SHOWS RECORDS");
}

static void draw_demo_overlay(SDL_Renderer *renderer, SDL_Texture *font) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 155);
    {
        SDL_Rect banner = {0, 0, LOGICAL_W, 34};
        SDL_RenderFillRect(renderer, &banner);
    }
    draw_centered(renderer, font, 10, 1, "DEMO  PRESS ANY KEY");
}


static void draw_replay_overlay(SDL_Renderer *renderer, SDL_Texture *font,
                                const char *status) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 165);
    {
        SDL_Rect banner = {0, 0, LOGICAL_W, 54};
        SDL_RenderFillRect(renderer, &banner);
    }
    draw_centered(renderer, font, 8, 1, "LAST REPLAY  PRESS ANY KEY");
    if (status && *status) draw_centered(renderer, font, 30, 1, status);
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
