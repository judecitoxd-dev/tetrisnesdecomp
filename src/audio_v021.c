#include "audio.h"
#include "audio_ring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct MusicStep {
    float melody;
    float bass;
    unsigned char ticks;
} MusicStep;

/* Newly composed fallback loops; no cartridge note data is embedded. */
static const MusicStep MUSIC_1[] = {
    {329.63f,82.41f,2},{392.00f,82.41f,2},{493.88f,98.00f,2},{392.00f,98.00f,2},
    {349.23f,87.31f,2},{440.00f,87.31f,2},{523.25f,110.00f,2},{440.00f,110.00f,2},
    {392.00f,98.00f,2},{493.88f,98.00f,2},{587.33f,123.47f,2},{493.88f,123.47f,2},
    {349.23f,87.31f,2},{440.00f,87.31f,2},{329.63f,82.41f,2},{261.63f,65.41f,4}
};
static const MusicStep MUSIC_2[] = {
    {261.63f,65.41f,1},{293.66f,65.41f,1},{311.13f,77.78f,1},{392.00f,77.78f,1},
    {369.99f,73.42f,2},{311.13f,73.42f,2},{293.66f,65.41f,1},{349.23f,65.41f,1},
    {440.00f,87.31f,2},{349.23f,87.31f,2},{329.63f,82.41f,1},{392.00f,82.41f,1},
    {493.88f,98.00f,2},{392.00f,98.00f,2},{293.66f,73.42f,2},{246.94f,61.74f,4}
};
static const MusicStep MUSIC_3[] = {
    {220.00f,55.00f,2},{277.18f,69.30f,2},{329.63f,82.41f,2},{415.30f,103.83f,2},
    {392.00f,98.00f,1},{329.63f,82.41f,1},{277.18f,69.30f,2},{246.94f,61.74f,2},
    {293.66f,73.42f,2},{369.99f,92.50f,2},{440.00f,110.00f,2},{554.37f,138.59f,2},
    {493.88f,123.47f,1},{440.00f,110.00f,1},{369.99f,92.50f,2},{220.00f,55.00f,4}
};

#ifdef TETRIS_HAVE_SDL_MIXER
static const char *PACK_MUSIC_FILES[TETRIS_AUDIO_PACK_TRACK_COUNT] = {
    "track_01.ogg", "track_02.ogg", "track_03.ogg", "track_04.ogg",
    "track_05.ogg", "track_06.ogg", "track_07.ogg", "track_08.ogg",
    "track_09.ogg", "track_10.ogg"
};
static const char *PACK_MUSIC_ALIASES[TETRIS_AUDIO_MENU_MUSIC_COUNT] = {
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

static int desired_driver_track(const TetrisAudio *audio) {
    int track;
    if (!audio || audio->music_track < 0) return -1;
    track = 3 + audio->music_track;
    if (audio->rom_allegro) track += 3;
    return track;
}

static size_t atomic_ring_index(const SDL_atomic_t *value) {
    return (size_t)(unsigned int)SDL_AtomicGet((SDL_atomic_t *)value);
}

static size_t ring_used(const TetrisAudio *audio) {
    return tetris_audio_ring_used(atomic_ring_index(&audio->rom_ring_read),
                                  atomic_ring_index(&audio->rom_ring_write),
                                  audio->rom_ring_capacity);
}

static void ring_clear(TetrisAudio *audio) {
    const int write_index = SDL_AtomicGet(&audio->rom_ring_write);
    SDL_AtomicSet(&audio->rom_ring_read, write_index);
}

static bool ring_push(TetrisAudio *audio, const float *samples, size_t count) {
    size_t read_index;
    size_t write_index;
    size_t index;
    if (!audio || !audio->rom_ring_samples || !samples || count == 0u)
        return false;
    read_index = atomic_ring_index(&audio->rom_ring_read);
    write_index = atomic_ring_index(&audio->rom_ring_write);
    if (tetris_audio_ring_free(read_index, write_index,
                               audio->rom_ring_capacity) < count) return false;
    for (index = 0; index < count; ++index) {
        audio->rom_ring_samples[write_index] = samples[index];
        write_index = tetris_audio_ring_advance(
            write_index, 1u, audio->rom_ring_capacity);
    }
    SDL_MemoryBarrierRelease();
    SDL_AtomicSet(&audio->rom_ring_write, (int)write_index);
    return true;
}

static bool producer_select_track(TetrisAudio *audio, int track) {
    char error[128];
    if (track < 0) {
        tetris_rom_audio_stop_music(&audio->rom_audio);
        return true;
    }
    return tetris_rom_audio_select_track(&audio->rom_audio, track,
                                          error, sizeof(error));
}

static void producer_fail(TetrisAudio *audio) {
    SDL_AtomicSet(&audio->rom_producer_failed, 1);
    SDL_AtomicSet(&audio->rom_run_requested, 0);
}

static int rom_producer_thread(void *userdata) {
    TetrisAudio *audio = (TetrisAudio *)userdata;
    float frame[TETRIS_AUDIO_ROM_FRAME_CAPACITY];
    int selected_track = -2;
    (void)SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    while (!SDL_AtomicGet(&audio->rom_stop_requested)) {
        int requested_track;
        uint32_t events;
        bool flush;
        size_t written = 0u;
        char error[128];

        if (!SDL_AtomicGet(&audio->rom_run_requested) ||
            SDL_AtomicGet(&audio->rom_producer_failed)) {
            SDL_Delay(2u);
            continue;
        }

        SDL_LockMutex(audio->rom_mutex);
        requested_track = audio->rom_requested_track;
        events = audio->rom_pending_events;
        flush = audio->rom_flush_requested;
        audio->rom_pending_events = 0u;
        audio->rom_flush_requested = false;
        SDL_UnlockMutex(audio->rom_mutex);

        if (requested_track != selected_track) {
            ring_clear(audio);
            if (!producer_select_track(audio, requested_track)) {
                producer_fail(audio);
                continue;
            }
            selected_track = requested_track;
            audio->rom_selected_track = selected_track;
            flush = false;
        }
        if (flush) ring_clear(audio);
        if (events != 0u)
            tetris_rom_audio_apply_events(&audio->rom_audio, events);

        if (ring_used(audio) >= audio->rom_ring_target) {
            SDL_Delay(1u);
            continue;
        }
        if (tetris_audio_ring_free(
                atomic_ring_index(&audio->rom_ring_read),
                atomic_ring_index(&audio->rom_ring_write),
                audio->rom_ring_capacity) < TETRIS_AUDIO_ROM_FRAME_CAPACITY) {
            SDL_Delay(1u);
            continue;
        }
        if (!tetris_rom_audio_run_frame(
                &audio->rom_audio, frame, TETRIS_AUDIO_ROM_FRAME_CAPACITY,
                &written, error, sizeof(error)) || written == 0u) {
            producer_fail(audio);
            continue;
        }
        if (!ring_push(audio, frame, written)) SDL_Delay(1u);
    }
    return 0;
}

static size_t configured_ring_target(void) {
    size_t target = TETRIS_AUDIO_RING_TARGET_DEFAULT;
    const char *value = SDL_getenv("TETRIS_AUDIO_RING_TARGET");
    if (value && *value) {
        char *end = NULL;
        const unsigned long parsed = strtoul(value, &end, 10);
        if (end && *end == '\0' && parsed >= 2048ul &&
            parsed <= (unsigned long)(TETRIS_AUDIO_RING_CAPACITY - 2048u))
            target = (size_t)parsed;
    }
    return target;
}

static bool ensure_producer_resources(TetrisAudio *audio,
                                      char *error, size_t error_size) {
    if (!audio->rom_ring_samples) {
        audio->rom_ring_samples = (float *)malloc(
            TETRIS_AUDIO_RING_CAPACITY * sizeof(float));
        if (!audio->rom_ring_samples) {
            if (error && error_size)
                snprintf(error, error_size, "could not allocate APU ring buffer");
            return false;
        }
        audio->rom_ring_capacity = TETRIS_AUDIO_RING_CAPACITY;
        audio->rom_ring_target = configured_ring_target();
    }
    if (!audio->rom_mutex) {
        audio->rom_mutex = SDL_CreateMutex();
        if (!audio->rom_mutex) {
            if (error && error_size)
                snprintf(error, error_size, "could not create APU mutex: %s",
                         SDL_GetError());
            return false;
        }
    }
    return true;
}

static bool start_rom_producer(TetrisAudio *audio,
                               char *error, size_t error_size) {
    if (audio->rom_thread) return true;
    if (!ensure_producer_resources(audio, error, error_size)) return false;
    SDL_AtomicSet(&audio->rom_ring_read, 0);
    SDL_AtomicSet(&audio->rom_ring_write, 0);
    SDL_AtomicSet(&audio->rom_stop_requested, 0);
    SDL_AtomicSet(&audio->rom_producer_failed, 0);
    SDL_AtomicSet(&audio->rom_underruns, 0);
    audio->rom_thread = SDL_CreateThread(
        rom_producer_thread, "tetris-rom-apu", audio);
    if (!audio->rom_thread) {
        if (error && error_size)
            snprintf(error, error_size, "could not start APU producer: %s",
                     SDL_GetError());
        return false;
    }
    return true;
}

static void stop_rom_producer(TetrisAudio *audio) {
    if (!audio) return;
    SDL_AtomicSet(&audio->rom_run_requested, 0);
    if (audio->rom_thread) {
        SDL_AtomicSet(&audio->rom_stop_requested, 1);
        SDL_WaitThread(audio->rom_thread, NULL);
        audio->rom_thread = NULL;
    }
    SDL_AtomicSet(&audio->rom_stop_requested, 0);
    ring_clear(audio);
}

static void request_rom_control(TetrisAudio *audio, uint32_t events,
                                bool flush) {
    if (!audio || !audio->rom_mutex) return;
    SDL_LockMutex(audio->rom_mutex);
    audio->rom_requested_track = desired_driver_track(audio);
    audio->rom_pending_events |= events;
    audio->rom_flush_requested = audio->rom_flush_requested || flush;
    SDL_UnlockMutex(audio->rom_mutex);
}

static void audio_callback(void *userdata, Uint8 *stream, int length) {
    TetrisAudio *audio = (TetrisAudio *)userdata;
    float *samples = (float *)stream;
    const int count = length / (int)sizeof(float);
    const bool rom_running =
        SDL_AtomicGet(&audio->rom_run_requested) != 0 &&
        SDL_AtomicGet(&audio->rom_producer_failed) == 0;
    size_t read_index = atomic_ring_index(&audio->rom_ring_read);
    size_t write_index = atomic_ring_index(&audio->rom_ring_write);
    bool underrun = false;
    int index;

    SDL_MemoryBarrierAcquire();
    for (index = 0; index < count; ++index) {
        float sample = 0.0f;
        if (audio->enabled && rom_running) {
            if (read_index != write_index && audio->rom_ring_samples) {
                sample = audio->rom_ring_samples[read_index];
                read_index = tetris_audio_ring_advance(
                    read_index, 1u, audio->rom_ring_capacity);
            } else {
                underrun = true;
            }
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
    if (rom_running)
        SDL_AtomicSet(&audio->rom_ring_read, (int)read_index);
    if (underrun) (void)SDL_AtomicAdd(&audio->rom_underruns, 1);
}

static bool open_synth_device(TetrisAudio *audio) {
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    if (audio->device) return true;
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
    SDL_AtomicSet(&audio->rom_run_requested,
                  audio->rom_apu_active ? 1 : 0);
    SDL_PauseAudioDevice(audio->device, 0);
    return true;
}

static void close_synth_device(TetrisAudio *audio) {
    SDL_AtomicSet(&audio->rom_run_requested, 0);
    if (audio->device) SDL_CloseAudioDevice(audio->device);
    audio->device = 0;
    audio->rom_apu_active = false;
}

#ifdef TETRIS_HAVE_SDL_MIXER
static void make_pack_path(char *destination, size_t destination_size,
                           const char *directory, const char *filename) {
    const size_t length = strlen(directory);
    const bool has_separator = length > 0u &&
        (directory[length - 1u] == '/' || directory[length - 1u] == '\\');
    snprintf(destination, destination_size, "%s%s%s", directory,
             has_separator ? "" : "/", filename);
}

static void free_pack_objects(TetrisAudio *audio) {
    int index;
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    for (index = 0; index < TETRIS_AUDIO_PACK_TRACK_COUNT; ++index) {
        if (audio->pack_music[index]) Mix_FreeMusic(audio->pack_music[index]);
        audio->pack_music[index] = NULL;
    }
    for (index = 0; index < TETRIS_AUDIO_SFX_COUNT; ++index) {
        if (audio->pack_sfx[index]) Mix_FreeChunk(audio->pack_sfx[index]);
        audio->pack_sfx[index] = NULL;
    }
    audio->pack_selected_track = -2;
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
    const int driver_track = desired_driver_track(audio);
    const int index = driver_track - 1;
    if (!audio->pack_active) return;
    if (!audio->enabled || driver_track < 1 ||
        index >= TETRIS_AUDIO_PACK_TRACK_COUNT ||
        !audio->pack_music[index]) {
        Mix_HaltMusic();
        audio->pack_selected_track = -1;
        return;
    }
    if (audio->pack_selected_track == driver_track && Mix_PlayingMusic())
        return;
    Mix_HaltMusic();
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
    if (Mix_PlayMusic(audio->pack_music[index], -1) != 0) {
        fprintf(stderr, "Could not play cached OGG track: %s\n",
                Mix_GetError());
        audio->pack_selected_track = -2;
        return;
    }
    audio->pack_selected_track = driver_track;
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
    audio->rom_requested_track = 3;
    audio->rom_selected_track = -2;
    audio->pack_selected_track = -2;
    SDL_AtomicSet(&audio->rom_ring_read, 0);
    SDL_AtomicSet(&audio->rom_ring_write, 0);
    SDL_AtomicSet(&audio->rom_stop_requested, 0);
    SDL_AtomicSet(&audio->rom_run_requested, 0);
    SDL_AtomicSet(&audio->rom_producer_failed, 0);
    SDL_AtomicSet(&audio->rom_underruns, 0);
    return open_synth_device(audio);
}

void tetris_audio_shutdown(TetrisAudio *audio) {
    if (!audio) return;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) close_pack_backend(audio);
#endif
    close_synth_device(audio);
    stop_rom_producer(audio);
    if (audio->rom_mutex) SDL_DestroyMutex(audio->rom_mutex);
    free(audio->rom_ring_samples);
    memset(audio, 0, sizeof(*audio));
    audio->music_track = -1;
    audio->rom_selected_track = -2;
    audio->pack_selected_track = -2;
}

bool tetris_audio_attach_rom(TetrisAudio *audio, const NesRom *rom,
                             char *error, size_t error_size) {
    bool ok;
    if (!audio || !rom) {
        if (error && error_size)
            snprintf(error, error_size, "invalid buffered APU arguments");
        return false;
    }
    SDL_AtomicSet(&audio->rom_run_requested, 0);
    stop_rom_producer(audio);
    memset(&audio->rom_audio, 0, sizeof(audio->rom_audio));
    ok = tetris_rom_audio_init(&audio->rom_audio, rom, error, error_size);
    audio->rom_apu_available = ok;
    audio->rom_apu_failed = !ok;
    audio->rom_allegro = false;
    audio->rom_selected_track = -2;
    audio->rom_requested_track = desired_driver_track(audio);
    audio->rom_pending_events = 0u;
    audio->rom_flush_requested = true;
    if (!ok) {
        audio->rom_apu_active = false;
        return false;
    }
    if (!start_rom_producer(audio, error, error_size)) {
        audio->rom_apu_available = false;
        audio->rom_apu_active = false;
        audio->rom_apu_failed = true;
        return false;
    }
    audio->rom_apu_active = audio->device != 0 && !audio->pack_active;
    SDL_AtomicSet(&audio->rom_run_requested,
                  audio->rom_apu_active ? 1 : 0);
    if (error && error_size) error[0] = '\0';
    return true;
}

void tetris_audio_detach_rom(TetrisAudio *audio) {
    if (!audio) return;
    SDL_AtomicSet(&audio->rom_run_requested, 0);
    stop_rom_producer(audio);
    memset(&audio->rom_audio, 0, sizeof(audio->rom_audio));
    audio->rom_apu_available = false;
    audio->rom_apu_active = false;
    audio->rom_apu_failed = false;
    audio->rom_selected_track = -2;
    audio->rom_requested_track = -1;
    audio->rom_pending_events = 0u;
    audio->rom_flush_requested = false;
    SDL_AtomicSet(&audio->rom_producer_failed, 0);
    SDL_AtomicSet(&audio->rom_ring_read, 0);
    SDL_AtomicSet(&audio->rom_ring_write, 0);
}

void tetris_audio_play_events(TetrisAudio *audio, uint32_t events) {
    if (!audio || !audio->enabled || events == 0u) return;
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
    if (audio->rom_apu_active &&
        !SDL_AtomicGet(&audio->rom_producer_failed)) {
        request_rom_control(audio, events, false);
        return;
    }
    if (!audio->device) return;
    SDL_LockAudioDevice(audio->device);
    if (events & TETRIS_EVENT_COMPLETE)
        start_tone_locked(audio, 880.0, 420, 0.13f);
    else if (events & TETRIS_EVENT_GAME_OVER)
        start_tone_locked(audio, 55.0, 500, 0.18f);
    else if (events & TETRIS_EVENT_LEVEL_UP)
        start_tone_locked(audio, 660.0, 180, 0.12f);
    else if (events & TETRIS_EVENT_TETRIS)
        start_tone_locked(audio, 522.0, 180, 0.14f);
    else if (events & TETRIS_EVENT_LINE)
        start_tone_locked(audio, 330.0, 110, 0.12f);
    else if (events & TETRIS_EVENT_LOCK)
        start_tone_locked(audio, 75.0, 45, 0.10f);
    else if (events & TETRIS_EVENT_ROTATE)
        start_tone_locked(audio, 190.0, 30, 0.08f);
    else if (events & TETRIS_EVENT_MOVE)
        start_tone_locked(audio, 115.0, 20, 0.05f);
    SDL_UnlockAudioDevice(audio->device);
}

void tetris_audio_update_game(TetrisAudio *audio, const TetrisGame *game,
                              uint32_t events) {
    const bool allegro = game_uses_allegro(game);
    if (!audio) return;
    if (allegro != audio->rom_allegro) {
        audio->rom_allegro = allegro;
#ifdef TETRIS_HAVE_SDL_MIXER
        if (audio->pack_active) play_pack_music(audio);
        else
#endif
        if (audio->rom_apu_active)
            request_rom_control(audio, 0u, true);
    }
    tetris_audio_play_events(audio, events);
}

void tetris_audio_toggle(TetrisAudio *audio) {
    if (!audio) return;
    if (audio->device) SDL_LockAudioDevice(audio->device);
    audio->enabled = !audio->enabled;
    audio->sfx_remaining_samples = 0;
    if (audio->device) SDL_UnlockAudioDevice(audio->device);
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) {
        if (audio->enabled) play_pack_music(audio);
        else {
            Mix_HaltMusic();
            Mix_HaltChannel(-1);
        }
    }
#endif
}

void tetris_audio_cycle_music(TetrisAudio *audio) {
    if (!audio) return;
    ++audio->music_track;
    if (audio->music_track >= TETRIS_AUDIO_MENU_MUSIC_COUNT)
        audio->music_track = -1;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (audio->pack_active) {
        play_pack_music(audio);
        return;
    }
#endif
    if (audio->device) SDL_LockAudioDevice(audio->device);
    reset_synth_music(audio);
    if (audio->device) SDL_UnlockAudioDevice(audio->device);
    request_rom_control(audio, 0u, true);
}

const char *tetris_audio_music_label(const TetrisAudio *audio) {
    if (!audio || audio->music_track < 0) return "MUSIC OFF";
    if (audio->pack_active) {
        if (audio->music_track == 0) return "OGG MUSIC 1";
        if (audio->music_track == 1) return "OGG MUSIC 2";
        return "OGG MUSIC 3";
    }
    if (audio->rom_apu_active &&
        !SDL_AtomicGet((SDL_atomic_t *)&audio->rom_producer_failed)) {
        if (audio->music_track == 0) return "NES MUSIC 1 BUFFERED";
        if (audio->music_track == 1) return "NES MUSIC 2 BUFFERED";
        return "NES MUSIC 3 BUFFERED";
    }
    if (audio->music_track == 0) return "MUSIC 1";
    if (audio->music_track == 1) return "MUSIC 2";
    return "MUSIC 3";
}

const char *tetris_audio_backend_label(const TetrisAudio *audio) {
    if (!audio) return "DISABLED";
    if (audio->pack_active) return "OGG CACHE";
    if (audio->rom_apu_active &&
        !SDL_AtomicGet((SDL_atomic_t *)&audio->rom_producer_failed))
        return "ROM APU BUFFERED";
    if (audio->rom_apu_failed ||
        SDL_AtomicGet((SDL_atomic_t *)&audio->rom_producer_failed))
        return "SYNTH (APU FALLBACK)";
    return "SYNTH";
}

uint32_t tetris_audio_underrun_count(const TetrisAudio *audio) {
    if (!audio) return 0u;
    return (uint32_t)SDL_AtomicGet((SDL_atomic_t *)&audio->rom_underruns);
}

void tetris_audio_apply_settings(TetrisAudio *audio, bool enabled,
                                 int music_track) {
    if (!audio) return;
    if (music_track < -1) music_track = -1;
    if (music_track >= TETRIS_AUDIO_MENU_MUSIC_COUNT)
        music_track = TETRIS_AUDIO_MENU_MUSIC_COUNT - 1;
    if (audio->device) SDL_LockAudioDevice(audio->device);
    audio->enabled = enabled;
    audio->music_track = music_track;
    reset_synth_music(audio);
    if (!enabled) audio->sfx_remaining_samples = 0;
    if (audio->device) SDL_UnlockAudioDevice(audio->device);
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
    request_rom_control(audio, 0u, true);
}

bool tetris_audio_load_ogg_pack(TetrisAudio *audio, const char *directory,
                                char *error, size_t error_size) {
#ifndef TETRIS_HAVE_SDL_MIXER
    (void)audio;
    (void)directory;
    if (error && error_size)
        snprintf(error, error_size,
                 "This build has no SDL2_mixer OGG support.");
    return false;
#else
    char path[2048];
    bool previous_enabled;
    int previous_track;
    int index;
    if (!audio || !directory || !*directory) {
        if (error && error_size)
            snprintf(error, error_size, "Invalid audio-cache directory.");
        return false;
    }

    previous_enabled = audio->enabled;
    previous_track = audio->music_track;
    if (audio->pack_active) close_pack_backend(audio);
    SDL_AtomicSet(&audio->rom_run_requested, 0);
    close_synth_device(audio);

    if ((Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG) == 0) {
        if (error && error_size)
            snprintf(error, error_size, "OGG decoder unavailable: %s",
                     Mix_GetError());
        Mix_Quit();
        (void)open_synth_device(audio);
        return false;
    }
    if (Mix_OpenAudio(TETRIS_ROM_AUDIO_SAMPLE_RATE, AUDIO_S16SYS, 2, 1024) != 0) {
        if (error && error_size)
            snprintf(error, error_size, "Could not open SDL_mixer: %s",
                     Mix_GetError());
        Mix_Quit();
        (void)open_synth_device(audio);
        return false;
    }
    Mix_AllocateChannels(16);

    for (index = 0; index < TETRIS_AUDIO_PACK_TRACK_COUNT; ++index) {
        make_pack_path(path, sizeof(path), directory, PACK_MUSIC_FILES[index]);
        audio->pack_music[index] = Mix_LoadMUS(path);
        if (!audio->pack_music[index] && index >= 2 && index <= 4) {
            make_pack_path(path, sizeof(path), directory,
                           PACK_MUSIC_ALIASES[index - 2]);
            audio->pack_music[index] = Mix_LoadMUS(path);
        }
        if (!audio->pack_music[index]) {
            if (error && error_size)
                snprintf(error, error_size, "Missing or invalid %s: %s",
                         PACK_MUSIC_FILES[index], Mix_GetError());
            free_pack_objects(audio);
            Mix_CloseAudio();
            Mix_Quit();
            audio->enabled = previous_enabled;
            audio->music_track = previous_track;
            (void)open_synth_device(audio);
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
            audio->enabled = previous_enabled;
            audio->music_track = previous_track;
            (void)open_synth_device(audio);
            return false;
        }
    }

    audio->pack_active = true;
    audio->rom_apu_active = false;
    audio->enabled = previous_enabled;
    audio->music_track = previous_track;
    audio->pack_selected_track = -2;
    snprintf(audio->pack_path, sizeof(audio->pack_path), "%s", directory);
    play_pack_music(audio);
    if (error && error_size) error[0] = '\0';
    return true;
#endif
}
