#ifndef TETRIS_ROM_AUDIO_H
#define TETRIS_ROM_AUDIO_H

#include "cpu6502.h"
#include "nes_apu.h"
#include "rom.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TETRIS_ROM_AUDIO_SAMPLE_RATE 48000
#define TETRIS_ROM_AUDIO_TRACK_MIN 1
#define TETRIS_ROM_AUDIO_TRACK_MAX 10
#define TETRIS_ROM_AUDIO_RAM_SIZE 0x0800u
#define TETRIS_ROM_AUDIO_MAX_FRAME_WRITES 256u

typedef struct TetrisApuWrite {
    uint16_t address;
    uint8_t value;
} TetrisApuWrite;

typedef struct TetrisRomAudio {
    uint8_t ram[TETRIS_ROM_AUDIO_RAM_SIZE];
    const uint8_t *prg;
    size_t prg_size;
    Cpu6502 cpu;
    NesApu apu;
    double sample_fraction;
    uint64_t rendered_frames;
    uint64_t apu_write_count;
    uint64_t apu_write_hash;
    TetrisApuWrite frame_writes[TETRIS_ROM_AUDIO_MAX_FRAME_WRITES];
    size_t frame_write_count;
    bool frame_writes_overflow;
    bool initialized;
} TetrisRomAudio;

bool tetris_rom_audio_init(TetrisRomAudio *audio, const NesRom *rom,
                           char *error, size_t error_size);
/* Test/tool entry point for an already mapped 32 KiB PRG image. */
bool tetris_rom_audio_init_prg(TetrisRomAudio *audio,
                               const uint8_t *prg, size_t prg_size,
                               char *error, size_t error_size);
void tetris_rom_audio_reset(TetrisRomAudio *audio);
bool tetris_rom_audio_select_track(TetrisRomAudio *audio, int track,
                                   char *error, size_t error_size);
void tetris_rom_audio_stop_music(TetrisRomAudio *audio);
bool tetris_rom_audio_run_frame(TetrisRomAudio *audio,
                                float *samples, size_t capacity,
                                size_t *written,
                                char *error, size_t error_size);
void tetris_rom_audio_set_sound_effect(TetrisRomAudio *audio,
                                       int slot, uint8_t effect);
void tetris_rom_audio_apply_events(TetrisRomAudio *audio, uint32_t events);
uint8_t tetris_rom_audio_ram(const TetrisRomAudio *audio, uint16_t address);

#endif
