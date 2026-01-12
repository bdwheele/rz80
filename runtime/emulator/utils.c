#include "rz80.h"
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

void msleep(int millis) {
    usleep(millis * 1000);
}

