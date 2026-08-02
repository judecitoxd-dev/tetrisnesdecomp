        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    make_pref_file_path("scores.txt", score_path, sizeof(score_path));
    make_pref_file_path("settings.ini", settings_path, sizeof(settings_path));
    make_pref_file_path("last_replay.ttr", last_replay_path,
                        sizeof(last_replay_path));
    tetris_settings_init(&settings);
    (void)tetris_settings_load(&settings, settings_path);

    window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (settings.fullscreen) window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_Window *window = SDL_CreateWindow("Tetris NES PC Port v0.20",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        settings.window_width, settings.window_height, window_flags);
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
    SDL_RenderSetIntegerScale(renderer,
        settings.integer_scale ? SDL_TRUE : SDL_FALSE);

    NesRom rom = {0};
    SDL_Texture *font = NULL;
    if (!load_initial_rom(renderer, cli_rom_path, &rom, &font,
                          &settings, settings_path)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 2;
    }

    TetrisAudio audio;
    if (!tetris_audio_init(&audio)) {
        fprintf(stderr, "Audio disabled: %s\n", SDL_GetError());
        memset(&audio, 0, sizeof(audio));
        audio.music_track = -1;
        audio.rom_selected_track = -2;
        settings.audio_enabled = false;
    }
    attach_original_audio(&audio, &rom);
    load_optional_audio_pack(&audio, cli_audio_pack_path);
    tetris_audio_apply_settings(&audio, settings.audio_enabled,
                                settings.music_track);
    fprintf(stdout, "Audio backend: %s\n",
            tetris_audio_backend_label(&audio));

    TetrisHighScores scores;
    tetris_high_scores_init(&scores);
    (void)tetris_high_scores_load(&scores, score_path);

    SDL_GameController *controller = open_first_controller();
    TetrisTouchControls touch;
    TetrisGame game;
    TetrisDemoController demo;
    TetrisReplay recording;
    TetrisReplay playback;
    AppMenuState menu = {settings.last_mode, settings.last_level,
                         settings.last_height, false};
    AppOptionsState options = {0};
    AppResultState result;
    AppScreen screen = SCREEN_TITLE;
    PendingInput pending = {0};
    bool running = true;
    bool left = false;
    bool right = false;
    bool down = false;
    bool result_handled = false;
    bool recording_active = false;
    Uint32 last_activity = SDL_GetTicks();
    Uint32 demo_started = 0;
    uint32_t replay_index = 0;
    Uint32 replay_done_at = 0;
    char replay_status[80] = "";
    uint64_t previous = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    const double step = 1.0 / 60.0988;

    memset(&result, 0, sizeof(result));
    tetris_demo_reset(&demo);
    tetris_replay_init(&recording);
    tetris_replay_init(&playback);
    tetris_init(&game, (uint32_t)time(NULL), 0);
#ifdef __ANDROID__
    tetris_touch_init(&touch, &settings, true);
#else
    tetris_touch_init(&touch, &settings, SDL_GetNumTouchDevices() > 0);
#endif

    if (cli_replay_path) {
        (void)start_last_replay(&playback, &game, &audio, cli_replay_path,
                                &screen, &replay_index, &replay_done_at,
                                replay_status, sizeof(replay_status),
                                &left, &right, &down);
    }

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            TetrisTouchEvent touch_event;
            if (!controller && tetris_touch_handle_event(&touch, &event,
                    window, renderer, &touch_event)) {
                last_activity = SDL_GetTicks();
                if (touch.dirty) {
                    tetris_touch_store_settings(&touch, &settings);
                    save_settings_quiet(&settings, settings_path);
                    touch.dirty = false;
                }
                if (touch_event.action != TETRIS_TOUCH_NONE)
                    push_touch_key(touch_event.action, touch_event.pressed,
                                   screen, &game);
                continue;
            }
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED &&
                !settings.fullscreen) {
                settings.window_width = event.window.data1;
                settings.window_height = event.window.data2;
            }
            if (event.type == SDL_KEYDOWN ||
                event.type == SDL_CONTROLLERBUTTONDOWN ||
                event.type == SDL_DROPFILE) {
                last_activity = SDL_GetTicks();
            }
            if (event.type == SDL_CONTROLLERDEVICEADDED && !controller) {
                controller = SDL_GameControllerOpen(event.cdevice.which);
                if (controller) tetris_touch_release_all(&touch);
            }
            if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller) {
                SDL_Joystick *joy = SDL_GameControllerGetJoystick(controller);
                if (SDL_JoystickInstanceID(joy) == event.cdevice.which) {
                    SDL_GameControllerClose(controller);
                    controller = open_first_controller();
                    clear_held(&left, &right, &down);
                }
            }
            if (event.type == SDL_DROPFILE) {
                tetris_audio_detach_rom(&audio);
                if (load_candidate(renderer, event.drop.file, &rom, &font,
                                   &settings, settings_path, true)) {
                    if (screen == SCREEN_GAME)
                        finish_replay_recording(&recording, &game,
                                                last_replay_path,
                                                &recording_active);
                    screen = SCREEN_TITLE;
                    clear_held(&left, &right, &down);
                }
                attach_original_audio(&audio, &rom);
                SDL_free(event.drop.file);
            }

            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                const SDL_Keycode key = event.key.keysym.sym;
                char name_character = 0;
                if (screen == SCREEN_REPLAY) {
                    screen = SCREEN_TITLE;
                    replay_done_at = 0;
                    last_activity = SDL_GetTicks();
                    clear_held(&left, &right, &down);
                } else if (key == SDLK_AC_BACK) {
                    if (screen == SCREEN_TITLE) running = false;
                    else if (screen == SCREEN_GAME) {
                        finish_replay_recording(&recording, &game,
                                                last_replay_path,
                                                &recording_active);
                        screen = SCREEN_TITLE;
                        clear_held(&left, &right, &down);
                    } else {
                        SDL_Event back_event;
                        memset(&back_event, 0, sizeof(back_event));
                        back_event.type = SDL_KEYDOWN;
                        back_event.key.type = SDL_KEYDOWN;
                        back_event.key.state = SDL_PRESSED;
                        back_event.key.keysym.sym = SDLK_BACKSPACE;
                        back_event.key.keysym.scancode = SDL_SCANCODE_BACKSPACE;
                        (void)SDL_PushEvent(&back_event);
                    }
                } else if (key == SDLK_ESCAPE) {
                    running = false;
                } else if (screen == SCREEN_DEMO) {
                    screen = SCREEN_TITLE;
                    clear_held(&left, &right, &down);
                } else if (key == SDLK_F11) {
                    settings.fullscreen = !settings.fullscreen;
                    SDL_SetWindowFullscreen(window,
                        settings.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);