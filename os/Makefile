all: memory.cim 
	 
#ld80 [-O oformat] [-cmV] [-W warns] -o ofile [-s symfile] [-U name] ...
#     [-S symsize] input ...
#where oformat: ihex | hex | bin | binff | cmd
#        warns: extchain
#        input: [-l] [-P address] [-D address] [-C name,address] [-E entry]... file


memory.cim: bios/boot.rel bios/bios.rel zcpr1/zcpr.rel zsdos/zsdos.rel
	../bin/ld80 -o memory.cim \
		-C FBASE,e000 \
		bios/boot.rel zcpr1/zcpr.rel zsdos/zsdos.rel bios/bios.rel

bios/boot.rel:
	make -C bios

zcpr1/zcpr.rel:
	make -C zcpr1

zsdos/zsdos.rel:
	make -C zsdos
