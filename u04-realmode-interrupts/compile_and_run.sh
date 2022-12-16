#!/bin/bash
F_IN=./u*-latest.asm
F_OUT=./$(basename $F_IN .asm).bin
nasm -f bin $F_IN -o $F_OUT && \
ndisasm $F_OUT && \
qemu-system-x86_64 -hda $F_OUT -curses
# Hit ALT-2 or CTRL-ALT-2 then type 'q(uit)' to exit

