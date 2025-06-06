#include <stdio.h>
#include "monitor.h"

void monitor(struct emulator *emulator) {
    int pc = z80ex_get_reg(emulator->cpu, regPC);    
    fprintf(stderr, "[TRACE: PC %04x -> %02x]\n", pc, emulator->ram[pc]);
}