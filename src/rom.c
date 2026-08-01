#include "rom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TARGET_ROM_CRC32 0xD16EA396u

uint32_t nes_crc32(const uint8_t *data, size_t size) {
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (0xedb88320u & mask);
        }
    }
    return ~crc;
}

static bool fail(char *error, size_t error_size, const char *message) {
    if (error && error_size) snprintf(error, error_size, "%s", message);
    return false;
}

bool nes_rom_load(const char *path, NesRom *rom, char *error, size_t error_size) {
    memset(rom, 0, sizeof(*rom));
    FILE *file = fopen(path, "rb");
    if (!file) return fail(error, error_size, "Could not open the ROM file.");
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return fail(error, error_size, "Could not seek the ROM file.");
    }
    const long length = ftell(file);
    if (length < 16 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return fail(error, error_size, "The file is too small to be an NES ROM.");
    }
    rom->bytes = (uint8_t *)malloc((size_t)length);
    if (!rom->bytes) {
        fclose(file);
        return fail(error, error_size, "Out of memory while loading ROM.");
    }
    rom->size = (size_t)length;
    if (fread(rom->bytes, 1, rom->size, file) != rom->size) {
        fclose(file);
        nes_rom_free(rom);
        return fail(error, error_size, "Could not read the complete ROM.");
    }
    fclose(file);

    const uint8_t *h = rom->bytes;
    if (memcmp(h, "NES\x1a", 4) != 0) {
        nes_rom_free(rom);
        return fail(error, error_size, "Invalid iNES/NES 2.0 header.");
    }
    rom->nes2 = (h[7] & 0x0c) == 0x08;
    rom->mapper = (h[6] >> 4) | (h[7] & 0xf0);
    if (rom->nes2) rom->mapper |= (h[8] & 0x0f) << 8;

    const size_t trainer_size = (h[6] & 0x04) ? 512u : 0u;
    size_t prg_size = (size_t)h[4] * 16384u;
    size_t chr_size = (size_t)h[5] * 8192u;
    if (rom->nes2) {
        const uint8_t prg_msb = h[9] & 0x0f;
        const uint8_t chr_msb = h[9] >> 4;
        if (prg_msb != 0x0f) prg_size = (((size_t)prg_msb << 8) | h[4]) * 16384u;
        if (chr_msb != 0x0f) chr_size = (((size_t)chr_msb << 8) | h[5]) * 8192u;
    }
    const size_t payload = 16u + trainer_size;
    if (payload + prg_size + chr_size > rom->size) {
        nes_rom_free(rom);
        return fail(error, error_size, "ROM header sizes exceed file length.");
    }
    rom->prg = rom->bytes + payload;
    rom->prg_size = prg_size;
    rom->chr = rom->prg + prg_size;
    rom->chr_size = chr_size;
    rom->crc32 = nes_crc32(rom->bytes, rom->size);
    rom->exact_supported_dump = rom->crc32 == TARGET_ROM_CRC32;

    if (rom->mapper != 1 || rom->prg_size != 32768u || rom->chr_size < 8192u) {
        nes_rom_free(rom);
        return fail(error, error_size, "Unsupported ROM: expected the USA MMC1 release with CHR ROM.");
    }
    return true;
}

void nes_rom_free(NesRom *rom) {
    if (!rom) return;
    free(rom->bytes);
    memset(rom, 0, sizeof(*rom));
}


bool nes_rom_level_palette(const NesRom *rom, int level, uint8_t colors[4]) {
    const size_t palette_offset = 0x184cu;
    if (!rom || !rom->prg || !colors || rom->prg_size < palette_offset + 40u) return false;
    if (level < 0) level = 0;
    level %= 10;
    memcpy(colors, rom->prg + palette_offset + (size_t)level * 4u, 4u);
    return true;
}
