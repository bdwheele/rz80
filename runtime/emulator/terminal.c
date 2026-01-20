#include "rz80.h"
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>

void terminal_setup() {
    state->terminal.old_settings = calloc(1, sizeof(struct termios));
    state->terminal.new_settings = calloc(1, sizeof(struct termios));
    
    tcgetattr(fileno(stdin), state->terminal.old_settings);
    tcgetattr(fileno(stdin), state->terminal.new_settings);
    // from python's tty.setraw
    state->terminal.new_settings->c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP |
                     INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF);
    state->terminal.new_settings->c_oflag &= ~OPOST;
    state->terminal.new_settings->c_cflag &= ~(PARENB | CSIZE);
    state->terminal.new_settings->c_cflag |= CS8;
    state->terminal.new_settings->c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON |
                     IEXTEN | ISIG | NOFLSH | TOSTOP);
    state->terminal.new_settings->c_cc[VMIN] = 1;
    state->terminal.new_settings->c_cc[VTIME] = 0;
    tcsetattr(fileno(stdin), TCSAFLUSH, state->terminal.new_settings);
}

void terminal_restore() {
    tcsetattr(fileno(stdin), TCSAFLUSH, state->terminal.old_settings);
}


void terminal_poll() {
    fd_set fds;
    struct timeval tv = {
        tv.tv_sec = 0,
        tv.tv_usec = 0,
    };
    while(1) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        if(FD_ISSET(STDIN_FILENO, &fds)) {
            // There's a character there, so put it in the buffer, if we can
            char c;
            read(STDIN_FILENO, &c, 1);
            if((state->terminal.keywrite + 1) % KBD_BUF == state->terminal.keyread) {
                // buffer is full.
                //fputc('\a', stdout);
                debug("Keyboard buffer is full: %d, %d\n", state->terminal.keywrite, state->terminal.keyread);
            } else {
                if(c != 0) {
                    state->terminal.keywrite++;
                    state->terminal.keybuf[state->terminal.keywrite % KBD_BUF] = c;
                }
            }
        } else {
            break;
        }
    }
}


Z80EX_BYTE terminal_status() {
    if(state->terminal.keywrite  != state->terminal.keyread) {
        // key in the buffer
        return 0xff;
    } else {
        // buffer is empty
        return 0x00;
    }
}

char wait_for_key(int cycle, int empty_ok) {
    int retries = 2;
    while(1) {
        if(terminal_status() ) {
            state->terminal.keyread++;
            char c = state->terminal.keybuf[state->terminal.keyread % KBD_BUF];
            debug("Read key: %d\n", c);
            return c;
        }
        if(empty_ok && retries == 0) {
            debug("waited for a key, but got nothing\n");
            return -1;
        }
        retries--;
        msleep(cycle);
        if(state->clock.triggered) {
            poll_hardware();
        }
    }
}



Z80EX_BYTE terminal_read() {
    /* this has to be reasonably complicated, unfortuantely.
       If we get an escape it may be part of an escape sequence so we
       need to read as far as we can to decide what to send back. 
       If it turns out to be invalid (or, at least something we 
       don't understand) then we have to return whatever we
       read ahead on subsequent calls 
    */
    debug("Terminal read!\n");
    char c = wait_for_key(20, 0);
    if(c == 0x1b) {
        /* escape.  Give us just a little bit to see if there's
           another character.  If there isn't we'll just return the escape.
           If there is check to see if it's an escape sequence...and keep reading
           until we decide it's really an escape sequence (when an alternative 
           code will be sent back) or not...we we start returning the buffer.
        */
        debug("Got escape\n");
        // create a readahead buffer in case the processing fails
        int bpos = 0;
        char buffer[16]; 
        buffer[bpos++] = c;  // save the escape
        int kstate = 0, in_escape = 1;
        while(in_escape) {
            debug("Wait for next character kstate=%d\n", kstate);            
            c = wait_for_key(200, 1);
            if(c == -1) {
                // we didn't get anything, so just fall out of the loop.
                break;    
            }
            debug("Got next character %02x\n", c);
            buffer[bpos++] = c;
            switch(kstate) {
                case 0:
                    if(c == '[') {
                        debug("Got [\n");
                        // starting a CSI escape
                        kstate = 1;
                    } else if(c == 'Q' || c == 'q') {
                        // ESC + Q is quit the emulator
                        state->halted = 1;
                        return 0;
                    } else {
                        // don't recognize it, just return buffered data
                        in_escape = 0;
                    }
                    break;
                case 1:
                    debug("In state 1, got %c\n", c);
                    switch(c) {
                        case 'A':
                            // up
                            return 0x0b;
                            break;
                        case 'B':
                            // down
                            return 0x0a;
                            break;
                        case 'C':
                            // right
                            return 0x0c;
                            break;
                        case 'D':
                            // left
                            return 0x08;
                        default:
                            in_escape = 0;
                            break;
                    }
                    break;
                default:
                    // invalid state, just drop out.
                    in_escape = 0;
                    break;
            }
        }
        /* we fell out of the loop so that means we didn't get a valid
            escape sequence.  Just start returning the buffer */
        debug("Fell out of escape processing with char %02x\n", c);
        // take our buffer and put it into the main keyboard buffer.
        for(int i = 0; i < bpos; i++) {    
            debug("Inserting key %x into buffer for bpos %d\n", buffer[i], i);
            state->terminal.keywrite++;
            state->terminal.keybuf[state->terminal.keywrite % KBD_BUF] = buffer[i];
        }
        return wait_for_key(20, 0);
    } else if(c == 0x11) {
        // ctrl-q emulator exit escape hatch
        state->halted = 1;
        return c;
    } else {
        debug("normal key: %02x\n", c);
        return c;
    }
}

void terminal_write(char c) {
    static int in_escape = 0;    
    c &= 0x7f;  // not 8-bit clean?
    if(!in_escape) {
        switch(c) {
            case 0x1b:
                // an escape
                in_escape = 1;
                break;
            case 0x08:
                // backspace / cursor left
                fputc(c, stdout);
               break;
            case 0x12:
                // cursor right
                fprintf(stdout, "\e[C");
                //debug("term: Cursor right\n");
                break;
            //case 0x0A:
            //    // cursor down
            //    break;
            case 0x0B:
                // cursor up
                fprintf(stdout, "\e[A");
                //debug("term: cursor up\n");
                break;
            case 0x1A:
                // clear screen and home
                fprintf(stdout, "\e[2J");
                //debug("term: clear screen\n");
                break;
            //case 0x13:
            //    // carriage return
            //    break;
            case 0x18:
                // clear to end of line
                fprintf(stdout, "\e[K");
                break;
            case 0x17:
                // clear to end of screen
                fprintf(stdout, "\e[J");
                break;
            default:
                fputc(c, stdout);
                break;
        }    
    } else {
        char row, col;
        switch(in_escape) {
            case 1:
                // we don't know what kind of escape yet...
                switch(c) {
                    case 'B':
                        // enable attribute
                        in_escape = 0x100;
                        break;
                    case 'C':
                        // enable attribute
                        in_escape = 0x110;
                        break;
                    case 61:
                        // cursor position, wait for data
                        in_escape = 61;
                        break;
                    case 69:
                        // insert line (scroll down)
                        in_escape = 0;
                        debug("term: scroll down (unimplemented)\n");
                        // TODO: do someting
                        break;
                    case 82:
                        // insert line (scroll up)
                        in_escape = 0;
                        debug("term: insert line (unimplemented)\n");
                        // TODO: do something
                        break;
                    default:
                        // not a valid escape sequence, so just dump the
                        // original escape we got, and whatever this 
                        // character is.
                        debug("term: unknown escape %c (%d)\n", c, c);
                        fputc(0x1b, stdout);
                        fputc(c, stdout);
                        in_escape = 0;
                        break;
                }
                break;
            case 61:
            case 62:
                // we're in set cursor position, waiting for the 
                // row + 20 (61), or column + 20 (62)
                if(in_escape == 61) {
                    row = c - 32;
                    in_escape = 62;
                } else {
                    col = c - 32;
                    //debug("Setting cursor to position %d,%d\n", row + 1, col + 1);
                    fprintf(stdout, "\e[%d;%dH", row + 1, col + 1);
                    in_escape = 0;
                }
                break;
            case 0x100:
                // enable attribute
                switch(c) {
                    case '0':
                        //inverse
                        fprintf(stdout, "\e[7m");
                        break;
                    case '1':
                        // dim
                        fprintf(stdout, "\e[2m");
                        break;
                    case '2':
                        // blinking
                        fprintf(stdout, "\e[5m");
                        break;
                    case '3':
                        // underline
                        fprintf(stdout, "\e[4m");
                    default:
                        debug("term: disable unhandled attribute %c\n", c);
                        break;
                }
                in_escape = 0;
                break;
            case 0x110:
                // disable attribute
                switch(c) {
                    case '0':
                        //inverse
                        fprintf(stdout, "\e[27m");
                        break;
                    case '1':
                        // dim
                        fprintf(stdout, "\e[22m");
                        break;
                    case '2':
                        // blinking
                        fprintf(stdout, "\e[25m");
                        break;
                    case '3':
                        // underline
                        fprintf(stdout, "\e[24m");
                        break;
                    default:
                        debug("term: disable unhandled attribute %c\n", c);
                        break;
                }
                in_escape = 0;
                break;
            default:
                // no idea what this is.  Pass the original escape and
                // this char.    
                fputc(0x1b, stdout);
                fputc(c, stdout);
                debug("term: Unknown escape state %d, character %c (%d)\n", in_escape, c, c);
                in_escape = 0;
                break;
        }
    }
    fflush(stdout);
}
