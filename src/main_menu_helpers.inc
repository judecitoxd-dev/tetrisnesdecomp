#include "audio.h"
#include "songs.h"

static void save_settings_quiet(const TetrisSettings *settings,
                                const char *settings_path);

static void step_type_menu_music(TetrisSettings *settings,
                                 TetrisAudio *audio, int direction,
                                 const char *settings_path) {
    const int next = tetris_songs_step(settings->music_track, direction);
    if (next == settings->music_track) return;
    settings->music_track = tetris_songs_apply(
        audio, settings->audio_enabled, next);
    save_settings_quiet(settings, settings_path);
}
