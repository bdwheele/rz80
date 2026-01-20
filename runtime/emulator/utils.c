#define _POSIX_C_SOURCE 200809L
#include "rz80.h"
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

void msleep(int millis) {
    struct timespec ts = {
        .tv_sec = millis / 1000,
        .tv_nsec = (millis % 1000) * 1000000
    };
    struct timespec remaining;
    int x = 8;
    while(x = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &remaining)) {
        ts.tv_sec = remaining.tv_sec;
        ts.tv_nsec = remaining.tv_nsec;
    }
}

