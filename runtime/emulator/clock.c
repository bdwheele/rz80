#include "rz80.h"
#include <time.h>
#include <signal.h>

/* Emulator clock alarms */

void clock_handler(int signum) {
    state->clock.triggered = 1;
}

int clock_init() {
    state->clock.sevent = (struct sigevent *)calloc(1, sizeof(struct sigevent));
    state->clock.sevent->sigev_notify = SIGEV_SIGNAL;
    state->clock.sevent->sigev_signo = SIGALRM;
    signal(SIGALRM, clock_handler);
    timer_create(CLOCK_MONOTONIC, state->clock.sevent, &state->clock.timerid);

}

