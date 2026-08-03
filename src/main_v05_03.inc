                    save_settings_quiet(&settings, settings_path);
                } else if (key == SDLK_m) {
                    settings.audio_enabled = !settings.audio_enabled;
                    settings.music_track = tetris_songs_apply(
                        &audio, settings.audio_enabled, settings.music_track);
                    save_settings_quiet(&settings, settings_path);
                } else if (key == SDLK_n) {
                    settings.music_track = tetris_songs_step(
                        settings.music_track, 1);
                    settings.music_track = tetris_songs_apply(
                        &audio, settings.audio_enabled, settings.music_track);
                    save_settings_quiet(&settings, settings_path);
                } else if (screen == SCREEN_TITLE) {
                    if (key == SDLK_RETURN || key == SDLK_SPACE) {
                        screen = SCREEN_TYPE_SELECT;
                    } else if (key == SDLK_h) {
                        screen = SCREEN_RECORDS;
                    } else if (key == SDLK_o) {
                        screen = SCREEN_OPTIONS;
                    } else if (key == SDLK_l) {
                        tetris_audio_detach_rom(&audio);
                        (void)choose_and_load_rom(renderer, &rom, &font,
                                                  &settings, settings_path);
                        attach_original_audio(&audio, &rom);
                        tetris_songs_set_rom(&rom);
                        settings.music_track = tetris_songs_apply(
                            &audio, settings.audio_enabled,
                            settings.music_track);
                    } else if (key == SDLK_F8) {
                        (void)start_last_replay(&playback, &game, &audio,
                                                last_replay_path, &screen,
                                                &replay_index, &replay_done_at,
                                                replay_status,
                                                sizeof(replay_status),
                                                &left, &right, &down);
                    }
                } else if (screen == SCREEN_RECORDS) {
                    if (key == SDLK_RETURN || key == SDLK_SPACE ||
                        key == SDLK_BACKSPACE) screen = SCREEN_TITLE;
                } else if (screen == SCREEN_OPTIONS) {
                    if (key == SDLK_UP) {
                        if (--options.selected < 0) options.selected = OPTION_COUNT - 1;
                    } else if (key == SDLK_DOWN) {
                        options.selected = (options.selected + 1) % OPTION_COUNT;
                    } else if (key == SDLK_LEFT) {
                        apply_options(&settings, &options, -1, window,
                                      renderer, &audio, settings_path);
                    } else if (key == SDLK_RIGHT || key == SDLK_RETURN ||
                               key == SDLK_SPACE) {
                        apply_options(&settings, &options, 1, window,
                                      renderer, &audio, settings_path);
                    } else if (key == SDLK_BACKSPACE) {
                        screen = SCREEN_TITLE;
                    }
                } else if (screen == SCREEN_NAME_ENTRY) {
                    if (key == SDLK_LEFT) tetris_name_entry_move(&result.name, -1);
                    else if (key == SDLK_RIGHT) tetris_name_entry_move(&result.name, 1);
                    else if (key == SDLK_UP) tetris_name_entry_cycle(&result.name, 1);
                    else if (key == SDLK_DOWN) tetris_name_entry_cycle(&result.name, -1);
                    else if (key == SDLK_BACKSPACE) tetris_name_entry_backspace(&result.name);
                    else if (key == SDLK_RETURN || key == SDLK_SPACE)
                        save_name_entry(&scores, &result, score_path, &screen);
                    else if (key_to_name_character(key, &name_character))
                        tetris_name_entry_type(&result.name, name_character);
                } else if (screen == SCREEN_ENDING) {
                    if (key == SDLK_h) {
                        screen = SCREEN_RECORDS;
                    } else if (key == SDLK_RETURN || key == SDLK_SPACE ||
                               key == SDLK_r || key == SDLK_BACKSPACE) {
                        if (result.mode == TETRIS_MODE_A &&
                            !result.completed && result.rank >= 0)
                            screen = SCREEN_NAME_ENTRY;
                        else
                            screen = SCREEN_TITLE;
                    }
                } else if (screen == SCREEN_TYPE_SELECT) {
                    if (key == SDLK_LEFT || key == SDLK_RIGHT)
                        toggle_menu_mode(&menu);
                    if (key == SDLK_DOWN)
                        step_type_menu_music(&settings, &audio, 1,
                                             settings_path);
                    if (key == SDLK_UP)
                        step_type_menu_music(&settings, &audio, -1,
                                             settings_path);
                    if (key == SDLK_RETURN || key == SDLK_SPACE)
                        screen = SCREEN_LEVEL_SELECT;
                    if (key == SDLK_BACKSPACE) screen = SCREEN_TITLE;
                } else if (screen == SCREEN_LEVEL_SELECT) {
                    if (menu.mode == TETRIS_MODE_B) {
                        if (key == SDLK_UP || key == SDLK_DOWN)
                            menu.selecting_height = !menu.selecting_height;
                        if (key == SDLK_LEFT) change_menu_value(&menu, -1);
                        if (key == SDLK_RIGHT) change_menu_value(&menu, 1);
                    } else {
                        if (key == SDLK_LEFT) change_menu_value(&menu, -1);
                        if (key == SDLK_RIGHT) change_menu_value(&menu, 1);
                        if (key == SDLK_UP) change_menu_value(&menu, 5);
                        if (key == SDLK_DOWN) change_menu_value(&menu, -5);
                    }
                    if (key == SDLK_RETURN || key == SDLK_SPACE)
                        start_selected_game(&game, &menu, &screen,
                            &left, &right, &down, &result_handled,
                            &audio, &settings, settings_path, &recording,
                            &recording_active);
                    if (key == SDLK_BACKSPACE) screen = SCREEN_TYPE_SELECT;
                } else if (screen == SCREEN_GAME) {
                    switch (key) {
                        case SDLK_LEFT: left = true; break;
                        case SDLK_RIGHT: right = true; break;
                        case SDLK_DOWN: down = true; break;
                        case SDLK_UP:
                        case SDLK_x: pending.rotate_cw = true; break;
                        case SDLK_z: pending.rotate_ccw = true; break;
                        case SDLK_RETURN:
                            if (game.phase == TETRIS_PHASE_GAME_OVER ||
                                game.phase == TETRIS_PHASE_COMPLETE) {
                                finish_replay_recording(&recording, &game,
                                                        last_replay_path,
                                                        &recording_active);
                                screen = SCREEN_LEVEL_SELECT;
                                clear_held(&left, &right, &down);
                            }
                            break;
                        case SDLK_SPACE:
                            if (game.phase == TETRIS_PHASE_GAME_OVER ||
                                game.phase == TETRIS_PHASE_COMPLETE) {
                                finish_replay_recording(&recording, &game,
                                                        last_replay_path,
                                                        &recording_active);
                                screen = SCREEN_LEVEL_SELECT;
                                clear_held(&left, &right, &down);
                            } else if (settings.hard_drop_enabled) {
                                pending.hard_drop = true;
                            }
                            break;
                        case SDLK_p: pending.pause = true; break;
                        case SDLK_r:
                            pending.restart = true;
                            result_handled = false;
                            break;
                        case SDLK_TAB: pending.toggle_next = true; break;
                        case SDLK_h:
                            finish_replay_recording(&recording, &game,
                                                    last_replay_path,
