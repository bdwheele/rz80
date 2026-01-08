#!/bin/bash
#
# Build the RZ80 distribution disks...
#
for n in ../original/KAYPRO2X/*; do
    DSK=`basename $n | tr '[:upper:]' '[:lower:]'`.img
    ./make_floppy.sh $DSK
    cpmcp -f rz80_floppy $DSK $n/* 0:
done


# Make the boot disk
DR_SRC="../original/DR_Binaries"
KP2X_SRC="../original/KAYPRO2X"
MS_SRC="../original/MICROSOFT"
./make_floppy.sh boot.img
# user 0 is the system stuff
cpmcp -f rz80_floppy boot.img  \
      $DR_SRC/{ASM,DDT,DU,DUMP,ED,FILES,FINDBAD1,L80}.COM \
      $DR_SRC/{LINKER,LOAD,LOADER,M80,MAC,MSTATUS,PIP}.COM \
      $DR_SRC/{STAT,SUBMIT,XDIR,XSUB,Z80ASM}.COM \
      $DR_SRC/Z80.LIB \
      ../original/ppip.com \
      $KP2X_SRC/63KBOOT/DU2.COM \
      0:
# user 1 is basic
cpmcp -f rz80_floppy boot.img  \
      $MS_SRC/BASIC-80/V5-21/*.* \
      ../original/ppip.com \
      1:

# user 2 is wordstar
cpmcp -f rz80_floppy boot.img  \
      $KP2X_SRC/WS33/* \
      ../original/ppip.com \
      2:

# user 3 is dbase 2
cpmcp -f rz80_floppy boot.img  \
      $KP2X_SRC/DBASE2/* \
      ../original/ppip.com \
      3:

# user 4 is multiplan
cpmcp -f rz80_floppy boot.img  \
      $MS_SRC/MULTIPLAN/V1-06/* \
      ../original/ppip.com \
      4:








