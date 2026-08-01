#ifndef TETRIS_AUDIO_H
#define TETRIS_AUDIO_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

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

    bool enabled;
} TetrisAudio;

bool tetris_audio_init(TetrisAudio *audio);
void tetris_audio_shutdown(TetrisAudio *audio);
void tetris_audio_play_events(TetrisAudio *audio, uint32_t events);
void tetris_audio_toggle(TetrisAudio *audio);
void tetris_audio_cycle_music(TetrisAudio *audio);
const char *tetris_audio_music_label(const TetrisAudio *audio);

#endif
