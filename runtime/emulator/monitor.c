#include <stdio.h>
#include "monitor.h"
#include "emulator.h"
#include "z80ex_dasm.h"
#include "z80ex.h"
#include "memory.h"

char op_buffer[128];
char *dump_opcode(struct emulator *emulator, int addr) {
    int t0, t1;
    z80ex_dasm((char *)&op_buffer, 128, 0, &t0, &t1, read_debug_memory, addr, emulator->ram);
    return (char *)&op_buffer;
}


#define GETREG(reg) z80ex_get_reg(emulator->cpu, reg)
void dump_regs(struct emulator *emulator) {
    printf("PC: %04x  AF: %04x BC: %04x DE: %04x HL: %04x IX: %04x\n",
           GETREG(regPC), GETREG(regAF), GETREG(regBC), GETREG(regHL), GETREG(regIX)); 
    //printf("SP: %04x  AF' %04x BC' %04x DE' %04x HL' %04x IY: %04x\n",
    //       GETREG(regSP), GETREG(regAF_), GETREG(regBC_), GETREG(regHL_), GETREG(regIY));
}

char offset_buffer[32];
char *base_offset(struct emulator *emulator, int addr) {
    char *base = "MBASE";
    int offset = 0;
    if(addr >= emulator->base.bbase) {
        // bios
        base = "BBASE";
        offset = emulator->base.bbase;
    } else if(addr >= emulator->base.fbase) {
        // bdos
        base = "FBASE";
        offset = emulator->base.fbase;
    } else if(addr >= emulator->base.cbase) {
        // ccp
        base = "CBASE";
        offset = emulator->base.cbase;
    } else if(addr >= 0x100) {
        // tbase
        base = "TBASE";
        offset = 0x100;
    }
    snprintf((char *)offset_buffer, 128, "%s+%04x", base, addr - offset);
    return (char *)offset_buffer;
}


/*
    Monitor 
    s - single step
    d - deposit
    x - examine
    r - registers
    g - go to address
    c - continue at pc
at some point in the future.  Right now, it's just dumping what's going on.
*/
void monitor(struct emulator *emulator) {
    dump_regs(emulator);
    int pc = GETREG(regPC);
    printf("%04x (%s): %s\n", pc, base_offset(emulator, pc), dump_opcode(emulator, pc));
    z80ex_step(emulator->cpu);
}