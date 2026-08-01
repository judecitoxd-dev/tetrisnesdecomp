#include "audio.h"
#include "game.h"

#include <stdio.h>
#include <string.h>

typedef struct MusicStep {
    float melody;
    float bass;
    unsigned char ticks;
} MusicStep;

/* Original, newly composed fallback loops. No note data is copied from the cartridge. */
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

#ifdef TETRIS_HAVE_SDL_MIXER
static const char *PACK_MUSIC_FILES[TETRIS_AUDIO_MUSIC_COUNT] = {
    "music_1.ogg", "music_2.ogg", "music_3.ogg"
};

static const char *PACK_SFX_FILES[TETRIS_AUDIO_SFX_COUNT] = {
    "move.ogg", "rotate.ogg", "lock.ogg", "line.ogg",
    "tetris.ogg", "level_up.ogg", "game_over.ogg", "complete.ogg"
};
#endif

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
    int index;
    for (index = 0; index < count; ++index) {
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
        samples[index] = sample;
    }
}

static bool open_synth_device(TetrisAudio *audio) {
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
    audio->pack_active = false;
    SDL_PauseAudioDevice(audio->device, 0);
    return true;
}

static void close_synth_device(TetrisAudio *audio) {
    if (audio->device) SDL_CloseAudioDevice(audio->device);
    audio->device = 0;
}

static void reset_synth_music(TetrisAudio *audio) {
    audio->music_step = 0;
    audio->music_remaining_samples = 0;
    audio->music_phase_melody = 0.0;
    audio->music_phase_bass = 0.0;
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

#ifdef TETRIS_HAVE_SDL_MIXER
static void make_pack_path(char *destination, size_t destination_size,
                           const char *directory, const char *filename) {
    const size_t length = strlen(directory);
    const bool has_separator = length > 0 &&
        (directory[length - 1] == '/' || directory[length - 1] == '\\');
    snprintf(destination, destination_size, "%s%s%s", directory,
             has_separator ? "" : "/", filename);
}

static void free_pack_objects(TetrisAudio *audio) {
    int index;
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    for (index = 0; index < TETRIS_AUDIO_MUSIC_COUNT; ++index) {
        if (audio->pack_music[index]) Mix_FreeMusic(audio->pack_music[index]);
        audio->pack_music[index] = NULL;
    }
    for (index = 0; index < TETRIS_AUDIO_SFX_COUNT; ++index) {
        if (audio->pack_sfx[index]) Mix_FreeChunk(audio->pack_sfx[index]);
        audio->pack_sfx[index] = NULL;
    }
}

static void close_pack_backend(TetrisAudio *audio) {
    if (!audio->pack_active) return;
    free_pack_objects(audio);
    Mix_CloseAudio();
    Mix_Quit();
    audio->pack_active = false;
    audio->pack_path[0] = '\0';
}

static void play_pack_music(TetrisAudio *audio) {
    if (!audio->pack_active) return;
    if (!audio->enabled || audio->music_track < 0) {
        Mix_HaltMusic();
        return;
    }
    if (audio->music_track >= TETRIS_AUDIO_MUSIC_COUNT ||
        !audio->pack_music[audio->music_track]) {
        Mix_HaltMusic();
        return;
    }
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
    if (Mix_PlayMusic(audio->pack_music[audio->music_track], -1) != 0) {
        fprintf(stderr, "Could not play OGG music: %s\n", Mix_GetError());
    }
}

static int pack_sfx_index(uint32_t events) {
    if (events & TETRIS_EVENT_COMPLETE) return 7;
    if (events & TETRIS_EVENT_GAME_OVER) return 6;
    if (events & TETRIS_EVENT_LEVEL_UP) return 5;
    if (events & TETRIS_EVENT_TETRIS) return 4;
    if (events & TETRIS_EVENT_LINE) return 3;
    if (events & TETRIS_EVENT_LOCK) return 2;
    if (events & TETRIS_EVENT_ROTATE) return 1;
    if (events & TETRIS_EVENT_MOVE) return 0;
    return -1;
}
#endif

bool tetris_audio_init(TetrisAudio *audio) {
    if (!audio) return false;
    memset(audio, 0, sizeof(*audio));
    audio->enabled = true;
    audio->music_track = 0;
    return open_synth_device(audio);
}

void tetris_audio_shutdown(TetrisAudio *audio) {
    if (!audio) return;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) close_pack_backend(audio);
#endif
    close_synth_device(audio);
    memset(audio, 0, sizeof(*audio));
    audio->music_track = -1;
}

void tetris_audio_play_events(TetrisAudio *audio, uint32_t events) {
    if (!audio || !audio->enabled || events == 0) return;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) {
        const int index = pack_sfx_index(events);
        if (index >= 0 && audio->pack_sfx[index]) {
            Mix_VolumeChunk(audio->pack_sfx[index], MIX_MAX_VOLUME * 3 / 4);
            (void)Mix_PlayChannel(-1, audio->pack_sfx[index], 0);
        }
        return;
    }
#endif
    if (!audio->device) return;
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
    if (!audio) return;
    audio->enabled = !audio->enabled;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) {
        if (audio->enabled) play_pack_music(audio);
        else {
            Mix_HaltMusic();
            Mix_HaltChannel(-1);
        }
        return;
    }
#endif
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    audio->sfx_remaining_samples = 0;
    SDL_UnlockAudioDevice(audio->device);
}

void tetris_audio_cycle_music(TetrisAudio *audio) {
    if (!audio) return;
    ++audio->music_track;
    if (audio->music_track > 2) audio->music_track = -1;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) {
        play_pack_music(audio);
        return;
    }
#endif
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    reset_synth_music(audio);
    SDL_UnlockAudioDevice(audio->device);
}

const char *tetris_audio_music_label(const TetrisAudio *audio) {
    if (!audio || audio->music_track < 0) return "MUSIC OFF";
    if (audio->pack_active) {
        if (audio->music_track == 0) return "OGG 1";
        if (audio->music_track == 1) return "OGG 2";
        return "OGG 3";
    }
    if (audio->music_track == 0) return "MUSIC 1";
    if (audio->music_track == 1) return "MUSIC 2";
    return "MUSIC 3";
}

const char *tetris_audio_backend_label(const TetrisAudio *audio) {
    return audio && audio->pack_active ? "OGG PACK" : "SYNTH";
}

void tetris_audio_apply_settings(TetrisAudio *audio, bool enabled, int music_track) {
    if (!audio) return;
    if (music_track < -1) music_track = -1;
    if (music_track > 2) music_track = 2;
    audio->enabled = enabled;
    audio->music_track = music_track;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) {
        if (enabled) play_pack_music(audio);
        else {
            Mix_HaltMusic();
            Mix_HaltChannel(-1);
        }
        return;
    }
#endif
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    reset_synth_music(audio);
    if (!enabled) audio->sfx_remaining_samples = 0;
    SDL_UnlockAudioDevice(audio->device);
}

bool tetris_audio_load_ogg_pack(TetrisAudio *audio, const char *directory,
                                char *error, size_t error_size) {
#ifndef TETRIS_HAVE_SDL_MIXER
    (void)audio;
    (void)directory;
    if (error && error_size) {
        snprintf(error, error_size,
                 "This build has no SDL2_mixer OGG support.");
    }
    return false;
#else
    char path[2048];
    bool previous_enabled;
    int previous_track;
    int index;
    if (!audio || !directory || !*directory) {
        if (error && error_size) snprintf(error, error_size, "Invalid audio-pack directory.");
        return false;
    }

    previous_enabled = audio->enabled;
    previous_track = audio->music_track;
    if (audio->pack_active) close_pack_backend(audio);
    close_synth_device(audio);

    if ((Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG) == 0) {
        if (error && error_size)
            snprintf(error, error_size, "OGG decoder unavailable: %s", Mix_GetError());
        Mix_Quit();
        (void)open_synth_device(audio);
        return false;
    }
    if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 1024) != 0) {
        if (error && error_size)
            snprintf(error, error_size, "Could not open SDL_mixer audio: %s", Mix_GetError());
        Mix_Quit();
        (void)open_synth_device(audio);
        return false;
    }
    Mix_AllocateChannels(16);

    for (index = 0; index < TETRIS_AUDIO_MUSIC_COUNT; ++index) {
        make_pack_path(path, sizeof(path), directory, PACK_MUSIC_FILES[index]);
        audio->pack_music[index] = Mix_LoadMUS(path);
        if (!audio->pack_music[index]) {
            if (error && error_size)
                snprintf(error, error_size, "Missing or invalid %s: %s",
                         PACK_MUSIC_FILES[index], Mix_GetError());
            free_pack_objects(audio);
            Mix_CloseAudio();
            Mix_Quit();
            (void)open_synth_device(audio);
            audio->enabled = previous_enabled;
            audio->music_track = previous_track;
            return false;
        }
    }
    for (index = 0; index < TETRIS_AUDIO_SFX_COUNT; ++index) {
        make_pack_path(path, sizeof(path), directory, PACK_SFX_FILES[index]);
        audio->pack_sfx[index] = Mix_LoadWAV(path);
        if (!audio->pack_sfx[index]) {
            if (error && error_size)
                snprintf(error, error_size, "Missing or invalid %s: %s",
                         PACK_SFX_FILES[index], Mix_GetError());
            free_pack_objects(audio);
            Mix_CloseAudio();
            Mix_Quit();
            (void)open_synth_device(audio);
            audio->enabled = previous_enabled;
            audio->music_track = previous_track;
            return false;
        }
    }

    audio->pack_active = true;
    audio->enabled = previous_enabled;
    audio->music_track = previous_track;
    snprintf(audio->pack_path, sizeof(audio->pack_path), "%s", directory);
    play_pack_music(audio);
    if (error && error_size) error[0] = '\0';
    return true;
#endif
}
