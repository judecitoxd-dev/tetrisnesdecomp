                                                    &recording_active);
                            screen = SCREEN_RECORDS;
                            break;
                        case SDLK_BACKSPACE:
                            finish_replay_recording(&recording, &game,
                                                    last_replay_path,
                                                    &recording_active);
                            screen = SCREEN_TITLE;
                            clear_held(&left, &right, &down);
                            break;
                        default: break;
                    }
                }
            }

            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_LEFT) left = false;
                if (event.key.keysym.sym == SDLK_RIGHT) right = false;
                if (event.key.keysym.sym == SDLK_DOWN) down = false;
            }

            if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                const Uint8 button = event.cbutton.button;
                if (screen == SCREEN_REPLAY || screen == SCREEN_DEMO) {
                    screen = SCREEN_TITLE;
                    replay_done_at = 0;
                    clear_held(&left, &right, &down);
                } else if (screen == SCREEN_TITLE) {
                    if (button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_A)
                        screen = SCREEN_TYPE_SELECT;
                    if (button == SDL_CONTROLLER_BUTTON_Y) screen = SCREEN_RECORDS;
                    if (button == SDL_CONTROLLER_BUTTON_X) screen = SCREEN_OPTIONS;
                    if (button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
                        (void)start_last_replay(&playback, &game, &audio,
                            last_replay_path, &screen, &replay_index,
                            &replay_done_at, replay_status,
                            sizeof(replay_status), &left, &right, &down);
                } else if (screen == SCREEN_RECORDS) {
                    if (button == SDL_CONTROLLER_BUTTON_A ||
                        button == SDL_CONTROLLER_BUTTON_B ||
                        button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_BACK)
                        screen = SCREEN_TITLE;
                } else if (screen == SCREEN_OPTIONS) {
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
                        if (--options.selected < 0) options.selected = OPTION_COUNT - 1;
                    }
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                        options.selected = (options.selected + 1) % OPTION_COUNT;
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                        apply_options(&settings, &options, -1, window,
                                      renderer, &audio, settings_path);
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT ||
                        button == SDL_CONTROLLER_BUTTON_A)
                        apply_options(&settings, &options, 1, window,
                                      renderer, &audio, settings_path);
                    if (button == SDL_CONTROLLER_BUTTON_B ||
                        button == SDL_CONTROLLER_BUTTON_BACK)
                        screen = SCREEN_TITLE;
                } else if (screen == SCREEN_NAME_ENTRY) {
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                        tetris_name_entry_move(&result.name, -1);
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                        tetris_name_entry_move(&result.name, 1);
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_UP)
                        tetris_name_entry_cycle(&result.name, 1);
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                        tetris_name_entry_cycle(&result.name, -1);
                    if (button == SDL_CONTROLLER_BUTTON_X)
                        tetris_name_entry_backspace(&result.name);
                    if (button == SDL_CONTROLLER_BUTTON_A ||
                        button == SDL_CONTROLLER_BUTTON_START)
                        save_name_entry(&scores, &result, score_path, &screen);
                } else if (screen == SCREEN_ENDING) {
                    if (button == SDL_CONTROLLER_BUTTON_Y) screen = SCREEN_RECORDS;
                    if (button == SDL_CONTROLLER_BUTTON_A ||
                        button == SDL_CONTROLLER_BUTTON_B ||
                        button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_BACK)
                        screen = SCREEN_TITLE;
                } else if (screen == SCREEN_TYPE_SELECT) {
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT ||
                        button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT ||
                        button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
                        button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                        toggle_menu_mode(&menu);
                    if (button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_A)
                        screen = SCREEN_LEVEL_SELECT;
                    if (button == SDL_CONTROLLER_BUTTON_B ||
                        button == SDL_CONTROLLER_BUTTON_BACK)
                        screen = SCREEN_TITLE;
                } else if (screen == SCREEN_LEVEL_SELECT) {
                    if (menu.mode == TETRIS_MODE_B) {
                        if (button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
                            button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                            menu.selecting_height = !menu.selecting_height;
                        if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                            change_menu_value(&menu, -1);
                        if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                            change_menu_value(&menu, 1);
                    } else {
                        if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                            change_menu_value(&menu, -1);
                        if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                            change_menu_value(&menu, 1);
                        if (button == SDL_CONTROLLER_BUTTON_DPAD_UP)
                            change_menu_value(&menu, 5);
