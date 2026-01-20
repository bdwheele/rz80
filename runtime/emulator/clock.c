#define _POSIX_C_SOURCE 199309L
#include "rz80.h"
#include <time.h>
#include <signal.h>


/* Emulator clock alarms */

void clock_handler(int signum) {
    state->clock.triggered = 1;
    state->clock.ticks++;
}

void clock_init() {
    struct sigaction sa = {
        .sa_handler = &clock_handler,
        .sa_mask = 0,
        .sa_flags = 0,
    };
    struct sigevent se = {
        .sigev_notify = SIGEV_SIGNAL,
        .sigev_signo = SIGUSR1
    };
    sigaction(SIGUSR1, &sa, NULL);
    timer_create(CLOCK_MONOTONIC, &se, &state->clock.timerid);
}

void clock_start() {
    // start the timer to start calling the handler ever TICK ms
    struct itimerspec its = {
        .it_interval.tv_sec = 0,
        .it_interval.tv_nsec = 1000000000 / TICKS_PER_SECOND,
        .it_value.tv_sec = 0,
        .it_value.tv_nsec = 1000000000 / TICKS_PER_SECOND,
    };
    debug("Clock ticks happen every %ld nanoseconds\n", its.it_interval.tv_nsec);
    timer_settime(state->clock.timerid, 0, &its, NULL);
}

int ticks_for_ms(int ms) {
    // return the number of ticks needed for the given number of ms.
    int ticks = (ms / 1000.0) * TICKS_PER_SECOND;
    return ticks;
}