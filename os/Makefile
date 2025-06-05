all: memory.cim 
	 
#ld80 [-O oformat] [-cmV] [-W warns] -o ofile [-s symfile] [-U name] ...
#     [-S symsize] input ...
#where oformat: ihex | hex | bin | binff | cmd
#        warns: extchain
#        input: [-l] [-P address] [-D address] [-C name,address] [-E entry]... file

.PHONY: bios zcpr1 zsdos 
memory.cim: bios zcpr1 zsdos
	../bin/ld80 -m -O bin -o memory.cim -s /dev/tty \
		-P 0x0000 bios/boot.rel \
		-P 0xe660 zcpr1/zcpr.rel \
		-P 0xee60 zsdos/zsdos.rel \
		-P 0xfc60 bios/bios.rel 

bios:
	make -C bios

zcpr1:
	make -C zcpr1

zsdos:
	make -C zsdos
