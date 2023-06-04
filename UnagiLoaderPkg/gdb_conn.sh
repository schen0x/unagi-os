#!/bin/bash
ENTRY_ADDR=$(readelf -h ../build/kernel.elf | grep "Entry point address" | awk '{print $NF}')
gdb -ex "target remote 127.0.0.1:1234" -ex "add-symbol-file ../build/kernel.elf ${ENTRY_ADDR}" -ex "b *KernelMain" -ex "layout split" 
