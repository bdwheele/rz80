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

#define ReadA() ((z80ex_get_reg(cpu, regAF) & 0xff00) >> 8)
#define ReadBC() (z80ex_get_reg(cpu, regBC))
#define ReadC() (z80ex_get_reg(cpu, regBC) & 0xff)
#define ReadDE() (z80ex_get_reg(cpu, regDE))
#define ReadE() (z80ex_get_reg(cpu, regDE) & 0xff)
#define ReadF() (z80ex_get_reg(cpu, regAF) & 0xff)
#define ReadHL() (z80ex_get_reg(cpu, regHL))


#define WriteA(v) z80ex_set_reg(cpu, regAF, (((v & 0xff)) << 8) + ReadF())
#define WriteHL(v) z80ex_set_reg(cpu, regHL, v)



fd_set fds;
struct timeval tv;





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


    FD_ZERO(&fds); 
    FD_SET(STDIN_FILENO, &fds); 

    tv.tv_usec = 50;   

    int lastpc = 0xffff;
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