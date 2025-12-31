#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include "posix_hw.h"
/*
    Hardware implementation for POSIX-based virtual devices
*/

#define KBD_BUF_SIZE 80

struct posix_state {
    int drives[3];
    int con_in;
    int con_out;
    int list_out;
    int aux_in;
    int aux_out;
    fd_set con_set;
    struct timeval con_tv;
    char kbd_buffer[KBD_BUF_SIZE];
    int kbd_rd;
    int kbd_wr;
};


void poll_keyboard(struct emulator *emulator) {
    // read the keyboad and fill up the buffer */
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    //DEBUG("Polling keyboard\n");
    while(1) {

        /*        
        select(state->con_in + 1, &state->con_set, NULL, NULL, &state->con_tv); 
        if(!FD_ISSET(state->con_in, &state->con_set)) {
            // no characters are waiting
            DEBUG("No character waiting on %i\n", STDIN_FILENO);
            break;
        } else {
            if((state->kbd_wr + 1) % KBD_BUF_SIZE == state->kbd_rd) {
                // buffer is full, just break out of the loop
                DEBUG("KBD Buffer is full\n");
                break;
            } else {
                char next_char;
                if(read(STDIN_FILENO, &next_char, 1) > 0) {
                    DEBUG("Read char %c\n", next_char);                                
                    state->kbd_buffer[state->kbd_wr] = next_char;
                    state->kbd_wr = (state->kbd_wr + 1) % KBD_BUF_SIZE;
                }
            }
        }
            */

        char next_char;
        if(read(STDIN_FILENO, &next_char, 1) > 0) {
            DEBUG("Read char %c\n", next_char);
            state->kbd_buffer[state->kbd_wr] = next_char;
            state->kbd_wr = (state->kbd_wr + 1) % KBD_BUF_SIZE;

        } else {
            //DEBUG("NO character ready\n");
        }
    }
}



void posix_poll(struct emulator *emulator) {
    /* Poll the hardware. */ 
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    poll_keyboard(emulator);    

}


void posix_warmboot(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
}

int posix_console_ready(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    return state->kbd_wr != state->kbd_rd? DEV_READY : DEV_BUSY;
    /*
    select(state->con_in + 1, &state->con_set, NULL, NULL, &state->con_tv); 
    return FD_ISSET(state->con_in, &state->con_set)? DEV_READY : DEV_BUSY;
    */
}


char posix_console_read(struct emulator *emulator) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    /*
    char chr = 0;
    do {
        read(STDIN_FILENO, &chr, 1);
    } while(!chr);              
    return chr;
    */
    
    while(state->kbd_rd == state->kbd_wr) {
        // buffer is empty.  Poll until we have something
        poll_keyboard(emulator);
    }
    char chr = state->kbd_buffer[state->kbd_rd];
    DEBUG("Character read: %c\n", chr);
    state->kbd_rd = (state->kbd_rd + 1) % KBD_BUF_SIZE;
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


#define OP_NONE 0
#define OP_READ 1
#define OP_WRITE 2

int block_op(struct posix_state *state, int op, int disk, int lba, void *buffer) {
    if(lseek(state->drives[disk], lba * 128, SEEK_SET) < 0) {
        perror("lseek: ");
        return DISK_ERR;
    }
    int e = 0;
    char *opname = "None";
    switch(op) {
        case OP_NONE:
            break;
        case OP_READ:
            e = read(state->drives[disk], buffer, 128);
            opname = "read: ";
            break;
        case OP_WRITE:
            e = write(state->drives[disk], buffer, 128);
            opname = "write: ";
            break;
    }
    if(e < 0) {
        perror(opname);
        return DISK_ERR;
    }
    return DISK_OK;
}


int posix_home_disk(struct emulator *emulator, int disk) {
    // home the head
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    //int e = lseek(state->drives[disk], 0, SEEK_SET);
    //fprintf(stderr, "LSEEK: %d\n", e);
    return block_op(state, OP_NONE, disk, 0, NULL);
}


int posix_read_block(struct emulator *emulator, int disk, int lba, void *buffer) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;    
    return block_op(state, OP_READ, disk, lba, buffer);
}


int posix_write_block(struct emulator *emulator, int disk, int lba, void *buffer) {
    struct posix_state *state = (struct posix_state *)emulator->hwimpldata;
    return block_op(state, OP_WRITE, disk, lba, buffer);
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
        int e = open(filename, O_RDWR);
        if(e < 0) {
            perror(filename);
        }       
        state->drives[i] = e;
    }

    // set up the select on stdin
    FD_ZERO(&(state->con_set));
    FD_SET(state->con_in, &state->con_set);

    state->con_tv.tv_sec = 0;
    state->con_tv.tv_usec = 100;

    // set up the keyboard buffer
    state->kbd_rd = state->kbd_wr = 0;

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);


}