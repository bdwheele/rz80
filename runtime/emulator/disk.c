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

int disk_read(struct state *state) {
    //fseek(state->disk[state->drive], lba_from_dts(state) * 128, SEEK_SET);
    //fread(state->memory + state->dma, 1, 128, state->disk[state->drive]);
    int fd = open(state->disk_names[state->drive], O_RDONLY);
    if(fd < 0) {
        setA(1); // error on open
    } else {
        if(lseek(fd, lba_from_dts(state) * 128, SEEK_SET) < 0) {
            setA(1); //error on seek
        } else {
            if(read(fd, state->memory + state->dma, 128) < 0) {
                setA(1); // error on read
            } else {
                setA(0);
            }
        }
        close(fd);
    }
}

int disk_write(struct state *state) {
    //fseek(state->disk[state->drive], lba_from_dts(state) * 128, SEEK_SET);
    //fwrite(state->memory + state->dma, 1, 128, state->disk[state->drive]);
    int fd = open(state->disk_names[state->drive], O_RDWR);
    if(fd < 0) {
        printf("Error opening %s for write: %d\n", state->disk_names[state->drive], errno);
        setA(fd == EROFS? 2 : 1); // error on open
    } else {
        if(lseek(fd, lba_from_dts(state) * 128, SEEK_SET) < 0) {
            setA(1); // error on seek
        } else {
            if(write(fd, state->memory + state->dma, 128) < 0) {
                printf("Error writing %s error: %d\n", state->disk_names[state->drive], errno);
                setA(1);
            } else {
                setA(0);
            }
        }
        close(fd);
    }

}
