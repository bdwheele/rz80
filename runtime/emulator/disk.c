#include "rz80.h"
#include "fcntl.h"
#include "unistd.h"
#include "errno.h"

int lba_from_dts(struct state *state) {
    int16_t dpb_addr = mem_read_word(state, state->dpbase + state->drive * 16 + 10);
    int trklen = mem_read_word(state, dpb_addr);
    int lba = trklen * state->track + state->sector;
    return lba;   
}

#define READ 1
#define WRITE 2

#define fail(x) {setA(x); state->disk[state->drive].fd = -1; return -1; }


int disk_op(struct state *state, int operation) {
    int fd = state->disk[state->drive].fd;
    if(fd < 0) {
        // something has happened and we need (re)open the disk
        fd = open(state->disk[state->drive].name, O_RDWR | O_SYNC);
        if(fd < 0) {
            fail(1);
        }
        state->disk[state->drive].fd = fd;
    }

    if(state->disk[state->drive].type == DISK_FLOPPY) {
        // if we're changing tracks we'll end up with a head settle time
        if(state->track != state->disk[state->drive].track) {
            msleep(15);
        }
        // Fake the track seek time. Teac FD-235 has a 3ms track-to-track seek.
        msleep(3 * abs(state->track - state->disk[state->drive].track));
        state->disk[state->drive].track = state->track;
    
    }

    if(lseek(fd, lba_from_dts(state) * 128, SEEK_SET) < 0) {
        // the seek failed.  blech.
        fail(1);
    }
    int res;
    if(operation == READ) {
        res = read(fd, state->memory + state->dma, 128);
    } else {
        res = write(fd, state->memory + state->dma, 128);
    }
    if(res < 0) {
        fail(1);
    } 
    setA(0);
    return 0;
}


int disk_read(struct state *state) {
    return disk_op(state, READ);
}

int disk_write(struct state *state) {
    return disk_op(state, WRITE);
}


void disk_init(struct state *state) {
    // I probably do some sanity checking
}
