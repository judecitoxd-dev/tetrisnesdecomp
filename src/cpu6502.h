#ifndef TETRIS_CPU6502_H
#define TETRIS_CPU6502_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t (*Cpu6502Read)(void *userdata, uint16_t address);
typedef void (*Cpu6502Write)(void *userdata, uint16_t address, uint8_t value);

typedef struct Cpu6502 {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t sp;
    uint8_t p;
    uint16_t pc;
    uint64_t instructions;
    Cpu6502Read read;
    Cpu6502Write write;
    void *userdata;
    bool stopped;
    uint8_t last_opcode;
} Cpu6502;

void cpu6502_init(Cpu6502 *cpu, Cpu6502Read read_cb,
                  Cpu6502Write write_cb, void *userdata);
void cpu6502_reset_state(Cpu6502 *cpu);
bool cpu6502_call(Cpu6502 *cpu, uint16_t address, uint32_t instruction_limit,
                  char *error, unsigned error_size);

#endif
