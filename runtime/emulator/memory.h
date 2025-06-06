#include "z80ex.h"
#include "emulator.h"

void load_memory_image(Z80EX_CONTEXT *cpu, struct emulator *emulator);
Z80EX_BYTE read_memory(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *data);
Z80EX_BYTE read_debug_memory(Z80EX_WORD addr, void *data);
void write_memory(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *data);
