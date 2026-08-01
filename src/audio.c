#include "audio.h"
#include "game.h"

#include <string.h>

typedef struct MusicStep {
    float melody;
    float bass;
    unsigned char ticks;
} MusicStep;

/* Original, newly composed loops. No note data is copied from the cartridge. */
static const MusicStep MUSIC_1[] = {
    {329.63f, 82.41f, 2}, {392.00f, 82.41f, 2}, {493.88f, 98.00f, 2}, {392.00f, 98.00f, 2},
    {349.23f, 87.31f, 2}, {440.00f, 87.31f, 2}, {523.25f,110.00f, 2}, {440.00f,110.00f, 2},
    {392.00f, 98.00f, 2}, {493.88f, 98.00f, 2}, {587.33f,123.47f, 2}, {493.88f,123.47f, 2},
    {349.23f, 87.31f, 2}, {440.00f, 87.31f, 2}, {329.63f, 82.41f, 2}, {261.63f, 65.41f, 4}
};

static const MusicStep MUSIC_2[] = {
    {261.63f, 65.41f, 1}, {293.66f, 65.41f, 1}, {311.13f, 77.78f, 1}, {392.00f, 77.78f, 1},
    {369.99f, 73.42f, 2}, {311.13f, 73.42f, 2}, {293.66f, 65.41f, 1}, {349.23f, 65.41f, 1},
    {440.00f, 87.31f, 2}, {349.23f, 87.31f, 2}, {329.63f, 82.41f, 1}, {392.00f, 82.41f, 1},
    {493.88f, 98.00f, 2}, {392.00f, 98.00f, 2}, {293.66f, 73.42f, 2}, {246.94f, 61.74f, 4}
};

static const MusicStep MUSIC_3[] = {
    {220.00f, 55.00f, 2}, {277.18f, 69.30f, 2}, {329.63f, 82.41f, 2}, {415.30f,103.83f, 2},
    {392.00f, 98.00f, 1}, {329.63f, 82.41f, 1}, {277.18f, 69.30f, 2}, {246.94f, 61.74f, 2},
    {293.66f, 73.42f, 2}, {369.99f, 92.50f, 2}, {440.00f,110.00f, 2}, {554.37f,138.59f, 2},
    {493.88f,123.47f, 1}, {440.00f,110.00f, 1}, {369.99f, 92.50f, 2}, {220.00f, 55.00f, 4}
};

static const MusicStep *music_data(int track, int *length) {
    if (track == 0) {
        *length = (int)(sizeof(MUSIC_1) / sizeof(MUSIC_1[0]));
        return MUSIC_1;
    }
    if (track == 1) {
        *length = (int)(sizeof(MUSIC_2) / sizeof(MUSIC_2[0]));
        return MUSIC_2;
    }
    if (track == 2) {
        *length = (int)(sizeof(MUSIC_3) / sizeof(MUSIC_3[0]));
        return MUSIC_3;
    }
    *length = 0;
    return NULL;
}

static void load_music_step(TetrisAudio *audio) {
    int length = 0;
    const MusicStep *steps = music_data(audio->music_track, &length);
    if (!steps || length == 0) {
        audio->music_remaining_samples = 0;
        audio->music_frequency_melody = 0.0;
        audio->music_frequency_bass = 0.0;
        return;
    }
    if (audio->music_step < 0 || audio->music_step >= length) audio->music_step = 0;
    audio->music_frequency_melody = steps[audio->music_step].melody;
    audio->music_frequency_bass = steps[audio->music_step].bass;
    audio->music_remaining_samples =
        (audio->sample_rate * 115 * steps[audio->music_step].ticks) / 1000;
    audio->music_step = (audio->music_step + 1) % length;
}

static float square_sample(double *phase, double frequency, int sample_rate) {
    float sample;
    if (frequency <= 0.0) return 0.0f;
    sample = *phase < 0.5 ? 1.0f : -1.0f;
    *phase += frequency / (double)sample_rate;
    if (*phase >= 1.0) *phase -= 1.0;
    return sample;
}

static float triangle_sample(double *phase, double frequency, int sample_rate) {
    double value;
    if (frequency <= 0.0) return 0.0f;
    value = *phase < 0.5 ? (*phase * 4.0 - 1.0) : (3.0 - *phase * 4.0);
    *phase += frequency / (double)sample_rate;
    if (*phase >= 1.0) *phase -= 1.0;
    return (float)value;
}

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    TetrisAudio *audio = (TetrisAudio *)userdata;
    float *samples = (float *)stream;
    const int count = len / (int)sizeof(float);
    for (int i = 0; i < count; ++i) {
        float sample = 0.0f;
        if (audio->enabled && audio->music_track >= 0) {
            if (audio->music_remaining_samples <= 0) load_music_step(audio);
            sample += square_sample(&audio->music_phase_melody,
                                    audio->music_frequency_melody,
                                    audio->sample_rate) * 0.055f;
            sample += triangle_sample(&audio->music_phase_bass,
                                      audio->music_frequency_bass,
                                      audio->sample_rate) * 0.045f;
            --audio->music_remaining_samples;
        }
        if (audio->enabled && audio->sfx_remaining_samples > 0 &&
            audio->sfx_frequency > 0.0) {
            sample += square_sample(&audio->sfx_phase, audio->sfx_frequency,
                                    audio->sample_rate) * audio->sfx_volume;
            --audio->sfx_remaining_samples;
        }
        if (sample > 0.9f) sample = 0.9f;
        if (sample < -0.9f) sample = -0.9f;
        samples[i] = sample;
    }
}

static void start_tone(TetrisAudio *audio, double frequency,
                       int milliseconds, float volume) {
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    audio->sfx_frequency = frequency;
    audio->sfx_remaining_samples = (audio->sample_rate * milliseconds) / 1000;
    audio->sfx_volume = volume;
    audio->sfx_phase = 0.0;
    SDL_UnlockAudioDevice(audio->device);
}

bool tetris_audio_init(TetrisAudio *audio) {
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    memset(audio, 0, sizeof(*audio));
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
    audio->music_track = 0;
    audio->music_step = 0;
    SDL_PauseAudioDevice(audio->device, 0);
    return true;
}

void tetris_audio_shutdown(TetrisAudio *audio) {
    if (audio->device) SDL_CloseAudioDevice(audio->device);
    memset(audio, 0, sizeof(*audio));
}

void tetris_audio_play_events(TetrisAudio *audio, uint32_t events) {
    if (!audio->device || !audio->enabled || events == 0) return;
    if (events & TETRIS_EVENT_COMPLETE) {
        start_tone(audio, 880.0, 420, 0.13f);
    } else if (events & TETRIS_EVENT_GAME_OVER) {
        start_tone(audio, 55.0, 500, 0.18f);
    } else if (events & TETRIS_EVENT_LEVEL_UP) {
        start_tone(audio, 660.0, 180, 0.12f);
    } else if (events & TETRIS_EVENT_TETRIS) {
        start_tone(audio, 522.0, 180, 0.14f);
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
    audio->sfx_remaining_samples = 0;
    SDL_UnlockAudioDevice(audio->device);
}

void tetris_audio_cycle_music(TetrisAudio *audio) {
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    ++audio->music_track;
    if (audio->music_track > 2) audio->music_track = -1;
    audio->music_step = 0;
    audio->music_remaining_samples = 0;
    audio->music_phase_melody = 0.0;
    audio->music_phase_bass = 0.0;
    SDL_UnlockAudioDevice(audio->device);
}

const char *tetris_audio_music_label(const TetrisAudio *audio) {
    if (!audio || audio->music_track < 0) return "MUSIC OFF";
    if (audio->music_track == 0) return "MUSIC 1";
    if (audio->music_track == 1) return "MUSIC 2";
    return "MUSIC 3";
}
