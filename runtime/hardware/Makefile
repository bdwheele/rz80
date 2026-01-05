.PHONY: all clean

Z80EX=../z80ex-1.1.21
ARCH_ARGS=-march=armv6zk -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp
EM_SOURCE=../emulator

all:
	# get the development environment
	mkdir -p cache/sysroot
	cd cache && wget -c https://dl-cdn.alpinelinux.org/alpine/v3.23/main/armhf/musl-dev-1.2.5-r21.apk
	tar -C cache/sysroot -xf cache/musl-dev-1.2.5-r21.apk

	# build the emulator binary
	arm-linux-gnu-gcc --sysroot=cache/sysroot -c $(ARCH_ARGS) -I $(Z80EX)/include  -I $(Z80EX) -DZ80EX_VERSION_MAJOR=1 -DZ80EX_VERSION_MINOR=21 -DZ80EX_API_REVISION=1 $(Z80EX)/z80ex*.c
	arm-linux-gnu-gcc --sysroot=cache/sysroot -c $(ARCH_ARGS) -I $(Z80EX)/include  $(EM_SOURCE)/*.c	
	arm-linux-gnu-gcc --sysroot=cache/sysroot -static -o emulator-arm *.o 

	# download the alpine distribution
	mkdir -p cache/alpine
	cd cache && wget -c https://dl-cdn.alpinelinux.org/alpine/v3.23/releases/armhf/alpine-rpi-3.23.2-armhf.tar.gz
	tar -C cache/alpine -xf cache/alpine-rpi-3.23.2-armhf.tar.gz

	# build the disk image
	dd if=/dev/zero of=rpi0.img bs=1M count=256
	echo -e "label: mbr\n1MiB,128MiB,0x06,*\n-,32Mib,0x52,-" | sfdisk rpi0.img
	MTOOLSRC=mtoolsrc mformat c:
	MTOOLSRC=mtoolsrc mcopy -s cache/alpine/* c:
	MTOOLSRC=mtoolsrc mmd c:/rz80
	MTOOLSRC=mtoolsrc mcopy emulator-arm ../../os/system.rom ../emulator/disk_a.img c:/rz80
	MTOOLSRC=mtoolsrc mcopy usercfg.txt cmdline.txt c:
	MTOOLSRC=mtoolsrc mdir c:


clean:
	rm -rf cache
	rm *.o emulator-arm


