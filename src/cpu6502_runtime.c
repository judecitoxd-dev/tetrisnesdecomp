/*
 * Compile the established instruction decoder in this translation unit, then
 * replace only its host-call wrapper. This keeps the opcode implementation and
 * cycle accounting unchanged while removing the synthetic $8000 return target.
 */
#define cpu6502_call cpu6502_call_with_sentinel_legacy
#include "cpu6502.c"
#undef cpu6502_call

bool cpu6502_call(Cpu6502 *cpu, uint16_t address, uint32_t instruction_limit,
                  char *error, unsigned error_size) {
    uint32_t count = 0;

    if (!cpu || !cpu->read || !cpu->write) {
        if (error && error_size)
            snprintf(error, error_size, "invalid CPU callbacks");
        return false;
    }
    if (cpu->call_active) {
        if (error && error_size)
            snprintf(error, error_size,
                     "recursive host cpu6502_call is not supported");
        return false;
    }

    cpu->stopped = false;
    cpu->sp = 0xFDu;
    cpu->pc = address;
    cpu->call_entry_sp = cpu->sp;
    cpu->call_active = true;
    cpu->call_returned = false;

    while (!cpu->call_returned) {
        if (count++ >= instruction_limit) {
            if (error && error_size) {
                snprintf(error, error_size,
                         "6502 call at $%04X exceeded %u instructions (pc=$%04X)",
                         address, instruction_limit, cpu->pc);
            }
            cpu->call_active = false;
            return false;
        }

        /*
         * A top-level RTS is the host-call boundary. Nested JSR/RTS pairs and
         * IRQ/RTI frames lower SP, so they continue through the normal decoder.
         */
        if (cpu->sp == cpu->call_entry_sp && rd(cpu, cpu->pc) == 0x60u) {
            ++cpu->pc;
            ++cpu->instructions;
            cpu->last_opcode = 0x60u;
            cpu->last_cycles = 6u;
            cpu->p |= F_U;
            cpu->cycles += 6u;
            if (cpu->cycle) cpu->cycle(cpu->userdata, 6u);
            cpu->call_returned = true;
            cpu->call_active = false;
            break;
        }

        if (!step(cpu, error, error_size)) {
            cpu->call_active = false;
            return false;
        }
    }

    if (error && error_size) error[0] = '\0';
    return true;
}
