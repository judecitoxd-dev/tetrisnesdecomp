#include "cpu6502.h"

#define F_I 0x04u
#define F_B 0x10u
#define F_U 0x20u

static uint8_t read_byte(Cpu6502 *cpu, uint16_t address) {
    return cpu->read ? cpu->read(cpu->userdata, address) : 0xFFu;
}

static void push_byte(Cpu6502 *cpu, uint8_t value) {
    if (cpu->write)
        cpu->write(cpu->userdata, (uint16_t)(0x0100u | cpu->sp), value);
    --cpu->sp;
}

void cpu6502_request_irq(Cpu6502 *cpu) {
    uint16_t vector;
    if (!cpu) return;
    cpu->irq_pending = true;
    if (cpu->p & F_I) return;

    /* IRQ is sampled between instructions and takes seven CPU cycles. */
    push_byte(cpu, (uint8_t)(cpu->pc >> 8));
    push_byte(cpu, (uint8_t)cpu->pc);
    push_byte(cpu, (uint8_t)((cpu->p & ~F_B) | F_U));
    cpu->p = (uint8_t)((cpu->p | F_I | F_U) & ~F_B);
    vector = (uint16_t)read_byte(cpu, 0xFFFEu) |
             ((uint16_t)read_byte(cpu, 0xFFFFu) << 8);
    cpu->pc = vector;
    cpu->irq_pending = false;
    ++cpu->irq_count;
    cpu->cycles += 7u;
    cpu->last_cycles = 7u;
    if (cpu->cycle) cpu->cycle(cpu->userdata, 7u);
}

void cpu6502_clear_irq(Cpu6502 *cpu) {
    if (cpu) cpu->irq_pending = false;
}
