#!/bin/bash 
# gdb --args target remote | qemu-system-x86_64 -hda ./latest.bin -S -gdb stdio
# gdb -ex "target remote 127.0.0.1:1234"
gdb -ex "target remote 127.0.0.1:1234" -ex "add-symbol-file ./build/kernelfull.o 0x100000" -ex "b _start" -ex "c" -ex "layout asm" -ex "b *kernel_main" -ex "b *k_heap_table_mm_init" -ex "b *k_heap_table_mm_malloc"

