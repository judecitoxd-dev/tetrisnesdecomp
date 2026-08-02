#include "cpu6502.h"

#include <stdio.h>
#include <string.h>

typedef struct Memory {
    uint8_t bytes[65536];
} Memory;

static uint8_t read_byte(void *userdata, uint16_t address) {
    return ((Memory *)userdata)->bytes[address];
}

static void write_byte(void *userdata, uint16_t address, uint8_t value) {
    ((Memory *)userdata)->bytes[address] = value;
}

static int run_call(uint16_t address, Memory *memory,
                    uint64_t expected_cycles, uint8_t expected_value) {
    Cpu6502 cpu;
    char error[128];
    cpu6502_init(&cpu, read_byte, write_byte, memory);
    if (!cpu6502_call(&cpu, address, 100, error, sizeof(error))) {
        fprintf(stderr, "call at $%04X failed: %s\n", address, error);
        return 1;
    }
    if (cpu.cycles != expected_cycles || memory->bytes[0x20] != expected_value ||
        cpu.call_active || !cpu.call_returned || cpu.sp != 0xFDu) {
        fprintf(stderr,
                "call at $%04X mismatch: cycles=%llu value=%u sp=%02X active=%d returned=%d\n",
                address, (unsigned long long)cpu.cycles,
                memory->bytes[0x20], cpu.sp,
                cpu.call_active ? 1 : 0, cpu.call_returned ? 1 : 0);
        return 1;
    }
    return 0;
}

static int test_every_address_boundary(void) {
    Memory memory;
    memset(&memory, 0, sizeof(memory));
    memory.bytes[0x0000] = 0xE6; /* INC $20 */
    memory.bytes[0x0001] = 0x20;
    memory.bytes[0x0002] = 0x60; /* RTS */
    if (run_call(0x0000u, &memory, 11u, 1u) != 0) return 1;

    memset(&memory, 0, sizeof(memory));
    memory.bytes[0x7FFF] = 0xE6;
    memory.bytes[0x8000] = 0x20;
    memory.bytes[0x8001] = 0x60;
    if (run_call(0x7FFFu, &memory, 11u, 1u) != 0) return 1;

    memset(&memory, 0, sizeof(memory));
    memory.bytes[0x8000] = 0xE6;
    memory.bytes[0x8001] = 0x20;
    memory.bytes[0x8002] = 0x60;
    if (run_call(0x8000u, &memory, 11u, 1u) != 0) return 1;

    memset(&memory, 0, sizeof(memory));
    memory.bytes[0xFFFD] = 0xE6;
    memory.bytes[0xFFFE] = 0x20;
    memory.bytes[0xFFFF] = 0x60;
    if (run_call(0xFFFDu, &memory, 11u, 1u) != 0) return 1;
    return 0;
}

static int test_nested_jsr_uses_stack_depth(void) {
    Memory memory;
    memset(&memory, 0, sizeof(memory));
    memory.bytes[0x8000] = 0x20; /* JSR $9000 */
    memory.bytes[0x8001] = 0x00;
    memory.bytes[0x8002] = 0x90;
    memory.bytes[0x8003] = 0x60; /* top-level RTS */
    memory.bytes[0x9000] = 0xE6; /* INC $20 */
    memory.bytes[0x9001] = 0x20;
    memory.bytes[0x9002] = 0x60; /* nested RTS */
    return run_call(0x8000u, &memory, 17u, 1u);
}

int main(void) {
    if (test_every_address_boundary() != 0) return 1;
    if (test_nested_jsr_uses_stack_depth() != 0) return 1;
    puts("CPU call-frame tests passed for all address boundaries.");
    return 0;
}
