#!/bin/env python3
from ctypes import *
from pathlib import Path
import logging
import sys
import argparse
import tty
import termios
import atexit
import os
import select

memory: Memory = None
z80: Z80 = None
disk: Disk = None

def main():
    global memory, z80, disk
    parser = argparse.ArgumentParser()
    parser.add_argument("--trace", type=str, help="Trace file")
    parser.add_argument("--debug", default=False, action="store_true", help="Enable debugging")
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.debug else logging.INFO)
    memory = Memory("../emulator/retro_z80.mem")
    z80 = Z80()
    disk = Disk(None)
    #for n in range(500):

    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    tty.setraw(fd)
    atexit.register(lambda: termios.tcsetattr(fd, termios.TCSANOW, old_settings))

    if args.trace:
        tracefile = open(args.trace, "w")
    while not z80.halt:
        if args.trace:
            print(z80.disassemble(z80.get_reg("PC")), file=tracefile)
        z80.step()




class Disk:
    def __init__(self, disks):
        self.dma = 0
        self.drive = 0
        self.track = 0
        self.sector = 0

        self.disks = ["../emulator/disk_a.img",
                      "../emulator/disk_b.img",
                      "../emulator/disk_c.img"]
        self.files = []
        for f in self.disks:
            self.files.append(open(f, "r+b"))

    def get_lba(self):
        dpb_addr = memory.read_word(memory.dpbase + self.drive * 16 + 10)
        trklen = memory.read_word(dpb_addr)
        lba = trklen * self.track + self.sector
        return lba


    def read_block(self):
        self.files[self.drive].seek(self.get_lba() * 128, os.SEEK_SET)
        buffer = self.files[self.drive].read(128)
        for i, b in enumerate(buffer):
            memory.write_byte(self.dma + i, b)

    def write_block(self):
        self.files[self.drive].seek(self.get_lba() * 128, os.SEEK_SET)
        for i in range(128):
            b = memory.read_byte(self.dma + i)
            self.files[self.drive].write(bytes([b]))


        

class Memory:
    def __init__(self, memfile):
        self.memfile = memfile
        self.ram = bytearray([0] * 65536)
        self.initialize()

    def read_byte(self, addr):
        return self.ram[addr]

    def write_byte(self, addr, value):
        self.ram[addr] = value

    def read_word(self, addr):
        return self.ram[addr] + self.ram[addr + 1] * 256
    
    def write_word(self, addr, value):
        self.ram[addr] = value & 0xff
        self.ram[addr + 1] = (value & 0xff00) >> 8

    def initialize(self):
        for x in range(65536):
            self.ram[x] = 0
        # load the memory image    
        for i, x in enumerate(Path("../../os/memory.cim").read_bytes()):
            self.ram[i] = x

        self.cbase = self.read_word(0x40)
        self.fbase = self.read_word(0x42)
        self.bbase = self.read_word(0x44)
        self.dpbase = self.read_word(0x46)

        # we need to do some relocations...
        logging.debug(f"Bases:  ccp @ {self.cbase:04x}, bdos @ {self.fbase:04x}, bios @ {self.bbase:04x}")
        logging.debug(f"First BIOS word at {self.bbase:04x} is {self.read_word(self.bbase):04x}")
        """
    // The CCP, BDOS, and BIOS start at the word stored at memory location at 0x40
    int ccp_offset = emulator->base.cbase;
    DEBUG("CCP Base at %04x\n", ccp_offset);
    lseek(emulator->memory_fd, ccp_offset, SEEK_SET);
    read(emulator->memory_fd, emulator->ram + ccp_offset, 0x10000);
    DEBUG("First BIOS word at %04x is %04x\n", ReadMemWord(0x44), ReadMemWord(ReadMemWord(0x44)));
    // store the dph base 
    emulator->dph_base = ReadMemWord(DPBASE);    
"""

    def curdisk(self):
        return self.read_byte(0x04)
    
    def iobyte(self):
        return self.read_byte(0x03)
    
    def base_relative(self, addr):
        if addr >= self.dpbase:
            return f"DPBASE:{addr - self.dpbase:04x}"
        elif addr >= self.bbase:
            return f"  BIOS:{addr - self.bbase:04x}"
        elif addr >= self.fbase:
            return f"  BDOS:{addr - self.fbase:04x}"
        elif addr >= self.cbase:
            return f"   CCP:{addr - self.cbase:04x}"
        else:
            return f"       {addr:04x}"

class Z80:
    def __init__(self):
        self.z80ex = CDLL("../z80ex-1.1.21/lib/libz80ex.so.1.1.21")
        self.z80ex_dasm = CDLL("../z80ex-1.1.21/lib/libz80ex_dasm.so.1.1.21")

        # destroy CPU
        self.z80ex.z80ex_destroy.argtypes = [c_void_p]
        self.z80ex.z80ex_destroy.restype = None

        # do next opcode (instruction or prefix), return number of T-states*/
        self.z80ex.z80ex_step.argtypes = [c_void_p]
        self.z80ex.z80ex_step.restype = c_int

        # returns type of the last opcode, processed with z80ex_step.
        # type will be 0 for complete instruction, or dd/fd/cb/ed for opcode prefix.*/
        self.z80ex.z80ex_last_op_type.argtypes = [c_void_p]
        self.z80ex.z80ex_last_op_type.restype = c_int8

        # maskable interrupt
        # returns number of T-states if interrupt was accepted, otherwise 0*/
        self.z80ex.z80ex_int.argtypes = [c_void_p]
        self.z80ex.z80ex_int.restype = c_int

        # non-maskable interrupt
        # returns number of T-states (11 if interrupt was accepted, or 0 if processor
        # is doing an instruction right now)
        self.z80ex.z80ex_nmi.argtypes = [c_void_p]
        self.z80ex.z80ex_nmi.restype = c_int

        # reset CPU
        self.z80ex.z80ex_reset.argtypes = [c_void_p]
        self.z80ex.z80ex_reset.restype = None

        # get register value
        self.z80ex.z80ex_get_reg.argtypes = [c_void_p, c_int]
        self.z80ex.z80ex_get_reg.restype = c_uint16

        # set register value (for 1-byte registers lower byte of <value> will be used)*/
        self.z80ex.z80ex_set_reg.argtypes = [c_void_p, c_int, c_uint16]
        self.z80ex.z80ex_set_reg.restype = None
        

        self.z80ex.z80ex_create.restype = c_void_p
        self.cpu = self.z80ex.z80ex_create(Z80.read_memory_cb, None,
                                           Z80.write_memory_cb, None,
                                           Z80.read_port_cb, None,
                                           Z80.write_port_cb, None,
                                           None, None)

        self.halt = False

    @CFUNCTYPE(c_ubyte, c_uint16, c_void_p)
    @staticmethod
    def dasm_read_memory_cb(address, udata):
        #logging.debug(f"reading address {address:04x}")
        try:
            #logging.debug(f"Dasm Reading memory {address:04x} => {memory.read_byte(address):04x}")
            return memory.read_byte(address)
        except:
            exit(1)


    def disassemble(self, addr):
        #extern int z80ex_dasm(char *output, int output_size, unsigned flags, int *t_states, int *t_states2,
	    #z80ex_dasm_readbyte_cb readbyte_cb, Z80EX_WORD addr, void *user_data);
        buffer = create_string_buffer(b'\0', 256)
        t_states = c_int()
        t_states2 = c_int()
        #print(f"Disassembling {addr:04x}")
        x = self.z80ex_dasm.z80ex_dasm(buffer, 256, 0, byref(t_states), byref(t_states2), Z80.dasm_read_memory_cb, addr, None)
        byteb = []
        for i in range(x):
            byteb.append(f"{Z80.dasm_read_memory_cb(addr + i, None):02x}")
        regs = []
        for r in ('AF', 'BC', 'DE', 'HL', 'PC', 'SP', 'IX', 'IY'):
            regs.append(f"{r}={z80.get_reg(r):04x}")

        return f"{memory.base_relative(addr)} {(" ".join(byteb)).ljust(12)}  {str(buffer.value, encoding='utf-8').ljust(15)} {",".join(regs)}"

    def reset(self):
        """Reset the z80 cpu"""
        self.z80ex.z80ex_reset(self.cpu)
    
    def step(self):
        """Step the CPU"""
        while True:
            self.z80ex.z80ex_step(self.cpu)
            if self.z80ex.z80ex_last_op_type(self.cpu) == 0:
                break
            
        #print(f"stepped, {x}")
    
    reg_map = {"AF": (0, 0), "A": (0, 1), "F": (0, 2),
                "BC": (1, 0), "B": (1, 1), "C": (1, 2),
                "DE": (2, 0), "D": (2, 1), "E": (2, 2),
                "HL": (3, 0), "H": (3, 1), "L": (3, 2),
                "AF'": (4, 0), "A'": (4, 1), "F'": (4, 2),
                "BC'": (5, 0), "B'": (5, 1), "C'": (5, 2),
                "DE'": (6, 0), "D'": (6, 1), "E'": (6, 2),
                "HL'": (7, 0), "H'": (7, 1), "L'": (7, 2),
                "IX": (8, 0), "IY": (9, 0), "PC": (10, 0), "SP": (11, 0)}

    def get_reg(self, register):
        """return a register"""
        index, part = Z80.reg_map[register]
        v = self.z80ex.z80ex_get_reg(self.cpu, index)
        if part == 0:
            v &= 0xffff
        elif part == 1:
            v = (v & 0xff00) >> 8
        elif part == 2:
            v = v & 0xff
        return v
    
    def set_reg(self, register, value):
        index, part = Z80.reg_map[register]
        if part == 0:
            self.z80ex.z80ex_set_reg(self.cpu, index, value & 0xffff)
        else:
            v = self.z80ex.z80ex_get_reg(self.cpu, index)
            if part == 1:
                v = (v & 0x00ff) | ((value & 0xff) << 8)
            else:
                v = (v & 0xff00) & (value & 0xff)
            self.z80ex.z80ex_set_reg(self.cpu, index, v)

    @CFUNCTYPE(c_ubyte, c_void_p, c_uint16, c_int, c_void_p)
    @staticmethod
    def read_memory_cb(cpu_data, address, state, udata):
        #logging.debug(f"reading address {address:04x}")
        try:
            #logging.debug(f"Reading memory {address:04x} => {memory.read_byte(address):04x}")
            return memory.read_byte(address)
        except:
            exit(1)


    @CFUNCTYPE(None, c_void_p, c_uint16, c_uint8, c_void_p)
    @staticmethod
    def write_memory_cb(cpu_data, address, value, udata):
        #logging.debug(f"Writing memory {value} => {address:04x}")
        memory.write_byte(address, value)

    @CFUNCTYPE(c_uint8, c_void_p, c_uint16, c_void_p)
    @staticmethod
    def read_port_cb(cpu_data, port, udata):
        logging.debug(f"reading port {port}")
        return 0

    @CFUNCTYPE(None, c_void_p, c_uint16, c_uint8, c_void_p)
    @staticmethod
    def write_port_cb(cpu_data, port, value, udata):
        port &= 0xff
        svcs = ['Cold boot', 'Warm boot', 'console status', 'console input', 'console output',
                'list output', 'punch output', 'reader in', 'home disk',
                'select disk', 'set track', 'set sector', 'set dma', 'read block',
                'write block', 'list status', 'sector translate']
        try:
            service = svcs[port]
        except:
            service = "Unknown"
        logging.debug(f"Writing value {value} => port {port} ({service}), pc={z80.get_reg("PC"):04x}, af={z80.get_reg("AF"):04x}, bc={z80.get_reg("BC"):04x}, hl={z80.get_reg("HL"):04x} ")

        match port:
            case 1:
                # warm boot
                logging.debug("Warm boot")

                """
                load_memory_image(cpu, emulator);
                emulator->hwimpl->warmboot(emulator);
                """               
            case 2:
                # console status
                readable, _, _ = select.select([sys.stdin], [], [], 0.0)                
                z80.set_reg("A", 0xff if readable else 0x00)  # always ready
                 
            case 3:
                # console input
                i = ord(sys.stdin.read(1))
                if i == 0x11:
                    z80.halt = True
                z80.set_reg("A", i)
            case 4:
                # console output
                sys.stdout.write(chr(z80.get_reg("C") & 0x7f))
                sys.stdout.flush()
                
            case 5:
                # list output
                logging.debug("list output")
                # emulator->hwimpl->list_write(emulator, ReadC() & 0x7f);
            case 6: 
                # punch output
                logging.debug("punch output")
                # emulator->hwimpl->aux_write(emulator, ReadC() & 0x7f);
            case 7:
                # reader in
                logging.debug("reader in")
                # WriteA(emulator->hwimpl->aux_read(emulator));

            case 8: 
                # home disk
                logging.debug("Home disk")
                disk.track = 0
            case 9: # select disk
                dsk = z80.get_reg('C')
                if dsk < 0 or dsk > 2:
                    z80.set_reg('HL', 0)
                else:
                    disk.drive = z80.get_reg('C')
                    #note: if bit 0 of E is 0 then it's a new disk, otherwise it's been seen before
                    #    WriteHL(emulator->dph_base + dsk * 16);  // each dph entry is 16 bytes
                    z80.set_reg('HL', memory.dpbase + dsk * 16)
            case 10: # set track
                disk.track = z80.get_reg('BC')
            case 11: # set sector            
                disk.sector = z80.get_reg("C")
            case 12: # set dma
                disk.dma = z80.get_reg('BC')
            case 13: # read block
                """
                WriteA(emulator->hwimpl->read_block(emulator, emulator->bios.disk, 
                    lba_for_dts(emulator, emulator->bios.disk, emulator->bios.track,
                                emulator->bios.sector), emulator->ram + emulator->bios.dma));
                break;
                """
                disk.read_block()
            case 14: # write block
                """
                WriteA(emulator->hwimpl->write_block(emulator, emulator->bios.disk, 
                    lba_for_dts(emulator, emulator->bios.disk, emulator->bios.track,
                                emulator->bios.sector), emulator->ram + emulator->bios.dma));
                break;
                """
                disk.write_block()
                z80.set_reg('A', 0)
            case 15: # list status
                """
                WriteA(emulator->hwimpl->list_status(emulator) == DEV_READY? 0xff :00); 
                break;
                """
            case 16: # sector translation            
                # hardware sector translation
                z80.set_reg('HL', z80.get_reg('BC'))
            case _:
                print(f"Unknown port {port:02x} write.\n")


        """
if(port  > 8)
        DEBUG("Entry: Port call %d, pc: %04x af: %04x, bc: %04x, hl: %04x\n", port & 0xff, 
            z80ex_get_reg(cpu, regPC), z80ex_get_reg(cpu, regAF), ReadBC(), ReadHL());
    switch(port & 0xff) {
        case 1: // warm boot, memory refresh
            load_memory_image(cpu, emulator);
            emulator->hwimpl->warmboot(emulator);
            break;        
        case 2: // console status           
            WriteA(emulator->hwimpl->console_ready(emulator) == DEV_READY? 0xff : 0);
            break;
        case 3: // console input                
            break;        
        case 4: // console output
            emulator->hwimpl->console_write(emulator, ReadC() & 0x7f);
            break;
        case 5: // list output
                        
            break;            
        case 6: // punch output          
             
            break;
        case 7: // reader in
            
            break;
        case 8: // home disk
            emulator->hwimpl->home_disk(emulator, emulator->bios.disk);
            break;
        case 9: // select disk
            {    
                int dsk = ReadC();
                if(dsk < 0 || dsk > 2) {
                    WriteHL(0);
                } else {
                    emulator->bios.disk = dsk;
                    // note: if bit 0 of E is 0 then it's a new disk, otherwise it's been seen before
                    WriteHL(emulator->dph_base + dsk * 16);  // each dph entry is 16 bytes
                }
                DEBUG("SelDisk C=%02x, HL=%04x, disk: %02x\n", ReadC(), ReadHL(), ReadMem(CURDISK));
            }
            break;
        case 10: // set track
            emulator->bios.track = ReadBC();
            DEBUG("SetTrack BC=%04x\n", ReadBC());
            break;
        case 11: // set sector            
            emulator->bios.sector = ReadC();
            DEBUG("SetSector BC=%04x\n", ReadBC());
            break;
        case 12: // set dma
            if(ReadBC() != 0) 
                emulator->bios.dma = ReadBC();
            DEBUG("SetDMA BC=%04x dma at %04x\n", ReadBC(), emulator->bios.dma);
            break;
        case 13: // read block
            WriteA(emulator->hwimpl->read_block(emulator, emulator->bios.disk, 
                   lba_for_dts(emulator, emulator->bios.disk, emulator->bios.track,
                               emulator->bios.sector), emulator->ram + emulator->bios.dma));
            break;
        case 14: // write block
            WriteA(emulator->hwimpl->write_block(emulator, emulator->bios.disk, 
                lba_for_dts(emulator, emulator->bios.disk, emulator->bios.track,
                            emulator->bios.sector), emulator->ram + emulator->bios.dma));
            break;
        case 15: // list status
            WriteA(emulator->hwimpl->list_status(emulator) == DEV_READY? 0xff :00); 
            break;
        case 16: // sector translation            
            WriteHL(ReadBC()); // we do "hardware" sector translation.
            break;
        default:
            printf("Unknown port %02x write.\n", port);
    }
"""
        return


if __name__ == "__main__":
    main()