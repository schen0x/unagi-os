#!/bin/bash
#ENTRY_ADDR=$(readelf -h ./build/kernel.elf | grep "Entry point address" | awk '{print $NF}')
#TEXT_SEG_INDEX=$(readelf -l ./build/kernel.elf | grep ".text" | awk '{print $(NF-1)}')
TEXT_START=$(readelf -l ./build/kernel.elf | grep -B 1 "R E" | head -n 1| awk '{print$(NF-1)}')
# printf "0x%X\n" $((0xA000 + 0x1000))
TEXT=$(readelf -S ./build/Loader.debug | grep " .text" | awk '{print$(NF-1)}')
DATA=$(readelf -S ./build/Loader.debug | grep " .data" | awk '{print$(NF-1)}')
# printf -v TEXT_ADDR "0x%X\n" $((0x6AEE000 + 0x$TEXT))
# printf -v DATA_ADDR "0x%X\n" $((0x6AEE000 + 0x$TEXT + 0x$DATA))

DEBUGLOG=/run/shm/debug.loggdb
# 0x0003DFDC000
LOADEREFI_BASE=$(grep "Loader.efi" $DEBUGLOG | awk '{print$(NF-2)}')
printf -v TEXT_ADDR "0x%X\n" $(($LOADEREFI_BASE + 0x$TEXT))
printf -v DATA_ADDR "0x%X\n" $(($LOADEREFI_BASE + 0x$TEXT + 0x$DATA))
#printf -v DATA_ADDR "0x%X\n" $(($LOADEREFI_BASE + 0x$DATA))

# Compile and run a first pass, copy the debug.log to debug.loggdb;
# Then because the img is the same, the Loader.efi seems to be loaded to the same address (it works for now)
#gdb -ex "target remote 127.0.0.1:1234" -ex "add-symbol-file ./build/kernel.elf ${TEXT_START}" -ex "b *KernelMain" -ex "layout split" -ex "add-symbol-file ./build/Loader.debug ${TEXT_ADDR} -s .data ${DATA_ADDR}" -ex "b *UefiMain" -ex "b *UefiMain+779"
gdb -ex "target remote 127.0.0.1:1234" -ex "add-symbol-file ./build/kernel.elf ${TEXT_START}" -ex "b *KernelMain" -ex "layout split" -ex "c" -ex "target record-full" -ex "b *debug_break"

# gdb -ex "target remote 127.0.0.1:1234" -ex "add-symbol-file ./build/kernel.elf ${ENTRY_ADDR}" -ex "b *KernelMain" -ex "layout split" -ex "add-symbol-file ./build/Loader.debug ${TEXT_ADDR} -s .data ${DATA_ADDR}" -ex "b *UefiMain"
#gdb -ex "target remote 127.0.0.1:1234" -ex "layout split" -ex "add-symbol-file ./build/Loader.debug ${TEXT_ADDR} -s .data ${DATA_ADDR}" -ex "b *UefiMain"
