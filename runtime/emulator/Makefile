DEPS=main.c memory.c debug.c disk.c cpu.c terminal.c rz80.h
emulator: $(DEPS)
	gcc -o emulator $(DEPS) ../z80ex-1.1.21/z80ex.o ../z80ex-1.1.21/z80ex_dasm.o

emulator-static: $(DEPS)
	gcc -static -o emulator-static $(DEPS) ../z80ex-1.1.21/z80ex.o ../z80ex-1.1.21/z80ex_dasm.o




