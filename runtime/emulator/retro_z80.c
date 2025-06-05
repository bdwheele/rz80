#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include "z80ex.h"

#define MEMORY_IMAGE "retro_z80.mem"

#define IOBYTE 3
#define CURDISK 4
#define DPHSIZE 16
#define DPBASE 0x46
#define CCPBASE 0x40

typedef struct {
    char *filename;
    FILE *handle;
} disk_t;


typedef struct {
    Z80EX_BYTE *ram;
    FILE *memory_image;
    disk_t drives[3];
    size_t dpbase;
    int disk;
    int track;
    int sector;
    int dma;
} system_t;


#define ReadA() ((z80ex_get_reg(cpu, regAF) & 0xff00) >> 8)
#define ReadBC() (z80ex_get_reg(cpu, regBC))
#define ReadC() (z80ex_get_reg(cpu, regBC) & 0xff)
#define ReadDE() (z80ex_get_reg(cpu, regDE))
#define ReadE() (z80ex_get_reg(cpu, regDE) & 0xff)
#define ReadF() (z80ex_get_reg(cpu, regAF) & 0xff)
#define ReadHL() (z80ex_get_reg(cpu, regHL))
#define ReadMem(addr) (system->ram[addr] & 0xff)
#define ReadMemWord(addr) (ReadMem(addr) + (ReadMem(addr + 1) << 8))

#define WriteA(v) z80ex_set_reg(cpu, regAF, (((v & 0xff)) << 8) + ReadF())
#define WriteHL(v) z80ex_set_reg(cpu, regHL, v)
#define WriteMem(addr, v) system->ram[addr] = v & 0xff
#define WriteMemWord(addr, v) WriteMem(addr, v); WriteMem(addr + 1, v >> 8)

#define DEBUG(args...) fprintf(stderr, "DEBUG: " args)



void load_memory_image(Z80EX_CONTEXT *cpu, system_t *system) {
    // Load the system memory from memory image.  
    // first, load the low storage page
    fseek(system->memory_image, 0, SEEK_SET);
    fread(system->ram, 0x100, 1, system->memory_image);
    DEBUG("Initial ops: %02x %04x\n", ReadMem(0), ReadMemWord(1));
    
    // The CCP, BDOS, and BIOS start at the word stored at memory location at 0x40
    int ccp_offset = ReadMemWord(CCPBASE);
    DEBUG("CCP Base at %04x\n", ccp_offset);
    fseek(system->memory_image, ccp_offset, SEEK_SET);
    fread(system->ram + ccp_offset, 0x10000 - ccp_offset, 1, system->memory_image);
    DEBUG("First BIOS word at %04x is %04x\n", ReadMemWord(0x44), ReadMemWord(ReadMemWord(0x44)));
    // store the dp base 
    system->dpbase = ReadMemWord(DPBASE);
    DEBUG("TEST WORD at 0xfff0 %04x, byte 0: %02x, byte 1: %02x\n", ReadMemWord(0xfff0), ReadMem(0xfff0), ReadMem(0xfff1));
}


/* The callbacks */
Z80EX_BYTE read_memory(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *data) {
    system_t *system = (system_t *)data;
    return system->ram[addr];
}

void write_memory(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *data) {
    system_t *system = (system_t *)data;
    system->ram[addr] = value;
}

char *text = "DIR\nB:\r\nDIR\r\n";

char is_crlf = 0;
char status_flip = 0;
char nextchar = 0;

fd_set fds;
struct timeval tv;


void write_port(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *data) {
    system_t *system = (system_t *)data;
    /* These are calls for the bios to handle*/
    //int dsk = ReadMem(CURDISK) & 0x0f;
    int dsk = system->disk;
    char chr = 0;
    DEBUG("Port call %d, pc: %04x bc: %04x, hl: %04x, dsk: %02x\n", port & 0xff, z80ex_get_reg(cpu, regPC), ReadBC(), ReadHL(), dsk);
    switch(port & 0xff) {
        case 1: // warm boot
            load_memory_image(cpu, system);
            break;        
        case 2: // console status            
            select(STDIN_FILENO+1, &fds, NULL, NULL, &tv); 
            if (FD_ISSET(STDIN_FILENO, &fds)){
                WriteA(0xff);
            } else {
                WriteA(0x00);
            }
            DEBUG("Console status %02x, nextchar: %02x\n", ReadA(), nextchar);
            break;
        case 3: // console input                
            do {
                read(STDIN_FILENO, &nextchar, 1);
            } while(!nextchar);              
            chr = nextchar;
            nextchar = 0;            
            WriteA(chr);
            DEBUG("Character read: %02x, text= %p, af reg: %04x\n", chr, text, z80ex_get_reg(cpu, regAF));
            break;
        
        
        case 4: // console output
            DEBUG("Character write: %02x\n", ReadC() & 0x7f);
            chr = ReadC() & 0x7f;
            putc(chr, stdout);
            fflush(stdout);
            break;
        case 5: // list output
            putc(ReadC() & 0x7f, stderr);
            break;            
        case 6: // punch output            
        case 7: // reader in
            break;
        case 8: // home disk
            // nothing to do
            break;
        case 9: // select disk
            dsk = ReadC();
            if(dsk < 0 || dsk > 2) {
                WriteHL(0);
            } else {
                system->disk = dsk;
                // note: if bit 0 of E is 0 then it's a new disk, otherwise it's been seen before
                WriteHL(system->dpbase + dsk * 16);  // each dp entry is 16 bytes
            }
            DEBUG("SelDisk C=%02x, HL=%04x, disk: %02x\n", ReadC(), ReadHL(), ReadMem(CURDISK));
            break;
        case 10: // set track
            system->track = ReadBC();
            DEBUG("SetTrack BC=%04x\n", ReadBC());
            break;
        case 11: // set sector            
            system->sector = ReadC();
            DEBUG("SetSector BC=%04x\n", ReadBC());
            break;
        case 12: // set dma
            if(ReadBC() != 0) 
                system->dma = ReadBC();
            DEBUG("SetDMA BC=%04x dma at %04x\n", ReadBC(), system->dma);
            break;
        case 13: // read block
            {
                int lba = 0;            
                int dpbaddr = ReadMemWord(system->dpbase + system->disk * DPHSIZE + 10);
                int trklen = ReadMemWord(dpbaddr) * 128;
                int offset = trklen * system->track + 128 * system->sector;
                DEBUG("ReadBlock Drive %d, Track %d, Sector %d, Offset %x, dma %04x\n", system->disk, system->track, system->sector, offset, system->dma);
                fseek(system->drives[system->disk].handle, offset, SEEK_SET);
                fread(system->ram + system->dma, 128, 1, system->drives[system->disk].handle);
            }
            // a=0 for ok, a=1 for unrecoverable, a=255 for disk changed
            WriteA(0); 
            break;
        case 14: // write block
            {
                int lba = 0;            
                int dpbaddr = ReadMemWord(system->dpbase + system->disk * DPHSIZE + 10);
                int trklen = ReadMemWord(dpbaddr) * 128;
                int offset = trklen * system->track + 128 * system->sector;
                DEBUG("WriteBlock Drive %d, Track %d, Sector %d, Offset %x, dma %04x\n", system->disk, system->track, system->sector, offset, system->dma);
                fseek(system->drives[system->disk].handle, offset, SEEK_SET);
                fwrite(system->ram + system->dma, 128, 1, system->drives[system->disk].handle);
            }
            // a=0 for ok, a=1 for unrecoverable, a=255 for disk changed
            WriteA(0); 

            break;
        case 15: // list status
            WriteA(0xff);  // we're always ready to print.
            break;
        case 16: // sector translation            
            WriteHL(ReadBC()); // we don't do sector translation.
            break;
        default:
            printf("Unknown port %02x write.\n", port);
    }

}

struct termios orig_termios;

void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_iflag &= ~(ICRNL);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VTIME] = 1;
    raw.c_cc[VMIN] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


int main(int argc, char *argv[]) {
    system_t *sys = (system_t *)calloc(1, sizeof(system_t));
    sys->ram = malloc(0x10000);
    sys->memory_image = fopen(MEMORY_IMAGE, "r");    
    char *filename_pattern = "disk_x.img";
    for(char i = 0; i <= 2; i++) {
        char *filename = strdup(filename_pattern);
        filename[5] = i + 'a';
        sys->drives[i].filename = filename;
        sys->drives[i].handle = fopen(filename, "r+");
    }
    
    Z80EX_CONTEXT *cpu = z80ex_create(read_memory, sys, 
                                      write_memory, sys,
                                      NULL, NULL,
                                      write_port, sys, 
                                      NULL, NULL);
    
    load_memory_image(cpu, sys);

    // reset starts at PC 0
    z80ex_set_reg(cpu, regPC, 0);

    enableRawMode();

    FD_ZERO(&fds); 
    FD_SET(STDIN_FILENO, &fds); 

    tv.tv_usec = 50;   

    int lastpc = 0xffff;
    while(1) {
        int pc = z80ex_get_reg(cpu, regPC);
        //if(pc < lastpc || (pc - lastpc) > 5) 
        //    printf("[TRACE: PC %04x -> %02x]", pc, sys->ram[pc]);
        z80ex_step(cpu);
        lastpc = pc;
    }

}