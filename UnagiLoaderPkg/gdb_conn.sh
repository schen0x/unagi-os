#!/bin/bash
gdb -ex "target remote 127.0.0.1:1234" -ex "add-symbol-file ../build/kernel.elf 0x101120" -ex "b *KernelMain" -ex "layout split" 
