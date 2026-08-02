#include "cpu6502.h"
#include "nes_apu.h"

#include <stdio.h>
#include <string.h>

typedef struct IrqMemory {
    uint8_t bytes[65536];
    Cpu6502 *cpu;
    unsigned callback_cycles;
    bool requested;
} IrqMemory;

static uint8_t read_byte(void *userdata, uint16_t address) {
    return ((IrqMemory *)userdata)->bytes[address];
}

static void write_byte(void *userdata, uint16_t address, uint8_t value) {
    ((IrqMemory *)userdata)->bytes[address] = value;
}

static void cycle_callback(void *userdata, unsigned cycles) {
    IrqMemory *memory = (IrqMemory *)userdata;
    memory->callback_cycles += cycles;
    if (!memory->requested && memory->callback_cycles >= 2u) {
        memory->requested = true;
        cpu6502_request_irq(memory->cpu);
    }
}

static int test_irq_entry_and_rti(void) {
    Cpu6502 cpu;
    IrqMemory memory;
    char error[128];
    memset(&memory, 0, sizeof(memory));

    /* $8000 is cpu6502_call's synthetic return target, so use $8100. */
    memory.bytes[0x8100] = 0x58; /* CLI */
    memory.bytes[0x8101] = 0xEA; /* NOP after RTI */
    memory.bytes[0x8102] = 0x60; /* RTS */
    memory.bytes[0x9000] = 0xE6; /* INC $20 */
    memory.bytes[0x9001] = 0x20;
    memory.bytes[0x9002] = 0x40; /* RTI */
    memory.bytes[0xFFFE] = 0x00;
    memory.bytes[0xFFFF] = 0x90;

    cpu6502_init(&cpu, read_byte, write_byte, &memory);
    memory.cpu = &cpu;
    cpu6502_set_cycle_callback(&cpu, cycle_callback);
    if (!cpu6502_call(&cpu, 0x8100, 100, error, sizeof(error))) {
        fprintf(stderr, "IRQ CPU call failed: %s\n", error);
        return 1;
    }
    if (memory.bytes[0x20] != 1u || cpu.irq_count != 1u ||
        cpu.cycles != 28u || memory.callback_cycles != 28u) {
        fprintf(stderr,
                "IRQ mismatch: value=%u count=%llu cycles=%llu callback=%u\n",
                memory.bytes[0x20], (unsigned long long)cpu.irq_count,
                (unsigned long long)cpu.cycles, memory.callback_cycles);
        return 1;
    }
    return 0;
}

static int test_masked_irq_is_deferred(void) {
    Cpu6502 cpu;
    IrqMemory memory;
    memset(&memory, 0, sizeof(memory));
    cpu6502_init(&cpu, read_byte, write_byte, &memory);
    cpu6502_request_irq(&cpu);
    if (!cpu.irq_pending || cpu.irq_count != 0u || cpu.cycles != 0u) {
        fputs("masked IRQ was not deferred\n", stderr);
        return 1;
    }
    cpu6502_clear_irq(&cpu);
    if (cpu.irq_pending) return 1;
    return 0;
}

static int test_frame_counter_delay(void) {
    NesApu apu;
    int index;
    nes_apu_init(&apu, 48000, NULL, NULL);

    apu.cpu_cycles = 0;
    nes_apu_schedule_frame_counter(&apu, 0x80u);
    if (apu.frame_counter_delay != 4u || apu.frame_counter != 0u) return 1;
    for (index = 0; index < 3; ++index)
        nes_apu_clock_frame_counter_delay(&apu);
    if (apu.frame_counter != 0u || apu.frame_counter_delay != 1u) {
        fputs("even-cycle $4017 applied too early\n", stderr);
        return 1;
    }
    nes_apu_clock_frame_counter_delay(&apu);
    if (apu.frame_counter != 0x80u || apu.frame_counter_delay != 0u) {
        fputs("even-cycle $4017 did not apply after four cycles\n", stderr);
        return 1;
    }

    apu.cpu_cycles = 1;
    nes_apu_schedule_frame_counter(&apu, 0x00u);
    if (apu.frame_counter_delay != 3u) return 1;
    nes_apu_clock_frame_counter_delay(&apu);
    nes_apu_clock_frame_counter_delay(&apu);
    if (apu.frame_counter != 0x80u) {
        fputs("odd-cycle $4017 applied too early\n", stderr);
        return 1;
    }
    nes_apu_clock_frame_counter_delay(&apu);
    if (apu.frame_counter != 0x00u) {
        fputs("odd-cycle $4017 did not apply after three cycles\n", stderr);
        return 1;
    }

    apu.frame_irq = 1;
    nes_apu_schedule_frame_counter(&apu, 0x40u);
    if (apu.frame_irq) {
        fputs("IRQ inhibit did not clear frame IRQ immediately\n", stderr);
        return 1;
    }
    return 0;
}

int main(void) {
    if (test_irq_entry_and_rti() != 0) return 1;
    if (test_masked_irq_is_deferred() != 0) return 1;
    if (test_frame_counter_delay() != 0) return 1;
    puts("IRQ and $4017 timing tests passed.");
    return 0;
}
