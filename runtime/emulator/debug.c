#include "rz80.h"

Z80EX_BYTE debug_mem_read(Z80EX_WORD addr, void *user_data) {
    return mem_read_byte(user_data, addr);
}


void trace(struct state *state, uint16_t addr) {
    static char *range[] = {"DPBASE", "  BIOS", "  BDOS", "   CCP", "     "};
    int base_addr = 0;
    char *base_name = range[4];
    if(addr >= state->dpbase) {
        base_name = range[0];
        base_addr = state->dpbase;
    } else if(addr >= state->bbase) {
        base_name = range[1];
        base_addr = state->bbase;
    } else if(addr >= state->fbase) {
        base_name = range[2];
        base_addr = state->fbase;
    } else if(addr >= state->cbase) {
        base_name = range[2];
        base_addr = state->cbase;
    }
    char disasm[32], byte_dump[16];
    int t_state, t_state2;
    int bytecount = z80ex_dasm(disasm, 32, 0, &t_state, &t_state2, debug_mem_read, addr, state);
    char *p = byte_dump;
    for(int i = 0; i < 5; i++) {
        if(i < bytecount) {
            sprintf(p, "%02x ", mem_read_byte(state, addr + i));
        } else {
            sprintf(p, "   ");
        }
        p += 3;
        *p = ' ';
    }
    *p = 0;

    if(state->log) 
        fprintf(state->log,
                "%6s:%04x %-12s %-15s AF=%04x BC=%04x DE=%04x SP=%04x HL=%04x\n",
                base_name, addr - base_addr, 
                byte_dump, disasm,
                z80ex_get_reg(state->cpu, regAF), z80ex_get_reg(state->cpu, regBC),
                z80ex_get_reg(state->cpu, regDE), z80ex_get_reg(state->cpu, regSP),
                z80ex_get_reg(state->cpu, regHL));

}
