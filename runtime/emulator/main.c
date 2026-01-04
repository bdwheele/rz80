#include <string.h>
#include <malloc.h>
#include <unistd.h>

#include "rz80.h"
extern char *optarg;

int main(int argc, char *argv[]) {
    // set up the default system state
    struct state *state = calloc(1, sizeof(struct state));
    state->disk_names[0] = "disk_a.img";
    state->disk_names[1] = "disk_b.img";
    state->disk_names[2] = "disk_c.img";
    for(int i=0; i < 3; i++) {
        state->disk_fd[i] = -1;
    }    
    
    // parse arguments
    int opt;
    char *romname = "system.rom";
    while ((opt = getopt(argc, argv, "tpl:r:a:b:c:T:")) != -1) {
        switch (opt) {
            case 't':
                info("Instruction tracing enabled\n");
                state->trace_instruction = 1;
                break;
            case 'p':
                info("BIOS Port tracing enabled\n");
                state->trace_port = 1;
                break;
            case 'l':
                info("Using %s as the log file\n", optarg);
                state->log = fopen(optarg, "a");
                if(!state->log) {
                    perror("Failed to open log: ");
                    exit(1);
                }   
                break;
            case 'r':
                info("Using %s as the rom file\n", optarg);
                romname = optarg;
                break;
            case 'a':
                info("Using %s for drive a\n", optarg);
                state->disk_names[0] = optarg;
                break;
            case 'b':
                info("Using %s for drive b\n", optarg);
                state->disk_names[1] = optarg;
                break;
            case 'c':
                info("Using %s for drive c\n", optarg);
                state->disk_names[2] = optarg;
                break;
            case 'T':
                state->throttle = strtol(optarg, NULL, 10);
                info("Waiting %ld microseconds between every 100 instructions\n", state->throttle);
                break;
            default:
                fprintf(stderr, "Usage %s []\n", argv[0]);
                exit(0);
                break;                
        }
    }
    
    // cpu
    state->cpu = z80ex_create(cpu_mem_read, state,
                              cpu_mem_write, state,
                              cpu_port_read, state,
                              cpu_port_write, state,
                              NULL, NULL);
    // memory systems
    state->memory = calloc(65536, 1);
    load_rom(state, romname);
    mem_init(state);

    // disk systems
    disk_init(state);

    // terminal
    terminal_setup(state);

    // main loop   
    int counter = 0;
    while(!state->halted) {
        if(state->trace_instruction) {
            trace(state, getPC());
        }
        while(1) {
            z80ex_step(state->cpu);
            if(z80ex_last_op_type(state->cpu) == 0) {
                break;
            }
        }
        if(counter++ > 100 && state->throttle) {
            usleep(state->throttle);
            counter = 0;
        }
    }
    terminal_restore(state);
}