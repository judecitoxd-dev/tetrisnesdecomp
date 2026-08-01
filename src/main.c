#include "app.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void make_score_path(char *path, size_t path_size) {
    char *preference_path = SDL_GetPrefPath("YlPorts", "TetrisNESPC");
    if (preference_path) {
        snprintf(path, path_size, "%sscores.txt", preference_path);
        SDL_free(preference_path);
    } else {
        snprintf(path, path_size, "tetris_scores.txt");
    }
}

static void maybe_submit_score(TetrisHighScores *scores, const TetrisGame *game,
                               const char *score_path, bool *submitted) {
    if (*submitted) return;
    if (game->phase != TETRIS_PHASE_GAME_OVER &&
        game->phase != TETRIS_PHASE_COMPLETE) return;
    *submitted = true;
    if (game->score <= 0) return;
    if (tetris_high_scores_submit(scores, game->mode, "PLAYER", game->score,
                                  game->start_level, game->start_height)) {
        if (!tetris_high_scores_save(scores, score_path)) {
            fprintf(stderr, "Could not save local scores to %s\n", score_path);
        }
    }
}

static void start_selected_game(TetrisGame *game, const AppMenuState *menu,
                                AppScreen *screen, bool *left, bool *right,
                                bool *down, bool *score_submitted) {
    begin_game(game, menu);
    *screen = SCREEN_GAME;
    *score_submitted = false;
    clear_held(left, right, down);
}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                 SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Tetris NES PC Port v0.3",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
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
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    NesRom rom = {0};
    SDL_Texture *font = NULL;
    const char *rom_path = argc > 1 ? argv[1] : "Tetris (USA).nes";
    if (!load_rom_and_font(renderer, rom_path, &rom, &font)) {
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
    }

    char score_path[1024];
    TetrisHighScores scores;
    make_score_path(score_path, sizeof(score_path));
    tetris_high_scores_init(&scores);
    (void)tetris_high_scores_load(&scores, score_path);

    SDL_GameController *controller = open_first_controller();
    TetrisGame game;
    AppMenuState menu = {TETRIS_MODE_A, 0, 0, false};
    tetris_init(&game, (uint32_t)time(NULL), 0);
    AppScreen screen = SCREEN_TITLE;
    bool running = true;
    bool left = false;
    bool right = false;
    bool down = false;
    bool fullscreen = false;
    bool score_submitted = false;
    PendingInput pending = {0};
    uint64_t previous = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    const double step = 1.0 / 60.0988;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_CONTROLLERDEVICEADDED && !controller) {
                controller = SDL_GameControllerOpen(event.cdevice.which);
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
                if (load_rom_and_font(renderer, event.drop.file, &rom, &font)) {
                    screen = SCREEN_TITLE;
                    clear_held(&left, &right, &down);
                }
                SDL_free(event.drop.file);
            }

            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                const SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) {
                    running = false;
                } else if (key == SDLK_F11) {
                    fullscreen = !fullscreen;
                    SDL_SetWindowFullscreen(window,
                        fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                } else if (key == SDLK_m) {
                    tetris_audio_toggle(&audio);
                } else if (key == SDLK_n) {
                    tetris_audio_cycle_music(&audio);
                } else if (screen == SCREEN_TITLE) {
                    if (key == SDLK_RETURN || key == SDLK_SPACE) {
                        screen = SCREEN_TYPE_SELECT;
                    } else if (key == SDLK_h) {
                        screen = SCREEN_RECORDS;
                    }
                } else if (screen == SCREEN_RECORDS) {
                    if (key == SDLK_RETURN || key == SDLK_SPACE ||
                        key == SDLK_BACKSPACE) screen = SCREEN_TITLE;
                } else if (screen == SCREEN_TYPE_SELECT) {
                    if (key == SDLK_LEFT || key == SDLK_RIGHT ||
                        key == SDLK_UP || key == SDLK_DOWN) {
                        toggle_menu_mode(&menu);
                    }
                    if (key == SDLK_RETURN || key == SDLK_SPACE) {
                        screen = SCREEN_LEVEL_SELECT;
                    }
                    if (key == SDLK_BACKSPACE) screen = SCREEN_TITLE;
                } else if (screen == SCREEN_LEVEL_SELECT) {
                    if (menu.mode == TETRIS_MODE_B) {
                        if (key == SDLK_UP || key == SDLK_DOWN) {
                            menu.selecting_height = !menu.selecting_height;
                        }
                        if (key == SDLK_LEFT) change_menu_value(&menu, -1);
                        if (key == SDLK_RIGHT) change_menu_value(&menu, 1);
                    } else {
                        if (key == SDLK_LEFT) change_menu_value(&menu, -1);
                        if (key == SDLK_RIGHT) change_menu_value(&menu, 1);
                        if (key == SDLK_UP) change_menu_value(&menu, 5);
                        if (key == SDLK_DOWN) change_menu_value(&menu, -5);
                    }
                    if (key == SDLK_RETURN || key == SDLK_SPACE) {
                        start_selected_game(&game, &menu, &screen,
                                            &left, &right, &down,
                                            &score_submitted);
                    }
                    if (key == SDLK_BACKSPACE) screen = SCREEN_TYPE_SELECT;
                } else if (screen == SCREEN_GAME) {
                    switch (key) {
                        case SDLK_LEFT: left = true; break;
                        case SDLK_RIGHT: right = true; break;
                        case SDLK_DOWN: down = true; break;
                        case SDLK_UP:
                        case SDLK_x: pending.rotate_cw = true; break;
                        case SDLK_z: pending.rotate_ccw = true; break;
                        case SDLK_SPACE: pending.hard_drop = true; break;
                        case SDLK_p: pending.pause = true; break;
                        case SDLK_r:
                            pending.restart = true;
                            score_submitted = false;
                            break;
                        case SDLK_TAB: pending.toggle_next = true; break;
                        case SDLK_h: screen = SCREEN_RECORDS; break;
                        case SDLK_BACKSPACE:
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
                if (screen == SCREEN_TITLE) {
                    if (button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_A) {
                        screen = SCREEN_TYPE_SELECT;
                    }
                    if (button == SDL_CONTROLLER_BUTTON_Y) screen = SCREEN_RECORDS;
                } else if (screen == SCREEN_RECORDS) {
                    if (button == SDL_CONTROLLER_BUTTON_A ||
                        button == SDL_CONTROLLER_BUTTON_B ||
                        button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_BACK) {
                        screen = SCREEN_TITLE;
                    }
                } else if (screen == SCREEN_TYPE_SELECT) {
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT ||
                        button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT ||
                        button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
                        button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                        toggle_menu_mode(&menu);
                    }
                    if (button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_A) {
                        screen = SCREEN_LEVEL_SELECT;
                    }
                    if (button == SDL_CONTROLLER_BUTTON_B ||
                        button == SDL_CONTROLLER_BUTTON_BACK) {
                        screen = SCREEN_TITLE;
                    }
                } else if (screen == SCREEN_LEVEL_SELECT) {
                    if (menu.mode == TETRIS_MODE_B) {
                        if (button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
                            button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                            menu.selecting_height = !menu.selecting_height;
                        }
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
                        if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                            change_menu_value(&menu, -5);
                    }
                    if (button == SDL_CONTROLLER_BUTTON_START ||
                        button == SDL_CONTROLLER_BUTTON_A) {
                        start_selected_game(&game, &menu, &screen,
                                            &left, &right, &down,
                                            &score_submitted);
                    }
                    if (button == SDL_CONTROLLER_BUTTON_B ||
                        button == SDL_CONTROLLER_BUTTON_BACK) {
                        screen = SCREEN_TYPE_SELECT;
                    }
                } else if (screen == SCREEN_GAME) {
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) left = true;
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) right = true;
                    if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) down = true;
                    if (button == SDL_CONTROLLER_BUTTON_A) pending.rotate_cw = true;
                    if (button == SDL_CONTROLLER_BUTTON_B) pending.rotate_ccw = true;
                    if (button == SDL_CONTROLLER_BUTTON_X) pending.hard_drop = true;
                    if (button == SDL_CONTROLLER_BUTTON_Y) pending.toggle_next = true;
                    if (button == SDL_CONTROLLER_BUTTON_START) {
                        if (game.phase == TETRIS_PHASE_GAME_OVER ||
                            game.phase == TETRIS_PHASE_COMPLETE) {
                            pending.restart = true;
                            score_submitted = false;
                        } else {
                            pending.pause = true;
                        }
                    }
                    if (button == SDL_CONTROLLER_BUTTON_BACK) {
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
                    TetrisInput input = {0};
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
                    tetris_tick(&game, &input);
                    tetris_audio_play_events(&audio, tetris_consume_events(&game));
                    maybe_submit_score(&scores, &game, score_path, &score_submitted);
                    consumed_pending = true;
                }
                accumulator -= step;
            }
            if (consumed_pending) memset(&pending, 0, sizeof(pending));
        }

        render(renderer, font, screen, &game, &menu,
               !rom.exact_supported_dump, &audio, &rom, &scores);
        SDL_Delay(1);
    }

    (void)tetris_high_scores_save(&scores, score_path);
    if (controller) SDL_GameControllerClose(controller);
    tetris_audio_shutdown(&audio);
    SDL_DestroyTexture(font);
    nes_rom_free(&rom);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
