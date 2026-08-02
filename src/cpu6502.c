#include "cpu6502.h"

#include <stdio.h>
#include <string.h>

enum {
    F_C = 0x01,
    F_Z = 0x02,
    F_I = 0x04,
    F_D = 0x08,
    F_B = 0x10,
    F_U = 0x20,
    F_V = 0x40,
    F_N = 0x80
};

static const uint8_t BASE_CYCLES[256] = {
    [0x00]=7,[0x01]=6,[0x05]=3,[0x06]=5,[0x08]=3,[0x09]=2,[0x0A]=2,[0x0D]=4,[0x0E]=6,
    [0x10]=2,[0x11]=5,[0x15]=4,[0x16]=6,[0x18]=2,[0x19]=4,[0x1D]=4,[0x1E]=7,
    [0x20]=6,[0x21]=6,[0x24]=3,[0x25]=3,[0x26]=5,[0x28]=4,[0x29]=2,[0x2A]=2,[0x2C]=4,[0x2D]=4,[0x2E]=6,
    [0x30]=2,[0x31]=5,[0x35]=4,[0x36]=6,[0x38]=2,[0x39]=4,[0x3D]=4,[0x3E]=7,
    [0x40]=6,[0x41]=6,[0x45]=3,[0x46]=5,[0x48]=3,[0x49]=2,[0x4A]=2,[0x4C]=3,[0x4D]=4,[0x4E]=6,
    [0x50]=2,[0x51]=5,[0x55]=4,[0x56]=6,[0x58]=2,[0x59]=4,[0x5D]=4,[0x5E]=7,
    [0x60]=6,[0x61]=6,[0x65]=3,[0x66]=5,[0x68]=4,[0x69]=2,[0x6A]=2,[0x6C]=5,[0x6D]=4,[0x6E]=6,
    [0x70]=2,[0x71]=5,[0x75]=4,[0x76]=6,[0x78]=2,[0x79]=4,[0x7D]=4,[0x7E]=7,
    [0x81]=6,[0x84]=3,[0x85]=3,[0x86]=3,[0x88]=2,[0x8A]=2,[0x8C]=4,[0x8D]=4,[0x8E]=4,
    [0x90]=2,[0x91]=6,[0x94]=4,[0x95]=4,[0x96]=4,[0x98]=2,[0x99]=5,[0x9A]=2,[0x9D]=5,
    [0xA0]=2,[0xA1]=6,[0xA2]=2,[0xA4]=3,[0xA5]=3,[0xA6]=3,[0xA8]=2,[0xA9]=2,[0xAA]=2,[0xAC]=4,[0xAD]=4,[0xAE]=4,
    [0xB0]=2,[0xB1]=5,[0xB4]=4,[0xB5]=4,[0xB6]=4,[0xB8]=2,[0xB9]=4,[0xBA]=2,[0xBC]=4,[0xBD]=4,[0xBE]=4,
    [0xC0]=2,[0xC1]=6,[0xC4]=3,[0xC5]=3,[0xC6]=5,[0xC8]=2,[0xC9]=2,[0xCA]=2,[0xCC]=4,[0xCD]=4,[0xCE]=6,
    [0xD0]=2,[0xD1]=5,[0xD5]=4,[0xD6]=6,[0xD8]=2,[0xD9]=4,[0xDD]=4,[0xDE]=7,
    [0xE0]=2,[0xE1]=6,[0xE4]=3,[0xE5]=3,[0xE6]=5,[0xE8]=2,[0xE9]=2,[0xEA]=2,[0xEC]=4,[0xED]=4,[0xEE]=6,
    [0xF0]=2,[0xF1]=5,[0xF5]=4,[0xF6]=6,[0xF8]=2,[0xF9]=4,[0xFD]=4,[0xFE]=7
};

static uint8_t rd(Cpu6502 *cpu, uint16_t address) {
    return cpu->read ? cpu->read(cpu->userdata, address) : 0xFFu;
}

static void wr(Cpu6502 *cpu, uint16_t address, uint8_t value) {
    if (cpu->write) cpu->write(cpu->userdata, address, value);
}

static uint8_t fetch8(Cpu6502 *cpu) { return rd(cpu, cpu->pc++); }

static uint16_t fetch16(Cpu6502 *cpu) {
    uint16_t lo = fetch8(cpu);
    uint16_t hi = fetch8(cpu);
    return (uint16_t)(lo | (hi << 8));
}

static uint16_t read16_zp(Cpu6502 *cpu, uint8_t address) {
    uint16_t lo = rd(cpu, address);
    uint16_t hi = rd(cpu, (uint8_t)(address + 1u));
    return (uint16_t)(lo | (hi << 8));
}

static void push(Cpu6502 *cpu, uint8_t value) {
    wr(cpu, (uint16_t)(0x0100u | cpu->sp), value);
    --cpu->sp;
}

static uint8_t pull(Cpu6502 *cpu) {
    ++cpu->sp;
    return rd(cpu, (uint16_t)(0x0100u | cpu->sp));
}

static void set_nz(Cpu6502 *cpu, uint8_t value) {
    cpu->p = (uint8_t)((cpu->p & ~(F_N | F_Z)) |
                       (value == 0 ? F_Z : 0) | (value & F_N));
}

static void compare(Cpu6502 *cpu, uint8_t lhs, uint8_t rhs) {
    uint16_t result = (uint16_t)lhs - rhs;
    cpu->p = (uint8_t)((cpu->p & ~F_C) | (lhs >= rhs ? F_C : 0));
    set_nz(cpu, (uint8_t)result);
}

static void adc(Cpu6502 *cpu, uint8_t value) {
    uint16_t sum = (uint16_t)cpu->a + value + ((cpu->p & F_C) ? 1u : 0u);
    uint8_t result = (uint8_t)sum;
    cpu->p = (uint8_t)((cpu->p & ~(F_C | F_V)) |
        (sum > 0xFFu ? F_C : 0) |
        ((~(cpu->a ^ value) & (cpu->a ^ result) & 0x80u) ? F_V : 0));
    cpu->a = result;
    set_nz(cpu, cpu->a);
}

static void sbc(Cpu6502 *cpu, uint8_t value) { adc(cpu, (uint8_t)~value); }

static uint8_t asl_value(Cpu6502 *cpu, uint8_t value) {
    cpu->p = (uint8_t)((cpu->p & ~F_C) | ((value & 0x80u) ? F_C : 0));
    value = (uint8_t)(value << 1);
    set_nz(cpu, value);
    return value;
}

static uint8_t lsr_value(Cpu6502 *cpu, uint8_t value) {
    cpu->p = (uint8_t)((cpu->p & ~F_C) | ((value & 1u) ? F_C : 0));
    value >>= 1;
    set_nz(cpu, value);
    return value;
}

static uint8_t rol_value(Cpu6502 *cpu, uint8_t value) {
    uint8_t carry = (cpu->p & F_C) ? 1u : 0u;
    cpu->p = (uint8_t)((cpu->p & ~F_C) | ((value & 0x80u) ? F_C : 0));
    value = (uint8_t)((value << 1) | carry);
    set_nz(cpu, value);
    return value;
}

static uint8_t ror_value(Cpu6502 *cpu, uint8_t value) {
    uint8_t carry = (cpu->p & F_C) ? 0x80u : 0u;
    cpu->p = (uint8_t)((cpu->p & ~F_C) | ((value & 1u) ? F_C : 0));
    value = (uint8_t)((value >> 1) | carry);
    set_nz(cpu, value);
    return value;
}

static uint16_t zp(Cpu6502 *cpu) { return fetch8(cpu); }
static uint16_t zpx(Cpu6502 *cpu) { return (uint8_t)(fetch8(cpu) + cpu->x); }
static uint16_t zpy(Cpu6502 *cpu) { return (uint8_t)(fetch8(cpu) + cpu->y); }
static uint16_t abs_addr(Cpu6502 *cpu) { return fetch16(cpu); }
static uint16_t indexed(Cpu6502 *cpu, uint16_t base, uint8_t index) {
    uint16_t result = (uint16_t)(base + index);
    if ((base & 0xFF00u) != (result & 0xFF00u)) cpu->page_crossed = true;
    return result;
}
static uint16_t absx(Cpu6502 *cpu) { return indexed(cpu, fetch16(cpu), cpu->x); }
static uint16_t absy(Cpu6502 *cpu) { return indexed(cpu, fetch16(cpu), cpu->y); }
static uint16_t indx(Cpu6502 *cpu) {
    return read16_zp(cpu, (uint8_t)(fetch8(cpu) + cpu->x));
}
static uint16_t indy(Cpu6502 *cpu) {
    uint16_t base = read16_zp(cpu, fetch8(cpu));
    return indexed(cpu, base, cpu->y);
}

static void branch(Cpu6502 *cpu, bool condition) {
    int8_t offset = (int8_t)fetch8(cpu);
    if (condition) {
        uint16_t old_pc = cpu->pc;
        cpu->pc = (uint16_t)(cpu->pc + offset);
        cpu->branch_extra = (uint8_t)(1u +
            (((old_pc ^ cpu->pc) & 0xFF00u) != 0 ? 1u : 0u));
    }
}

static bool page_cross_penalty(uint8_t op) {
    switch (op) {
    case 0x11: case 0x19: case 0x1D:
    case 0x31: case 0x39: case 0x3D:
    case 0x51: case 0x59: case 0x5D:
    case 0x71: case 0x79: case 0x7D:
    case 0xB1: case 0xB9: case 0xBC: case 0xBD: case 0xBE:
    case 0xD1: case 0xD9: case 0xDD:
    case 0xF1: case 0xF9: case 0xFD:
        return true;
    default:
        return false;
    }
}

static bool illegal(Cpu6502 *cpu, uint8_t opcode,
                    char *error, unsigned error_size) {
    if (error && error_size) {
        snprintf(error, error_size,
                 "unsupported 6502 opcode $%02X at $%04X",
                 opcode, (uint16_t)(cpu->pc - 1u));
    }
    cpu->stopped = true;
    return false;
}

static bool step(Cpu6502 *cpu, char *error, unsigned error_size) {
    uint8_t op = fetch8(cpu);
    uint16_t a;
    uint8_t v;
    unsigned cycles;
    cpu->last_opcode = op;
    cpu->page_crossed = false;
    cpu->branch_extra = 0;
    ++cpu->instructions;

#define LOAD(reg, addr_expr) do { cpu->reg = rd(cpu, (addr_expr)); set_nz(cpu, cpu->reg); } while (0)
#define STORE(reg, addr_expr) wr(cpu, (addr_expr), cpu->reg)
#define LOGIC_AND(addr_expr) do { cpu->a &= rd(cpu, (addr_expr)); set_nz(cpu, cpu->a); } while (0)
#define LOGIC_ORA(addr_expr) do { cpu->a |= rd(cpu, (addr_expr)); set_nz(cpu, cpu->a); } while (0)
#define LOGIC_EOR(addr_expr) do { cpu->a ^= rd(cpu, (addr_expr)); set_nz(cpu, cpu->a); } while (0)
#define INC_MEM(addr_expr) do { a = (addr_expr); v = (uint8_t)(rd(cpu, a) + 1u); wr(cpu, a, v); set_nz(cpu, v); } while (0)
#define DEC_MEM(addr_expr) do { a = (addr_expr); v = (uint8_t)(rd(cpu, a) - 1u); wr(cpu, a, v); set_nz(cpu, v); } while (0)
#define ASL_MEM(addr_expr) do { a = (addr_expr); v = asl_value(cpu, rd(cpu, a)); wr(cpu, a, v); } while (0)
#define LSR_MEM(addr_expr) do { a = (addr_expr); v = lsr_value(cpu, rd(cpu, a)); wr(cpu, a, v); } while (0)
#define ROL_MEM(addr_expr) do { a = (addr_expr); v = rol_value(cpu, rd(cpu, a)); wr(cpu, a, v); } while (0)
#define ROR_MEM(addr_expr) do { a = (addr_expr); v = ror_value(cpu, rd(cpu, a)); wr(cpu, a, v); } while (0)
#define BIT_TEST(addr_expr) do { v = rd(cpu, (addr_expr)); cpu->p = (uint8_t)((cpu->p & ~(F_N|F_V|F_Z)) | (v & (F_N|F_V)) | ((cpu->a & v) == 0 ? F_Z : 0)); } while (0)

    switch (op) {
    case 0x00: return illegal(cpu, op, error, error_size);
    case 0x01: LOGIC_ORA(indx(cpu)); break;
    case 0x05: LOGIC_ORA(zp(cpu)); break;
    case 0x06: ASL_MEM(zp(cpu)); break;
    case 0x08: push(cpu, (uint8_t)(cpu->p | F_B | F_U)); break;
    case 0x09: cpu->a |= fetch8(cpu); set_nz(cpu, cpu->a); break;
    case 0x0A: cpu->a = asl_value(cpu, cpu->a); break;
    case 0x0D: LOGIC_ORA(abs_addr(cpu)); break;
    case 0x0E: ASL_MEM(abs_addr(cpu)); break;
    case 0x10: branch(cpu, !(cpu->p & F_N)); break;
    case 0x11: LOGIC_ORA(indy(cpu)); break;
    case 0x15: LOGIC_ORA(zpx(cpu)); break;
    case 0x16: ASL_MEM(zpx(cpu)); break;
    case 0x18: cpu->p &= (uint8_t)~F_C; break;
    case 0x19: LOGIC_ORA(absy(cpu)); break;
    case 0x1D: LOGIC_ORA(absx(cpu)); break;
    case 0x1E: ASL_MEM(absx(cpu)); break;
    case 0x20: a=fetch16(cpu); push(cpu,(uint8_t)((cpu->pc-1u)>>8)); push(cpu,(uint8_t)(cpu->pc-1u)); cpu->pc=a; break;
    case 0x21: LOGIC_AND(indx(cpu)); break;
    case 0x24: BIT_TEST(zp(cpu)); break;
    case 0x25: LOGIC_AND(zp(cpu)); break;
    case 0x26: ROL_MEM(zp(cpu)); break;
    case 0x28: cpu->p=(uint8_t)((pull(cpu)|F_U)&~F_B); break;
    case 0x29: cpu->a&=fetch8(cpu); set_nz(cpu,cpu->a); break;
    case 0x2A: cpu->a=rol_value(cpu,cpu->a); break;
    case 0x2C: BIT_TEST(abs_addr(cpu)); break;
    case 0x2D: LOGIC_AND(abs_addr(cpu)); break;
    case 0x2E: ROL_MEM(abs_addr(cpu)); break;
    case 0x30: branch(cpu,(cpu->p&F_N)!=0); break;
    case 0x31: LOGIC_AND(indy(cpu)); break;
    case 0x35: LOGIC_AND(zpx(cpu)); break;
    case 0x36: ROL_MEM(zpx(cpu)); break;
    case 0x38: cpu->p|=F_C; break;
    case 0x39: LOGIC_AND(absy(cpu)); break;
    case 0x3D: LOGIC_AND(absx(cpu)); break;
    case 0x3E: ROL_MEM(absx(cpu)); break;
    case 0x40: cpu->p=(uint8_t)((pull(cpu)|F_U)&~F_B); {uint16_t lo=pull(cpu),hi=pull(cpu);cpu->pc=(uint16_t)(lo|(hi<<8));} break;
    case 0x41: LOGIC_EOR(indx(cpu)); break;
    case 0x45: LOGIC_EOR(zp(cpu)); break;
    case 0x46: LSR_MEM(zp(cpu)); break;
    case 0x48: push(cpu,cpu->a); break;
    case 0x49: cpu->a^=fetch8(cpu); set_nz(cpu,cpu->a); break;
    case 0x4A: cpu->a=lsr_value(cpu,cpu->a); break;
    case 0x4C: cpu->pc=fetch16(cpu); break;
    case 0x4D: LOGIC_EOR(abs_addr(cpu)); break;
    case 0x4E: LSR_MEM(abs_addr(cpu)); break;
    case 0x50: branch(cpu,!(cpu->p&F_V)); break;
    case 0x51: LOGIC_EOR(indy(cpu)); break;
    case 0x55: LOGIC_EOR(zpx(cpu)); break;
    case 0x56: LSR_MEM(zpx(cpu)); break;
    case 0x58: cpu->p&=(uint8_t)~F_I; break;
    case 0x59: LOGIC_EOR(absy(cpu)); break;
    case 0x5D: LOGIC_EOR(absx(cpu)); break;
    case 0x5E: LSR_MEM(absx(cpu)); break;
    case 0x60: {uint16_t lo=pull(cpu),hi=pull(cpu);cpu->pc=(uint16_t)((lo|(hi<<8))+1u);} break;
    case 0x61: adc(cpu,rd(cpu,indx(cpu))); break;
    case 0x65: adc(cpu,rd(cpu,zp(cpu))); break;
    case 0x66: ROR_MEM(zp(cpu)); break;
    case 0x68: cpu->a=pull(cpu); set_nz(cpu,cpu->a); break;
    case 0x69: adc(cpu,fetch8(cpu)); break;
    case 0x6A: cpu->a=ror_value(cpu,cpu->a); break;
    case 0x6C: a=fetch16(cpu);cpu->pc=(uint16_t)(rd(cpu,a)|((uint16_t)rd(cpu,(uint16_t)((a&0xFF00u)|((a+1u)&0x00FFu)))<<8));break;
    case 0x6D: adc(cpu,rd(cpu,abs_addr(cpu))); break;
    case 0x6E: ROR_MEM(abs_addr(cpu)); break;
    case 0x70: branch(cpu,(cpu->p&F_V)!=0); break;
    case 0x71: adc(cpu,rd(cpu,indy(cpu))); break;
    case 0x75: adc(cpu,rd(cpu,zpx(cpu))); break;
    case 0x76: ROR_MEM(zpx(cpu)); break;
    case 0x78: cpu->p|=F_I; break;
    case 0x79: adc(cpu,rd(cpu,absy(cpu))); break;
    case 0x7D: adc(cpu,rd(cpu,absx(cpu))); break;
    case 0x7E: ROR_MEM(absx(cpu)); break;
    case 0x81: STORE(a,indx(cpu)); break;
    case 0x84: STORE(y,zp(cpu)); break;
    case 0x85: STORE(a,zp(cpu)); break;
    case 0x86: STORE(x,zp(cpu)); break;
    case 0x88: --cpu->y; set_nz(cpu,cpu->y); break;
    case 0x8A: cpu->a=cpu->x; set_nz(cpu,cpu->a); break;
    case 0x8C: STORE(y,abs_addr(cpu)); break;
    case 0x8D: STORE(a,abs_addr(cpu)); break;
    case 0x8E: STORE(x,abs_addr(cpu)); break;
    case 0x90: branch(cpu,!(cpu->p&F_C)); break;
    case 0x91: STORE(a,indy(cpu)); break;
    case 0x94: STORE(y,zpx(cpu)); break;
    case 0x95: STORE(a,zpx(cpu)); break;
    case 0x96: STORE(x,zpy(cpu)); break;
    case 0x98: cpu->a=cpu->y; set_nz(cpu,cpu->a); break;
    case 0x99: STORE(a,absy(cpu)); break;
    case 0x9A: cpu->sp=cpu->x; break;
    case 0x9D: STORE(a,absx(cpu)); break;
    case 0xA0: cpu->y=fetch8(cpu); set_nz(cpu,cpu->y); break;
    case 0xA1: LOAD(a,indx(cpu)); break;
    case 0xA2: cpu->x=fetch8(cpu); set_nz(cpu,cpu->x); break;
    case 0xA4: LOAD(y,zp(cpu)); break;
    case 0xA5: LOAD(a,zp(cpu)); break;
    case 0xA6: LOAD(x,zp(cpu)); break;
    case 0xA8: cpu->y=cpu->a; set_nz(cpu,cpu->y); break;
    case 0xA9: cpu->a=fetch8(cpu); set_nz(cpu,cpu->a); break;
    case 0xAA: cpu->x=cpu->a; set_nz(cpu,cpu->x); break;
    case 0xAC: LOAD(y,abs_addr(cpu)); break;
    case 0xAD: LOAD(a,abs_addr(cpu)); break;
    case 0xAE: LOAD(x,abs_addr(cpu)); break;
    case 0xB0: branch(cpu,(cpu->p&F_C)!=0); break;
    case 0xB1: LOAD(a,indy(cpu)); break;
    case 0xB4: LOAD(y,zpx(cpu)); break;
    case 0xB5: LOAD(a,zpx(cpu)); break;
    case 0xB6: LOAD(x,zpy(cpu)); break;
    case 0xB8: cpu->p&=(uint8_t)~F_V; break;
    case 0xB9: LOAD(a,absy(cpu)); break;
    case 0xBA: cpu->x=cpu->sp; set_nz(cpu,cpu->x); break;
    case 0xBC: LOAD(y,absx(cpu)); break;
    case 0xBD: LOAD(a,absx(cpu)); break;
    case 0xBE: LOAD(x,absy(cpu)); break;
    case 0xC0: compare(cpu,cpu->y,fetch8(cpu)); break;
    case 0xC1: compare(cpu,cpu->a,rd(cpu,indx(cpu))); break;
    case 0xC4: compare(cpu,cpu->y,rd(cpu,zp(cpu))); break;
    case 0xC5: compare(cpu,cpu->a,rd(cpu,zp(cpu))); break;
    case 0xC6: DEC_MEM(zp(cpu)); break;
    case 0xC8: ++cpu->y; set_nz(cpu,cpu->y); break;
    case 0xC9: compare(cpu,cpu->a,fetch8(cpu)); break;
    case 0xCA: --cpu->x; set_nz(cpu,cpu->x); break;
    case 0xCC: compare(cpu,cpu->y,rd(cpu,abs_addr(cpu))); break;
    case 0xCD: compare(cpu,cpu->a,rd(cpu,abs_addr(cpu))); break;
    case 0xCE: DEC_MEM(abs_addr(cpu)); break;
    case 0xD0: branch(cpu,!(cpu->p&F_Z)); break;
    case 0xD1: compare(cpu,cpu->a,rd(cpu,indy(cpu))); break;
    case 0xD5: compare(cpu,cpu->a,rd(cpu,zpx(cpu))); break;
    case 0xD6: DEC_MEM(zpx(cpu)); break;
    case 0xD8: cpu->p&=(uint8_t)~F_D; break;
    case 0xD9: compare(cpu,cpu->a,rd(cpu,absy(cpu))); break;
    case 0xDD: compare(cpu,cpu->a,rd(cpu,absx(cpu))); break;
    case 0xDE: DEC_MEM(absx(cpu)); break;
    case 0xE0: compare(cpu,cpu->x,fetch8(cpu)); break;
    case 0xE1: sbc(cpu,rd(cpu,indx(cpu))); break;
    case 0xE4: compare(cpu,cpu->x,rd(cpu,zp(cpu))); break;
    case 0xE5: sbc(cpu,rd(cpu,zp(cpu))); break;
    case 0xE6: INC_MEM(zp(cpu)); break;
    case 0xE8: ++cpu->x; set_nz(cpu,cpu->x); break;
    case 0xE9: sbc(cpu,fetch8(cpu)); break;
    case 0xEA: break;
    case 0xEC: compare(cpu,cpu->x,rd(cpu,abs_addr(cpu))); break;
    case 0xED: sbc(cpu,rd(cpu,abs_addr(cpu))); break;
    case 0xEE: INC_MEM(abs_addr(cpu)); break;
    case 0xF0: branch(cpu,(cpu->p&F_Z)!=0); break;
    case 0xF1: sbc(cpu,rd(cpu,indy(cpu))); break;
    case 0xF5: sbc(cpu,rd(cpu,zpx(cpu))); break;
    case 0xF6: INC_MEM(zpx(cpu)); break;
    case 0xF8: cpu->p|=F_D; break;
    case 0xF9: sbc(cpu,rd(cpu,absy(cpu))); break;
    case 0xFD: sbc(cpu,rd(cpu,absx(cpu))); break;
    case 0xFE: INC_MEM(absx(cpu)); break;
    default: return illegal(cpu,op,error,error_size);
    }
#undef LOAD
#undef STORE
#undef LOGIC_AND
#undef LOGIC_ORA
#undef LOGIC_EOR
#undef INC_MEM
#undef DEC_MEM
#undef ASL_MEM
#undef LSR_MEM
#undef ROL_MEM
#undef ROR_MEM
#undef BIT_TEST
    cpu->p |= F_U;
    cycles = BASE_CYCLES[op] + cpu->branch_extra;
    if (cpu->page_crossed && page_cross_penalty(op)) ++cycles;
    cpu->last_cycles = (uint8_t)cycles;
    cpu->cycles += cycles;
    if (cpu->cycle) cpu->cycle(cpu->userdata, cycles);
    return true;
}

void cpu6502_reset_state(Cpu6502 *cpu) {
    Cpu6502Read read_cb;
    Cpu6502Write write_cb;
    Cpu6502Cycle cycle_cb;
    void *userdata;
    if (!cpu) return;
    read_cb=cpu->read; write_cb=cpu->write; cycle_cb=cpu->cycle; userdata=cpu->userdata;
    memset(cpu,0,sizeof(*cpu));
    cpu->read=read_cb; cpu->write=write_cb; cpu->cycle=cycle_cb; cpu->userdata=userdata;
    cpu->sp=0xFD; cpu->p=F_I|F_U;
}

void cpu6502_init(Cpu6502 *cpu, Cpu6502Read read_cb,
                  Cpu6502Write write_cb, void *userdata) {
    if (!cpu) return;
    memset(cpu,0,sizeof(*cpu));
    cpu->read=read_cb; cpu->write=write_cb; cpu->userdata=userdata;
    cpu6502_reset_state(cpu);
}

void cpu6502_set_cycle_callback(Cpu6502 *cpu, Cpu6502Cycle cycle_cb) {
    if (cpu) cpu->cycle=cycle_cb;
}

bool cpu6502_call(Cpu6502 *cpu, uint16_t address, uint32_t instruction_limit,
                  char *error, unsigned error_size) {
    const uint16_t sentinel=0x7FFFu;
    uint32_t count=0;
    if (!cpu||!cpu->read||!cpu->write) {
        if(error&&error_size) snprintf(error,error_size,"invalid CPU callbacks");
        return false;
    }
    cpu->stopped=false; cpu->sp=0xFD;
    push(cpu,(uint8_t)(sentinel>>8)); push(cpu,(uint8_t)sentinel); cpu->pc=address;
    while(cpu->pc!=(uint16_t)(sentinel+1u)) {
        if(count++>=instruction_limit) {
            if(error&&error_size) snprintf(error,error_size,
                "6502 call at $%04X exceeded %u instructions (pc=$%04X)",
                address,instruction_limit,cpu->pc);
            return false;
        }
        if(!step(cpu,error,error_size)) return false;
    }
    if(error&&error_size) error[0]='\0';
    return true;
}
