#include "game.h"
#include "rom.h"
#include "rom_audio.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Scenario {
    const char *name;
    uint32_t events;
} Scenario;

static const Scenario SCENARIOS[] = {
    {"move", TETRIS_EVENT_MOVE},
    {"rotate", TETRIS_EVENT_ROTATE},
    {"lock", TETRIS_EVENT_LOCK},
    {"line", TETRIS_EVENT_LINE},
    {"tetris", TETRIS_EVENT_TETRIS},
    {"level", TETRIS_EVENT_LEVEL_UP},
    {"game-over", TETRIS_EVENT_GAME_OVER},
    {"complete", TETRIS_EVENT_COMPLETE}
};

static const Scenario *find_scenario(const char *name) {
    size_t index;
    for (index = 0; index < sizeof(SCENARIOS) / sizeof(SCENARIOS[0]); ++index) {
        if (strcmp(name, SCENARIOS[index].name) == 0) return &SCENARIOS[index];
    }
    return NULL;
}

static void write_u16(FILE *file, uint16_t value) {
    fputc((int)(value & 0xffu), file);
    fputc((int)(value >> 8), file);
}

static void write_u32(FILE *file, uint32_t value) {
    write_u16(file, (uint16_t)value);
    write_u16(file, (uint16_t)(value >> 16));
}

static bool write_wav_header(FILE *file, uint32_t sample_count) {
    const uint32_t data_size = sample_count * 2u;
    if (fwrite("RIFF", 1, 4, file) != 4) return false;
    write_u32(file, 36u + data_size);
    if (fwrite("WAVEfmt ", 1, 8, file) != 8) return false;
    write_u32(file, 16u);
    write_u16(file, 1u);
    write_u16(file, 1u);
    write_u32(file, TETRIS_ROM_AUDIO_SAMPLE_RATE);
    write_u32(file, TETRIS_ROM_AUDIO_SAMPLE_RATE * 2u);
    write_u16(file, 2u);
    write_u16(file, 16u);
    if (fwrite("data", 1, 4, file) != 4) return false;
    write_u32(file, data_size);
    return !ferror(file);
}

static int16_t float_to_pcm(float value) {
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    return (int16_t)lrintf(value * 32767.0f);
}

static void write_trace_row(FILE *trace, uint32_t frame,
                            const TetrisRomAudio *audio) {
    size_t index;
    fprintf(trace, "%u,%u,%u,%u,%u,",
            frame, audio->last_frame_cpu_cycles,
            audio->last_driver_cycles, audio->last_stall_cycles,
            nes_apu_irq_pending(&audio->apu) ? 1u : 0u);
    for (index = 0; index < audio->frame_write_count; ++index) {
        const TetrisApuWrite *write = &audio->frame_writes[index];
        if (index) fputc('|', trace);
        fprintf(trace, "%04X=%02X", write->address, write->value);
    }
    if (audio->frame_writes_overflow) fputs("|OVERFLOW", trace);
    fputc('\n', trace);
}

static int self_test(void) {
    size_t index;
    uint32_t combined = 0;
    for (index = 0; index < sizeof(SCENARIOS) / sizeof(SCENARIOS[0]); ++index) {
        if (!find_scenario(SCENARIOS[index].name) || SCENARIOS[index].events == 0)
            return 1;
        combined |= SCENARIOS[index].events;
    }
    if ((combined & (TETRIS_EVENT_MOVE | TETRIS_EVENT_ROTATE |
                     TETRIS_EVENT_LOCK | TETRIS_EVENT_LINE |
                     TETRIS_EVENT_TETRIS | TETRIS_EVENT_LEVEL_UP |
                     TETRIS_EVENT_GAME_OVER | TETRIS_EVENT_COMPLETE)) == 0)
        return 1;
    puts("apu_scenario self-test: OK scenarios=8 wav=enabled isolated=enabled");
    return 0;
}

static void usage(const char *program) {
    fprintf(stderr,
            "Usage: %s <Tetris (USA).nes> <scenario> <frames> <trace.csv> [output.wav] [--isolated]\n"
            "       %s --self-test\n"
            "Scenarios: move rotate lock line tetris level game-over complete\n",
            program, program);
}

int main(int argc, char **argv) {
    NesRom rom;
    TetrisRomAudio audio;
    const Scenario *scenario;
    FILE *trace;
    FILE *wav = NULL;
    const char *wav_path = NULL;
    bool isolated = false;
    char error[256];
    char *end = NULL;
    unsigned long frame_count;
    uint32_t frame;
    uint32_t total_samples = 0;
    float samples[1024];
    int argument;

    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) return self_test();
    if (argc < 5 || argc > 7) {
        usage(argv[0]);
        return 2;
    }
    scenario = find_scenario(argv[2]);
    if (!scenario) {
        fprintf(stderr, "Unknown APU scenario: %s\n", argv[2]);
        return 2;
    }
    frame_count = strtoul(argv[3], &end, 10);
    if (!end || *end || frame_count < 4ul || frame_count > 36000ul) {
        fputs("Frames must be between 4 and 36000.\n", stderr);
        return 2;
    }
    for (argument = 5; argument < argc; ++argument) {
        if (strcmp(argv[argument], "--isolated") == 0) {
            isolated = true;
        } else if (!wav_path) {
            wav_path = argv[argument];
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    memset(&rom, 0, sizeof(rom));
    if (!nes_rom_load(argv[1], &rom, error, sizeof(error))) {
        fprintf(stderr, "ROM error: %s\n", error);
        return 1;
    }
    if (!tetris_rom_audio_init(&audio, &rom, error, sizeof(error)) ||
        !tetris_rom_audio_select_track(&audio, 3, error, sizeof(error))) {
        fprintf(stderr, "APU driver error: %s\n", error);
        nes_rom_free(&rom);
        return 1;
    }
    if (isolated) tetris_rom_audio_stop_music(&audio);

    trace = fopen(argv[4], "wb");
    if (!trace) {
        fprintf(stderr, "Could not create trace: %s\n", argv[4]);
        nes_rom_free(&rom);
        return 1;
    }
    fputs("frame,cpu_cycles,driver_cycles,dmc_stall_cycles,irq,apu_writes\n",
          trace);

    if (wav_path) {
        wav = fopen(wav_path, "wb+");
        if (!wav || !write_wav_header(wav, 0)) {
            fprintf(stderr, "Could not create WAV: %s\n", wav_path);
            if (wav) fclose(wav);
            fclose(trace);
            nes_rom_free(&rom);
            return 1;
        }
    }

    for (frame = 0; frame < (uint32_t)frame_count; ++frame) {
        size_t written = 0;
        size_t sample_index;
        if (frame == 2u)
            tetris_rom_audio_apply_events(&audio, scenario->events);
        if (!tetris_rom_audio_run_frame(&audio, samples, 1024u, &written,
                                        error, sizeof(error))) {
            fprintf(stderr, "APU frame %u: %s\n", frame, error);
            if (wav) fclose(wav);
            fclose(trace);
            nes_rom_free(&rom);
            return 1;
        }
        write_trace_row(trace, frame, &audio);
        if (wav) {
            if (UINT32_MAX - total_samples < written) {
                fputs("WAV exceeds 32-bit RIFF size.\n", stderr);
                fclose(wav);
                fclose(trace);
                nes_rom_free(&rom);
                return 1;
            }
            for (sample_index = 0; sample_index < written; ++sample_index)
                write_u16(wav, (uint16_t)float_to_pcm(samples[sample_index]));
            total_samples += (uint32_t)written;
        }
    }
    if (fclose(trace) != 0) {
        fputs("Could not finalize scenario trace.\n", stderr);
        if (wav) fclose(wav);
        nes_rom_free(&rom);
        return 1;
    }
    if (wav && (ferror(wav) || fseek(wav, 0, SEEK_SET) != 0 ||
                !write_wav_header(wav, total_samples) || fclose(wav) != 0)) {
        fputs("Could not finalize scenario WAV.\n", stderr);
        nes_rom_free(&rom);
        return 1;
    }

    printf("SCENARIO=%s\nFRAMES=%lu\nISOLATED=%u\nCPU_CYCLES=%llu\nAPU_WRITES=%llu\nTRACE=%s\n",
           scenario->name, frame_count, isolated ? 1u : 0u,
           (unsigned long long)audio.rendered_cpu_cycles,
           (unsigned long long)audio.apu_write_count, argv[4]);
    if (wav_path) printf("WAV=%s\nSAMPLES=%u\n", wav_path, total_samples);
    nes_rom_free(&rom);
    return 0;
}
