#!/bin/bash
F_IN=./u01-latest.asm
F_OUT=./u01-latest.bin
nasm -f bin $F_IN -o $F_OUT && \
ndisasm $F_OUT && \
qemu-system-x86_64 -hda $F_OUT
