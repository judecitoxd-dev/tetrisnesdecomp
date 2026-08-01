#ifndef TETRIS_AUDIO_H
#define TETRIS_AUDIO_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct TetrisAudio {
    SDL_AudioDeviceID device;
    int sample_rate;
    double phase;
    double frequency;
    float volume;
    int remaining_samples;
    bool enabled;
} TetrisAudio;

bool tetris_audio_init(TetrisAudio *audio);
void tetris_audio_shutdown(TetrisAudio *audio);
void tetris_audio_play_events(TetrisAudio *audio, uint32_t events);
void tetris_audio_toggle(TetrisAudio *audio);

#endif
