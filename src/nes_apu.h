#ifndef TETRIS_NES_APU_H
#define TETRIS_NES_APU_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t (*NesApuMemoryRead)(void *userdata, uint16_t address);

typedef struct NesApuPulse {
    uint8_t regs[4];
    uint8_t length_counter;
    uint8_t envelope_start;
    uint8_t envelope_divider;
    uint8_t envelope_decay;
    uint8_t sweep_divider;
    uint8_t sweep_reload;
    double phase;
} NesApuPulse;

typedef struct NesApuTriangle {
    uint8_t regs[4];
    uint8_t length_counter;
    uint8_t linear_counter;
    uint8_t linear_reload;
    double phase;
} NesApuTriangle;

typedef struct NesApuNoise {
    uint8_t regs[4];
    uint8_t length_counter;
    uint8_t envelope_start;
    uint8_t envelope_divider;
    uint8_t envelope_decay;
    uint16_t shift_register;
    double phase;
} NesApuNoise;

typedef struct NesApuDmc {
    uint8_t regs[4];
    uint8_t output_level;
    uint8_t shift_register;
    uint8_t bits_remaining;
    uint8_t sample_buffer;
    uint8_t sample_buffer_empty;
    uint8_t silence;
    uint16_t current_address;
    uint16_t bytes_remaining;
    double phase;
} NesApuDmc;

typedef struct NesApu {
    int sample_rate;
    double cpu_clock_hz;
    double quarter_frame_phase;
    double half_frame_phase;
    double dc_input;
    double dc_output;
    uint8_t status;
    uint8_t frame_counter;
    NesApuPulse pulse[2];
    NesApuTriangle triangle;
    NesApuNoise noise;
    NesApuDmc dmc;
    NesApuMemoryRead memory_read;
    void *memory_userdata;
} NesApu;

void nes_apu_init(NesApu *apu, int sample_rate,
                  NesApuMemoryRead memory_read, void *memory_userdata);
void nes_apu_reset(NesApu *apu);
void nes_apu_write(NesApu *apu, uint16_t address, uint8_t value);
uint8_t nes_apu_read_status(const NesApu *apu);
void nes_apu_render(NesApu *apu, float *samples, size_t count);

#ifdef __cplusplus
}
#endif

#endif
