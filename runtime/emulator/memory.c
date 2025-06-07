#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "memory.h"

void load_memory_image(Z80EX_CONTEXT *cpu, struct emulator *emulator) {
    // Load the system memory from memory image.  
    // first, load the low storage page
    lseek(emulator->memory_fd, 0, SEEK_SET);
    read(emulator->memory_fd, emulator->ram, 0x100);
    DEBUG("Initial ops: %02x %04x\n", ReadMem(0), ReadMemWord(1));
    
    // load the bases
    emulator->base.cbase = ReadMemWord(CBASE);
    emulator->base.fbase = ReadMemWord(FBASE);
    emulator->base.bbase = ReadMemWord(BBASE);
    emulator->base.dph_base = ReadMemWord(DPBASE);


    // The CCP, BDOS, and BIOS start at the word stored at memory location at 0x40
    int ccp_offset = emulator->base.cbase;
    DEBUG("CCP Base at %04x\n", ccp_offset);
    lseek(emulator->memory_fd, ccp_offset, SEEK_SET);
    read(emulator->memory_fd, emulator->ram + ccp_offset, 0x10000);
    DEBUG("First BIOS word at %04x is %04x\n", ReadMemWord(0x44), ReadMemWord(ReadMemWord(0x44)));
    // store the dph base 
    emulator->dph_base = ReadMemWord(DPBASE);    
}


/* The callbacks */
Z80EX_BYTE read_memory(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *data) {
    struct emulator *emulator = (struct emulator *)data;
    Z80EX_BYTE v = emulator->ram[addr];
    //int pc = z80ex_get_reg(cpu, regPC) - 1;
    //if(m1_state != 1 && pc != addr)
    //    fprintf(stderr, "[%04x (%d) TRACE MEMORY READ %04x => %02x]\n", pc, m1_state, addr, v);
    return v;
}

Z80EX_BYTE read_debug_memory(Z80EX_WORD addr, void *data) {
    char *ram = data;
    return ram[addr];
}

void write_memory(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *data) {
    struct emulator *emulator = (struct emulator *)data;
    //fprintf(stderr, "[%04x TRACE MEMORY WRITE %04x => %02x]\n", z80ex_get_reg(cpu, regPC), addr, value);
    emulator->ram[addr] = value;
}
