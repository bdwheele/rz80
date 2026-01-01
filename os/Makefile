all: system.rom
	 

.PHONY: bios zcpr1 zsdos 

system.rom: bios zcpr1 zsdos
	../bin/ld80 -m -O ihex -o system.hex -s /dev/tty \
		-P 0xe680 zcpr1/zcpr.rel \
		-P 0xee70 zsdos/zsdos.rel \
		-P 0xfc70 bios/bios.rel
	objcopy -I ihex -O binary system.hex system.rom



bios:
	make -C bios

zcpr1:
	make -C zcpr1

zsdos:
	make -C zsdos

clean:
	rm -f system.hex system.rom
	make -C zsdos clean
	make -C zcpr1 clean
	make -C bios clean
