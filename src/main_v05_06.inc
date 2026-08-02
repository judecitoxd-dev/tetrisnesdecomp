                        tetris_tick(&game, &replay_input);
                        tetris_audio_update_game(&audio, &game,
                                                 tetris_consume_events(&game));
                        if (replay_index == playback.frame_count) {
                            const uint64_t actual = tetris_state_hash(&game);
                            snprintf(replay_status, sizeof(replay_status),
                                     actual == playback.final_hash
                                        ? "VERIFIED STATE MATCH"
                                        : "DESYNC STATE MISMATCH");
                            replay_done_at = SDL_GetTicks();
                            fprintf(stdout,
                                "Replay expected=%016" PRIx64
                                " actual=%016" PRIx64 "\n",
                                playback.final_hash, actual);
                        }
                    } else if (replay_done_at &&
                        (Uint32)(SDL_GetTicks() - replay_done_at) >=
                            REPLAY_RESULT_MS) {
                        screen = SCREEN_TITLE;
                        last_activity = SDL_GetTicks();
                    }
                }
                accumulator -= step;
            }
            if (consumed_pending) memset(&pending, 0, sizeof(pending));
        }

        render(renderer, font, screen, &game, &menu, &result, &options,
               &settings, replay_status, !rom.exact_supported_dump,
               &audio, &rom, &scores);
        tetris_touch_render(renderer, font, &touch, controller != NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }

    if (screen == SCREEN_GAME)
        finish_replay_recording(&recording, &game, last_replay_path,
                                &recording_active);
    if (!settings.fullscreen)
        SDL_GetWindowSize(window, &settings.window_width,
                          &settings.window_height);
    settings.audio_enabled = audio.enabled;
    settings.music_track = audio.music_track;
    tetris_touch_store_settings(&touch, &settings);
    save_settings_quiet(&settings, settings_path);
    (void)tetris_high_scores_save(&scores, score_path);
    tetris_replay_free(&recording);
    tetris_replay_free(&playback);
    if (controller) SDL_GameControllerClose(controller);
    tetris_audio_shutdown(&audio);
    free_rom_graphics();
    SDL_DestroyTexture(font);
    nes_rom_free(&rom);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
