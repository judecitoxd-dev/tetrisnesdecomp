static void step_type_menu_music(TetrisSettings *settings,
                                 TetrisAudio *audio, int direction,
                                 const char *settings_path) {
    const int next = tetris_settings_step_music(settings->music_track,
                                                 direction);
    if (next == settings->music_track) return;
    settings->music_track = next;
    tetris_audio_apply_settings(audio, settings->audio_enabled,
                                settings->music_track);
    save_settings_quiet(settings, settings_path);
}
