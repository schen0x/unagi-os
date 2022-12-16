#!/bin/bash
F_IN=./u02-latest.asm
F_OUT=./u02-latest.bin
nasm -f bin $F_IN -o $F_OUT && \
ndisasm $F_OUT && \
qemu-system-x86_64 -hda $F_OUT
