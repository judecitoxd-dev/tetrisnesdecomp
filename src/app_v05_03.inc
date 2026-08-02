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
    if (tetris_rom_screen_available(TETRIS_ROM_SCREEN_ENTER_HIGH_SCORE)) {
        static const int row_y[3] = {0x9f, 0xaf, 0xbf};
        static const int name_x[6] = {0x48, 0x50, 0x58, 0x60, 0x68, 0x70};
        const int rank = result->rank < 0 ? 0 : (result->rank > 2 ? 2 : result->rank);
        const int screen_y = 320 + rank * 32;
        tetris_rom_screen_render(renderer, TETRIS_ROM_SCREEN_ENTER_HIGH_SCORE);
        /* This OAM tile has background priority, so draw the letter after it. */
        if (nes_cursor_visible()) {
            if (tetris_rom_sprite_available(TETRIS_ROM_SPRITE_HIGH_SCORE_CURSOR)) {
                tetris_rom_sprite_render(renderer,
                    TETRIS_ROM_SPRITE_HIGH_SCORE_CURSOR,
                    name_x[result->name.cursor], row_y[rank]);
            } else {
                SDL_Rect cursor = {208 + result->name.cursor * 16,
                                   screen_y, 16, 16};
                SDL_SetRenderDrawColor(renderer, 212, 136, 32, 255);
                SDL_RenderFillRect(renderer, &cursor);
            }
        }
        draw_text(renderer, font, 296, 48, 2,
                  result->mode == TETRIS_MODE_A ? "A" : "B");
        draw_text(renderer, font, 208, screen_y, 2, result->name.text);
        snprintf(value, sizeof(value), "%06d",
                 result->score > 999999 ? 999999 : result->score);
        draw_text(renderer, font, 320, screen_y, 2, value);
        snprintf(value, sizeof(value), "%02d",
                 result->level > 99 ? 99 : result->level);
        draw_text(renderer, font, 448, screen_y, 2, value);
        draw_centered(renderer, font, 438, 1,
                      "ARROWS EDIT  ENTER SAVES");
        return;
    }
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

static bool render_original_a_ending(SDL_Renderer *renderer,
                                     SDL_Texture *font,
                                     const AppResultState *result) {
    static const unsigned body_sprite[5] = {
        0x3eu, 0x41u, 0x44u, 0x47u, 0x4au
    };
    static const unsigned jet_sprite[5][2] = {
        {0x3fu, 0x40u}, {0x42u, 0x43u}, {0x45u, 0x46u},
        {0x48u, 0x49u}, {0x23u, 0x24u}
    };
    static const int rocket_x[5] = {0x54, 0x54, 0x50, 0x48, 0xa0};
    static const int initial_y[5] = {0xbf, 0xbf, 0xbf, 0xbf, 0xc7};
    char value[32];
    unsigned elapsed_frames;
    unsigned delay = 0;
    int rocket_class;
    int rocket_y;
    bool over_120k;

    if (!result || result->mode != TETRIS_MODE_A || result->score < 30000)
        return false;
    if (result->score < 50000) rocket_class = 0;
    else if (result->score < 70000) rocket_class = 1;
    else if (result->score < 100000) rocket_class = 2;
    else if (result->score < 120000) rocket_class = 3;
    else rocket_class = 4;
    over_120k = rocket_class == 4;
    if (!tetris_rom_type_a_background_available(over_120k) ||
        !tetris_rom_type_a_sprite_available(body_sprite[rocket_class]))
        return false;

    tetris_rom_type_a_background_render(renderer, over_120k);
    snprintf(value, sizeof(value), "%06d",
             result->score > 999999 ? 999999 : result->score);
    draw_text(renderer, font, 288, 64, 2, value);

    elapsed_frames = (unsigned)(((Uint32)(SDL_GetTicks() -
        result->ending_started_ms) * 60u) / 1000u);
    rocket_y = initial_y[rocket_class];
    for (unsigned frame = 0; frame < elapsed_frames && rocket_y > 0; ++frame) {
        if (delay != 0xf0u) {
            ++delay;
        } else if (!(rocket_y >= 0xb0 && (frame & 1u) != 0u)) {
            --rocket_y;
            if (rocket_y > 0 && rocket_y < 0x80) --rocket_y;
        }
    }
    if (rocket_y > 0) {
        const unsigned jet = jet_sprite[rocket_class][(elapsed_frames >> 1) & 1u];
        tetris_rom_type_a_sprite_render(renderer, body_sprite[rocket_class],
                                        rocket_x[rocket_class], rocket_y);
        tetris_rom_type_a_sprite_render(renderer, jet,
                                        rocket_x[rocket_class], rocket_y);
    }
    draw_centered(renderer, font, 450, 1,
                  result->rank >= 0 ? "ENTER NAME  H RECORDS" :
                                      "ENTER RETURNS  H RECORDS");
    return true;
}

static bool render_original_b_ending(SDL_Renderer *renderer,
                                     SDL_Texture *font,
                                     const AppResultState *result) {
    TetrisRomScreen screen;
    char value[32];
    int level_digit;
    int ending_height;
    int level_bonus;
    int height_bonus;
    int base_score;
    if (!result || result->mode != TETRIS_MODE_B) return false;
    level_digit = result->level % 10;
    if (level_digit < 0) level_digit += 10;
    ending_height = result->height;
    if (ending_height < 0) ending_height = 0;
    if (ending_height > 5) ending_height = 5;
    screen = level_digit == 9
        ? (TetrisRomScreen)(TETRIS_ROM_SCREEN_B_ENDING_CASTLE_H0 + ending_height)
        : TETRIS_ROM_SCREEN_B_ENDING_NORMAL;
    if (!tetris_rom_screen_available(screen)) return false;

    level_bonus = level_digit * 1000;
    height_bonus = ending_height * 1000;
    base_score = result->score - level_bonus - height_bonus;
    if (base_score < 0) base_score = 0;

    tetris_rom_screen_render(renderer, screen);
    snprintf(value, sizeof(value), "%06d", base_score > 999999 ? 999999 : base_score);
    draw_text(renderer, font, 288, 64, 2, value);
    snprintf(value, sizeof(value), "%05d", level_bonus);
    draw_text(renderer, font, 320, 80, 2, value);
    snprintf(value, sizeof(value), "%05d", height_bonus);
    draw_text(renderer, font, 320, 96, 2, value);
    snprintf(value, sizeof(value), "%06d",
             result->score > 999999 ? 999999 : result->score);
    draw_text(renderer, font, 288, 144, 2, value);
    {
        const unsigned frame = (unsigned)(((Uint32)(SDL_GetTicks() -
            result->ending_started_ms) * 60u) / 1000u);
        if (level_digit == 9)
            tetris_rom_concert_render(renderer, ending_height, frame);
        else
            tetris_rom_cathedral_render(renderer, result->level,
                                        ending_height, frame);
    }
    draw_centered(renderer, font, 450, 1, "ENTER RETURNS  H RECORDS");
    return true;
}

static void render_ending(SDL_Renderer *renderer, SDL_Texture *font,
                          const AppResultState *result) {
    const unsigned tick = SDL_GetTicks();
    const int travel = (int)((tick / 8u) % 520u);
    const int rocket_y = LOGICAL_H + 40 - travel;
    char value[64];
    if (render_original_a_ending(renderer, font, result)) return;
    if (render_original_b_ending(renderer, font, result)) return;

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