#include "nes_apu.h"

static uint16_t pulse_timer(const NesApuPulse *pulse) {
    return (uint16_t)(((uint16_t)(pulse->regs[3] & 0x07u) << 8) |
                      pulse->regs[2]);
}

static void clock_envelope_pulse(NesApuPulse *pulse) {
    const uint8_t period = pulse->regs[0] & 0x0Fu;
    if (pulse->envelope_start) {
        pulse->envelope_start = 0;
        pulse->envelope_decay = 15;
        pulse->envelope_divider = period;
    } else if (pulse->envelope_divider == 0) {
        pulse->envelope_divider = period;
        if (pulse->envelope_decay > 0) --pulse->envelope_decay;
        else if (pulse->regs[0] & 0x20u) pulse->envelope_decay = 15;
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
        if (noise->envelope_decay > 0) --noise->envelope_decay;
        else if (noise->regs[0] & 0x20u) noise->envelope_decay = 15;
    } else {
        --noise->envelope_divider;
    }
}

static uint16_t sweep_target(const NesApuPulse *pulse, int channel) {
    const uint16_t timer = pulse_timer(pulse);
    const uint8_t shift = pulse->regs[1] & 0x07u;
    const uint16_t delta = shift ? (uint16_t)(timer >> shift) : 0;
    if (!shift) return timer;
    if (pulse->regs[1] & 0x08u)
        return (uint16_t)(timer - delta - (channel == 0 ? 1u : 0u));
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

static void clock_quarter_frame(NesApu *apu) {
    clock_envelope_pulse(&apu->pulse[0]);
    clock_envelope_pulse(&apu->pulse[1]);
    clock_envelope_noise(&apu->noise);
    if (apu->triangle.linear_reload) {
        apu->triangle.linear_counter = apu->triangle.regs[0] & 0x7Fu;
    } else if (apu->triangle.linear_counter > 0) {
        --apu->triangle.linear_counter;
    }
    if (!(apu->triangle.regs[0] & 0x80u))
        apu->triangle.linear_reload = 0;
}

static void clock_half_frame(NesApu *apu) {
    if (!(apu->pulse[0].regs[0] & 0x20u) &&
        apu->pulse[0].length_counter > 0) --apu->pulse[0].length_counter;
    if (!(apu->pulse[1].regs[0] & 0x20u) &&
        apu->pulse[1].length_counter > 0) --apu->pulse[1].length_counter;
    if (!(apu->triangle.regs[0] & 0x80u) &&
        apu->triangle.length_counter > 0) --apu->triangle.length_counter;
    if (!(apu->noise.regs[0] & 0x20u) &&
        apu->noise.length_counter > 0) --apu->noise.length_counter;
    clock_sweep(&apu->pulse[0], 0);
    clock_sweep(&apu->pulse[1], 1);
}

void nes_apu_schedule_frame_counter(NesApu *apu, uint8_t value) {
    if (!apu) return;
    apu->pending_frame_counter = value;
    /* 2A03 applies the write after 3 or 4 cycles according to APU parity. */
    apu->frame_counter_delay = (apu->cpu_cycles & 1u) ? 3u : 4u;
    if (value & 0x40u) apu->frame_irq = 0;
}

void nes_apu_clock_frame_counter_delay(NesApu *apu) {
    uint8_t value;
    if (!apu || apu->frame_counter_delay == 0) return;
    --apu->frame_counter_delay;
    if (apu->frame_counter_delay != 0) return;
    value = apu->pending_frame_counter;
    apu->frame_counter = value;
    apu->frame_sequence_cycle = 0;
    if (value & 0x40u) apu->frame_irq = 0;
    if (value & 0x80u) {
        clock_quarter_frame(apu);
        clock_half_frame(apu);
    }
}
