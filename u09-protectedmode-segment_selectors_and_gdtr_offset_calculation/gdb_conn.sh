#!/bin/bash
# gdb --args target remote | qemu-system-x86_64 -hda ./latest.bin -S -gdb stdio
gdb -ex "target remote 127.0.0.1:1234"

