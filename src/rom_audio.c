#include "rom_audio.h"
#include "game.h"

#include <stdio.h>
#include <string.h>

#define UPDATE_AUDIO_ENTRY 0xE000u
#define INIT_AUDIO_ENTRY 0xE006u
#define MUSIC_TRACK_ADDR 0x06F5u
#define SOUND_EFFECT_SLOT0_ADDR 0x06F0u
#define NTSC_FRAME_RATE 60.0988
#define NTSC_CPU_CLOCK 1789773.0
#define CALL_INSTRUCTION_LIMIT 200000u
#define FNV_OFFSET UINT64_C(1469598103934665603)
#define FNV_PRIME UINT64_C(1099511628211)

static void set_error(char *error, size_t error_size, const char *message) {
    if (error && error_size) snprintf(error, error_size, "%s", message);
}

static uint8_t mapped_read(void *userdata, uint16_t address) {
    TetrisRomAudio *audio = (TetrisRomAudio *)userdata;
    if (address < 0x2000u) return audio->ram[address & 0x07FFu];
    if (address == 0x4015u) return nes_apu_read_status(&audio->apu);
    if (address >= 0x8000u && audio->prg && audio->prg_size == 0x8000u)
        return audio->prg[address - 0x8000u];
    return 0;
}

static void hash_apu_write(TetrisRomAudio *audio,
                           uint16_t address, uint8_t value) {
    const uint8_t bytes[3] = {
        (uint8_t)(address >> 8), (uint8_t)address, value
    };
    size_t index;
    for (index = 0; index < sizeof(bytes); ++index) {
        audio->apu_write_hash ^= bytes[index];
        audio->apu_write_hash *= FNV_PRIME;
    }
    ++audio->apu_write_count;
    if (audio->frame_write_count < TETRIS_ROM_AUDIO_MAX_FRAME_WRITES) {
        TetrisApuWrite *write = &audio->frame_writes[audio->frame_write_count++];
        write->address = address;
        write->value = value;
    } else {
        audio->frame_writes_overflow = true;
    }
}

static void mapped_write(void *userdata, uint16_t address, uint8_t value) {
    TetrisRomAudio *audio = (TetrisRomAudio *)userdata;
    if (address < 0x2000u) {
        audio->ram[address & 0x07FFu] = value;
        return;
    }
    if (address >= 0x4000u && address <= 0x4017u) {
        hash_apu_write(audio, address, value);
        nes_apu_write(&audio->apu, address, value);
    }
}

static void mapped_cycles(void *userdata, unsigned cycles) {
    TetrisRomAudio *audio = (TetrisRomAudio *)userdata;
    uint32_t stalls;
    nes_apu_advance_cycles(&audio->apu, cycles);
    do {
        stalls = nes_apu_consume_stall_cycles(&audio->apu);
        if (stalls) {
            audio->last_stall_cycles += stalls;
            audio->cpu.cycles += stalls;
            nes_apu_advance_cycles(&audio->apu, stalls);
        }
    } while (stalls != 0);
}

static bool init_common(TetrisRomAudio *audio,
                        const uint8_t *prg, size_t prg_size,
                        char *error, size_t error_size) {
    if (!audio || !prg) {
        set_error(error, error_size, "invalid ROM audio arguments");
        return false;
    }
    if (prg_size != 0x8000u) {
        set_error(error, error_size,
                  "the original audio driver requires a 32 KiB PRG image");
        return false;
    }
    memset(audio, 0, sizeof(*audio));
    audio->prg = prg;
    audio->prg_size = prg_size;
    audio->apu_write_hash = FNV_OFFSET;
    nes_apu_init(&audio->apu, TETRIS_ROM_AUDIO_SAMPLE_RATE,
                 mapped_read, audio);
    cpu6502_init(&audio->cpu, mapped_read, mapped_write, audio);
    cpu6502_set_cycle_callback(&audio->cpu, mapped_cycles);
    if (!cpu6502_call(&audio->cpu, INIT_AUDIO_ENTRY,
                      CALL_INSTRUCTION_LIMIT, error,
                      (unsigned)error_size)) {
        return false;
    }
    audio->initialized = true;
    if (error && error_size) error[0] = '\0';
    return true;
}

bool tetris_rom_audio_init(TetrisRomAudio *audio, const NesRom *rom,
                           char *error, size_t error_size) {
    if (!rom || !rom->prg) {
        set_error(error, error_size, "no ROM is loaded");
        return false;
    }
    if (!rom->exact_supported_dump) {
        if (error && error_size) {
            snprintf(error, error_size,
                     "original APU offsets are verified only for CRC32 D16EA396; got %08X",
                     rom->crc32);
        }
        return false;
    }
    return init_common(audio, rom->prg, rom->prg_size, error, error_size);
}

bool tetris_rom_audio_init_prg(TetrisRomAudio *audio,
                               const uint8_t *prg, size_t prg_size,
                               char *error, size_t error_size) {
    return init_common(audio, prg, prg_size, error, error_size);
}

void tetris_rom_audio_reset(TetrisRomAudio *audio) {
    const uint8_t *prg;
    size_t prg_size;
    char ignored[128];
    if (!audio) return;
    prg = audio->prg;
    prg_size = audio->prg_size;
    if (!prg || prg_size != 0x8000u) {
        memset(audio, 0, sizeof(*audio));
        return;
    }
    (void)init_common(audio, prg, prg_size, ignored, sizeof(ignored));
}

bool tetris_rom_audio_select_track(TetrisRomAudio *audio, int track,
                                   char *error, size_t error_size) {
    if (!audio || !audio->initialized) {
        set_error(error, error_size, "ROM audio driver is not initialized");
        return false;
    }
    if (track < TETRIS_ROM_AUDIO_TRACK_MIN ||
        track > TETRIS_ROM_AUDIO_TRACK_MAX) {
        set_error(error, error_size, "track must be between 1 and 10");
        return false;
    }
    audio->ram[MUSIC_TRACK_ADDR] = (uint8_t)track;
    if (error && error_size) error[0] = '\0';
    return true;
}

void tetris_rom_audio_stop_music(TetrisRomAudio *audio) {
    if (!audio || !audio->initialized) return;
    audio->ram[MUSIC_TRACK_ADDR] = 0xFFu;
}

bool tetris_rom_audio_run_frame(TetrisRomAudio *audio,
                                float *samples, size_t capacity,
                                size_t *written,
                                char *error, size_t error_size) {
    const double exact_samples =
        (double)TETRIS_ROM_AUDIO_SAMPLE_RATE / NTSC_FRAME_RATE;
    const double exact_cycles = NTSC_CPU_CLOCK / NTSC_FRAME_RATE;
    const uint64_t before_cycles = audio ? audio->cpu.cycles : 0;
    uint32_t frame_cycles;
    uint32_t driver_cycles;
    uint32_t idle_cycles;
    size_t count;
    if (written) *written = 0;
    if (!audio || !audio->initialized || !samples) {
        set_error(error, error_size, "invalid ROM audio frame arguments");
        return false;
    }
    audio->frame_write_count = 0;
    audio->frame_writes_overflow = false;
    audio->last_stall_cycles = 0;
    if (!cpu6502_call(&audio->cpu, UPDATE_AUDIO_ENTRY,
                      CALL_INSTRUCTION_LIMIT, error,
                      (unsigned)error_size)) {
        return false;
    }
    driver_cycles = (uint32_t)(audio->cpu.cycles - before_cycles);
    audio->cycle_fraction += exact_cycles;
    frame_cycles = (uint32_t)audio->cycle_fraction;
    audio->cycle_fraction -= (double)frame_cycles;
    if (frame_cycles < driver_cycles) frame_cycles = driver_cycles;
    idle_cycles = frame_cycles - driver_cycles;

    audio->sample_fraction += exact_samples;
    count = (size_t)audio->sample_fraction;
    audio->sample_fraction -= (double)count;
    if (count > capacity) {
        set_error(error, error_size, "audio frame buffer is too small");
        return false;
    }
    nes_apu_render_cycles(&audio->apu, samples, count, idle_cycles);
    (void)nes_apu_consume_stall_cycles(&audio->apu);
    audio->last_frame_cpu_cycles = frame_cycles;
    audio->last_driver_cycles = driver_cycles;
    audio->rendered_cpu_cycles += frame_cycles;
    ++audio->rendered_frames;
    if (written) *written = count;
    if (error && error_size) error[0] = '\0';
    return true;
}

void tetris_rom_audio_set_sound_effect(TetrisRomAudio *audio,
                                       int slot, uint8_t effect) {
    if (!audio || !audio->initialized || slot < 0 || slot > 4) return;
    audio->ram[SOUND_EFFECT_SLOT0_ADDR + (uint16_t)slot] = effect;
}

void tetris_rom_audio_apply_events(TetrisRomAudio *audio, uint32_t events) {
    uint8_t pulse_effect = 0;
    if (!audio || !audio->initialized || events == 0) return;
    if (events & TETRIS_EVENT_GAME_OVER)
        tetris_rom_audio_set_sound_effect(audio, 0, 2);
    if (events & TETRIS_EVENT_TETRIS) pulse_effect = 4;
    else if (events & TETRIS_EVENT_LEVEL_UP) pulse_effect = 6;
    else if (events & TETRIS_EVENT_LINE) pulse_effect = 10;
    else if (events & TETRIS_EVENT_LOCK) pulse_effect = 7;
    else if (events & TETRIS_EVENT_ROTATE) pulse_effect = 5;
    else if (events & TETRIS_EVENT_MOVE) pulse_effect = 3;
    if (pulse_effect)
        tetris_rom_audio_set_sound_effect(audio, 1, pulse_effect);
    if (events & TETRIS_EVENT_COMPLETE)
        audio->ram[MUSIC_TRACK_ADDR] = 2;
}

uint8_t tetris_rom_audio_ram(const TetrisRomAudio *audio, uint16_t address) {
    if (!audio || address >= TETRIS_ROM_AUDIO_RAM_SIZE) return 0;
    return audio->ram[address];
}
