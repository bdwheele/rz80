#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <termios.h>
#include <time.h>
#include <sys/types.h>
#include "z80ex.h"
#include "z80ex_dasm.h"

#ifndef __RZ80_H

#define DISK_NONE 0
#define DISK_FLOPPY 1
#define DISK_FIXED 2
#define DISK_IMAGE 0
#define DISK_PHYSICAL 1
#define DISK_MODE_READ 0
#define DISK_MODE_WRITE 1
#define DISK_MOTOR_ON 1
#define DISK_MOTOR_OFF 0


#define EVENT_DISK_MOTOR_A 0x0100
#define EVENT_DISK_MOTOR_B 0x0101
#define EVENTS 2


#define KBD_BUF 16

struct state {
    int trace_instruction;
    int trace_port;
    int halted;
    FILE *log;
    Z80EX_CONTEXT *cpu;
    uint8_t *memory;
    uint8_t *rom;
    uint16_t romsize;
    uint16_t dma;
    uint8_t drive;
    uint16_t track;
    uint16_t sector;
    uint16_t cbase;
    uint16_t fbase;
    uint16_t bbase;
    uint16_t dpbase;
    struct {
        int mode;
        char *name;
        int fd;
        int track;
        int motor;
        int type;
        int devtype;
    } disk[3];
    struct {
        char valid;
        char dirty;
        uint8_t drive;
        int lbn;
        uint8_t *buffer;
    } deblock;
    struct termios *old_settings;
    struct termios *new_settings;
    long throttle;
    struct {
        int after;
        int id;
        void (*callback)(int id);
    } events[EVENTS];
    struct {
        char buffer[KBD_BUF];
        int read;
        int write;
    } keybuf;
    struct {
        timer_t timerid;
        struct sigevent *sevent;
        int triggered;
    } clock;
    int ticks;
};

// how long a tick is, in milliseconds
#define TICK 50

/* For reasons I don't want to go into, I need the system state to be globally shared */
extern struct state *state;


/* RAM and ROM*/
uint8_t mem_read_byte(uint16_t addr);
void mem_write_byte(uint16_t addr, uint8_t value);
uint16_t mem_read_word(uint16_t addr);
void mem_write_word(uint16_t addr, uint16_t value);
void mem_reset();
void load_rom(char *filename);


/* DEBUGGING */
#define debug(fmt, ...) if(state->log) fprintf(state->log, fmt, ##__VA_ARGS__) && fflush(state->log)
#define error(fmt, ...) fprintf(stderr, "ERROR: " fmt, ##__VA_ARGS__) 
#define info(fmt, ...) fprintf(stderr, "INFO: " fmt, ##__VA_ARGS__)
#define warning(fmt, ...) fprintf(stderr, "WARNING: " fmt, ##__VA_ARGS__)
Z80EX_BYTE debug_mem_read(Z80EX_WORD addr, void *user_data);
void trace(uint16_t addr);

/* Disk */
void disk_reset();
int disk_read();
int disk_write(int type);
void stop_motor(int id);
void start_motor(int id);
void disk_motor(int disk, int mode);


/* CPU */
#define _get(reg) z80ex_get_reg(state->cpu, reg)
#define _set(reg, value) z80ex_set_reg(state->cpu, reg, value)
#define _get_low(reg) (z80ex_get_reg(state->cpu, reg) & 0xff)
#define _get_high(reg) (z80ex_get_reg(state->cpu, reg) >> 8)
#define _set_low(reg, value) (z80ex_set_reg(state->cpu, reg, z80ex_get_reg(state->cpu, reg) & 0xff00 + value))
#define _set_high(reg, value) (z80ex_set_reg(state->cpu, reg, \
                               (z80ex_get_reg(state->cpu, reg) & 0xff) + (value << 8)))
#define getA() _get_high(regAF)
#define setA(value) _set_high(regAF, value)
#define getC() _get_low(regBC)
#define setC(value) _set_low(regBC, value)
#define getBC() _get(regBC)
#define setBC(value) _set(regBC, value)
#define getHL() _get(regHL)
#define setHL(value) _set(regHL, value)
#define getPC() _get(regPC)
Z80EX_BYTE cpu_mem_read(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *user_data);
void cpu_mem_write(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *user_data);
Z80EX_BYTE cpu_port_read(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *user_data);
void cpu_port_write(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *user_data);

/* Terminal */
void terminal_setup();
void terminal_restore();
Z80EX_BYTE terminal_status(int us_delay);
Z80EX_BYTE terminal_read();
void terminal_write(char c);

/* Utils */
void msleep(int millis);

/* Events */
int calibrate_timer(uint16_t addr);
void event_add(int id, int after, void (*callback)(int id));
void event_cancel(int id);
void event_handler(int after);
void event_reset();




#define __RZ80_H
#endif
