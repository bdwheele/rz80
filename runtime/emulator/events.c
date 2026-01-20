#include "rz80.h"
#include <sys/time.h>
#include <math.h>
#include <unistd.h>


inline double gettime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}


 
#define INSTR_COUNT 10000000  /* 10,000,000 instructions */

int calibrate_timer(uint16_t addr) {
    /* create an infinite loop at the given memory address and run it for
       a bunch of iterations to figure out roughly how many instructions
       are executed in a millisecond */
    struct timeval start, end;
    uint16_t oldpc = z80ex_get_reg(state->cpu, regPC);

    /* build the program in memory at the address */
    *(state->memory + addr) = 0xc3;  // jmp
    *(state->memory + addr + 1) = addr & 0xff; // low byte of dest
    *(state->memory + addr + 2) = (addr >> 8); // high byte of dest

    gettimeofday(&start, NULL);
    z80ex_set_reg(state->cpu, regPC, addr);
    int counter = 0;

    for(int i = 0; i < INSTR_COUNT; i++) {
        
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
    gettimeofday(&end, NULL);

    double start_time = start.tv_sec + (start.tv_usec / 1000000.0);
    double end_time = end.tv_sec + (end.tv_usec / 1000000.0);
    double duration = end_time - start_time;
    double instruction_ms = 0.001 / (duration / INSTR_COUNT);
    z80ex_set_reg(state->cpu, regPC, oldpc);
    return (int)ceil(instruction_ms);
}

/* THings dealing with the events list */
void event_add(int id, int after, void (*callback)(int id)) {
    int is_available = -1;
    for(int i = 0; i < EVENTS; i++) {
        if(state->events[i].id == id) {
            // we're just going to update the after field.
            state->events[i].after = after;
            return;
        }        
        if(is_available == -1 && state->events[i].after == 0) {
            is_available = i;
        }         
    }
    if(is_available != -1) {
        state->events[is_available].after = after;
        state->events[is_available].id = id;
        state->events[is_available].callback = callback;
    } else {
        // FFFFFFFFFFFFFFFFFFFFFFUUUUUUUUUUUUU....
        // no slots available.
        state->halted = 1;
        printf("ERROR: There are no slots for a new event\n");
    }
}

void event_cancel(int id) {
    for(int i = 0; i < EVENTS; i++) {
        if(state->events[i].id == id) {
            state->events[i].id = 0;
            state->events[i].after = 0;
            break;
        }
    }
}


void event_handler(int after) {
    for(int i = 0; i < EVENTS; i++) {
        if(state->events[i].after > 0 && after >= state->events[i].after) {
            state->events[i].callback(state->events[i].id);
            // clear the event
            state->events[i].id = 0;
            state->events[i].after = 0; 
        }
    }
}

void event_reset() {
    for(int i = 0; i < EVENTS; i++) {
        state->events[i].id = 0;
        state->events[i].after = 0;
    }
}