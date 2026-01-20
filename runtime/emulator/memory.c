#include "rz80.h"

uint8_t mem_read_byte(uint16_t addr) {
    return state->memory[addr];
}

void mem_write_byte(uint16_t addr, uint8_t value) {
    state->memory[addr] = value;
}

uint16_t mem_read_word(uint16_t addr) {
    return state->memory[addr] + (state->memory[addr + 1] * 256);
}

void mem_write_word(uint16_t addr, uint16_t value) {
    state->memory[addr] = value & 0xff;
    state->memory[addr + 1] = (value & 0xff00) >> 8;
}

void mem_reset() {
    // reload the rom
    for(int i = 0; i < state->romsize; i++) {
        state->memory[i + state->cbase] = state->rom[i];
    }
    // reset the jump vectors to make sure
    mem_write_byte(0, 0xc3);
    mem_write_word(1, state->bbase);
    mem_write_byte(5, 0xc3);
    mem_write_word(6, state->fbase);
}


#define le_word(p) (*(p) + (*((p) + 1) * 256))
void load_rom(char *filename) {
    printf("Allocating rom space, state = %p, filename=%s\n", state, filename);
    state->rom = calloc(8192, 1);
    printf("Loading rom %s\n", filename);
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

