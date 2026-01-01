#include "rz80.h"
#include <termios.h>

void terminal_setup(struct state *state) {
    state->old_settings = calloc(1, sizeof(struct termios));
    state->new_settings = calloc(1, sizeof(struct termios));
    
    tcgetattr(fileno(stdin), state->old_settings);
    tcgetattr(fileno(stdin), state->new_settings);
    // from python's tty.setraw
    state->new_settings->c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP |
                     INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF);
    state->new_settings->c_oflag &= ~OPOST;
    state->new_settings->c_cflag &= ~(PARENB | CSIZE);
    state->new_settings->c_cflag |= CS8;
    state->new_settings->c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON |
                     IEXTEN | ISIG | NOFLSH | TOSTOP);
    state->new_settings->c_cc[VMIN] = 1;
    state->new_settings->c_cc[VTIME] = 0;
    tcsetattr(fileno(stdin), TCSAFLUSH, state->new_settings);

}

void terminal_restore(struct state *state) {
    tcsetattr(fileno(stdin), TCSAFLUSH, state->old_settings);
}


