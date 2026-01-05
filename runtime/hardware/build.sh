#!/bin/bash
set -x 
mkdir -p cache/sysroot cache/alpine

# download things...
cd cache
for n in https://dl-cdn.alpinelinux.org/alpine/v3.23/main/armhf/musl-dev-1.2.5-r21.apk \
         https://dl-cdn.alpinelinux.org/alpine/v3.23/releases/armhf/alpine-rpi-3.23.2-armhf.tar.gz; do
    m=`basename $n`
    if [ ! -e $m ]; then
        wget -c $n
    fi
done

# unpack the musl dev environment
tar -C sysroot -xf musl-dev-1.2.5-r21.apk 2>&1 | grep -v APK-TOOLS

# build the emulator
Z80EX=../../z80ex-1.1.21
#ARCH_ARGS=-march=armv6zk -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp
ARCH_ARGS="-march=armv6  -mfloat-abi=hard -mfpu=vfp"
EM_SOURCE=../../emulator

arm-linux-gnu-gcc --sysroot=sysroot -static -o emulator-arm \
    $ARCH_ARGS \
    -I $Z80EX -I $Z80EX/include -I $EM_SOURCE \
    -DWORDS_LITTLE_ENDIAN -DZ80EX_VERSION_STR=1.1.21 -DZ80EX_API_REVISION=1 -DZ80EX_VERSION_MAJOR=1 -DZ80EX_VERSION_MINOR=1 \
    $Z80EX/z80ex*.c $EM_SOURCE/*.c \

# unpack the alpine distro
tar -C alpine -xf alpine-rpi-3.23.2-armhf.tar.gz

# patch the distro
cp ../cmdline.txt ../usercfg.txt alpine
mkdir alpine/rz80
cp emulator-arm $EM_SOURCE/disk_a.img ../../../os/system.rom alpine/rz80

# make the disk image
export MTOOLSRC=../mtoolsrc
dd if=/dev/zero of=rpi0.img bs=1M count=256
echo -e "label: mbr\n1MiB,128MiB,0x06,*\n-,32Mib,0x52,-" | sfdisk rpi0.img
mformat c:
mcopy -s alpine/* c:


