#!/bin/bash
if [ "x$1" == "x" ]; then
    echo "Usage: $0 <disk name>"
    exit 1
fi

dd if=/dev/zero of=$1 bs=1024 count=1440

mkfs.cpm -f rz80_floppy $1

