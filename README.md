# Unagi OS

- [README-os](./README-os.md)


## ENVIRONMENT

```sh
# EDK2 (UEFI)
git clone https://github.com/tianocore/edk2.git $HOME/src/edk2
git log ## > commit e1f5c6249af08c1df2c6257e4bb6abbf6134318c (edk2-stable202305 ++)

# QEMU
qemu-system-i386 --version
qemu-system-x86_64 --version
## QEMU emulator version 8.0.0
## Copyright (c) 2003-2022 Fabrice Bellard and the QEMU Project developers

# CLANG (cpp && 64-bit OS)
clang --version
clang++ --version
## Ubuntu clang version 14.0.0-1ubuntu1
## Target: x86_64-pc-linux-gnu
## Thread model: posix
## InstalledDir: /usr/bin

# LLD (64-bit OS Linker)
ld.lld --version
## Ubuntu LLD 14.0.0 (compatible with GNU linkers)

# GCC (crosscompiler for 32-bit OS)
$HOME/opt/cross/bin/i686-elf-gcc --version
## i686-elf-gcc (GCC) 12.2.0
## Copyright (C) 2022 Free Software Foundation, Inc.
## This is free software; see the source for copying conditions.  There is NO
## warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# BINUTIL
$HOME/opt/cross/bin/i686-elf-ld --version
## GNU ld (GNU Binutils) 2.40
## Copyright (C) 2023 Free Software Foundation, Inc.
## This program is free software; you may redistribute it under the terms of
## the GNU General Public License version 3 or (at your option) a later version.
## This program has absolutely no warranty.

# BEAR
bear --version
# bear 3.0.18

# CCLS
ccls --version
# Ubuntu ccls version 0.20210330-1
# clang version 12.0.1-19ubuntu3
```

- EDK2 Conf/target.txt

```txt
# ...
ACTIVE_PLATFORM       = UnagiLoaderPkg/UnagiLoaderPkg.dsc
# ...
TARGET                = DEBUG
# ...
TARGET_ARCH           = X64
# ...
TOOL_CHAIN_CONF       = Conf/tools_def.txt
# ...
TOOL_CHAIN_TAG        = GCC5
# ...
BUILD_RULE_CONF = Conf/build_rule.txt
```

- [libcxx, llvm](https://github.com/llvm/llvm-project/tree/llvmorg-8.0.1)

- [newlib-cygwin, libcxx, freetype](./modules/unagios-build/devenv_src/stdlib/build-stdlib.sh)

- EDK2 OVMF

```sh
# git clone git@github.com:tianocore/edk2.git
# cd edk2
# git submodule update --init --recursive
# git checkout e1f5c6249af08c1df2c6257e4bb6abbf6134318c
make -C BaseTools
source ./edksetup.sh
build -p OvmfPkg/OvmfPkgX64.dsc -b DEBUG -a X64 -t GCC5
```

## DEBUGGING WITH GDB

- [How to debug OVMF with QEMU using GDB, tianocore doc](https://github.com/tianocore/tianocore.github.io/wiki/How-to-debug-OVMF-with-QEMU-using-GDB)

```sh
$(QEMU_BIN) $(QEMU_OPTIONS) -debugcon file:debug.log -global isa-debugcon.iobase=0x402
```

```qemu
Shell> fs0:
Fs0:\> SampleApp.efi
```

```gdb
# UEFI Firmware is loaded at base address 0x06AEE240
(gdb) file SampleApp.efi
Reading symbols from SampleApp.efi...(no debugging symbols found)...done.
(gdb) info files
Symbols from "/home/u-mypc/run-ovmf/hda-contents/SampleApp.efi".
Local exec file:
        `/home/u-mypc/run-ovmf/hda-contents/SampleApp.efi',
        file type pei-i386.
        Entry point: 0x756
        0x00000240 - 0x000028c0 is .text
        0x000028c0 - 0x00002980 is .data
        0x00002980 - 0x00002b00 is .reloc
# text = 0x00006AEE000  +  0x00000240 = 0x06AEE240
# data = 0x00006AEE000  +  0x00000240 + 0x000028c0 = 0x06AF0B00

# Unload the file
(gdb) file
No executable file now.
No symbol file now.

(gdb) add-symbol-file SampleApp.debug 0x06AEE240 -s .data 0x06AF0B00
add symbol table from file "SampleApp.debug" at

        .text_addr = 0x6aee240
        .data_addr = 0x6af0b00
(y or n) y
Reading symbols from SampleApp.debug...done.

(gdb) break UefiMainMySampleApp
Breakpoint 1 at 0x6aee496: file /home/u-uefi/src/edk2/SampleApp/SampleApp.c, line 40.

(gdb) target remote localhost:1234
Remote debugging using localhost:1234
0x07df6ba4 in ?? ()
(gdb) info locals
```

```qemu
Shell> fs0:
Fs0:\> SampleApp.efi
```
