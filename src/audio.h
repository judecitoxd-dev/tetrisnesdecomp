#ifndef TETRIS_AUDIO_H
#define TETRIS_AUDIO_H

#include <SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef TETRIS_HAVE_SDL_MIXER
#include <SDL_mixer.h>
#endif

#define TETRIS_AUDIO_PACK_PATH_LENGTH 1023
#define TETRIS_AUDIO_MUSIC_COUNT 3
#define TETRIS_AUDIO_SFX_COUNT 8

typedef struct TetrisAudio {
    SDL_AudioDeviceID device;
    int sample_rate;

    double sfx_phase;
    double sfx_frequency;
    float sfx_volume;
    int sfx_remaining_samples;

    int music_track;
    int music_step;
    int music_remaining_samples;
    double music_phase_melody;
    double music_phase_bass;
    double music_frequency_melody;
    double music_frequency_bass;

#ifdef TETRIS_HAVE_SDL_MIXER
    Mix_Music *pack_music[TETRIS_AUDIO_MUSIC_COUNT];
    Mix_Chunk *pack_sfx[TETRIS_AUDIO_SFX_COUNT];
#endif
    char pack_path[TETRIS_AUDIO_PACK_PATH_LENGTH + 1];
    bool pack_active;
    bool enabled;
} TetrisAudio;

bool tetris_audio_init(TetrisAudio *audio);
void tetris_audio_shutdown(TetrisAudio *audio);
void tetris_audio_play_events(TetrisAudio *audio, uint32_t events);
void tetris_audio_toggle(TetrisAudio *audio);
void tetris_audio_cycle_music(TetrisAudio *audio);
void tetris_audio_apply_settings(TetrisAudio *audio, bool enabled, int music_track);
const char *tetris_audio_music_label(const TetrisAudio *audio);
const char *tetris_audio_backend_label(const TetrisAudio *audio);

/*
 * Loads a user-created Ogg Vorbis pack from a directory. The port never ships
 * copyrighted audio. See docs/AUDIO_PACK.md for the expected filenames and a
 * conversion workflow based on the user's own legally obtained recordings.
 */
bool tetris_audio_load_ogg_pack(TetrisAudio *audio, const char *directory,
                                char *error, size_t error_size);

#endif
