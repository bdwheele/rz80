#include "port_handler.h"
#include "memory.h"

void write_port(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *data) {
    struct emulator *emulator = (struct emulator *)data;
    /* These are calls for the bios to handle*/
    DEBUG("Port call %d, pc: %04x bc: %04x, hl: %04x, dsk: %02x\n", port & 0xff, z80ex_get_reg(cpu, regPC), ReadBC(), ReadHL(), dsk);
    switch(port & 0xff) {
        case 1: // warm boot, memory refresh
            load_memory_image(cpu, emulator);
            break;        
        case 2: // console status           
            WriteA(emulator->hwimpl->console_ready(emulator) == DEV_READY? 0xff : 0)
        
            DEBUG("Console status %02x, nextchar: %02x\n", ReadA(), nextchar);
            break;
        case 3: // console input                
            WriteA(emulator->hwimpl->console_read(emulator));
            DEBUG("Character read: %02x, text= %p, af reg: %04x\n", chr, text, z80ex_get_reg(cpu, regAF));
            break;        
        case 4: // console output
            DEBUG("Character write: %02x\n", ReadC() & 0x7f);
            emulator->hwimpl->console_write(emulator, ReadC() & 0x7f);
            break;
        case 5: // list output
            emulator->hwimpl->list_write(emulator, ReadC() & 0x7f);            
            break;            
        case 6: // punch output          
            emulator->hwimpl->aux_write(emulator, ReadC() & 0x7f); 
            break;
        case 7: // reader in
            WriteA(emulator->hwimpl->aux_read(emulator));
            break;
        case 8: // home disk
            emulator->hwimpl->home_disk(emulator, emulator->bios.disk);
            break;
        case 9: // select disk
            dsk = ReadC();
            if(dsk < 0 || dsk > 2) {
                WriteHL(0);
            } else {
                emulator->bios.disk = dsk;
                // note: if bit 0 of E is 0 then it's a new disk, otherwise it's been seen before
                WriteHL(emulator->dph_base + dsk * 16);  // each dph entry is 16 bytes
            }
            DEBUG("SelDisk C=%02x, HL=%04x, disk: %02x\n", ReadC(), ReadHL(), ReadMem(CURDISK));
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
            emulator->hwimpl->read_block(emulator, emulator->bios.disk, 
                                         emulator->bios.track, emulator->bios.sector,
                                         


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
            WriteA(emulator->hwimpl->list_status(emulator) == DEV_READY? 0xff :00); 
            break;
        case 16: // sector translation            
            WriteHL(ReadBC()); // we do "hardware" sector translation.
            break;
        default:
            printf("Unknown port %02x write.\n", port);
    }

}
