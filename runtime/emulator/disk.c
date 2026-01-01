#include "rz80.h"

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
