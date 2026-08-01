#ifndef TETRIS_ROM_H
#define TETRIS_ROM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct NesRom {
    uint8_t *bytes;
    size_t size;
    const uint8_t *prg;
    size_t prg_size;
    const uint8_t *chr;
    size_t chr_size;
    int mapper;
    bool nes2;
    uint32_t crc32;
    bool exact_supported_dump;
} NesRom;

bool nes_rom_load(const char *path, NesRom *rom, char *error, size_t error_size);
void nes_rom_free(NesRom *rom);
uint32_t nes_crc32(const uint8_t *data, size_t size);
bool nes_rom_level_palette(const NesRom *rom, int level, uint8_t colors[4]);

#endif
