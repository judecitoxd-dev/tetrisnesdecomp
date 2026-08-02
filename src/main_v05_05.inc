                        if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                            change_menu_value(&menu, -5);
                    }
                    if (button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_A)
                        start_selected_game(&game, &menu, &screen,
                            &left, &right, &down, &result_handled,
                            &settings, settings_path, &recording,
                            &recording_active);
                    if (button == SDL_CONTROLLER_BUTTON_B ||
                        button == SDL_CONTROLLER_BUTTON_BACK)
                        screen = SCREEN_TYPE_SELECT;
                } else if (screen == SCREEN_GAME) {
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) left = true;
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) right = true;
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) down = true;
                    if (button == SDL_CONTROLLER_BUTTON_A) pending.rotate_cw = true;
                    if (button == SDL_CONTROLLER_BUTTON_B) pending.rotate_ccw = true;
                    if (button == SDL_CONTROLLER_BUTTON_X &&
                        settings.hard_drop_enabled) pending.hard_drop = true;
                    if (button == SDL_CONTROLLER_BUTTON_Y) pending.toggle_next = true;
                    if (button == SDL_CONTROLLER_BUTTON_START) {
                        if (game.phase == TETRIS_PHASE_GAME_OVER ||
                            game.phase == TETRIS_PHASE_COMPLETE) {
                            pending.restart = true;
                            result_handled = false;
                        } else pending.pause = true;
                    }
                    if (button == SDL_CONTROLLER_BUTTON_BACK) {
                        finish_replay_recording(&recording, &game,
                                                last_replay_path,
                                                &recording_active);
                        screen = SCREEN_TITLE;
                        clear_held(&left, &right, &down);
                    }
                }
            }

            if (event.type == SDL_CONTROLLERBUTTONUP) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) left = false;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) right = false;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) down = false;
            }
        }

        if (screen == SCREEN_TITLE && settings.demo_enabled &&
            (Uint32)(SDL_GetTicks() - last_activity) >= DEMO_IDLE_MS) {
            start_demo(&game, &demo, &rom, &screen, &demo_started,
                       &left, &right, &down);
        }

        {
            const uint64_t now = SDL_GetPerformanceCounter();
            accumulator += (double)(now - previous) /
                           (double)SDL_GetPerformanceFrequency();
            previous = now;
        }
        if (accumulator > 0.25) accumulator = 0.25;

        {
            bool consumed_pending = false;
            while (accumulator >= step) {
                if (screen == SCREEN_GAME) {
                    TetrisInput input;
                    bool restarting;
                    memset(&input, 0, sizeof(input));
                    input.left = left;
                    input.right = right;
                    input.down = down;
                    if (!consumed_pending) {
                        input.rotate_cw_pressed = pending.rotate_cw;
                        input.rotate_ccw_pressed = pending.rotate_ccw;
                        input.hard_drop_pressed = pending.hard_drop;
                        input.pause_pressed = pending.pause;
                        input.restart_pressed = pending.restart;
                        input.toggle_next_pressed = pending.toggle_next;
                    }
                    if (!settings.hard_drop_enabled)
                        input.hard_drop_pressed = false;
                    restarting = input.restart_pressed &&
                        (game.phase == TETRIS_PHASE_GAME_OVER ||
                         game.phase == TETRIS_PHASE_COMPLETE);
                    if (recording_active &&
                        !tetris_replay_append(&recording, &input)) {
                        fprintf(stderr, "Replay recording reached its limit.\n");
                        recording_active = false;
                    }
                    tetris_tick(&game, &input);
                    tetris_audio_update_game(&audio, &game,
                                             tetris_consume_events(&game));
                    if (input.toggle_next_pressed) {
                        settings.show_next = game.show_next;
                        save_settings_quiet(&settings, settings_path);
                    }
                    if (restarting) {
                        begin_replay_recording(&recording, &game,
                                               &recording_active);
                    } else if (game.phase == TETRIS_PHASE_GAME_OVER ||
                               game.phase == TETRIS_PHASE_COMPLETE) {
                        finish_replay_recording(&recording, &game,
                                                last_replay_path,
                                                &recording_active);
                    }
                    begin_result_flow(&scores, &game, &result, &screen,
                                      &result_handled, &left, &right, &down);
                    consumed_pending = true;
                } else if (screen == SCREEN_DEMO) {
                    TetrisInput demo_input = tetris_demo_next_input(&demo, &game);
                    tetris_tick(&game, &demo_input);
                    tetris_audio_update_game(&audio, &game,
                                             tetris_consume_events(&game));
                    if (game.phase == TETRIS_PHASE_GAME_OVER ||
                        tetris_demo_is_finished(&demo) ||
                        (Uint32)(SDL_GetTicks() - demo_started) >=
                            (tetris_demo_uses_rom_script(&demo)
                                ? DEMO_ROM_MAX_MS : DEMO_MAX_MS)) {
                        screen = SCREEN_TITLE;
                        last_activity = SDL_GetTicks();
                    }
                } else if (screen == SCREEN_REPLAY) {
                    if (replay_index < playback.frame_count) {
                        TetrisInput replay_input =
                            tetris_replay_input(&playback, replay_index++);
