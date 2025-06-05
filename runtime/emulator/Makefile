all: z80ex.o z80ex.h z80ex_common.h retro_z80

retro_z80: retro_z80.c z80ex.o
	gcc -c retro_z80.c
	gcc -o retro_z80 retro_z80.o z80ex.o



z80ex.h:
	cp z80ex/include/z80ex.h .

z80ex_common.h:
	cp z80ex/include/z80ex_common.h .

z80ex.o: 
	make -C z80ex z80ex.o
	cp z80ex/z80ex.o .

