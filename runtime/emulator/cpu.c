#include "rz80.h"


Z80EX_BYTE cpu_mem_read(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *user_data) {
    return mem_read_byte(addr);
}

void cpu_mem_write(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *user_data) {
    mem_write_byte(addr, value);
}


Z80EX_BYTE cpu_port_read(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *user_data) {
    debug("Read port %d\n", port);
}

void cpu_port_write(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *user_data) {
    port &= 0xff;
    static char *svcs[] = {"Cold boot", "Warm boot", "console status", "console input", "console output",
                           "list output", "punch output", "reader in", "home disk",
                           "select disk", "set track", "set sector", "set dma", "read block",
                           "write block", "list status", "sector translate"};
    char *svc_name = "Unknown";
    if(port < 17) {
        svc_name = svcs[port];
    }
    if(state->trace_port)
        debug("Port write %02x (%s), value (%02x) PC=%04x, AF=%04x, BC=%04x, DE=%04x SP=%04x HL=%04x\n",
            port, svc_name,  value,
            z80ex_get_reg(state->cpu, regPC), z80ex_get_reg(state->cpu, regAF), z80ex_get_reg(state->cpu, regBC),
            z80ex_get_reg(state->cpu, regDE), z80ex_get_reg(state->cpu, regSP), z80ex_get_reg(state->cpu, regHL));
    switch(port) {
        case 1:
            // warm boot
            mem_reset();
            disk_reset();
            break;
        case 2:
            // console status
            setA(terminal_status());
            break;        
        case 3:
            // console input
            setA(terminal_read());
            break;
        case 4:
            // console output
            terminal_write(getC());
            break;
        case 5:
            // list output (C)
            break;
        case 6:
            // punch output (C)
            break;
        case 7:
            // reader in (A)
            break;
        case 8:
            /// home disk
            state->track = 0;
            break;
        case 9:
            // select disk
            int dsk = getC();
            if(dsk < 0 || dsk > 2 || state->disk[dsk].type == DISK_NONE) {
                setHL(0);
            } else {
                state->drive = dsk;
                // if bit zero of E is 0 then it's a new disk, otherwise it's been seen before...
                setHL(state->dpbase + dsk * 16);
            }
            break;
        case 10:
            // set track
            state->track = getBC();
            break;
        case 11:
            // set sector
            state->sector = getBC();
            break;
        case 12:
            // set dma
            state->dma = getBC();
            break;
        case 13:
            // read block
            setA(disk_read());
            break;
        case 14:
            // write block
            setA(disk_write(getC()));
            break;
        case 15:
            // list status
            break;
        case 16:
            // translate sector
            setHL(getBC());
            break;
        default:
            debug("Unknown port %02x write.\n", port);
    }
}
