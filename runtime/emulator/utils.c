#include "rz80.h"
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

void msleep(int millis) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000 * millis;
    nanosleep(&ts, NULL);
    //usleep(millis * 1000);
}

