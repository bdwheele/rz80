#include <string.h>
#include <malloc.h>
#include <unistd.h>

#include "rz80.h"
extern char *optarg;

struct state *state;

void poll_hardware() {
    state->clock.triggered = 0;
    // poll the hardware
    terminal_poll();
    disk_poll();
}


int main(int argc, char *argv[]) {
    // set up the default system state
    state = calloc(1, sizeof(struct state));
    state->disk[0].name = "disk_a.img";
    state->disk[0].type = DISK_FLOPPY;
    state->disk[1].name = "disk_b.img";
    state->disk[1].type = DISK_NONE;
    state->disk[2].name = "disk_c.img";
    state->disk[2].type = DISK_FIXED;
    for(int i=0; i < 3; i++) {
        state->disk[i].fd = -1;
    }    
    state->deblock.buffer = calloc(1, 512);
    
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
                state->disk[0].name = optarg;
                break;
            case 'b':
                info("Using %s for drive b\n", optarg);
                state->disk[1].name = optarg;
                state->disk[1].type = DISK_FLOPPY;
                break;
            case 'c':
                info("Using %s for drive c\n", optarg);
                state->disk[2].name = optarg;
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
    load_rom(romname);
    mem_reset();

    // disk systems
    disk_reset();
    // terminal
    terminal_setup();

    clock_init();
    clock_start();

    // main loop   
    int sleep_counter = 0, inst_counter = 0, event_timer = 0;
    while(!state->halted) {
        if(state->clock.triggered) {
            poll_hardware();
        }
        if(state->trace_instruction) {
            trace(getPC());
        }
        while(1) {
            z80ex_step(state->cpu);
            if(z80ex_last_op_type(state->cpu) == 0) {
                break;
            }
        }
        if(sleep_counter++ > 100 && state->throttle) {
            usleep(state->throttle);
            sleep_counter = 0;
        }
    }
    terminal_restore();
}