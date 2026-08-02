#include "cpu6502.h"
#include "game.h"
#include "nes_apu.h"
#include "rom_audio.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MUSIC_TRACK_ADDR 0x06F5u
#define SFX_SLOT0_ADDR 0x06F0u
#define SFX_SLOT1_ADDR 0x06F1u

typedef struct CpuTestMemory {
    uint8_t bytes[65536];
    unsigned callback_cycles;
} CpuTestMemory;

static uint8_t cpu_test_read(void *userdata, uint16_t address) {
    return ((CpuTestMemory *)userdata)->bytes[address];
}

static void cpu_test_write(void *userdata, uint16_t address, uint8_t value) {
    ((CpuTestMemory *)userdata)->bytes[address] = value;
}

static void cpu_test_cycles(void *userdata, unsigned cycles) {
    ((CpuTestMemory *)userdata)->callback_cycles += cycles;
}

static int test_cpu_cycles(void) {
    Cpu6502 cpu;
    CpuTestMemory memory;
    char error[128];
    memset(&memory, 0, sizeof(memory));
    memory.bytes[0x8100] = 0xA2; /* LDX #1: 2 */
    memory.bytes[0x8101] = 0x01;
    memory.bytes[0x8102] = 0xBD; /* LDA $00FF,X: 4 + page crossing */
    memory.bytes[0x8103] = 0xFF;
    memory.bytes[0x8104] = 0x00;
    memory.bytes[0x8105] = 0xD0; /* BNE +1: 2 + taken */
    memory.bytes[0x8106] = 0x01;
    memory.bytes[0x8107] = 0xEA; /* skipped */
    memory.bytes[0x8108] = 0xEA; /* NOP: 2 */
    memory.bytes[0x8109] = 0x60; /* RTS: 6 */
    memory.bytes[0x0100] = 1;
    cpu6502_init(&cpu, cpu_test_read, cpu_test_write, &memory);
    cpu6502_set_cycle_callback(&cpu, cpu_test_cycles);
    if (!cpu6502_call(&cpu, 0x8100, 100, error, sizeof(error))) {
        fprintf(stderr, "CPU cycle test failed: %s\n", error);
        return 1;
    }
    if (cpu.cycles != 18u || memory.callback_cycles != 18u) {
        fprintf(stderr, "unexpected CPU cycles: %llu / %u\n",
                (unsigned long long)cpu.cycles, memory.callback_cycles);
        return 1;
    }
    return 0;
}

static int test_cycle_clocked_apu(void) {
    NesApu apu;
    uint8_t before;
    nes_apu_init(&apu, 48000, NULL, NULL);
    nes_apu_write(&apu, 0x4015, 0x01);
    nes_apu_write(&apu, 0x4000, 0x0F);
    nes_apu_write(&apu, 0x4003, 0xF8);
    before = apu.pulse[0].length_counter;
    if (before == 0) return 1;
    nes_apu_advance_cycles(&apu, 14913);
    if (apu.pulse[0].length_counter >= before) {
        fputs("half-frame length clock did not run\n", stderr);
        return 1;
    }
    nes_apu_advance_cycles(&apu, 14916);
    if (!nes_apu_irq_pending(&apu) ||
        !(nes_apu_read_status(&apu) & 0x40u) ||
        nes_apu_irq_pending(&apu)) {
        fputs("frame IRQ lifecycle failed\n", stderr);
        return 1;
    }
    return 0;
}

static int test_dmc_stall_and_irq(void) {
    NesApu apu;
    uint8_t memory[65536];
    memset(memory, 0xAA, sizeof(memory));
    nes_apu_init(&apu, 48000, cpu_test_read, memory);
    nes_apu_write(&apu, 0x4010, 0x8F);
    nes_apu_write(&apu, 0x4012, 0x00);
    nes_apu_write(&apu, 0x4013, 0x00);
    nes_apu_write(&apu, 0x4015, 0x10);
    nes_apu_advance_cycles(&apu, 1);
    if (nes_apu_consume_stall_cycles(&apu) != 4u ||
        !nes_apu_irq_pending(&apu) ||
        !(nes_apu_read_status(&apu) & 0x80u)) {
        fputs("DMC stall/IRQ test failed\n", stderr);
        return 1;
    }
    nes_apu_write(&apu, 0x4015, 0x00);
    if (nes_apu_irq_pending(&apu)) {
        fputs("DMC IRQ clear failed\n", stderr);
        return 1;
    }
    return 0;
}

static void build_test_prg(uint8_t prg[0x8000]) {
    static const uint8_t update[] = {
        0xA9,0x01, 0x8D,0x15,0x40,
        0xA9,0x9F, 0x8D,0x00,0x40,
        0xA9,0xFD, 0x8D,0x02,0x40,
        0xA9,0x08, 0x8D,0x03,0x40,
        0x60
    };
    memset(prg, 0xEA, 0x8000u);
    prg[0x6000u] = 0x4C;
    prg[0x6001u] = 0x00;
    prg[0x6002u] = 0xE1;
    prg[0x6006u] = 0x60;
    memcpy(prg + 0x6100u, update, sizeof(update));
}

static int test_event_mapping(TetrisRomAudio *audio) {
    char error[128];
    if (!tetris_rom_audio_select_track(audio, 3, error, sizeof(error))) {
        fprintf(stderr, "track selection failed: %s\n", error);
        return 1;
    }
    if (tetris_rom_audio_ram(audio, MUSIC_TRACK_ADDR) != 3u) return 1;
    tetris_rom_audio_stop_music(audio);
    if (tetris_rom_audio_ram(audio, MUSIC_TRACK_ADDR) != 0xFFu) return 1;
    tetris_rom_audio_apply_events(audio, TETRIS_EVENT_MOVE);
    if (tetris_rom_audio_ram(audio, SFX_SLOT1_ADDR) != 3u) return 1;
    tetris_rom_audio_apply_events(audio, TETRIS_EVENT_ROTATE);
    if (tetris_rom_audio_ram(audio, SFX_SLOT1_ADDR) != 5u) return 1;
    tetris_rom_audio_apply_events(audio, TETRIS_EVENT_LOCK);
    if (tetris_rom_audio_ram(audio, SFX_SLOT1_ADDR) != 7u) return 1;
    tetris_rom_audio_apply_events(audio, TETRIS_EVENT_LINE);
    if (tetris_rom_audio_ram(audio, SFX_SLOT1_ADDR) != 10u) return 1;
    tetris_rom_audio_apply_events(
        audio, TETRIS_EVENT_LINE | TETRIS_EVENT_TETRIS);
    if (tetris_rom_audio_ram(audio, SFX_SLOT1_ADDR) != 4u) return 1;
    tetris_rom_audio_apply_events(audio, TETRIS_EVENT_LEVEL_UP);
    if (tetris_rom_audio_ram(audio, SFX_SLOT1_ADDR) != 6u) return 1;
    tetris_rom_audio_apply_events(audio, TETRIS_EVENT_GAME_OVER);
    if (tetris_rom_audio_ram(audio, SFX_SLOT0_ADDR) != 2u) return 1;
    tetris_rom_audio_apply_events(audio, TETRIS_EVENT_COMPLETE);
    if (tetris_rom_audio_ram(audio, MUSIC_TRACK_ADDR) != 2u) return 1;
    return 0;
}

static int test_rom_driver(void) {
    uint8_t *prg = (uint8_t *)malloc(0x8000u);
    TetrisRomAudio audio;
    float samples[1024];
    char error[256];
    size_t written = 0;
    double energy = 0.0;
    size_t index;
    if (!prg) return 1;
    build_test_prg(prg);
    if (!tetris_rom_audio_init_prg(&audio, prg, 0x8000u,
                                   error, sizeof(error))) {
        fprintf(stderr, "init failed: %s\n", error);
        free(prg);
        return 1;
    }
    if (test_event_mapping(&audio) != 0) {
        fputs("original event mapping failed\n", stderr);
        free(prg);
        return 1;
    }
    if (!tetris_rom_audio_run_frame(&audio, samples, 1024u, &written,
                                    error, sizeof(error))) {
        fprintf(stderr, "frame failed: %s\n", error);
        free(prg);
        return 1;
    }
    for (index = 0; index < written; ++index)
        energy += fabs((double)samples[index]);
    if (written < 790u || written > 800u || energy < 0.01 ||
        audio.apu_write_count < 4u ||
        audio.last_frame_cpu_cycles < 29779u ||
        audio.last_frame_cpu_cycles > 29782u ||
        audio.last_driver_cycles == 0u) {
        fprintf(stderr,
                "unexpected cycle render: samples=%zu energy=%f writes=%llu frame=%u driver=%u\n",
                written, energy,
                (unsigned long long)audio.apu_write_count,
                audio.last_frame_cpu_cycles, audio.last_driver_cycles);
        free(prg);
        return 1;
    }
    free(prg);
    return 0;
}

int main(void) {
    if (test_cpu_cycles() != 0) return 1;
    if (test_cycle_clocked_apu() != 0) return 1;
    if (test_dmc_stall_and_irq() != 0) return 1;
    if (test_rom_driver() != 0) return 1;
    puts("apu cycle tests: OK");
    return 0;
}
