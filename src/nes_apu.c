#include "nes_apu.h"

#include <math.h>
#include <string.h>

#define NES_CPU_NTSC 1789773.0

static const uint8_t LENGTH_TABLE[32] = {
    10,254,20,2,40,4,80,6,160,8,60,10,14,12,26,14,
    12,16,24,18,48,20,96,22,192,24,72,26,16,28,32,30
};

static const uint16_t NOISE_PERIOD_TABLE[16] = {
    4,8,16,32,64,96,128,160,202,254,380,508,762,1016,2034,4068
};

static const uint16_t DMC_PERIOD_TABLE[16] = {
    428,380,340,320,286,254,226,214,190,160,142,128,106,85,72,54
};

static const uint8_t DUTY_TABLE[4][8] = {
    {0,1,0,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,1,1,0,0,0},
    {1,0,0,1,1,1,1,1}
};

static const uint8_t TRIANGLE_TABLE[32] = {
    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};

static void pulse_reset(NesApuPulse *pulse) {
    memset(pulse, 0, sizeof(*pulse));
}

static void triangle_reset(NesApuTriangle *triangle) {
    memset(triangle, 0, sizeof(*triangle));
}

static void noise_reset(NesApuNoise *noise) {
    memset(noise, 0, sizeof(*noise));
    noise->shift_register = 1;
}

static void dmc_restart(NesApu *apu) {
    NesApuDmc *dmc = &apu->dmc;
    dmc->current_address = (uint16_t)(0xC000u + ((uint16_t)dmc->regs[2] << 6));
    dmc->bytes_remaining = (uint16_t)(((uint16_t)dmc->regs[3] << 4) + 1u);
}

static void dmc_reset(NesApuDmc *dmc) {
    memset(dmc, 0, sizeof(*dmc));
    dmc->sample_buffer_empty = 1;
    dmc->silence = 1;
    dmc->bits_remaining = 8;
}

void nes_apu_reset(NesApu *apu) {
    int sample_rate;
    NesApuMemoryRead read_cb;
    void *read_userdata;
    if (!apu) return;
    sample_rate = apu->sample_rate > 0 ? apu->sample_rate : 48000;
    read_cb = apu->memory_read;
    read_userdata = apu->memory_userdata;
    memset(apu, 0, sizeof(*apu));
    apu->sample_rate = sample_rate;
    apu->cpu_clock_hz = NES_CPU_NTSC;
    apu->memory_read = read_cb;
    apu->memory_userdata = read_userdata;
    pulse_reset(&apu->pulse[0]);
    pulse_reset(&apu->pulse[1]);
    triangle_reset(&apu->triangle);
    noise_reset(&apu->noise);
    dmc_reset(&apu->dmc);
}

void nes_apu_init(NesApu *apu, int sample_rate,
                  NesApuMemoryRead memory_read, void *memory_userdata) {
    if (!apu) return;
    memset(apu, 0, sizeof(*apu));
    apu->sample_rate = sample_rate > 0 ? sample_rate : 48000;
    apu->cpu_clock_hz = NES_CPU_NTSC;
    apu->memory_read = memory_read;
    apu->memory_userdata = memory_userdata;
    nes_apu_reset(apu);
}

static uint16_t pulse_timer(const NesApuPulse *pulse) {
    return (uint16_t)(((uint16_t)(pulse->regs[3] & 0x07u) << 8) |
                      pulse->regs[2]);
}

static uint16_t triangle_timer(const NesApuTriangle *triangle) {
    return (uint16_t)(((uint16_t)(triangle->regs[3] & 0x07u) << 8) |
                      triangle->regs[2]);
}

static uint8_t pulse_volume(const NesApuPulse *pulse) {
    if (pulse->regs[0] & 0x10u) return pulse->regs[0] & 0x0Fu;
    return pulse->envelope_decay;
}

static uint8_t noise_volume(const NesApuNoise *noise) {
    if (noise->regs[0] & 0x10u) return noise->regs[0] & 0x0Fu;
    return noise->envelope_decay;
}

static void clock_envelope_pulse(NesApuPulse *pulse) {
    const uint8_t period = pulse->regs[0] & 0x0Fu;
    if (pulse->envelope_start) {
        pulse->envelope_start = 0;
        pulse->envelope_decay = 15;
        pulse->envelope_divider = period;
    } else if (pulse->envelope_divider == 0) {
        pulse->envelope_divider = period;
        if (pulse->envelope_decay > 0) {
            --pulse->envelope_decay;
        } else if (pulse->regs[0] & 0x20u) {
            pulse->envelope_decay = 15;
        }
    } else {
        --pulse->envelope_divider;
    }
}

static void clock_envelope_noise(NesApuNoise *noise) {
    const uint8_t period = noise->regs[0] & 0x0Fu;
    if (noise->envelope_start) {
        noise->envelope_start = 0;
        noise->envelope_decay = 15;
        noise->envelope_divider = period;
    } else if (noise->envelope_divider == 0) {
        noise->envelope_divider = period;
        if (noise->envelope_decay > 0) {
            --noise->envelope_decay;
        } else if (noise->regs[0] & 0x20u) {
            noise->envelope_decay = 15;
        }
    } else {
        --noise->envelope_divider;
    }
}

static void clock_quarter_frame(NesApu *apu) {
    clock_envelope_pulse(&apu->pulse[0]);
    clock_envelope_pulse(&apu->pulse[1]);
    clock_envelope_noise(&apu->noise);

    if (apu->triangle.linear_reload) {
        apu->triangle.linear_counter = apu->triangle.regs[0] & 0x7Fu;
    } else if (apu->triangle.linear_counter > 0) {
        --apu->triangle.linear_counter;
    }
    if (!(apu->triangle.regs[0] & 0x80u)) {
        apu->triangle.linear_reload = 0;
    }
}

static uint16_t sweep_target(const NesApuPulse *pulse, int channel) {
    const uint16_t timer = pulse_timer(pulse);
    const uint8_t shift = pulse->regs[1] & 0x07u;
    uint16_t delta;
    if (shift == 0) return timer;
    delta = (uint16_t)(timer >> shift);
    if (pulse->regs[1] & 0x08u) {
        return (uint16_t)(timer - delta - (channel == 0 ? 1u : 0u));
    }
    return (uint16_t)(timer + delta);
}

static void clock_sweep(NesApuPulse *pulse, int channel) {
    const uint8_t period = (pulse->regs[1] >> 4) & 0x07u;
    const uint8_t shift = pulse->regs[1] & 0x07u;
    if (pulse->sweep_divider == 0 &&
        (pulse->regs[1] & 0x80u) && shift != 0) {
        const uint16_t current = pulse_timer(pulse);
        const uint16_t target = sweep_target(pulse, channel);
        if (current >= 8u && target <= 0x07FFu) {
            pulse->regs[2] = (uint8_t)target;
            pulse->regs[3] = (uint8_t)((pulse->regs[3] & 0xF8u) |
                                       ((target >> 8) & 0x07u));
        }
    }
    if (pulse->sweep_divider == 0 || pulse->sweep_reload) {
        pulse->sweep_divider = period;
        pulse->sweep_reload = 0;
    } else {
        --pulse->sweep_divider;
    }
}

static void clock_half_frame(NesApu *apu) {
    if (!(apu->pulse[0].regs[0] & 0x20u) && apu->pulse[0].length_counter > 0)
        --apu->pulse[0].length_counter;
    if (!(apu->pulse[1].regs[0] & 0x20u) && apu->pulse[1].length_counter > 0)
        --apu->pulse[1].length_counter;
    if (!(apu->triangle.regs[0] & 0x80u) && apu->triangle.length_counter > 0)
        --apu->triangle.length_counter;
    if (!(apu->noise.regs[0] & 0x20u) && apu->noise.length_counter > 0)
        --apu->noise.length_counter;
    clock_sweep(&apu->pulse[0], 0);
    clock_sweep(&apu->pulse[1], 1);
}

static void write_status(NesApu *apu, uint8_t value) {
    apu->status = value & 0x1Fu;
    if (!(value & 0x01u)) apu->pulse[0].length_counter = 0;
    if (!(value & 0x02u)) apu->pulse[1].length_counter = 0;
    if (!(value & 0x04u)) apu->triangle.length_counter = 0;
    if (!(value & 0x08u)) apu->noise.length_counter = 0;
    if (!(value & 0x10u)) {
        apu->dmc.bytes_remaining = 0;
    } else if (apu->dmc.bytes_remaining == 0) {
        dmc_restart(apu);
    }
}

void nes_apu_write(NesApu *apu, uint16_t address, uint8_t value) {
    if (!apu) return;
    if (address >= 0x4000u && address <= 0x4003u) {
        NesApuPulse *pulse = &apu->pulse[0];
        pulse->regs[address - 0x4000u] = value;
        if (address == 0x4001u) pulse->sweep_reload = 1;
        if (address == 0x4003u) {
            if (apu->status & 0x01u)
                pulse->length_counter = LENGTH_TABLE[(value >> 3) & 0x1Fu];
            pulse->envelope_start = 1;
            pulse->phase = 0.0;
        }
        return;
    }
    if (address >= 0x4004u && address <= 0x4007u) {
        NesApuPulse *pulse = &apu->pulse[1];
        pulse->regs[address - 0x4004u] = value;
        if (address == 0x4005u) pulse->sweep_reload = 1;
        if (address == 0x4007u) {
            if (apu->status & 0x02u)
                pulse->length_counter = LENGTH_TABLE[(value >> 3) & 0x1Fu];
            pulse->envelope_start = 1;
            pulse->phase = 0.0;
        }
        return;
    }
    if (address >= 0x4008u && address <= 0x400Bu) {
        NesApuTriangle *triangle = &apu->triangle;
        triangle->regs[address - 0x4008u] = value;
        if (address == 0x400Bu) {
            if (apu->status & 0x04u)
                triangle->length_counter = LENGTH_TABLE[(value >> 3) & 0x1Fu];
            triangle->linear_reload = 1;
            triangle->phase = 0.0;
        }
        return;
    }
    if (address >= 0x400Cu && address <= 0x400Fu) {
        NesApuNoise *noise = &apu->noise;
        noise->regs[address - 0x400Cu] = value;
        if (address == 0x400Fu) {
            if (apu->status & 0x08u)
                noise->length_counter = LENGTH_TABLE[(value >> 3) & 0x1Fu];
            noise->envelope_start = 1;
        }
        return;
    }
    if (address >= 0x4010u && address <= 0x4013u) {
        apu->dmc.regs[address - 0x4010u] = value;
        if (address == 0x4011u) apu->dmc.output_level = value & 0x7Fu;
        return;
    }
    if (address == 0x4015u) {
        write_status(apu, value);
        return;
    }
    if (address == 0x4017u) {
        apu->frame_counter = value;
        apu->quarter_frame_phase = 0.0;
        apu->half_frame_phase = 0.0;
        if (value & 0x80u) {
            clock_quarter_frame(apu);
            clock_half_frame(apu);
        }
    }
}

uint8_t nes_apu_read_status(const NesApu *apu) {
    uint8_t status = 0;
    if (!apu) return 0;
    if (apu->pulse[0].length_counter) status |= 0x01u;
    if (apu->pulse[1].length_counter) status |= 0x02u;
    if (apu->triangle.length_counter) status |= 0x04u;
    if (apu->noise.length_counter) status |= 0x08u;
    if (apu->dmc.bytes_remaining) status |= 0x10u;
    return status;
}

static double pulse_sample(NesApu *apu, int channel) {
    NesApuPulse *pulse = &apu->pulse[channel];
    const uint16_t timer = pulse_timer(pulse);
    const uint16_t target = sweep_target(pulse, channel);
    const uint8_t enable = (uint8_t)(1u << channel);
    double frequency;
    int step;
    if (!(apu->status & enable) || pulse->length_counter == 0 ||
        timer < 8u || target > 0x07FFu)
        return 0.0;
    frequency = apu->cpu_clock_hz / (16.0 * ((double)timer + 1.0));
    pulse->phase += frequency / (double)apu->sample_rate;
    pulse->phase -= floor(pulse->phase);
    step = (int)(pulse->phase * 8.0) & 7;
    if (!DUTY_TABLE[(pulse->regs[0] >> 6) & 3u][step]) return 0.0;
    return (double)pulse_volume(pulse);
}

static double triangle_sample(NesApu *apu) {
    NesApuTriangle *triangle = &apu->triangle;
    const uint16_t timer = triangle_timer(triangle);
    double frequency;
    int step;
    if (!(apu->status & 0x04u) || triangle->length_counter == 0 ||
        triangle->linear_counter == 0 || timer < 2u)
        return 0.0;
    frequency = apu->cpu_clock_hz / (32.0 * ((double)timer + 1.0));
    triangle->phase += frequency / (double)apu->sample_rate;
    triangle->phase -= floor(triangle->phase);
    step = (int)(triangle->phase * 32.0) & 31;
    return (double)TRIANGLE_TABLE[step];
}

static double noise_sample(NesApu *apu) {
    NesApuNoise *noise = &apu->noise;
    const uint16_t period = NOISE_PERIOD_TABLE[noise->regs[2] & 0x0Fu];
    const int tap = (noise->regs[2] & 0x80u) ? 6 : 1;
    const double frequency = apu->cpu_clock_hz / (double)period;
    if (!(apu->status & 0x08u) || noise->length_counter == 0) return 0.0;
    noise->phase += frequency / (double)apu->sample_rate;
    while (noise->phase >= 1.0) {
        const uint16_t feedback =
            (uint16_t)((noise->shift_register ^ (noise->shift_register >> tap)) & 1u);
        noise->shift_register =
            (uint16_t)((noise->shift_register >> 1) | (feedback << 14));
        noise->phase -= 1.0;
    }
    if (noise->shift_register & 1u) return 0.0;
    return (double)noise_volume(noise);
}

static void dmc_fill_buffer(NesApu *apu) {
    NesApuDmc *dmc = &apu->dmc;
    if (!dmc->sample_buffer_empty || dmc->bytes_remaining == 0) return;
    dmc->sample_buffer = apu->memory_read
        ? apu->memory_read(apu->memory_userdata, dmc->current_address)
        : 0;
    dmc->sample_buffer_empty = 0;
    dmc->current_address = dmc->current_address == 0xFFFFu
        ? 0x8000u : (uint16_t)(dmc->current_address + 1u);
    --dmc->bytes_remaining;
    if (dmc->bytes_remaining == 0 && (dmc->regs[0] & 0x40u)) dmc_restart(apu);
}

static double dmc_sample(NesApu *apu) {
    NesApuDmc *dmc = &apu->dmc;
    const uint16_t period = DMC_PERIOD_TABLE[dmc->regs[0] & 0x0Fu];
    const double frequency = apu->cpu_clock_hz / (double)period;
    if (!(apu->status & 0x10u)) return (double)dmc->output_level;
    dmc_fill_buffer(apu);
    dmc->phase += frequency / (double)apu->sample_rate;
    while (dmc->phase >= 1.0) {
        if (!dmc->silence) {
            if (dmc->shift_register & 1u) {
                if (dmc->output_level <= 125u) dmc->output_level += 2u;
            } else if (dmc->output_level >= 2u) {
                dmc->output_level -= 2u;
            }
        }
        dmc->shift_register >>= 1;
        if (--dmc->bits_remaining == 0) {
            dmc->bits_remaining = 8;
            if (dmc->sample_buffer_empty) {
                dmc->silence = 1;
            } else {
                dmc->silence = 0;
                dmc->shift_register = dmc->sample_buffer;
                dmc->sample_buffer_empty = 1;
                dmc_fill_buffer(apu);
            }
        }
        dmc->phase -= 1.0;
    }
    return (double)dmc->output_level;
}

static void clock_frame_sequencer(NesApu *apu) {
    apu->quarter_frame_phase += 240.0 / (double)apu->sample_rate;
    while (apu->quarter_frame_phase >= 1.0) {
        clock_quarter_frame(apu);
        apu->quarter_frame_phase -= 1.0;
    }
    apu->half_frame_phase += 120.0 / (double)apu->sample_rate;
    while (apu->half_frame_phase >= 1.0) {
        clock_half_frame(apu);
        apu->half_frame_phase -= 1.0;
    }
}

void nes_apu_render(NesApu *apu, float *samples, size_t count) {
    size_t i;
    if (!apu || !samples) return;
    for (i = 0; i < count; ++i) {
        const double p1 = pulse_sample(apu, 0);
        const double p2 = pulse_sample(apu, 1);
        const double tri = triangle_sample(apu);
        const double noi = noise_sample(apu);
        const double dmc = dmc_sample(apu);
        double pulse_mix = 0.0;
        double tnd_mix = 0.0;
        double mixed;
        double high_pass;
        if (p1 + p2 > 0.0)
            pulse_mix = 95.88 / (8128.0 / (p1 + p2) + 100.0);
        if (tri > 0.0 || noi > 0.0 || dmc > 0.0) {
            const double denominator =
                tri / 8227.0 + noi / 12241.0 + dmc / 22638.0;
            if (denominator > 0.0)
                tnd_mix = 159.79 / (1.0 / denominator + 100.0);
        }
        mixed = pulse_mix + tnd_mix;
        high_pass = mixed - apu->dc_input + 0.995 * apu->dc_output;
        apu->dc_input = mixed;
        apu->dc_output = high_pass;
        if (high_pass > 1.0) high_pass = 1.0;
        if (high_pass < -1.0) high_pass = -1.0;
        samples[i] = (float)high_pass;
        clock_frame_sequencer(apu);
    }
}
