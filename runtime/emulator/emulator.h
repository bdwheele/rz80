#ifndef _EMULATOR_H_
#define _EMULATOR_H_
#include "z80ex.h"

// return values for disk block read/write
#define DISK_OK 0
#define DISK_ERR 1
#define DISK_RO 2
#define DISK_CHANGED 0xff
#define DEV_READY 0
#define DEV_BUSY 1

// important addresses in emulator RAM
#define IOBYTE 3
#define CURDISK 4
#define DPHSIZE 16
#define DPBASE 0x46
#define CCPBASE 0x40

// macros to read/write memory
#define ReadMem(addr) (emulator->ram[addr] & 0xff)
#define ReadMemWord(addr) (ReadMem(addr) + (ReadMem(addr + 1) << 8))
#define WriteMem(addr, v) emulator->ram[addr] = v & 0xff
#define WriteMemWord(addr, v) WriteMem(addr, v); WriteMem(addr + 1, v >> 8)

#define DEBUG(args...) fprintf(stderr, "DEBUG: " args)

struct emulator;

struct hwimpl {
    void *(*startup)(struct emulator *emulator);
    void (*shutdown)(struct emulator *emulator);
    void (*poll)(struct emulator *emulator);
    void (*warmboot)(struct emulator *emulator);
    int (*console_ready)(struct emulator *emulator);
    char (*console_read)(struct emulator *emulator);
    void (*console_write)(struct emulator *emulator, char c);
    int (*list_status)(struct emulator *emulator);
    void (*list_write)(struct emulator *emulator, char c);
    char (*aux_read)(struct emulator *emulator);
    void (*aux_write)(struct emulator *emulator, char c);
    int (*home_disk)(struct emulator *emulator, int disk);
    int (*read_block)(struct emulator *emulator, int disk, long offset, void *buffer);
    int (*write_block)(struct emulator *emulator, int disk, long offset, void *buffer);
};

struct emulator {
    Z80EX_CONTEXT *cpu;
    Z80EX_BYTE *ram;
    struct hwimpl *hwimpl;
    void *hwimpldata;
    // emulator state
    int memory_fd;
    int dph_base;
    int halt;

    // bios state
    struct {
        int disk;
        int dma;
        int track;
        int sector;
    } bios;
};






#endif