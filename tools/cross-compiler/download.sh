#!/bin/bash
#
# Download a cross compiler for the raspberry pi 0 (armv6)
# Fedora's cross compiler only supports armv7 and above. 
#
wget -c "https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/Raspberry%20Pi%20GCC%20Cross-Compiler%20Toolchains/Buster/GCC%2014.2.0/Raspberry%20Pi%201%2C%20Zero/cross-gcc-14.2.0-pi_0-1.tar.gz/download" \
    -O /tmp/cross-gcc-14.2.0-pi_0-1.tar.gz
tar -C ../../../bin -xf /tmp/cross-gcc-14.2.0-pi_0-1.tar.gz
rm /tmp/cross-gcc-14.2.0-pi_0-1.tar.gz
