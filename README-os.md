# OS30DAYS

## TABLE OF CONTENT


## DAY1 A BOOTABLE IMAGE

### RESULT

- ![d01-os-hello_world](./img/d01-os-hello_world.png)

```sh
IMG=d01/helloos.img; qemu-system-x86_64 -drive file=$IMG,format=raw,if=floppy
```

- ![d01-voyager0-greeting](./img/d01-voyager0-greeting.png)

```sh
ASM=d01/voyager0.asm; IMG=$(mktemp) && nasm $ASM -o $IMG && qemu-system-x86_64 -drive file=$IMG,format=raw,if=floppy
```

### Edit raw img

```sh
# The first bootable image, 1440KB FDA, FAT12
dd if=/dev/zero of=os.img bs=1024 count=1440
sudo mkfs -t fat os.img
# Then modified the bytes. (with VIM XXD)
```

### NASM to img

```sh
nasm os.nas -o os.img
```

- asm with Data Byte

```asm
DB 0xeb, 0x4e, 0x90, 0x48, 0x45, 0x4c, 0x4c, 0x4f
; ...
RESB 1469432
```

### BOOT SECTOR OF FAT

- [Boot Sector of FAT](https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Boot_Sector)
- [Volume Boot Record](https://en.wikipedia.org/wiki/Volume_boot_record)
- [DOS 4.0 EBPB](https://en.wikipedia.org/wiki/BIOS_parameter_block)
- [Local Block Addressing (LBA)](https://en.wikipedia.org/wiki/Logical_block_addressing)

- We essentially wrote an Boot Sector of MBR?
- WHAT IS `0xeb, 0x4e, 0x90`? It is A JUMP INSTRUCTION. `JMP 0x4e; NOP;` When an address is dectected during boot, chain access will start, execution will be passed to this command. This instruction will then skip over the non-executable of the sector.
- WHAT IS `0x55, 0xaa`? It is a "boot sector signature" (see VBR). Also, 0x1fe+2 == 0x200 == 512 Byte, the end of the sector.

- the BIOS Parameter Block (BPB) used in the book is the DOS 4.0 EBPB for FAT12, FAT16, FAT16B and HPFS (51bytes).


### FAT12 EBPB, floppy disk (textbook)

- [voyager0_os.asm](./d01/voyager0.asm)


## QEMU

```sh
# to boot from a raw img
qemu-system-x86_64 -drive file=helloos.img,format=raw
# compile and boot
ASM=fat12_os.asm; IMG=$(mktemp) && nasm $ASM -o $IMG && qemu-system-x86_64 -drive file=$IMG,format=raw,if=floppy
```


## PATCH BINARY WITH VIM

```sh
nvim example.bin
:%!xxd # display as mail safe hex dump
:%!xxd -r # to revert a mail safe hex dump to binary
```

