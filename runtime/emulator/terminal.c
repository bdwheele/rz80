#include "rz80.h"
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>

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

Z80EX_BYTE terminal_status(struct state *state, int usdelay) {
    fd_set fds;
    FD_ZERO(&fds);
    int stdin_fileno = fileno(stdin);
    FD_SET(stdin_fileno, &fds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = usdelay; // 100us wait time
    select(stdin_fileno + 1, &fds, NULL, NULL, &tv);
    if(FD_ISSET(stdin_fileno, &fds)) {
        //debug("STDIN is ready\n");
        return 0xff;        
    } else {
        //debug("STDIN is NOT ready\n");
        return 0x00;
    }
}

#define KBUF 10
#define BPOS(x) (x % KBUF)
Z80EX_BYTE terminal_read(struct state *state) {
    /* this has to be reasonably complicated, unfortuantely.
       If we get an escape it may be part of an escape sequence so we
       need to read as far as we can to decide what to send back. 
       If it turns out to be invalid (or, at least something we 
       don't understand) then we have to return whatever we
       read ahead on subsequent calls 
    */
    static int bread = 1, bwrite = 0;
    static char buffer[KBUF];
    char c;
    if(BPOS(bread) < BPOS(bwrite)) {
        bread++;
        debug("bread=%d, bwrite=%d, char=%02x\n", bread, bwrite, buffer[BPOS(bread - 1)]);
        return buffer[BPOS(bread - 1)];
    }
    read(0, &c, 1);
    if(c == 0x1b) {
        /* escape.  Give us just a little bit to see if there's
           another character.  If there isn't we'll just return the escape.
           If there is check to see if it's an escape sequence...and keep reading
           until we decide it's really an escape sequence (when an alternative 
           code will be sent back) or not...we we start returning the buffer.
        */
        debug("Got escape\n");
        bwrite++;
        buffer[BPOS(bwrite)] = c;
        int kstate = 0, in_escape = 1;
        int stdin_fileno = fileno(stdin);
        while(in_escape) {
            debug("Wait for next character kstate=%d\n", kstate);            
            if(terminal_status(state, 200000)) {
                read(0, &c, 1);
                debug("Got next character %02x\n", c);
                bwrite++;
                buffer[BPOS(bwrite)] = c;   // stash the current character
                switch(kstate) {
                    case 0:
                        if(c == '[') {
                            debug("Got [\n");
                            // starting a CSI escape
                            kstate = 1;
                        } else if(c == 'Q' || c == 'q') {
                            // ESC + Q is quit the emulator
                            state->halted = 1;
                            bread = bwrite;
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
                                bread = bwrite;
                                return 0x0b;
                                break;
                            case 'B':
                                // down
                                bread = bwrite;
                                return 0x0a;
                                break;
                            case 'C':
                                // right
                                bread = bwrite;
                                return 0x0c;
                                break;
                            case 'D':
                                // left
                                bread = bwrite;
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
            } else {
                /* If there's nothing here so just fall out */
                debug("stream wasn't ready...\n");
                in_escape = 0;
            }
        }
        /* we fell out of the loop so that means we didn't get a valid
            escape sequence.  Just start returning the buffer */
        debug("Fell out of escape processing with char %02x\n", c);
        bread++;
        return buffer[BPOS(bread - 1)]; 
    } else if(c == 0x11) {
        //state->halted = 1;
        return c;
    } else {
        debug("normal key: %02x\n", c);
        return c;
    }
}


void terminal_write(struct state *state, char c) {
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
