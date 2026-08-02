#ifndef TETRIS_CPU6502_H
#define TETRIS_CPU6502_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t (*Cpu6502Read)(void *userdata, uint16_t address);
typedef void (*Cpu6502Write)(void *userdata, uint16_t address, uint8_t value);
typedef void (*Cpu6502Cycle)(void *userdata, unsigned cycles);

typedef struct Cpu6502 {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t sp;
    uint8_t p;
    uint16_t pc;
    uint64_t instructions;
    uint64_t cycles;
    uint64_t irq_count;
    Cpu6502Read read;
    Cpu6502Write write;
    Cpu6502Cycle cycle;
    void *userdata;
    bool stopped;
    bool irq_pending;
    bool call_active;
    bool call_returned;
    bool page_crossed;
    uint8_t call_entry_sp;
    uint8_t branch_extra;
    uint8_t last_opcode;
    uint8_t last_cycles;
} Cpu6502;

void cpu6502_init(Cpu6502 *cpu, Cpu6502Read read_cb,
                  Cpu6502Write write_cb, void *userdata);
void cpu6502_set_cycle_callback(Cpu6502 *cpu, Cpu6502Cycle cycle_cb);
void cpu6502_request_irq(Cpu6502 *cpu);
void cpu6502_clear_irq(Cpu6502 *cpu);
void cpu6502_reset_state(Cpu6502 *cpu);
bool cpu6502_call(Cpu6502 *cpu, uint16_t address, uint32_t instruction_limit,
                  char *error, unsigned error_size);

#endif
