#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include "z80ex.h"
#include "z80ex_dasm.h"

#include "emulator.h"
#include "posix_hw.h"
#include "memory.h"
#include "port_handler.h"
#include "monitor.h"

#define MEMORY_IMAGE "emulator.mem"

int main(int argc, char *argv[]) {
    /* Create the main emulator structure*/
    struct emulator *emulator = (struct emulator *)calloc(1, sizeof(struct emulator));
    emulator->cpu = z80ex_create(read_memory, emulator, 
                                 write_memory, emulator,
                                 NULL, NULL,
                                 write_port, emulator, 
                                 NULL, NULL);
    emulator->ram = (Z80EX_BYTE *)malloc(0x10000);
    emulator->memory_fd = open(MEMORY_IMAGE, O_RDONLY);
    load_memory_image(emulator->cpu, emulator);

    // Populate the hardware implementation
    posix_init(emulator);

    // Set up the initial emulator state.
    emulator->halt = 0;
    z80ex_set_reg(emulator->cpu, regPC, 0);

    while(1) {
        // poll the hardware state
        if(emulator->hwimpl->poll)
            emulator->hwimpl->poll(emulator);

        if(emulator->halt) {
            monitor(emulator);
        } else {
            z80ex_step(emulator->cpu);
        }
    }

}