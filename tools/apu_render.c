#include "rom.h"
#include "rom_audio.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_BUFFER_SAMPLES 1024u
#define NTSC_FRAME_RATE 60.0988

static void write_u16(FILE *file, uint16_t value) {
    fputc((int)(value & 0xFFu), file);
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

static void build_self_test_prg(uint8_t prg[0x8000]) {
    static const uint8_t update[] = {
        0xA9,0x01,0x8D,0x15,0x40,
        0xA9,0x9F,0x8D,0x00,0x40,
        0xA9,0xFD,0x8D,0x02,0x40,
        0xA9,0x08,0x8D,0x03,0x40,
        0x60
    };
    memset(prg, 0xEA, 0x8000u);
    prg[0x6000u] = 0x4C; /* $E000: JMP $E100 */
    prg[0x6001u] = 0x00;
    prg[0x6002u] = 0xE1;
    prg[0x6006u] = 0x60;
    memcpy(prg + 0x6100u, update, sizeof(update));
}

static int self_test(void) {
    uint8_t *prg = (uint8_t *)malloc(0x8000u);
    TetrisRomAudio audio;
    float samples[FRAME_BUFFER_SAMPLES];
    char error[256];
    uint64_t hash = UINT64_C(1469598103934665603);
    double energy = 0.0;
    int frame;
    if (!prg) return 1;
    build_self_test_prg(prg);
    if (!tetris_rom_audio_init_prg(&audio, prg, 0x8000u,
                                   error, sizeof(error))) {
        fprintf(stderr, "apu_render self-test init: %s\n", error);
        free(prg);
        return 1;
    }
    for (frame = 0; frame < 4; ++frame) {
        size_t count = 0;
        size_t index;
        if (!tetris_rom_audio_run_frame(&audio, samples,
                                        FRAME_BUFFER_SAMPLES, &count,
                                        error, sizeof(error))) {
            fprintf(stderr, "apu_render self-test frame: %s\n", error);
            free(prg);
            return 1;
        }
        for (index = 0; index < count; ++index) {
            int16_t pcm = float_to_pcm(samples[index]);
            energy += fabs((double)samples[index]);
            hash ^= (uint8_t)pcm;
            hash *= UINT64_C(1099511628211);
            hash ^= (uint8_t)((uint16_t)pcm >> 8);
            hash *= UINT64_C(1099511628211);
        }
    }
    free(prg);
    if (energy < 0.01 || audio.apu_write_count < 16u) {
        fprintf(stderr, "apu_render self-test produced no useful audio\n");
        return 1;
    }
    printf("apu_render self-test: OK hash=%016llx writes=%llu\n",
           (unsigned long long)hash,
           (unsigned long long)audio.apu_write_count);
    return 0;
}

static void usage(const char *program) {
    fprintf(stderr,
            "Usage: %s <Tetris (USA).nes> <track 1-10> <seconds> <output.wav> [trace.csv]\n"
            "       %s --self-test\n",
            program, program);
}

int main(int argc, char **argv) {
    NesRom rom;
    TetrisRomAudio audio;
    FILE *output = NULL;
    FILE *trace = NULL;
    float samples[FRAME_BUFFER_SAMPLES];
    char error[256];
    char *end = NULL;
    long track;
    double seconds;
    uint64_t frames;
    uint64_t frame;
    uint32_t total_samples = 0;

    if (argc == 2 && strcmp(argv[1], "--self-test") == 0)
        return self_test();
    if (argc != 5 && argc != 6) {
        usage(argv[0]);
        return 2;
    }
    track = strtol(argv[2], &end, 10);
    if (!end || *end || track < 1 || track > 10) {
        fputs("Track must be an integer from 1 through 10.\n", stderr);
        return 2;
    }
    end = NULL;
    seconds = strtod(argv[3], &end);
    if (!end || *end || seconds <= 0.0 || seconds > 3600.0) {
        fputs("Seconds must be greater than 0 and no more than 3600.\n", stderr);
        return 2;
    }

    memset(&rom, 0, sizeof(rom));
    if (!nes_rom_load(argv[1], &rom, error, sizeof(error))) {
        fprintf(stderr, "ROM error: %s\n", error);
        return 1;
    }
    if (!tetris_rom_audio_init(&audio, &rom, error, sizeof(error)) ||
        !tetris_rom_audio_select_track(&audio, (int)track,
                                       error, sizeof(error))) {
        fprintf(stderr, "APU driver error: %s\n", error);
        nes_rom_free(&rom);
        return 1;
    }

    output = fopen(argv[4], "wb+");
    if (!output || !write_wav_header(output, 0)) {
        fprintf(stderr, "Could not create WAV: %s\n", argv[4]);
        if (output) fclose(output);
        nes_rom_free(&rom);
        return 1;
    }
    if (argc == 6) {
        trace = fopen(argv[5], "wb");
        if (!trace) {
            fprintf(stderr, "Could not create APU trace: %s\n", argv[5]);
            fclose(output);
            nes_rom_free(&rom);
            return 1;
        }
        fputs("frame,apu_writes\n", trace);
    }
    frames = (uint64_t)ceil(seconds * NTSC_FRAME_RATE);
    for (frame = 0; frame < frames; ++frame) {
        size_t count = 0;
        size_t index;
        if (!tetris_rom_audio_run_frame(&audio, samples,
                                        FRAME_BUFFER_SAMPLES, &count,
                                        error, sizeof(error))) {
            fprintf(stderr, "APU frame %llu: %s\n",
                    (unsigned long long)frame, error);
            if (trace) fclose(trace);
            fclose(output);
            nes_rom_free(&rom);
            return 1;
        }
        if (trace) {
            size_t write_index;
            fprintf(trace, "%llu,", (unsigned long long)frame);
            for (write_index = 0;
                 write_index < audio.frame_write_count; ++write_index) {
                const TetrisApuWrite *write = &audio.frame_writes[write_index];
                if (write_index) fputc('|', trace);
                fprintf(trace, "%04X=%02X", write->address, write->value);
            }
            if (audio.frame_writes_overflow) fputs("|OVERFLOW", trace);
            fputc('\n', trace);
        }
        for (index = 0; index < count; ++index) {
            const int16_t pcm = float_to_pcm(samples[index]);
            write_u16(output, (uint16_t)pcm);
        }
        if (UINT32_MAX - total_samples < count) {
            fputs("WAV exceeds 32-bit RIFF size.\n", stderr);
            fclose(output);
            nes_rom_free(&rom);
            return 1;
        }
        total_samples += (uint32_t)count;
    }
    if (trace && fclose(trace) != 0) {
        fputs("Could not finalize APU trace.\n", stderr);
        fclose(output);
        nes_rom_free(&rom);
        return 1;
    }
    if (ferror(output) || fseek(output, 0, SEEK_SET) != 0 ||
        !write_wav_header(output, total_samples) || fclose(output) != 0) {
        fputs("Could not finalize WAV file.\n", stderr);
        nes_rom_free(&rom);
        return 1;
    }

    printf("ROM_CRC32=%08X\n", rom.crc32);
    printf("TRACK=%ld\n", track);
    printf("FRAMES=%llu\n", (unsigned long long)frames);
    printf("SAMPLES=%u\n", total_samples);
    printf("APU_WRITES=%llu\n", (unsigned long long)audio.apu_write_count);
    printf("APU_WRITE_HASH=%016llx\n",
           (unsigned long long)audio.apu_write_hash);
    printf("OUTPUT=%s\n", argv[4]);
    if (argc == 6) printf("TRACE=%s\n", argv[5]);
    nes_rom_free(&rom);
    return 0;
}
