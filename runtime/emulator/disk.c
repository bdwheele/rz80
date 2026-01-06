#include "rz80.h"
#include "fcntl.h"
#include "unistd.h"
#include "errno.h"

#define READ 1
#define WRITE 2
#define fail(x) {close(state->disk[state->drive].fd); state->disk[state->drive].fd = -1; return x; }


void disk_reset(struct state *state) {
    // I probably do some sanity checking
    state->deblock.valid = 0;
}

int llbn_from_dts(struct state *state) {
    /* get logical block number from the bios disk parameter table */
    int16_t dpb_addr = mem_read_word(state, state->dpbase + state->drive * 16 + 10);
    int trklen = mem_read_word(state, dpb_addr);
    int lbn = trklen * state->track + state->sector;
    return lbn;   
}

int block_op(struct state *state, int operation, int drive, int lbn) {    
    int fd = state->disk[drive].fd;
    if(state->disk[drive].mode == DISK_MODE_READ && operation == WRITE) {
        // the disk was open for reading but we need to write.  Close the
        // current handle so it can be reopened r/w
        close(state->disk[drive].fd);
        fd = -1;
    }
    
    if(fd < 0) {
        // something has happened and we need (re)open the disk
        fd = open(state->disk[drive].name, (operation == WRITE? O_RDWR : O_RDONLY) | O_SYNC);
        if(fd < 0) {
            fail(operation == WRITE? 2 : 1);
        }
        state->disk[drive].mode = operation == WRITE?  DISK_MODE_WRITE : DISK_MODE_READ;
        state->disk[drive].fd = fd;
    }
    if(lseek(fd, lbn * 512, SEEK_SET) < 0) {
        // the seek failed.  blech.
        fail(1);
    }
    int res;
    if(operation == READ) {
        res = read(fd, state->deblock.buffer, 512);
    } else {
        res = write(fd, state->deblock.buffer, 512);
    }
    if(res < 0) {
        fail(1);
    } 
    return 0;
}

int disk_read(struct state *state) {
    int llbn = llbn_from_dts(state);
    int plbn = llbn / 4;
    int loff = llbn % 4;
    int read_cache = 0, flush_cache = 0;
    int cache_valid = state->deblock.valid & (state->deblock.drive == state->drive) & (state->deblock.lbn == plbn);
    if(!cache_valid) {
        //debug("READ Disk %d/%d/%d (%d/%d), Cache is not valid (v:%d, d:%d, l:%d)\n", 
        //      state->drive, state->track, state->sector, plbn, loff,
        //      state->deblock.valid, state->deblock.drive, state->deblock.lbn);
            
        if(state->deblock.valid && state->deblock.dirty) {
            // we have the wrong block in cache and it's dirty so we write it.
            //debug("Flushing old block\n");
            if(block_op(state, WRITE, state->deblock.drive, state->deblock.lbn)) {
                // failed to write dirty block
                return 1;
            }
            state->deblock.dirty = 0;
        }
        state->deblock.valid = 0;
        state->deblock.drive  = state->drive;
        state->deblock.lbn = plbn;
        if(block_op(state, READ, state->deblock.drive, state->deblock.lbn)) {
            // failed to read the new block
            return 1;
        }
        state->deblock.valid = 1;
        //debug("READ updated cache v:%d, d:%d, l:%d\n", 
        //      state->deblock.valid, state->deblock.drive, state->deblock.lbn);
    } else {
        //debug("READ Disk %d/%d/%d (%d/%d), Cache is valid (v:%d, d:%d, l:%d)\n", 
        //      state->drive, state->track, state->sector, plbn, loff,
        //      state->deblock.valid, state->deblock.drive, state->deblock.lbn);
    }
    // return the data from the cache.
    for(int i = 0; i < 128; i++) {
        *(state->memory + state->dma + i) = *(state->deblock.buffer + (loff * 128) + i);
    }
    return 0;
}

int disk_write(struct state *state, int type) {
    int llbn = llbn_from_dts(state);
    int plbn = llbn / 4;
    int loff = llbn % 4;
    int cache_valid = state->deblock.valid & (state->deblock.drive == state->drive) & (state->deblock.lbn == plbn);
    if(!cache_valid) {
        if(state->deblock.valid && state->deblock.dirty) {
            // we have the wrong block in cache and it's dirty so we write it.
            if(block_op(state, WRITE, state->deblock.drive, state->deblock.lbn)) {
                // failed to write dirty block
                return 1;
            }
            state->deblock.dirty = 0;
        }
        state->deblock.valid = 0;
        state->deblock.drive  = state->drive;
        state->deblock.lbn = plbn;
        if(block_op(state, READ, state->deblock.drive, state->deblock.lbn)) {
            // failed to read the new block
            return 1;
        }
        state->deblock.valid = 1;
    }
    // write our data into the cache and mark it dirty.
    for(int i = 0; i < 128; i++) {
        *(state->deblock.buffer + (loff * 128) + i) = *(state->memory + state->dma + i);
    }
    state->deblock.dirty = 1;
    
    if(type == 1) {
        // a directory write, so we have to flush it now.
        if(block_op(state, WRITE, state->deblock.drive, state->deblock.lbn)) {
            return 1;
        }
        state->deblock.dirty = 0;
    }
    return 0;
}

