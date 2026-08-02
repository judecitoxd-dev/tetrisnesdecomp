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

static void build_test_prg(uint8_t prg[0x8000]) {
    static const uint8_t update[] = {
        0xA9,0x01,
        0x8D,0x15,0x40,
        0xA9,0x9F,
        0x8D,0x00,0x40,
        0xA9,0xFD,
        0x8D,0x02,0x40,
        0xA9,0x08,
        0x8D,0x03,0x40,
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
    if (tetris_rom_audio_ram(audio, MUSIC_TRACK_ADDR) != 3u) {
        fputs("normal music command was not stored\n", stderr);
        return 1;
    }
    tetris_rom_audio_stop_music(audio);
    if (tetris_rom_audio_ram(audio, MUSIC_TRACK_ADDR) != 0xFFu) {
        fputs("music stop command was not stored\n", stderr);
        return 1;
    }

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
        audio.apu_write_count < 4u) {
        fprintf(stderr,
                "unexpected render: samples=%zu energy=%f writes=%llu\n",
                written, energy,
                (unsigned long long)audio.apu_write_count);
        free(prg);
        return 1;
    }
    free(prg);
    return 0;
}

int main(void) {
    if (test_rom_driver() != 0) return 1;
    puts("apu tests: OK");
    return 0;
}
