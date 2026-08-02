#ifndef TETRIS_AUDIO_H
#define TETRIS_AUDIO_H

#include "game.h"
#include "rom_audio.h"

#include <SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef TETRIS_HAVE_SDL_MIXER
#include <SDL_mixer.h>
#endif

#define TETRIS_AUDIO_PACK_PATH_LENGTH 1023
#define TETRIS_AUDIO_MENU_MUSIC_COUNT 3
#define TETRIS_AUDIO_PACK_TRACK_COUNT 10
#define TETRIS_AUDIO_MUSIC_COUNT TETRIS_AUDIO_PACK_TRACK_COUNT
#define TETRIS_AUDIO_SFX_COUNT 8
#define TETRIS_AUDIO_ROM_FRAME_CAPACITY 1024u
#define TETRIS_AUDIO_RING_CAPACITY 32768u
#define TETRIS_AUDIO_RING_TARGET_DEFAULT 8192u

/*
 * The SDL callback must only copy ready samples. The original 6502/APU driver
 * is rendered on a producer thread and stored in a single-producer,
 * single-consumer ring buffer.
 */
SDL_AudioDeviceID tetris_open_audio_device_buffered(
    const char *device, int iscapture, const SDL_AudioSpec *desired,
    SDL_AudioSpec *obtained, int allowed_changes);

#ifndef TETRIS_AUDIO_DEVICE_IMPLEMENTATION
#define SDL_OpenAudioDevice(device, iscapture, desired, obtained, allowed) \
    tetris_open_audio_device_buffered((device), (iscapture), (desired), \
                                      (obtained), (allowed))
#endif

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

    TetrisRomAudio rom_audio;
    float *rom_ring_samples;
    size_t rom_ring_capacity;
    size_t rom_ring_target;
    SDL_atomic_t rom_ring_read;
    SDL_atomic_t rom_ring_write;
    SDL_atomic_t rom_stop_requested;
    SDL_atomic_t rom_run_requested;
    SDL_atomic_t rom_producer_failed;
    SDL_atomic_t rom_underruns;
    SDL_Thread *rom_thread;
    SDL_mutex *rom_mutex;
    uint32_t rom_pending_events;
    int rom_requested_track;
    int rom_selected_track;
    bool rom_flush_requested;
    bool rom_apu_available;
    bool rom_apu_active;
    bool rom_apu_failed;
    bool rom_allegro;

#ifdef TETRIS_HAVE_SDL_MIXER
    Mix_Music *pack_music[TETRIS_AUDIO_PACK_TRACK_COUNT];
    Mix_Chunk *pack_sfx[TETRIS_AUDIO_SFX_COUNT];
#endif
    int pack_selected_track;
    char pack_path[TETRIS_AUDIO_PACK_PATH_LENGTH + 1];
    bool pack_active;
    bool enabled;
} TetrisAudio;

bool tetris_audio_init(TetrisAudio *audio);
void tetris_audio_shutdown(TetrisAudio *audio);
bool tetris_audio_attach_rom(TetrisAudio *audio, const NesRom *rom,
                             char *error, size_t error_size);
void tetris_audio_detach_rom(TetrisAudio *audio);
void tetris_audio_play_events(TetrisAudio *audio, uint32_t events);
void tetris_audio_update_game(TetrisAudio *audio, const TetrisGame *game,
                              uint32_t events);
void tetris_audio_toggle(TetrisAudio *audio);
void tetris_audio_cycle_music(TetrisAudio *audio);
void tetris_audio_apply_settings(TetrisAudio *audio, bool enabled,
                                 int music_track);
const char *tetris_audio_music_label(const TetrisAudio *audio);
const char *tetris_audio_backend_label(const TetrisAudio *audio);
uint32_t tetris_audio_underrun_count(const TetrisAudio *audio);

/* Loads the user-created Ogg Vorbis cache generated from their legal ROM. */
bool tetris_audio_load_ogg_pack(TetrisAudio *audio, const char *directory,
                                char *error, size_t error_size);

#endif
