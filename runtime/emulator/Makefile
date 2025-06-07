emulator: emulator.c emulator.h memory.c memory.h monitor.c monitor.h port_handler.c \
	port_handler.h posix_hw.c posix_hw.h
	gcc -c emulator.c memory.c monitor.c port_handler.c posix_hw.c
	gcc -o emulator emulator.o memory.o monitor.o port_handler.o posix_hw.o \
		../z80ex-1.1.21/z80ex.o ../z80ex-1.1.21/z80ex_dasm.o



#retro_z80: retro_z80.c 
#	gcc -c retro_z80.c -I../z80ex-1.1.21/include
#	gcc -o retro_z80 retro_z80.o ../z80ex-1.1.21/z80ex.o




