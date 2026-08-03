    begin_game(game, menu);
    game->show_next = settings->show_next;
    settings->music_track = tetris_songs_apply(
        audio, settings->audio_enabled, settings->music_track);
    settings->last_mode = menu->mode;
    settings->last_level = menu->level;
    settings->last_height = menu->height;
    save_settings_quiet(settings, settings_path);
    begin_replay_recording(recording, game, recording_active);
    *screen = SCREEN_GAME;
    *result_handled = false;
    clear_held(left, right, down);
}

static void start_demo(TetrisGame *game, TetrisDemoController *demo,
                       TetrisAudio *audio, const NesRom *rom,
                       AppScreen *screen, Uint32 *demo_started,
                       bool *left, bool *right, bool *down) {
    tetris_init_mode(game, 0x19891101u, 0, TETRIS_MODE_A, 0);
    if (!tetris_demo_reset_from_rom(demo, game, rom))
        tetris_demo_reset(demo);
    audio->music_track = tetris_songs_apply(
        audio, audio->enabled, audio->music_track);
    *screen = SCREEN_DEMO;
    *demo_started = SDL_GetTicks();
    clear_held(left, right, down);
}

static bool start_last_replay(TetrisReplay *playback, TetrisGame *game,
                              TetrisAudio *audio, const char *path,
                              AppScreen *screen, uint32_t *replay_index,
                              Uint32 *replay_done_at,
                              char *replay_status, size_t replay_status_size,
                              bool *left, bool *right, bool *down) {
    char error[256];
    if (!tetris_replay_load(playback, path, error, sizeof(error))) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                                 "Tetris NES PC Port", error, NULL);
        return false;
    }
    tetris_init_mode(game, playback->seed, playback->start_level,
                     playback->mode, playback->start_height);
    game->show_next = playback->initial_show_next;
    audio->music_track = tetris_songs_apply(
        audio, audio->enabled, audio->music_track);
    *replay_index = 0;
    *replay_done_at = 0;
    snprintf(replay_status, replay_status_size, "PLAYING %" PRIu32 " FRAMES",
             playback->frame_count);
    *screen = SCREEN_REPLAY;
    clear_held(left, right, down);
    return true;
}

static void begin_result_flow(TetrisHighScores *scores, const TetrisGame *game,
                              AppResultState *result, AppScreen *screen,
                              bool *result_handled, bool *left,
                              bool *right, bool *down) {
    int rank;
    if (*result_handled) return;
    if (game->phase != TETRIS_PHASE_GAME_OVER &&
        game->phase != TETRIS_PHASE_COMPLETE) return;
    *result_handled = true;
    rank = tetris_high_scores_rank(scores, game->mode, game->score);
    app_result_begin(result, game, rank);
    clear_held(left, right, down);
    if (game->mode == TETRIS_MODE_A &&
        game->phase == TETRIS_PHASE_GAME_OVER && game->score >= 30000) {
        *screen = SCREEN_ENDING;
    } else if (rank >= 0) {
        *screen = SCREEN_NAME_ENTRY;
    } else if (game->phase == TETRIS_PHASE_COMPLETE) {
        *screen = SCREEN_ENDING;
    }
}

static void save_name_entry(TetrisHighScores *scores,
                            const AppResultState *result,
                            const char *score_path, AppScreen *screen) {
    if (tetris_high_scores_submit(scores, result->mode, result->name.text,
                                  result->score, result->lines,
                                  result->level, result->height)) {
        if (!tetris_high_scores_save(scores, score_path)) {
            fprintf(stderr, "Could not save local scores to %s\n", score_path);
        }
    }
    *screen = result->completed ? SCREEN_ENDING : SCREEN_RECORDS;
}

static bool key_to_name_character(SDL_Keycode key, char *character) {
    if (key >= SDLK_a && key <= SDLK_z) {
        *character = (char)('A' + (key - SDLK_a));
        return true;
    }
    if (key >= SDLK_0 && key <= SDLK_9) {
        *character = (char)('0' + (key - SDLK_0));
        return true;
    }
    if (key == SDLK_MINUS) {
        *character = '-';
        return true;
    }
    return false;
}

static void apply_options(TetrisSettings *settings, AppOptionsState *options,
                          int direction, SDL_Window *window,
                          SDL_Renderer *renderer, TetrisAudio *audio,
                          const char *settings_path) {
    if (direction == 0) direction = 1;
    switch (options->selected) {
        case 0:
            settings->audio_enabled = !settings->audio_enabled;
            break;
        case 1:
            settings->music_track = tetris_songs_step(
                settings->music_track, direction);
            break;
        case 2:
            settings->fullscreen = !settings->fullscreen;
            SDL_SetWindowFullscreen(window,
                settings->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
            break;
        case 3:
            settings->integer_scale = !settings->integer_scale;
            SDL_RenderSetIntegerScale(renderer,
                settings->integer_scale ? SDL_TRUE : SDL_FALSE);
            break;
        case 4:
            settings->demo_enabled = !settings->demo_enabled;
            break;
        case 5:
            settings->hard_drop_enabled = !settings->hard_drop_enabled;
            break;
        case 6:
            settings->show_next = !settings->show_next;
            break;
        default:
            break;
    }
    settings->music_track = tetris_songs_apply(
        audio, settings->audio_enabled, settings->music_track);
    save_settings_quiet(settings, settings_path);
}

static void parse_arguments(int argc, char **argv,
                            const char **rom_path,
                            const char **replay_path,
                            const char **audio_pack_path) {
    int index;
    *rom_path = NULL;
    *replay_path = NULL;
    *audio_pack_path = NULL;
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--rom") == 0 && index + 1 < argc) {
            *rom_path = argv[++index];
        } else if (strcmp(argv[index], "--replay") == 0 && index + 1 < argc) {
            *replay_path = argv[++index];
        } else if (strcmp(argv[index], "--audio-pack") == 0 && index + 1 < argc) {
            *audio_pack_path = argv[++index];
        } else if (argv[index][0] != '-' && !*rom_path) {
            *rom_path = argv[index];
        }
    }
}

static SDL_Keycode touch_key_for_action(TetrisTouchAction action,
                                              AppScreen screen,
                                              const TetrisGame *game) {
    switch (action) {
        case TETRIS_TOUCH_UP: return SDLK_UP;
        case TETRIS_TOUCH_DOWN: return SDLK_DOWN;
        case TETRIS_TOUCH_LEFT: return SDLK_LEFT;
        case TETRIS_TOUCH_RIGHT: return SDLK_RIGHT;
        case TETRIS_TOUCH_A:
            if (screen == SCREEN_GAME) {
                if (game && (game->phase == TETRIS_PHASE_GAME_OVER ||
                             game->phase == TETRIS_PHASE_COMPLETE)) return SDLK_r;
                return SDLK_x;
            }
            return SDLK_RETURN;
        case TETRIS_TOUCH_B:
            return screen == SCREEN_GAME ? SDLK_z : SDLK_BACKSPACE;
        case TETRIS_TOUCH_DROP:
            return screen == SCREEN_GAME ? SDLK_SPACE : SDLK_RETURN;
        case TETRIS_TOUCH_START:
            return screen == SCREEN_GAME ? SDLK_p : SDLK_RETURN;
        case TETRIS_TOUCH_SELECT:
            if (screen == SCREEN_GAME) return SDLK_TAB;
            if (screen == SCREEN_TITLE) return SDLK_o;
            return SDLK_BACKSPACE;
        case TETRIS_TOUCH_ROM: return SDLK_l;
        default: return SDLK_UNKNOWN;
    }
}

static void push_touch_key(TetrisTouchAction action, bool pressed,
                           AppScreen screen, const TetrisGame *game) {
    SDL_Event key_event;
    const SDL_Keycode key = touch_key_for_action(action, screen, game);
    if (key == SDLK_UNKNOWN) return;
    memset(&key_event, 0, sizeof(key_event));
    key_event.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
    key_event.key.type = key_event.type;
    key_event.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    key_event.key.repeat = 0;
    key_event.key.keysym.sym = key;
    key_event.key.keysym.scancode = SDL_GetScancodeFromKey(key);
    (void)SDL_PushEvent(&key_event);
}

int main(int argc, char **argv) {
    const char *cli_rom_path;
    const char *cli_replay_path;
    const char *cli_audio_pack_path;
    char score_path[1024];
    char settings_path[1024];
    char last_replay_path[1024];
    TetrisSettings settings;
    Uint32 window_flags;

    parse_arguments(argc, argv, &cli_rom_path, &cli_replay_path,
                    &cli_audio_pack_path);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                 SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
