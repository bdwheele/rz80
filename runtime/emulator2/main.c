#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include "../z80ex-1.1.21/include/z80ex.h"
#include "../z80ex-1.1.21/include/z80ex_dasm.h"
#include <termios.h>
#include <stdlib.h>
#include <sys/select.h>

#define debug(fmt, ...) fprintf(state->log, fmt, ##__VA_ARGS__) 

struct state {
    int trace;
    int halted;
    FILE *log;
    Z80EX_CONTEXT *cpu;
    uint8_t *memory;
    uint8_t *rom;
    uint16_t romsize;
    uint16_t dma;
    uint8_t drive;
    uint16_t track;
    uint16_t sector;
    uint16_t cbase;
    uint16_t fbase;
    uint16_t bbase;
    uint16_t dpbase;
    FILE *disk[3];
};

uint8_t mem_read_byte(struct state *state, uint16_t addr) {
    return state->memory[addr];
}

void mem_write_byte(struct state *state, uint16_t addr, uint8_t value) {
    state->memory[addr] = value;
}

uint16_t mem_read_word(struct state *state, uint16_t addr) {
    return state->memory[addr] + (state->memory[addr + 1] * 256);
}

void mem_write_word(struct state *state, uint16_t addr, uint16_t value) {
    state->memory[addr] = value & 0xff;
    state->memory[addr + 1] = (value & 0xff00) >> 8;
}

void mem_init(struct state *state) {
    // reload the rom
    for(int i = 0; i < state->romsize; i++) {
        state->memory[i + state->cbase] = state->rom[i];
    }
    // reset the jump vectors to make sure
    mem_write_byte(state, 0, 0xc3);
    mem_write_word(state, 1, state->bbase);
    mem_write_byte(state, 5, 0xc3);
    mem_write_word(state, 6, state->fbase);
}

int lba_from_dts(struct state *state) {
    int16_t dpb_addr = mem_read_word(state, state->dpbase + state->drive * 16 + 10);
    int trklen = mem_read_word(state, dpb_addr);
    int lba = trklen * state->track + state->sector;
    return lba;   
}

int disk_read(struct state *state) {
    fseek(state->disk[state->drive], lba_from_dts(state) * 128, SEEK_SET);
    fread(state->memory + state->dma, 1, 128, state->disk[state->drive]);
}

int disk_write(struct state *state) {
    fseek(state->disk[state->drive], lba_from_dts(state) * 128, SEEK_SET);
    fwrite(state->memory + state->dma, 1, 128, state->disk[state->drive]);
}



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

    fprintf(state->log,
            "%6s:%04x %-12s %-15s AF=%04x BC=%04x DE=%04x SP=%04x HL=%04x\n",
            base_name, addr - base_addr, 
            byte_dump, disasm,
            z80ex_get_reg(state->cpu, regAF), z80ex_get_reg(state->cpu, regBC),
            z80ex_get_reg(state->cpu, regDE), z80ex_get_reg(state->cpu, regSP),
            z80ex_get_reg(state->cpu, regHL));

}



#define _get(reg) z80ex_get_reg(state->cpu, reg)
#define _set(reg, value) z80ex_set_reg(state->cpu, reg, value)
#define _get_low(reg) (z80ex_get_reg(state->cpu, reg) & 0xff)
#define _get_high(reg) (z80ex_get_reg(state->cpu, reg) >> 8)
#define _set_low(reg, value) (z80ex_set_reg(state->cpu, reg, z80ex_get_reg(state->cpu, reg) & 0xff00 + value))
#define _set_high(reg, value) (z80ex_set_reg(state->cpu, reg, \
                               (z80ex_get_reg(state->cpu, reg) & 0xff) + (value << 8)))
#define getA() _get_high(regAF)
#define setA(value) _set_high(regAF, value)
#define getC() _get_low(regBC)
#define setC(value) _set_low(regBC, value)
#define getBC() _get(regBC)
#define setBC(value) _set(regBC, value)
#define getHL() _get(regHL)
#define setHL(value) _set(regHL, value)
#define getPC() _get(regPC)


Z80EX_BYTE cpu_mem_read(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *user_data) {
    return mem_read_byte(user_data, addr);
}

void cpu_mem_write(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *user_data) {
    mem_write_byte(user_data, addr, value);
}

Z80EX_BYTE cpu_port_read(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *user_data) {

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
    struct state *state = (struct state *)user_data;
    debug("Port write %02x (%s), value (%02x) PC=%04x, AF=%04x, BC=%04x, DE=%04x SP=%04x HL=%04x\n",
        port, svc_name,  value,
        z80ex_get_reg(state->cpu, regPC), z80ex_get_reg(state->cpu, regAF), z80ex_get_reg(state->cpu, regBC),
            z80ex_get_reg(state->cpu, regDE), z80ex_get_reg(state->cpu, regSP), z80ex_get_reg(state->cpu, regHL));
    switch(port) {
        case 1:
            // warm boot
            mem_init(state);
            break;
        case 2:
            // console status
            fd_set fds;
            FD_ZERO(&fds);
            int stdin_fileno = fileno(stdin);
            FD_SET(stdin_fileno, &fds);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100; // 100us wait time
            select(stdin_fileno + 1, &fds, NULL, NULL, &tv);
            if(FD_ISSET(stdin_fileno, &fds)) {
                setA(0xff);
            } else {
                setA(0x00);
            }
            break;        
        case 3:
            // console input
            char c;
            fread(&c, 1, 1, stdin);
            if(c == 0x11) {
                // ctrl-q halts.
                state->halted = 1;
            } else {
                setA(c);
            }
            break;
        case 4:
            // console output
            fputc(getC() &0x7f, stdout);
            fflush(stdout);
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
            if(dsk < 0 || dsk > 2) {
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
            disk_read(state);
            break;
        case 14:
            // write block
            disk_write(state);
            setA(0);
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

#define le_word(p) (*(p) + (*((p) + 1) * 256))
void load_rom(struct state *state, char *filename) {
    state->rom = calloc(8192, 1);
    FILE *f = fopen(filename, "rb");
    state->romsize = fread(state->rom, 1, 8192, f);
    fclose(f);
    int sig = le_word(state->rom + state->romsize - 2);
    if(sig != 0x55aa) {
        printf("ERROR: ROM Image has invalid signature: %04x\n", sig);
        exit(1);
    }
    /* load our bases*/
    state->dpbase = le_word(state->rom + state->romsize - 4);
    state->bbase = le_word(state->rom + state->romsize - 6);
    state->fbase = le_word(state->rom + state->romsize - 8);
    state->cbase = le_word(state->rom + state->romsize - 10);
    debug("Bases loaded: CBASE=%04x, FBASE=%04x, BBASE=%04x, DPBASE=%04x\n", 
          state->cbase, state->fbase, state->bbase, state->dpbase);
}


int main(int argc, char *argv[]) {
    struct state *state = calloc(1, sizeof(struct state));
    for(int i = 0; i < argc; i++) {
        if(!strcmp(argv[i], "-t")) {
            state->trace = 1;
        }
    }
    // logging
    state->log = fopen("emulator.log", "a");
    if(!state->log) {
        perror("Failed to open log: ");
        exit(1);
    }   
    // cpu
    state->cpu = z80ex_create(cpu_mem_read, state,
                              cpu_mem_write, state,
                              cpu_port_read, state,
                              cpu_port_write, state,
                              NULL, NULL);
    // memory systems
    state->memory = calloc(65536, 1);
    load_rom(state, "system.rom");

    


    mem_init(state);

    // disk systems
    state->disk[0] = fopen("disk_a.img", "r+");
    state->disk[1] = fopen("disk_b.img", "r+");
    state->disk[2] = fopen("disk_c.img", "r+");

    // terminal
    struct termios old_settings, new_settings;
    tcgetattr(fileno(stdin), &old_settings);
    tcgetattr(fileno(stdin), &new_settings);
    // from python's tty.setraw
    new_settings.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP |
                     INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF);
    new_settings.c_oflag &= ~OPOST;
    new_settings.c_cflag &= ~(PARENB | CSIZE);
    new_settings.c_cflag |= CS8;
    new_settings.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON |
                     IEXTEN | ISIG | NOFLSH | TOSTOP);
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(fileno(stdin), TCSAFLUSH, &new_settings);

    // main loop   
    int counter = 100000;
    while(!state->halted) {
        if(state->trace) {
            trace(state, getPC());
        }
        while(1) {
            z80ex_step(state->cpu);
            if(z80ex_last_op_type(state->cpu) == 0) {
                break;
            }
        }
        counter--;
        if(counter < 0) {
            //state->halted = 1;
        }
    }

    tcsetattr(fileno(stdin), TCSAFLUSH, &old_settings);


}