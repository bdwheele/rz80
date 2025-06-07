#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include "posix_hw.h"
/*
    Hardware implementation for POSIX-based virtual devices
*/

struct posix_state {
    int drives[3];
    int con_in;
    int con_out;
    int list_out;
    int aux_in;
    int aux_out;
    fd_set con_set;
    struct timeval con_tv;
};


void posix_poll(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
}

void posix_warmboot(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
}

int posix_console_ready(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    select(state->con_in + 1, &state->con_set, NULL, NULL, &state->con_tv); 
    return FD_ISSET(state->con_in, &state->con_set)? DEV_READY : DEV_BUSY;
}


char posix_console_read(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    char chr = 0;
    do {
        read(STDIN_FILENO, &chr, 1);
    } while(!chr);              
    return chr;
}

void posix_console_write(struct emulator *emulator, char c) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    write(state->con_out, &c, 1);
}


int posix_list_status(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    // the list device is already ready.
    return DEV_READY;
}


void posix_list_write(struct emulator *emulator, char c) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    write(state->list_out, &c, 1);
}


char posix_aux_read(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    static int pos = 0;
    const char chars[] = "0123456789ABCDEF";
    return chars[(pos++ % 16)];
}


void posix_aux_write(struct emulator *emulator, char c) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    // do nothing.
}


int posix_home_disk(struct emulator *emulator, int disk) {
    // home the head
}


int posix_read_block(struct emulator *emulator, int disk, int lba, void *buffer) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;    
    lseek(state->drives[disk], lba * 128, SEEK_SET);
    read(state->drives[disk], buffer, 128);
    return DISK_OK;
}


int posix_write_block(struct emulator *emulator, int disk, int lba, void *buffer) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    lseek(state->drives[disk], lba * 128, SEEK_SET);
    write(state->drives[disk], buffer, 128);
    return DISK_OK;
}


struct hwimpl posix_impl = {
    .poll = posix_poll,
    .warmboot =posix_warmboot,
    .console_ready = posix_console_ready,
    .console_read = posix_console_read,
    .console_write = posix_console_write,
    .list_status = posix_list_status,
    .list_write = posix_list_write,
    .aux_read = posix_aux_read,
    .aux_write = posix_aux_write,
    .home_disk = posix_home_disk,
    .read_block = posix_read_block,
    .write_block = posix_write_block
};

/* Console Setup*/
struct termios orig_termios;


void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_iflag &= ~(ICRNL);
    //raw.c_oflag &= ~(OPOST);
    raw.c_cc[VTIME] = 1;
    raw.c_cc[VMIN] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}



// Initialize the emulator
void posix_init(struct emulator *emulator) {
    emulator->hwimpl = &posix_impl;

    struct posix_state *state = (struct posix_state *)calloc(1, sizeof(struct posix_state));
    emulator->hwimpldata = (void *)state;    
    
    // set up the terminal
    enableRawMode();
    state->con_in = STDIN_FILENO;
    state->con_out = STDOUT_FILENO;

    // send our list device output to a file.
    state->list_out = open("/tmp/list.out", O_CREAT | O_APPEND | O_WRONLY);

    // Open the disk files
    char *filename_pattern = "disk_x.img";
    for(char i = 0; i <= 2; i++) {
        char *filename = strdup(filename_pattern);
        filename[5] = i + 'a';        
        state->drives[i] = open(filename, O_RDWR);
    }

    // set up the select on stdin
    FD_ZERO(&(state->con_set));
    FD_SET(state->con_in, &state->con_set);

    state->con_tv.tv_sec = 0;
    state->con_tv.tv_usec = 100;


}