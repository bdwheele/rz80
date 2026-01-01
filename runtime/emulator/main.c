#include <string.h>
#include <malloc.h>
#include "rz80.h"

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
    /*
    state->disk[0] = fopen("disk_a.img", "r+");
    state->disk[1] = fopen("disk_b.img", "r+");
    state->disk[2] = fopen("disk_c.img", "r+");
        */
    state->disk_names[0] = "disk_a.img";
    state->disk_names[1] = "disk_b.img";
    state->disk_names[2] = "disk_c.img";
    for(int i=0; i < 3; i++) {
        state->disk_fd[i] = -1;
    }

    // terminal
    terminal_setup(state);

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

    terminal_restore(state);
}