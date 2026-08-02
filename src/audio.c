#include "audio.h"

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
    if (audio->music_step < 0 || audio->music_step >= length)
        audio->music_step = 0;
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

static int desired_rom_track(const TetrisAudio *audio) {
    int track;
    if (!audio || audio->music_track < 0) return -1;
    track = 3 + audio->music_track;
    if (audio->rom_allegro) track += 3;
    return track;
}

static void sync_rom_music_locked(TetrisAudio *audio) {
    const int desired = desired_rom_track(audio);
    char ignored[128];
    if (!audio || !audio->rom_apu_available ||
        audio->rom_selected_track == desired) return;
    if (desired < 0) {
        tetris_rom_audio_stop_music(&audio->rom_audio);
    } else if (!tetris_rom_audio_select_track(&audio->rom_audio, desired,
                                               ignored, sizeof(ignored))) {
        audio->rom_apu_failed = true;
        audio->rom_apu_active = false;
        return;
    }
    audio->rom_selected_track = desired;
}

static bool refill_rom_frame(TetrisAudio *audio) {
    size_t written = 0;
    char ignored[128];
    if (!audio || !audio->rom_apu_active || !audio->rom_apu_available)
        return false;
    if (!tetris_rom_audio_run_frame(
            &audio->rom_audio, audio->rom_frame_samples,
            TETRIS_AUDIO_ROM_FRAME_CAPACITY, &written,
            ignored, sizeof(ignored)) || written == 0) {
        audio->rom_apu_failed = true;
        audio->rom_apu_active = false;
        audio->rom_frame_count = 0;
        audio->rom_frame_index = 0;
        return false;
    }
    audio->rom_frame_count = written;
    audio->rom_frame_index = 0;
    return true;
}

static float next_rom_sample(TetrisAudio *audio) {
    if (audio->rom_frame_index >= audio->rom_frame_count &&
        !refill_rom_frame(audio)) return 0.0f;
    return audio->rom_frame_samples[audio->rom_frame_index++];
}

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    TetrisAudio *audio = (TetrisAudio *)userdata;
    float *samples = (float *)stream;
    const int count = len / (int)sizeof(float);
    int index;
    for (index = 0; index < count; ++index) {
        float sample = 0.0f;
        if (audio->enabled && audio->rom_apu_active) {
            sample = next_rom_sample(audio);
        } else if (audio->enabled) {
            if (audio->music_track >= 0) {
                if (audio->music_remaining_samples <= 0)
                    load_music_step(audio);
                sample += square_sample(&audio->music_phase_melody,
                                        audio->music_frequency_melody,
                                        audio->sample_rate) * 0.055f;
                sample += triangle_sample(&audio->music_phase_bass,
                                          audio->music_frequency_bass,
                                          audio->sample_rate) * 0.045f;
                --audio->music_remaining_samples;
            }
            if (audio->sfx_remaining_samples > 0 &&
                audio->sfx_frequency > 0.0) {
                sample += square_sample(&audio->sfx_phase,
                                        audio->sfx_frequency,
                                        audio->sample_rate) *
                          audio->sfx_volume;
                --audio->sfx_remaining_samples;
            }
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
    desired.freq = TETRIS_ROM_AUDIO_SAMPLE_RATE;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 512;
    desired.callback = audio_callback;
    desired.userdata = audio;
    audio->device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (!audio->device) return false;
    audio->sample_rate = obtained.freq;
    audio->pack_active = false;
    audio->rom_apu_active = audio->rom_apu_available;
    SDL_PauseAudioDevice(audio->device, 0);
    return true;
}

static void close_synth_device(TetrisAudio *audio) {
    if (audio->device) SDL_CloseAudioDevice(audio->device);
    audio->device = 0;
    audio->rom_apu_active = false;
}

static void reset_synth_music(TetrisAudio *audio) {
    audio->music_step = 0;
    audio->music_remaining_samples = 0;
    audio->music_phase_melody = 0.0;
    audio->music_phase_bass = 0.0;
}

static void start_tone_locked(TetrisAudio *audio, double frequency,
                              int milliseconds, float volume) {
    audio->sfx_frequency = frequency;
    audio->sfx_remaining_samples =
        (audio->sample_rate * milliseconds) / 1000;
    audio->sfx_volume = volume;
    audio->sfx_phase = 0.0;
}

static bool game_uses_allegro(const TetrisGame *game) {
    int x;
    if (!game || game->phase == TETRIS_PHASE_GAME_OVER ||
        game->phase == TETRIS_PHASE_COMPLETE) return false;
    for (x = 0; x < TETRIS_BOARD_W; ++x) {
        if (game->board[5][x] != 0) return true;
    }
    return false;
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
        if (audio->pack_music[index])
            Mix_FreeMusic(audio->pack_music[index]);
        audio->pack_music[index] = NULL;
    }
    for (index = 0; index < TETRIS_AUDIO_SFX_COUNT; ++index) {
        if (audio->pack_sfx[index])
            Mix_FreeChunk(audio->pack_sfx[index]);
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
    audio->rom_selected_track = -2;
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
    audio->rom_selected_track = -2;
}

bool tetris_audio_attach_rom(TetrisAudio *audio, const NesRom *rom,
                             char *error, size_t error_size) {
    bool ok;
    if (!audio || !rom) {
        if (error && error_size)
            snprintf(error, error_size, "invalid live APU arguments");
        return false;
    }
    if (audio->device) SDL_LockAudioDevice(audio->device);
    memset(&audio->rom_audio, 0, sizeof(audio->rom_audio));
    ok = tetris_rom_audio_init(&audio->rom_audio, rom, error, error_size);
    audio->rom_apu_available = ok;
    audio->rom_apu_active = ok && audio->device != 0 && !audio->pack_active;
    audio->rom_apu_failed = !ok;
    audio->rom_frame_count = 0;
    audio->rom_frame_index = 0;
    audio->rom_selected_track = -2;
    audio->rom_allegro = false;
    if (ok) sync_rom_music_locked(audio);
    if (audio->device) SDL_UnlockAudioDevice(audio->device);
    return ok;
}

void tetris_audio_detach_rom(TetrisAudio *audio) {
    if (!audio) return;
    if (audio->device) SDL_LockAudioDevice(audio->device);
    memset(&audio->rom_audio, 0, sizeof(audio->rom_audio));
    audio->rom_apu_available = false;
    audio->rom_apu_active = false;
    audio->rom_apu_failed = false;
    audio->rom_frame_count = 0;
    audio->rom_frame_index = 0;
    audio->rom_selected_track = -2;
    audio->rom_allegro = false;
    if (audio->device) SDL_UnlockAudioDevice(audio->device);
}

void tetris_audio_play_events(TetrisAudio *audio, uint32_t events) {
    if (!audio || !audio->enabled || events == 0) return;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) {
        const int index = pack_sfx_index(events);
        if (index >= 0 && audio->pack_sfx[index]) {
            Mix_VolumeChunk(audio->pack_sfx[index],
                            MIX_MAX_VOLUME * 3 / 4);
            (void)Mix_PlayChannel(-1, audio->pack_sfx[index], 0);
        }
        return;
    }
#endif
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    if (audio->rom_apu_active) {
        tetris_rom_audio_apply_events(&audio->rom_audio, events);
        if (events & TETRIS_EVENT_COMPLETE)
            audio->rom_selected_track = 2;
        SDL_UnlockAudioDevice(audio->device);
        return;
    }
    if (events & TETRIS_EVENT_COMPLETE) {
        start_tone_locked(audio, 880.0, 420, 0.13f);
    } else if (events & TETRIS_EVENT_GAME_OVER) {
        start_tone_locked(audio, 55.0, 500, 0.18f);
    } else if (events & TETRIS_EVENT_LEVEL_UP) {
        start_tone_locked(audio, 660.0, 180, 0.12f);
    } else if (events & TETRIS_EVENT_TETRIS) {
        start_tone_locked(audio, 522.0, 180, 0.14f);
    } else if (events & TETRIS_EVENT_LINE) {
        start_tone_locked(audio, 330.0, 110, 0.12f);
    } else if (events & TETRIS_EVENT_LOCK) {
        start_tone_locked(audio, 75.0, 45, 0.10f);
    } else if (events & TETRIS_EVENT_ROTATE) {
        start_tone_locked(audio, 190.0, 30, 0.08f);
    } else if (events & TETRIS_EVENT_MOVE) {
        start_tone_locked(audio, 115.0, 20, 0.05f);
    }
    SDL_UnlockAudioDevice(audio->device);
}

void tetris_audio_update_game(TetrisAudio *audio, const TetrisGame *game,
                              uint32_t events) {
    const bool allegro = game_uses_allegro(game);
    if (!audio) return;
    if (audio->device && audio->rom_apu_active &&
        allegro != audio->rom_allegro) {
        SDL_LockAudioDevice(audio->device);
        audio->rom_allegro = allegro;
        sync_rom_music_locked(audio);
        audio->rom_frame_count = 0;
        audio->rom_frame_index = 0;
        SDL_UnlockAudioDevice(audio->device);
    }
    tetris_audio_play_events(audio, events);
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
    sync_rom_music_locked(audio);
    audio->rom_frame_count = 0;
    audio->rom_frame_index = 0;
    SDL_UnlockAudioDevice(audio->device);
}

const char *tetris_audio_music_label(const TetrisAudio *audio) {
    if (!audio || audio->music_track < 0) return "MUSIC OFF";
    if (audio->pack_active) {
        if (audio->music_track == 0) return "OGG 1";
        if (audio->music_track == 1) return "OGG 2";
        return "OGG 3";
    }
    if (audio->rom_apu_active) {
        if (audio->music_track == 0) return "NES MUSIC 1";
        if (audio->music_track == 1) return "NES MUSIC 2";
        return "NES MUSIC 3";
    }
    if (audio->music_track == 0) return "MUSIC 1";
    if (audio->music_track == 1) return "MUSIC 2";
    return "MUSIC 3";
}

const char *tetris_audio_backend_label(const TetrisAudio *audio) {
    if (!audio) return "DISABLED";
    if (audio->pack_active) return "OGG PACK";
    if (audio->rom_apu_active) return "ROM APU";
    if (audio->rom_apu_failed) return "SYNTH (APU FALLBACK)";
    return "SYNTH";
}

void tetris_audio_apply_settings(TetrisAudio *audio, bool enabled,
                                 int music_track) {
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
    sync_rom_music_locked(audio);
    audio->rom_frame_count = 0;
    audio->rom_frame_index = 0;
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
        if (error && error_size)
            snprintf(error, error_size, "Invalid audio-pack directory.");
        return false;
    }

    previous_enabled = audio->enabled;
    previous_track = audio->music_track;
    if (audio->pack_active) close_pack_backend(audio);
    close_synth_device(audio);

    if ((Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG) == 0) {
        if (error && error_size)
            snprintf(error, error_size,
                     "OGG decoder unavailable: %s", Mix_GetError());
        Mix_Quit();
        (void)open_synth_device(audio);
        return false;
    }
    if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 1024) != 0) {
        if (error && error_size)
            snprintf(error, error_size,
                     "Could not open SDL_mixer audio: %s", Mix_GetError());
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
    audio->rom_apu_active = false;
    audio->enabled = previous_enabled;
    audio->music_track = previous_track;
    snprintf(audio->pack_path, sizeof(audio->pack_path), "%s", directory);
    play_pack_music(audio);
    if (error && error_size) error[0] = '\0';
    return true;
#endif
}
