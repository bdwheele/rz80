#!/bin/bash
#
# Build the RZ80 distribution disks...
#
for n in ../original/KAYPRO2X/*; do
    DSK=`basename $n | tr '[:upper:]' '[:lower:]'`.img
    ./make_floppy.sh $DSK
    cpmcp -f rz80_floppy $DSK $n/* 0:
done
