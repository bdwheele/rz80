#include <unistd.h>

void msleep(int millis) {
    usleep(millis * 1000);
}

