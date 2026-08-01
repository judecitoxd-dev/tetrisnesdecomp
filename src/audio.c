#include "audio.h"
#include "game.h"

#include <string.h>

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    TetrisAudio *audio = (TetrisAudio *)userdata;
    float *samples = (float *)stream;
    const int count = len / (int)sizeof(float);
    for (int i = 0; i < count; ++i) {
        float sample = 0.0f;
        if (audio->enabled && audio->remaining_samples > 0 && audio->frequency > 0.0) {
            sample = audio->phase < 0.5 ? audio->volume : -audio->volume;
            audio->phase += audio->frequency / (double)audio->sample_rate;
            if (audio->phase >= 1.0) audio->phase -= 1.0;
            --audio->remaining_samples;
        }
        samples[i] = sample;
    }
}

static void start_tone(TetrisAudio *audio, double frequency, int milliseconds, float volume) {
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    audio->frequency = frequency;
    audio->remaining_samples = (audio->sample_rate * milliseconds) / 1000;
    audio->volume = volume;
    audio->phase = 0.0;
    SDL_UnlockAudioDevice(audio->device);
}

bool tetris_audio_init(TetrisAudio *audio) {
    memset(audio, 0, sizeof(*audio));
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    SDL_zero(desired);
    desired.freq = 48000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 512;
    desired.callback = audio_callback;
    desired.userdata = audio;
    audio->device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (!audio->device) return false;
    audio->sample_rate = obtained.freq;
    audio->enabled = true;
    SDL_PauseAudioDevice(audio->device, 0);
    return true;
}

void tetris_audio_shutdown(TetrisAudio *audio) {
    if (audio->device) SDL_CloseAudioDevice(audio->device);
    memset(audio, 0, sizeof(*audio));
}

void tetris_audio_play_events(TetrisAudio *audio, uint32_t events) {
    if (!audio->device || !audio->enabled || events == 0) return;
    if (events & TETRIS_EVENT_GAME_OVER) {
        start_tone(audio, 55.0, 500, 0.18f);
    } else if (events & TETRIS_EVENT_LEVEL_UP) {
        start_tone(audio, 660.0, 180, 0.12f);
    } else if (events & TETRIS_EVENT_TETRIS) {
        start_tone(audio, 520.0, 180, 0.14f);
    } else if (events & TETRIS_EVENT_LINE) {
        start_tone(audio, 330.0, 110, 0.12f);
    } else if (events & TETRIS_EVENT_LOCK) {
        start_tone(audio, 75.0, 45, 0.10f);
    } else if (events & TETRIS_EVENT_ROTATE) {
        start_tone(audio, 190.0, 30, 0.08f);
    } else if (events & TETRIS_EVENT_MOVE) {
        start_tone(audio, 115.0, 20, 0.05f);
    }
}

void tetris_audio_toggle(TetrisAudio *audio) {
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    audio->enabled = !audio->enabled;
    audio->remaining_samples = 0;
    SDL_UnlockAudioDevice(audio->device);
}
