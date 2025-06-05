all: memory.cim 
	 
#ld80 [-O oformat] [-cmV] [-W warns] -o ofile [-s symfile] [-U name] ...
#     [-S symsize] input ...
#where oformat: ihex | hex | bin | binff | cmd
#        warns: extchain
#        input: [-l] [-P address] [-D address] [-C name,address] [-E entry]... file


memory.cim: bios/boot.rel bios/bios.rel zcpr1/zcpr.rel zsdos/zsdos.rel
	../bin/ld80 -m -O bin -o memory.cim -s /dev/tty \
		-P 0x0000 bios/boot.rel \
		-P 0xe690 zcpr1/zcpr.rel \
		-P 0xee80 zsdos/zsdos.rel \
		bios/bios.rel 

bios/boot.rel:
	make -C bios

zcpr1/zcpr.rel:
	make -C zcpr1

zsdos/zsdos.rel:
	make -C zsdos
