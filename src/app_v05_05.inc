            draw_demo_overlay(renderer, font);
            break;
        case SCREEN_REPLAY:
            render_game(renderer, font, game, audio, rom, scores);
            draw_replay_overlay(renderer, font, replay_status);
            break;
    }
}

bool load_rom_and_font_ex(SDL_Renderer *renderer, const char *path,
                          NesRom *rom, SDL_Texture **font, bool show_error,
                          char *error, size_t error_size) {
    char local_error[256];
    NesRom loaded;
    if (!error || error_size == 0) {
        error = local_error;
        error_size = sizeof(local_error);
    }
    error[0] = '\0';
    if (!nes_rom_load(path, &loaded, error, error_size)) {
        if (show_error) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                                 "Tetris NES PC Port", error, NULL);
        return false;
    }
    {
        SDL_Texture *new_font = create_chr_texture(renderer, &loaded);
        SDL_Texture *new_title = create_original_title_texture(renderer, &loaded);
        SDL_Texture *new_blocks[10][3];
        if (!new_font) {
            if (new_title) SDL_DestroyTexture(new_title);
            nes_rom_free(&loaded);
            snprintf(error, error_size, "Could not create the CHR texture.");
            if (show_error) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                                     "Tetris NES PC Port", error, NULL);
            return false;
        }
        if (!create_block_textures(renderer, &loaded, new_blocks)) {
            SDL_DestroyTexture(new_font);
            if (new_title) SDL_DestroyTexture(new_title);
            nes_rom_free(&loaded);
            snprintf(error, error_size,
                     "Could not create block tiles from the ROM CHR banks.");
            if (show_error) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                                     "Tetris NES PC Port", error, NULL);
            return false;
        }
        if (*font) SDL_DestroyTexture(*font);
        if (g_title_texture) SDL_DestroyTexture(g_title_texture);
        g_title_texture = new_title;
        destroy_block_textures();
        memcpy(g_block_textures, new_blocks, sizeof(g_block_textures));
        nes_rom_free(rom);
        *rom = loaded;
        *font = new_font;
    }
    fprintf(stdout, "Loaded ROM: %s\nCRC32: %08X%s\n", path, rom->crc32,
            rom->exact_supported_dump
                ? " (tested dump)" : " (compatible, unverified dump)");
    return true;
}

bool load_rom_and_font(SDL_Renderer *renderer, const char *path,
                       NesRom *rom, SDL_Texture **font) {
    return load_rom_and_font_ex(renderer, path, rom, font, true, NULL, 0);
}

void free_rom_graphics(void) {
    destroy_block_textures();
    if (g_title_texture) {
        SDL_DestroyTexture(g_title_texture);
        g_title_texture = NULL;
    }
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

void app_result_begin(AppResultState *result, const TetrisGame *game, int rank) {
    if (!result || !game) return;
    tetris_name_entry_init(&result->name, "PLAYER");
    result->mode = game->mode;
    result->score = game->score;
    result->level = game->start_level;
    result->height = game->start_height;
    result->rank = rank;
    result->completed = game->phase == TETRIS_PHASE_COMPLETE;
}
